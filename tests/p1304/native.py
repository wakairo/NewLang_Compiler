"""Real ordinary CLI C, then passive observations of each caller lifetime."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

cli, cc, destination = sys.argv[1:]
here = Path(__file__).resolve().parent
source = here / "P-named-callers.nl"
assert hashlib.sha256(source.read_bytes()).hexdigest() == \
    "ce5fea006721936844f3b78c952bb186844485a11e9f0b2ff402ac7f0116be25"
parent = Path(destination)
parent.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix="native-", dir=parent))
rows = []


def run(args, label):
    p = subprocess.run(list(map(str, args)), capture_output=True)
    (out / (label + ".stdout")).write_bytes(p.stdout)
    (out / (label + ".stderr")).write_bytes(p.stderr)
    rows.append({"label": label, "status": p.returncode})
    (out / "commands.json").write_text(json.dumps(rows, indent=2) + "\n")
    assert p.returncode == 0, (label, p.returncode, p.stderr, out)
    return p


raw = run([cli, source], "emit").stdout.decode()
(out / "raw.c").write_text(raw)
assert raw.count("nl_fn_13(") == 4


def add(text, old, observation):
    assert text.count(old) == 1, (old, out)
    return text.replace(old, old + observation)


s = add(raw, "#include <stdint.h>\n", "#include <assert.h>\n")
s = add(s, "typedef struct { uint8_t f0; uint8_t f1; } nl_type_10;\n",
        "static uint8_t *p_target, *p_sibling;\n"
        "static unsigned p_phase, p_calls;\n")
s = add(s, "    nl_type_10 nl_local_1 = { .f0 = 83, .f1 = 211, };\n",
        "    assert(p_phase == 0 && p_calls == 0); p_phase = 1;\n"
        "    p_target = &nl_local_1.f0; p_sibling = &nl_local_1.f1;\n")
s = add(s, "    nl_type_10 nl_local_13 = { .f1 = 109, .f0 = 37, };\n",
        "    assert(p_phase == 2 && p_calls == 1 && nl_local_12 == 83); p_phase = 3;\n"
        "    p_target = &nl_local_13.f1; p_sibling = &nl_local_13.f0;\n")
s = add(s, "static uint8_t nl_fn_13(uint8_t *nl_local_3, uint8_t nl_local_4)\n{\n",
        "    assert(nl_local_3 == p_target);\n"
        "    if (p_phase == 1) { assert(p_calls == 0 && *p_target == 83 && nl_local_4 == 167 && *p_sibling == 211); }\n"
        "    else { assert(p_phase == 3 && p_calls == 1 && *p_target == 109 && nl_local_4 == 83 && *p_sibling == 37); }\n"
        "    ++p_calls;\n")
s = add(s, "    *nl_local_3 = nl_local_4;\n",
        "    assert(*p_target == (p_phase == 1 ? 167 : 83));\n"
        "    assert(*p_sibling == (p_phase == 1 ? 211 : 37));\n")
s = add(s, "    const uint8_t nl_local_6 = nl_local_5;\n",
        "    assert(nl_local_6 == (p_phase == 1 ? 83 : 109));\n")
for symbol, expr, value in ((10, "nl_call_2", 83), (11, "nl_local_10", 83),
                            (22, "nl_call_8", 109), (23, "nl_local_22", 109),
                            (7, "nl_loan_result_1", 83),
                            (8, "nl_local_1.f0", 167), (9, "nl_local_1.f1", 211),
                            (19, "nl_loan_result_2", 109),
                            (20, "nl_local_13.f1", 83), (21, "nl_local_13.f0", 37)):
    s = add(s, f"    const uint8_t nl_local_{symbol} = {expr};\n",
            f"    assert(nl_local_{symbol} == {value});\n")
assert s.count("    return nl_local_7;\n") == 1
s = s.replace("    return nl_local_7;\n",
              "    assert(p_phase == 1 && p_calls == 1 && p_target == &nl_local_1.f0 && p_sibling == &nl_local_1.f1);\n"
              "    assert(*p_target == 167 && *p_sibling == 211); p_target = p_sibling = 0; p_phase = 2;\n"
              "    return nl_local_7;\n")
assert s.count("    return nl_local_19;\n") == 1
s = s.replace("    return nl_local_19;\n",
              "    assert(p_phase == 3 && p_calls == 2 && p_target == &nl_local_13.f1 && p_sibling == &nl_local_13.f0);\n"
              "    assert(*p_target == 83 && *p_sibling == 37); p_target = p_sibling = 0; p_phase = 4;\n"
              "    return nl_local_19;\n")
s = add(s, "    nl_fn_10();\n", "    assert(p_phase == 4 && p_calls == 2);\n")
assert s.count("nl_fn_13(") == 4
(out / "passive.c").write_text(s)
is_clang = b"clang" in subprocess.check_output([cc, "--version"]).lower()
for opt in ("-O0", "-O2"):
    for mode in ("raw", "passive"):
        label = opt[1:] + "-" + mode
        flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", opt]
        if is_clang and mode == "passive":
            flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                      "-fno-omit-frame-pointer"]
        run([cc, *flags, out / (mode + ".c"), "-o", out / label], label + "-build")
        p = run([out / label], label + "-run")
        assert not p.stderr
print("P1304 strict raw/passive O0/O2, per-lifetime original/old/current/sibling PASS", out)
