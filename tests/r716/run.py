"""R716 first-pass independent source, public validation and native execution.

Only frozen Issue #716 sources are used. This observer never seeds guest memory.
"""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


HASHES = {
    "A_true_v2": "f359b4f959cb7dd535a2244276e96e0625a28ea9ebce7c806dd6834f1a7bb394",
    "A_false_v2": "773d09e2a7b9afe0c2eec3ce5b8087cf22fcd6318404e9390eceb968479dce55",
    "B_true_v2": "b0f644f77d44988aa489b5055188f6e677e84e4ef4e0430868b69db0c7c9770c",
    "B_false_v2": "bbbc6a03fb8b7630f0a68b084d6fd55896125f022a861344dfc0b3deb66e5408",
    "N_wrong_type_v2": "1ec2fbf18278be5bed8ea6d0f608723290663ed2075c919e549fbd576315817a",
}
# (declared field index, initial physical bytes, selected normal arm,
#  old returned Copy, final physical bytes, write events per arm)
ORACLE = {
    "A_true_v2": (1, (61, 19), 0, 19, (61, 37), (1, 1)),
    "A_false_v2": (1, (61, 19), 1, 19, (61, 83), (1, 1)),
    "B_true_v2": (0, (74, 23), 0, 74, (106, 23), (1, 0)),
    "B_false_v2": (0, (74, 23), 1, 74, (74, 23), (1, 0)),
}


def run(*args):
    p = subprocess.run(list(map(str, args)), capture_output=True)
    return p


def require(test, reason):
    if not test:
        raise AssertionError(reason)


def instrument(code, proof, oracle):
    index, initial, selected, old, final, expected = oracle
    base, if_id = proof["base"], proof["if"]
    require(proof["index"] == index, "source issued wrong field index")
    require(proof["initial_root_fact"] != proof["post_root_fact"],
            "postjoin root must get new value fact")
    require(proof["initial_sibling_fact"] == proof["post_sibling_fact"],
            "disjoint sibling fact unexpectedly changed")
    additions = {}

    def add(position, content):
        additions[position] = additions.get(position, "") + content

    def unique(pattern, left=0, right=None):
        end = len(code) if right is None else right
        matches = list(re.finditer(pattern, code[left:end]))
        require(len(matches) == 1, "unique emitted C pattern missing: " + pattern)
        match = matches[0]
        return left + match.start(), left + match.end(), match

    def check(expected_bytes):
        return " || ".join("(*r716_f%d != %d)" % (i, value)
                           for i, value in enumerate(expected_bytes))

    _, ctor_end, _ = unique(
        r"(?:const )?nl_type_\d+ nl_local_%d = \{[^;]+;" % base)
    add(ctor_end,
        "\n    const volatile uint8_t *const r716_f0 = &nl_local_%d.f0;"
        "\n    const volatile uint8_t *const r716_f1 = &nl_local_%d.f1;"
        "\n    unsigned r716_arm = 99, r716_changes = 0;"
        "\n    if (%s) exit(91);" % (base, base, check(initial)))
    header_start, header_end, _ = unique(r"\n    if \([^;\n]+\) \{", ctor_end)
    split = code.index("\n    } else {", header_end)
    end = code.index("\n    }\n", split) + len("\n    }")
    arms = [(header_end, split), (split + len("\n    } else {"), end)]
    add(arms[0][0], "\n        r716_arm = 0;")
    add(arms[1][0], "\n        r716_arm = 1;")
    for a, (lo, hi) in enumerate(arms):
        events = [x for x in proof["arms"][a]["events"] if x["change"]]
        require(len(events) == expected[a] == proof["arms"][a]["changes"],
                "actual event counts differ from source oracle")
        if not events:
            require(not re.search(r"\*nl_local_\d+ = ", code[lo:hi]),
                    "unchanged arm has a hidden store")
        for event in events:
            ref, nid = event["ref"], event["node"]
            require(ref > 0, "missing source-bound ref identity")
            _, old_end, _ = unique(
                r"const uint8_t nl_replace_old_%d = \*nl_local_%d;" %
                (nid, ref), lo, hi)
            add(old_end,
                "\n            if (nl_local_%d != r716_f%d || "
                "nl_replace_old_%d != %d || %s) exit(91);" %
                (ref, index, nid, oracle[1][index], check(initial)))
            _, write_end, _ = unique(
                r"\*nl_local_%d = [^;]+;" % ref, lo, hi)
            expected_field = ((37, 83)[a] if index == 1 else 106)
            expected_state = list(initial)
            expected_state[index] = expected_field
            add(write_end, "\n            ++r716_changes;\n"
                "            if (%s) exit(91);" % check(expected_state))
    _, result_end, _ = unique(
        r"const uint8_t nl_local_%d = nl_if_result_%d;" %
        (proof["result_binding"], if_id), end)
    add(result_end,
        "\n    if (r716_arm != %d || r716_changes != %d || "
        "nl_local_%d != %d || %s) exit(91);" %
        (selected, expected[selected], proof["result_binding"], old,
         check(final)))
    for position in sorted(additions, reverse=True):
        code = code[:position] + additions[position] + code[position:]
    return "#include <stdlib.h>\n" + code


def exercise(compiler, probe, cc, sanitizer, directory):
    directory = Path(directory)
    records = {}
    with tempfile.TemporaryDirectory() as t:
        root = Path(t)
        for name, digest in HASHES.items():
            path = directory / (name + ".nl")
            require(hashlib.sha256(path.read_bytes()).hexdigest() == digest,
                    "frozen source SHA changed: " + name)
            checked = run(compiler, "--check", path)
            emitted = run(compiler, path)
            if name == "N_wrong_type_v2":
                require(checked.returncode != 0 and not checked.stdout and
                        b"error(" in checked.stderr,
                        "wrong type not semantically rejected: " + repr(checked))
                require(emitted.returncode != 0 and not emitted.stdout,
                        "partial C17 on invalid source: " + repr(emitted))
                preserved = root / "preserve.c"
                preserved.write_bytes(b"PRESERVED\n")
                named = run(compiler, "--emit-c17-to", preserved, path)
                require(named.returncode != 0 and not named.stdout and
                        preserved.read_bytes() == b"PRESERVED\n",
                        "named C17 output clobbered: " + repr(named))
                records[name] = {"negative": "semantic reject; zero C17; named preserved"}
                continue
            require(checked.returncode == 0 and not checked.stdout and
                    not checked.stderr,
                    "CHECK rejected frozen positive %s: %r" % (name, checked))
            require(emitted.returncode == 0 and emitted.stdout and
                    not emitted.stderr,
                    "C17 rejected frozen positive %s: %r" % (name, emitted))
            idx, initial, selected, old, final, changes = ORACLE[name]
            actual = run(probe, path, idx, changes[0], changes[1])
            require(actual.returncode == 0 and not actual.stderr,
                    "source-issued proof rejected %s: %r" % (name, actual))
            view = json.loads(actual.stdout)
            require(view["rejected_public"] >= 10, "too few hostile copied views")
            strict = emitted.stdout.decode("utf-8")
            observed = instrument(strict, view, ORACLE[name])
            for label, code in (("original", strict), ("passive", observed)):
                src = root / (name + "_" + label + ".c")
                src.write_text(code, encoding="utf-8", newline="\n")
                for opt in ("-O0", "-O2"):
                    exe = root / (name + "_" + label + opt)
                    argv = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic",
                            "-Werror", opt, src, "-o", exe]
                    if sanitizer != "none":
                        argv += ["-fsanitize=" + sanitizer,
                                 "-fno-sanitize-recover=all",
                                 "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
                    built = run(*argv)
                    require(built.returncode == 0,
                            "strict C17 compiler rejected %s/%s/%s: %s" %
                            (name, label, opt, built.stderr.decode()))
                    executed = run(exe)
                    require(executed.returncode == 0 and not executed.stdout and
                            not executed.stderr,
                            "native mismatch %s/%s/%s: %r" %
                            (name, label, opt, executed))
            records[name] = {"sha256": HASHES[name],
                             "source_issued": "valid/rejected-view-recovery",
                             "public_corruptions": view["rejected_public"],
                             "source_join": view,
                             "native": "unchanged+passive strict C17 O0/O2"}
    print(json.dumps(records, sort_keys=True))


if __name__ == "__main__":
    exercise(*sys.argv[1:])
