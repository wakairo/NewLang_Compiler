"""Bounded P1544 source/public/negative/native checks; immutable per-attempt logs."""
import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

MODE, CLI, PROBE, CC, WORK = sys.argv[1:]
ROOT = Path(__file__).resolve().parent
WORK = Path(WORK) / str(time.time_ns())
WORK.mkdir(parents=True)
FREEZE = json.loads((ROOT / "freeze-v1.json").read_text())
ORACLE = json.loads((ROOT / "oracle.json").read_text())
for name, expected in FREEZE["files"].items():
    path = ROOT / name if name == "oracle.json" else ROOT / "source" / name
    data = path.read_bytes()
    assert len(data) == expected["bytes"]
    assert hashlib.sha256(data).hexdigest() == expected["sha256"], name
assert os.sysconf("SC_LONG_BIT") == 64, "frozen current-target FIRST requires 64-bit host"


def record(kind, command, amount=1):
    name = os.environ.get("P1544_LEDGER")
    if not name:
        return
    path = Path(name)
    with Path(name + ".lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        data = json.loads(path.read_text())
        limits = {"selected": 200, "frontend": 2, "probe": 2, "native": 18, "oom_cases": 8}
        data["counts"][kind] += amount
        assert data["counts"][kind] <= limits[kind], (kind, data["counts"])
        if kind == "oom_cases":
            data["oom_sites"] = sorted(set(data["oom_sites"] + [MODE]))
            assert len(data["oom_sites"]) <= 3
        data["events"].append({"time_ns": time.time_ns(), "kind": kind,
                               "mode": MODE, "command": command, "charged": amount,
                               "logs": str(WORK)})
        path.write_text(json.dumps(data, indent=2) + "\n")


sequence = 0

def run(command, expect=0, selected=False):
    global sequence
    command = [str(x) for x in command]
    if selected:
        record("selected", command)
    sequence += 1
    prefix = WORK / f"{sequence:03}"
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    result = subprocess.run(command, capture_output=True, env=env, timeout=90)
    prefix.with_suffix(".stdout").write_bytes(result.stdout)
    prefix.with_suffix(".stderr").write_bytes(result.stderr)
    prefix.with_suffix(".json").write_text(json.dumps({"command": command, "exit": result.returncode}, indent=2))
    assert (result.returncode == 0 if expect == 0 else result.returncode != 0), (
        command, result.returncode, result.stdout.decode(errors="replace"),
        result.stderr.decode(errors="replace"), str(prefix))
    return result


def source(name):
    return ROOT / "source" / name


def expected(name):
    for case in ORACLE["actual"]:
        if case["source"] == name:
            return dict(case, divisor=7)
    return FREEZE["extra_actual"][name]


def positive(name):
    e = expected(name)
    result = run([CLI, "--check", source(name)], selected=True)
    assert not result.stdout and not result.stderr
    result = run([PROBE, "positive", source(name), e["amount"], e["divisor"]], selected=True)
    assert not result.stderr
    return json.loads(result.stdout)


def negative(name, diagnostic):
    result = run([CLI, "--check", source(name)], expect=1, selected=True)
    assert not result.stdout
    if diagnostic != "SOURCE":
        assert diagnostic.encode() in result.stderr, (diagnostic, result.stderr)
    else:
        assert b"USIZE-DIVISION-SOURCE-PRECISION" in result.stderr or b"P5-" in result.stderr
    output = WORK / (name + ".c")
    output.write_bytes(b"P1544 ORIGINAL OUTPUT\n")
    emission = run([CLI, "--emit-c17-to", output, source(name)], expect=1, selected=True)
    assert not emission.stdout and output.read_bytes() == b"P1544 ORIGINAL OUTPUT\n"
    if diagnostic != "SOURCE":
        assert diagnostic.encode() in emission.stderr


def compilers():
    gcc = shutil.which("gcc")
    clang = shutil.which("clang-23") or shutil.which("clang")
    assert gcc and clang, "native checks require real GCC and Clang"
    return gcc, clang


def guest(text, tag, compiler, sanitizer=None):
    c_file, executable = WORK / (tag + ".c"), WORK / tag
    c_file.write_text(text)
    flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2"]
    if sanitizer:
        flags += [f"-fsanitize={sanitizer}", "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
    if sanitizer == "address":
        flags += ["-fno-pie", "-no-pie"]
    command = [compiler, *flags, c_file, "-o", executable]
    record("native", [str(x) for x in command])
    run(command)
    result = run([executable])
    assert not result.stdout and not result.stderr
    print("PHYSICAL", tag, "PASS")


if MODE == "positive50":
    positive("P-ordinary-quotient.nl")
elif MODE == "positive0":
    positive("P-zero-numerator.nl")
elif MODE in ("zero", "range"):
    name = "N-zero-divisor.nl" if MODE == "zero" else "N-divisor-out-of-target.nl"
    negative(name, FREEZE["prospective_reject"][name])
elif MODE == "profile":
    for name, diagnostic in FREEZE["prospective_reject"].items():
        if name not in ("N-zero-divisor.nl", "N-divisor-out-of-target.nl"):
            negative(name, diagnostic)
elif MODE == "authenticity":
    run([PROBE, "attack", source("P-ordinary-quotient.nl")], selected=True)
elif MODE == "unknown":
    result = positive("P-unknown-event.nl")
    assert result["unknown_valid"] and result["forged_rejected"], result
elif MODE.endswith("-oom"):
    site = {"receipt-oom": 1, "result-oom": 2, "emitter-oom": 3}[MODE]
    record("oom_cases", ["failure and pristine retry", site], 2)
    result = run([PROBE, "oom", source("P-ordinary-quotient.nl"), site], selected=True)
    assert b"hits=1 no_clobber=PASS pristine_retry=PASS" in result.stdout
elif MODE == "native":
    gcc, clang = compilers()
    for name in ("P-ordinary-quotient.nl", "P-zero-numerator.nl", *list(FREEZE["extra_actual"])[:3]):
        symbols, e = positive(name), expected(name)
        result = run([CLI, source(name)], selected=True)
        assert not result.stderr
        text = result.stdout.decode()
        (WORK / (name + ".unmodified.c")).write_text(text)
        marker = "    return;\n}\n\nint main(void)"
        assert text.count(marker) == 1 and " / " in text and " + " in text
        reads = (f"    assert(nl_local_{symbols['groups']} == ((size_t)UINTMAX_C({e['quotient']})));\n"
                 f"    assert(nl_local_{symbols['successor']} == ((size_t)UINTMAX_C({e['successor']})));\n"
                 f"    assert(nl_local_{symbols['retained']} == ((size_t)UINTMAX_C({e['quotient']})));\n")
        observed = "#include <assert.h>\n" + text.replace(marker, reads + marker)
        configurations = [("gcc", gcc, None), ("clang", clang, None)]
        if name in ("P-ordinary-quotient.nl", "P-zero-numerator.nl"):
            configurations += [("asan", gcc, "address"), ("ubsan", clang, "undefined")]
        for label, compiler, sanitizer in configurations:
            guest(observed, name + "-" + label, compiler, sanitizer)
elif MODE == "controls":
    for name in ("C-known.nl", "C-guarded.nl"):
        run([CLI, "--check", source(name)], selected=True)
        text = run([CLI, source(name)], selected=True).stdout.decode()
        if name == "C-known.nl":
            assert " + " in text and " - " in text
        else:
            pattern = r"\(void\)(nl_qty_fn_\d+\(\));"
            assert len(re.findall(pattern, text)) == 1
            observed = "#include <assert.h>\n" + re.sub(pattern, r"assert(\1 == ((size_t)UINTMAX_C(50)));", text)
            for label, compiler in zip(("gcc", "clang"), compilers()):
                guest(observed, "guarded-" + label, compiler)
else:
    raise AssertionError(MODE)
print("P1544", MODE, "PASS", WORK)
