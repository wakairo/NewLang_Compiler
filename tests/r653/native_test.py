"""R653 independent oracle, genuine CLI, strict C17 native, passive host observer."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def run(args):
    return subprocess.run([str(x) for x in args], capture_output=True, text=True)


def checked_run(args, code=0):
    p = run(args)
    need(p.returncode == code, f"{args}: expected {code}, actual {p.returncode}, stderr={p.stderr[:1800]}, stdout={p.stdout[:500]}")
    return p


def git_blob(data):
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


def native(folder, cc, name, code, sanitizer, expected=0):
    c = folder / (name + ".c")
    c.write_text(code)
    flags = [] if sanitizer == "none" else [f"-fsanitize={sanitizer}", "-fno-sanitize-recover=all"]
    for opt in ("-O0", "-O2"):
        out = folder / (name + opt)
        build = run([cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra",
                     "-Werror", opt, *flags, c, "-o", out])
        need(build.returncode == 0, f"strict C17 build {name} {opt}: {build.stderr}")
        p = run([out])
        need(p.returncode == expected and not p.stderr and not p.stdout,
             f"native {name} {opt}, code={p.returncode}, stderr={p.stderr}, stdout={p.stdout}")


def observe_c(c, o):
    pairs = list(re.finditer(r"nl_type_\d+ (nl_local_\d+) = \{ \.f0 = (\d+), \.f1 = (\d+), \};", c))
    need(len(pairs) == 2, f"two authentic original carriers required; got {len(pairs)}")
    refs = list(re.finditer(r"uint8_t \*const (nl_local_\d+) = &(nl_local_\d+)\.f([01]);", c))
    need(len(refs) == 2, f"two original field writes required; got {len(refs)}")
    need(len(re.findall(r"(?m)^[ \t]*if \(nl_local_\d+\) \{$", c)) == 1,
         "one real Copy IF if/else required")
    output = c.replace("#include <stdint.h>", "#include <stdint.h>\n#include <stdlib.h>", 1)
    pairs_names = [p[1] for p in pairs]
    need(len(set(pairs_names)) == 2, "root aliases")
    for i, (p, original) in enumerate(zip(pairs, o["originals"])):
        need([int(p[2]), int(p[3])] == original, "authentic initial Pair mismatch")
        needle = p[0]
        checks = f'\n    if ({p[1]}.f0 != {original[0]} || {p[1]}.f1 != {original[1]}) exit(91);'
        checks += f'\n    const void *const r653_root_{i} = &{p[1]};'
        checks += f'\n    const uint8_t *const r653_sibling_{i} = &{p[1]}.f{1-o["fields"][i]};'
        need(needle in output, "initialization lost")
        output = output.replace(needle, needle + checks, 1)
    old_binding = list(re.finditer(r"const uint8_t (nl_local_\d+) = nl_loan_result_\d+;", c))
    need(len(old_binding) == 2, "two genuine old-value results required")
    for i, (ref, expected_field) in enumerate(zip(refs, o["fields"])):
        need(ref[2] == pairs_names[i] and int(ref[3]) == expected_field,
             "physical owner/field write routing mismatch")
        assign = re.search(r"\*" + re.escape(ref[1]) + r" = [^;]+;", c)
        need(assign is not None and assign.start() > ref.end(), "missing ordered actual write")
        capture = re.search(r"const uint8_t (nl_replace_old_\d+) = \*" +
                            re.escape(ref[1]) + r";", c)
        need(capture is not None and capture.end() < assign.start(), "missing actual old-value read")
        watched = [list(x) for x in o["originals"]]
        for j in range(i + 1):
            watched[j][o["fields"][j]] = o["updates"][j]
        check = f'\n    if ({capture[1]} != {o["old"][i]}) exit(91);'
        check += f'\n    if ((const void *)&{ref[2]} != r653_root_{i}) exit(91);'
        check += f'\n    if (&{ref[2]}.f{1-expected_field} != r653_sibling_{i}) exit(91);'
        for j in range(2):
            check += f'\n    if ({pairs_names[j]}.f0 != {watched[j][0]} || {pairs_names[j]}.f1 != {watched[j][1]}) exit(91);'
        output = output.replace(assign[0], assign[0] + check, 1)
    for i, m in enumerate(old_binding):
        output = output.replace(m[0], m[0] +
                                f'\n    if ({m[1]} != {o["old"][i]}) exit(91);', 1)
    return output, refs


def main():
    compiler, probe, cc, directory, sanitizer = sys.argv[1:]
    root = Path(directory)
    frozen = json.loads((root / "freeze.json").read_text())
    for filename, obj in frozen["cases"].items():
        data = (root / filename).read_bytes()
        need(git_blob(data) == obj["blob"], "independent pre-execution source was modified")
    with tempfile.TemporaryDirectory(prefix="r653-source-owned-") as td:
        work = Path(td)
        src = root / "bad_type.nl"
        for args in ([compiler, "--check", src], [compiler, src]):
            p = checked_run(args, 3)
            need(not p.stdout and "error(semantic)" in p.stderr,
                 f"invalid typed comparison not rejected: {p.stderr}")
        print("R653 negative bad_type: semantic 3; no generated C", flush=True)
        for name in ("true_world.nl", "false_world.nl"):
            src = root / name
            ck = checked_run([compiler, "--check", src])
            need(not ck.stdout and not ck.stderr, f"BLIND control CHECK0 expected: {name}")
            emit = checked_run([compiler, src], 4)
            need(not emit.stdout and "V1-BACKEND-UNSUPPORTED" in emit.stderr,
                 f"original BLIND backend4 control mismatch: {name}")
            print(f"R653 original BLIND preserved {name}: CHECK0/backend4/no C",
                  flush=True)
        calibrated = json.loads((root / "PHASE2_ORACLE.json").read_text())
        for filename, obj in calibrated["cases"].items():
            data = (root / filename).read_bytes()
            need(git_blob(data) == obj["blob"],
                 f"phase2 pre-execution source was modified: {filename}")
        for name, obj in calibrated["cases"].items():
            oracle = obj["oracle"]
            src = root / name
            ck = checked_run([compiler, "--check", src])
            need(not ck.stdout and not ck.stderr, "successful --check must be silent")
            emit = checked_run([compiler, src])
            need(bool(emit.stdout) and not emit.stderr, "source-issued actual C17 missing")
            repeat = checked_run([compiler, src])
            need(emit.stdout == repeat.stdout, "C17 nondeterminism")
            watched, refs = observe_c(emit.stdout, oracle)
            native(work, cc, name + "-plain", emit.stdout, sanitizer)
            native(work, cc, name + "-passive", watched, sanitizer)
            # Wrong-field observer control, not a forged checked receipt.
            target = refs[0][0]
            broken = watched.replace(target, target.replace(
                ".f" + str(oracle["fields"][0]),
                ".f" + str(1-oracle["fields"][0])), 1)
            need(broken != watched, "mutant not constructed")
            native(work, cc, name + "-wrong-field", broken, sanitizer, 91)
            print(json.dumps({"phase": "NON-BLIND calibrated", "case": name,
                              "checkpoint": "native complete; owned pending",
                              "source_sha256": hashlib.sha256(src.read_bytes()).hexdigest(),
                              "c17_sha256": hashlib.sha256(emit.stdout.encode()).hexdigest(),
                              "native": "strict C17 O0+O2 plain/passive PASS",
                              "negative_observer": "wrong-field 91",
                              "sanitizer": sanitizer}), flush=True)
            root_names = {"phase2_true.nl": ("larch", "willow"),
                          "phase2_false.nl": ("sable", "birch")}[name]
            pub = checked_run([probe, src, oracle["old"][0], oracle["old"][1],
                               oracle["fields"][0], oracle["fields"][1],
                               oracle["arm_values"][0], oracle["arm_values"][1],
                               *root_names])
            need("R653_OWNED_OK" in pub.stdout, "public owned proof/negative gate absent")
            print(json.dumps({"phase": "NON-BLIND calibrated", "case": name,
                              "checkpoint": "public owned",
                              "owned": pub.stdout.strip()}), flush=True)


if __name__ == "__main__":
    main()
