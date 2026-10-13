#!/usr/bin/env python3
"""R386 independent black-box generated-C evidence inspection and passive tap."""
import hashlib
import json
import pathlib
import re
import subprocess
import sys

here = pathlib.Path(__file__).resolve().parent
out = here / "evidence"
observer = here / "observer.c"
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
report = {"observer_sha256": sha(observer), "cases": {}, "controls": {}}

def run(cmd, timeout=30):
    return subprocess.run([str(x) for x in cmd], capture_output=True, timeout=timeout)

for cc in ("gcc-13", "clang"):
    exe = out / ("observer_selftest." + cc)
    cmd = [cc, "-std=c17", "-Wall", "-Wextra", "-Werror", "-pedantic",
           "-DR386_OBSERVER_SELFTEST", str(observer), "-o", str(exe)]
    c = run(cmd)
    p = run([exe]) if c.returncode == 0 else None
    report["controls"][cc] = {
        "compile_exit": c.returncode, "compile_stderr": c.stderr.decode(errors="replace"),
        "run_exit": p.returncode if p else None,
        "run_stdout": p.stdout.decode(errors="replace") if p else None
    }
    print("R386_CONTROLS", cc, json.dumps(report["controls"][cc]), flush=True)
    if c.returncode != 0 or p.returncode != 0 or b"11 cases" not in p.stdout:
        sys.exit("observer selftest failure")

cases = {
    "positive_a": {
        "bindings": [("nl_local_1", "nl_local_3", 240), ("nl_local_2", "nl_local_4", 1)],
        "locals": ("nl_local_8", "nl_local_12", "nl_local_1.f0", "nl_local_1.f1", "nl_local_2.f0", "nl_local_2.f1"),
        "snapshot": "R386_NATIVE_A old=0,255 alpha=240,199 beta=1,13\n"
    },
    "positive_b": {
        "bindings": [("nl_local_1", "nl_local_3", 1), ("nl_local_2", "nl_local_4", 250),
                     ("nl_local_1", "nl_local_5", 30)],
        "locals": ("nl_local_9", "nl_local_13", "nl_local_17", "nl_local_1.f0",
                   "nl_local_1.f1", "nl_local_2.f0", "nl_local_2.f1"),
        "snapshot": "R386_NATIVE_B old=255,42,1 alpha=30,0 beta=250,255\n"
    }
}

for name, case in cases.items():
    raw = (out / (name + ".c")).read_text()
    # Verify scope creates a fresh pointer, binds that pointer to the only named
    # callee, and supplies a distinct local-u8 actual on each call. This is
    # an inspection of candidate-produced C, never P tests/source.
    pat = (r"    \{\n        uint8_t \*const (nl_local_\d+) = &(nl_local_\d+)\.f0;\n"
           r"        \(void\)\1;\n        (nl_loan_result_\d+) = (nl_fn_\d+)\(\1, (nl_local_\d+)\);\n    \}")
    matches = re.findall(pat, raw)
    actual = [(root, arg) for _, root, _, _, arg in matches]
    expected = [(root, arg) for root, arg, _ in case["bindings"]]
    print("R386_BINDINGS", name, json.dumps(matches), flush=True)
    if actual != expected:
        sys.exit(f"scope/actual mismatch {name}: {actual} != {expected}")
    if len({call[3] for call in matches}) != 1:
        sys.exit("calls not targeted at exactly one named helper")
    for root, arg, value in case["bindings"]:
        if raw.count(f"const uint8_t {arg} = {value};") != 1:
            sys.exit(f"new actual literal mismatch {name}/{arg}")
    if not re.search(r"static uint8_t nl_fn_\d+\(uint8_t \*nl_local_\d+, uint8_t nl_local_\d+\)", raw):
        sys.exit("named u8 scoped helper absent")

    # Inject ONLY an observer prototype and a snapshot call at the very end
    # of the guest's entry function after all loan scopes. No call/return,
    # helper argument/formal, old-value capture, store or guest value changes.
    count = len(case["locals"])
    proto = f"void r386_observe_{name[-1]}(" + ", ".join(["unsigned"] * count) + ");\n"
    anchor = "#include <stdint.h>\n"
    end = "    (void)0;\n}\n\nstatic uint8_t "
    if raw.count(anchor) != 1 or raw.count(end) != 1:
        sys.exit(f"observation anchor mismatch in {name}")
    code = raw.replace(anchor, anchor + proto)
    tap = f"    r386_observe_{name[-1]}(" + ", ".join(case["locals"]) + ");\n"
    code = code.replace(end, tap + end)
    if code.replace(tap, "", 1).replace(proto, "", 1) != raw:
        sys.exit("observer attempted to modify guest program")
    altered = out / (name + ".observed.c")
    altered.write_text(code)
    case_record = {
        "original_c_sha256": sha(out / (name + ".c")),
        "observer_only_c_sha256": sha(altered),
        "guest_calls": matches,
        "tap": tap.strip(),
        "native": {}
    }
    for cc in ("gcc-13", "clang"):
        exe = out / (name + ".observed." + cc)
        compile_command = [cc, "-std=c17", "-Wall", "-Wextra", "-Werror", "-pedantic",
                           str(altered), str(observer), "-o", str(exe)]
        c = run(compile_command)
        p = run([exe]) if c.returncode == 0 else None
        result = {"cmd": compile_command, "compile_exit": c.returncode,
                  "compile_stderr": c.stderr.decode(errors="replace"),
                  "native_exit": p.returncode if p else None,
                  "native_stdout": p.stdout.decode(errors="replace") if p else None,
                  "native_stderr": p.stderr.decode(errors="replace") if p else None}
        case_record["native"][cc] = result
        print("R386_OBSERVED", name, cc, json.dumps(result), flush=True)
        if c.returncode != 0 or p.returncode != 0 or p.stdout.decode() != case["snapshot"]:
            sys.exit("R386 observed native mismatch")
    report["cases"][name] = case_record

# Real negative CLI status; unsupported profile is not a semantic rejection.
for name in ("invalid_escape", "invalid_actual_type", "invalid_read_ref", "invalid_field"):
    stderr = (out / (name + ".stderr")).read_text()
    contents = (out / (name + ".c")).read_bytes()
    if contents:
        sys.exit(f"invalid test emitted C bytes: {name}")
    report[name] = {"classification": "semantic" if "error(semantic)" in stderr
                    else "unsupported" if "error(unsupported)" in stderr else "other",
                    "stderr": stderr.strip()}
    print("R386_NEGATIVE", name, json.dumps(report[name]), flush=True)

(out / "observed_report.json").write_text(json.dumps(report, indent=2))
print("R386_EMPRICAL_OBSERVED_PASS_PAIR_ONLY", flush=True)
