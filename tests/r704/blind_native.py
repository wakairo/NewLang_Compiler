"""R #704 fresh BLIND: source/check, genuine public proof, strict C17/native.

Original emitted C remains byte-exact in separate-translation-unit execution.
A separate instrumented copy only adds passive assertions (no guest writes).
"""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

FROZEN = {
    "ledger_old.nl": {
        "sha": "4393cffe29e5c22b437f480e3b975315840b76bfd1d41960d4ecdf533d7efe75",
        "returned": 37, "inner_old": 37, "inner_new": 182,
        "inner": [91, 37], "inner_post": [91, 182],
        "outer": [43, 6], "outer_post": [43, 37],
        "old_outer": 6, "inner_field": 1, "outer_field": 1,
    },
    "switch_current.nl": {
        "sha": "40f7f101eb0d39589e9f1aa464bc5c957cce7c62ed37a7c18a5eea6fb202b769",
        "returned": 22, "inner_old": 147, "inner_new": 22,
        "inner": [147, 215], "inner_post": [22, 215],
        "outer": [64, 5], "outer_post": [64, 22],
        "old_outer": 5, "inner_field": 0, "outer_field": 1,
    },
}
NEG_SHA = "97ce93ef482d0b513dd67e3ad51639a7cfbf9be185e188451f2457a64ea744f7"


def invoke(*argv):
    return subprocess.run([str(x) for x in argv], capture_output=True)


def check(condition, reason):
    if not condition:
        raise AssertionError(reason)


def exactly(pattern, body):
    matches = list(re.finditer(pattern, body, re.S))
    check(len(matches) == 1, f"unique emitted relation missing {pattern!r}: {len(matches)}")
    return matches[0]


def insert_after(body, found, extra):
    # Only adds observations immediately after existing guest statements.
    before = found.group()
    check(body.count(before) == 1, f"ambiguous insertion: {before!r}")
    return body.replace(before, before + "\n    " + extra, 1)


def passive(emitted, o):
    helper = exactly(r"static uint8_t nl_fn_(\d+)\(void\)\n\{\n(.*?)\n\}", emitted)
    caller = exactly(r"static void nl_fn_(\d+)\(void\)\n\{\n(.*?)\n\}", emitted)
    fn_id, c_id = helper.group(1), caller.group(1)
    hb, cb = helper.group(2), caller.group(2)
    hctor = exactly(r"nl_type_\d+ nl_local_(\d+) = \{[^;]+;", hb)
    cctor = exactly(r"nl_type_\d+ nl_local_(\d+) = \{[^;]+;", cb)
    hroot, croot = hctor.group(1), cctor.group(1)
    hptr = exactly(rf"uint8_t \*const nl_local_(\d+) = &nl_local_{hroot}\.f{o['inner_field']};", hb)
    cptr = exactly(rf"uint8_t \*const nl_local_(\d+) = &nl_local_{croot}\.f{o['outer_field']};", cb)
    hstore = exactly(rf"\*nl_local_{hptr.group(1)} = [^;]+;", hb)
    actual_call = exactly(rf"const uint8_t nl_call_(\d+) = nl_fn_{fn_id}\(\);", cb)
    received = exactly(rf"const uint8_t nl_local_(\d+) = nl_call_{actual_call.group(1)};", cb)
    cstore = exactly(rf"\*nl_local_{cptr.group(1)} = nl_local_{received.group(1)};", cb)
    hreturn = exactly(r"return nl_local_(\d+);", hb)
    hreads = [exactly(rf"const uint8_t nl_local_(\d+) = nl_local_{hroot}\.f{i};", hb)
              for i in range(2)]
    creads = [exactly(rf"const uint8_t nl_local_(\d+) = nl_local_{croot}\.f{i};", cb)
              for i in range(2)]
    check(hctor.start() < hptr.start() < hstore.start() < hreturn.start(), "helper order")
    check(cctor.start() < actual_call.start() < received.start() <
          cptr.start() < cstore.start(), "caller order")
    # A return from the callee must be the real Copy carrier, not a seeded
    # constant. The injected tests never assign to a guest local or field.
    hb = insert_after(hb, hctor,
                      f"if (nl_local_{hroot}.f0 != {o['inner'][0]} || nl_local_{hroot}.f1 != {o['inner'][1]} || &nl_local_{hroot}.f0 == &nl_local_{hroot}.f1) exit(71);")
    hb = insert_after(hb, hstore,
                      f"if (nl_local_{hroot}.f0 != {o['inner_post'][0]} || nl_local_{hroot}.f1 != {o['inner_post'][1]}) exit(72);")
    # Refresh offset lookup after inserts.
    hreturn = exactly(r"return nl_local_(\d+);", hb)
    hb = hb[:hreturn.start()] + (
        f"if (nl_local_{hreturn.group(1)} != {o['returned']} || nl_local_{hroot}.f0 != {o['inner_post'][0]} || nl_local_{hroot}.f1 != {o['inner_post'][1]}) exit(73);\n    "
    ) + hb[hreturn.start():]
    cb = insert_after(cb, cctor,
                      f"if (nl_local_{croot}.f0 != {o['outer'][0]} || nl_local_{croot}.f1 != {o['outer'][1]} || &nl_local_{croot}.f0 == &nl_local_{croot}.f1) exit(74);")
    cb = insert_after(cb, actual_call,
                      f"if (nl_call_{actual_call.group(1)} != {o['returned']}) exit(75);")
    cb = insert_after(cb, cstore,
                      f"if (nl_local_{croot}.f0 != {o['outer_post'][0]} || nl_local_{croot}.f1 != {o['outer_post'][1]}) exit(76);")
    for ix in range(2):
        found = exactly(rf"const uint8_t nl_local_{creads[ix].group(1)} = nl_local_{croot}\.f{ix};", cb)
        cb = insert_after(cb, found,
                          f"if (nl_local_{creads[ix].group(1)} != {o['outer_post'][ix]}) exit(77);")
    # No literal substitution can masquerade as the actual callee invocation
    # or caller's actual store in the passive dataflow witness.
    check(hb.count(f"*nl_local_{hptr.group(1)} =") == 1, "callee store changed")
    check(cb.count(f"nl_fn_{fn_id}();") == 1 and
          cb.count(f"*nl_local_{cptr.group(1)} = nl_local_{received.group(1)};") == 1,
          "caller dataflow changed")
    p = emitted.replace("#include <stdint.h>", "#include <stdint.h>\n#include <stdlib.h>", 1)
    p = p.replace(helper.group(2), hb, 1).replace(caller.group(2), cb, 1)
    check(p != emitted, "observer not attached")
    return p


def native(cc, sanit, generated, observed, output):
    with tempfile.TemporaryDirectory(prefix="r704-genuine-") as tmp:
        folder = Path(tmp)
        for opt in ("-O0", "-O2"):
            flags = [] if sanit == "none" else [
                f"-fsanitize={sanit}", "-fno-sanitize-recover=all"]
            common = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", opt, *flags]
            original = folder / "generated.c"
            original.write_bytes(generated)
            consumer = folder / "consumer.c"
            consumer.write_text(
                "int nl_guest_main(void);\nint main(void) { return nl_guest_main(); }\n")
            guest_obj = folder / "guest.o"
            a = invoke(cc, *common, "-Dmain=nl_guest_main", "-c", original, "-o", guest_obj)
            check(a.returncode == 0, f"unaltered C17 guest compile {a.stderr!r}")
            exe = folder / "guest"
            a = invoke(cc, *common, consumer, guest_obj, "-o", exe)
            check(a.returncode == 0, f"separate C17 TU link {a.stderr!r}")
            a = invoke(exe)
            check(a.returncode == 0 and not a.stdout, f"original guest {a!r}")
            passive_src = folder / "passive.c"
            passive_src.write_text(observed)
            exe2 = folder / "passive"
            a = invoke(cc, *common, passive_src, "-o", exe2)
            check(a.returncode == 0, f"passive compile {a.stderr!r}")
            a = invoke(exe2)
            check(a.returncode == 0 and not a.stdout, f"passive guest {a!r}")
            output.append({"optimization": opt, "original_linked_other_TU": 0,
                           "passive_observed": 0})


def main(compiler, probe, cc, sanitizer, base):
    root = Path(base)
    neg = root / "readonly_reject.nl"
    check(hashlib.sha256(neg.read_bytes()).hexdigest() == NEG_SHA, "negative SHA drift")
    neg_check = invoke(compiler, "--check", neg)
    neg_emit = invoke(compiler, neg)
    check(neg_check.returncode == 3 and neg_emit.returncode == 3 and
          not neg_emit.stdout, f"read->write rejection expected: {neg_check!r} {neg_emit!r}")
    evidence = {"negative": {"check": neg_check.returncode, "emit": neg_emit.returncode,
                              "diagnostic": neg_check.stderr.decode(errors="replace")[:300]},
                "positive": {}, "sanitizer": sanitizer}
    for name, o in FROZEN.items():
        src = root / name
        check(hashlib.sha256(src.read_bytes()).hexdigest() == o["sha"], "BLIND source SHA drift")
        checked = invoke(compiler, "--check", src)
        check(checked.returncode == 0 and not checked.stdout and not checked.stderr,
              f"initial BLIND CHECK {name}: {checked!r}")
        emitted = invoke(compiler, src)
        check(emitted.returncode == 0 and emitted.stdout and not emitted.stderr,
              f"initial BLIND emit {name}: {emitted!r}")
        a = invoke(probe, src, o["returned"], o["inner_old"], o["inner_new"],
                   o["old_outer"], o["inner_field"], o["outer_field"])
        check(a.returncode == 0 and not a.stderr, f"source-issued owned {name}: {a!r}")
        owned = json.loads(a.stdout)
        obs = passive(emitted.stdout.decode("utf-8"), o)
        executions = []
        native(cc, sanitizer, emitted.stdout, obs, executions)
        evidence["positive"][name] = {
            "check": 0, "emit": 0, "source_sha256": o["sha"],
            "unaltered_c_sha256": hashlib.sha256(emitted.stdout).hexdigest(),
            "proof": owned, "native": executions}
    print(json.dumps(evidence, sort_keys=True))


if __name__ == "__main__":
    main(*sys.argv[1:])
