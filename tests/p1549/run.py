"""P1549 normal CLI, owned result, native values and failure regression tests."""
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parent
MODE, CLI, PROBE, OUTPUT = sys.argv[1:]
WORK = Path(OUTPUT) / str(time.time_ns())
WORK.mkdir(parents=True)
ORACLE = json.loads((ROOT / "oracle.json").read_text(encoding="utf-8"))
FREEZE = json.loads((ROOT / "freeze.json").read_text(encoding="utf-8"))
for name, expected in FREEZE["files"].items():
    path = ROOT / name if name == "oracle.json" else ROOT / "source" / name
    raw = path.read_bytes()
    assert len(raw) == expected["bytes"], name
    assert hashlib.sha256(raw).hexdigest() == expected["sha256"], name
assert os.sysconf("SC_LONG_BIT") == ORACLE["usize_target_bits"]

ENV = {
    **os.environ,
    "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
    "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1",
}
LIMITS = {"selected": 300, "native": 24, "oom_cases": 16}
sequence = 0


def charge(kind, command, amount=1):
    ledger_name = os.environ.get("P1549_LEDGER")
    if not ledger_name:
        return
    ledger = Path(ledger_name)
    with ledger.with_suffix(".lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        record = json.loads(ledger.read_text(encoding="utf-8"))
        count = record["counts"][kind] + amount
        assert count <= LIMITS[kind], (kind, count)
        record["counts"][kind] = count
        record["events"].append(
            dict(kind=kind, mode=MODE, command=command, charged=amount, raw=str(WORK))
        )
        ledger.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")


def run(command, success=True, guest=False):
    global sequence
    cmd = [str(part) for part in command]
    charge("selected", cmd)
    if guest:
        charge("native", cmd)
    sequence += 1
    stem = WORK / f"{sequence:03}"
    result = subprocess.run(cmd, capture_output=True, timeout=90, env=ENV)
    stem.with_suffix(".stdout").write_bytes(result.stdout)
    stem.with_suffix(".stderr").write_bytes(result.stderr)
    stem.with_suffix(".json").write_text(
        json.dumps({"command": cmd, "returncode": result.returncode}, indent=2) + "\n",
        encoding="utf-8",
    )
    assert (result.returncode == 0) == success, (
        cmd,
        result.returncode,
        result.stdout.decode(errors="replace"),
        result.stderr.decode(errors="replace"),
        str(stem),
    )
    return result


def source(name):
    return ROOT / "source" / name


def inspect(case):
    checked = run([CLI, "--check", source(case["source"])])
    assert not checked.stdout and not checked.stderr
    report = run([PROBE, "inspect", source(case["source"])])
    assert not report.stderr
    lines = report.stdout.decode().splitlines()
    assert "SOURCE PASS" in lines and "CHECK PASS" in lines
    assert any(line.startswith("DEFINITION unknown=") for line in lines)
    assert "WORLD seal=1 bytes=1" in lines

    calls = []
    formals = set()
    actual_ids = set()
    result_ids = set()
    bindings = {}
    for line in lines:
        if line.startswith("CALL "):
            _, actual, divisor, quotient, edge, value, formal, div_id, result, seal = line.split()
            assert seal == "seal=1"
            assert value not in actual_ids and formal not in formals
            assert result not in result_ids and int(div_id) < int(result)
            actual_ids.add(value)
            formals.add(formal)
            result_ids.add(result)
            calls.append(dict(actual=int(actual), divisor=int(divisor), quotient=int(quotient), edge=edge))
        elif line.startswith("BIND "):
            _, name, symbol, number, value = line.split()
            assert name not in bindings and int(value) > 0
            bindings[name] = (int(symbol), int(number))
    assert calls == case["calls"], calls
    assert {name: n for name, (_, n) in bindings.items()} == case["caller"]
    assert len({symbol for symbol, _ in bindings.values()}) == len(bindings)
    return {name: symbol for name, (symbol, _) in bindings.items()}


def guest_values(case, symbols, builds):
    name = case["source"]
    original = WORK / (name + ".c")
    result = run([CLI, "--emit-c17-to", original, source(name)])
    assert not result.stdout and not result.stderr
    program = original.read_text(encoding="utf-8")
    assert program.count(" / ") == 1 and "nonzero divisor" in program
    finish = "    return;\n}\n\nint main(void)\n"
    assert program.count(finish) == 1
    # Observe the genuine computed locals without changing the guest operations.
    observations = "".join(
        f'    printf("{key}=%zu\\n", nl_local_{symbols[key]});\n'
        for key in case["caller"]
    )
    instrumented = WORK / (name + ".observed.c")
    instrumented.write_text(
        "#include <stdio.h>\n" + program.replace(finish, observations + finish),
        encoding="utf-8",
    )
    wanted = "".join(f"{key}={value}\n" for key, value in case["caller"].items())
    for tag, cc, flags in builds:
        exe = WORK / (name + "." + tag)
        run([cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             *flags, instrumented, "-o", exe])
        execution = run([exe], guest=True)
        assert execution.stdout.decode() == wanted and not execution.stderr, (tag, name)
        print(f"{name} {tag} values PASS", flush=True)


def positive():
    gcc = shutil.which("gcc")
    clang = shutil.which("clang-23") or shutil.which("clang")
    assert gcc and clang, "P1549 requires GCC and Clang"
    builds = [
        ("gcc", gcc, ["-O2"]),
        ("clang", clang, ["-O2"]),
        ("asan", gcc, ["-O1", "-fsanitize=address", "-fno-omit-frame-pointer", "-no-pie"]),
        ("ubsan", gcc, ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"]),
    ]
    for case in ORACLE["positive"]:
        guest_values(case, inspect(case), builds)


def negative():
    for case in ORACLE["negative"]:
        name = case["source"]
        diagnostic = case["diagnostic"].encode()
        result = run([CLI, "--check", source(name)], success=False)
        assert not result.stdout and diagnostic in result.stderr
        dest = WORK / (name + ".c")
        sentinel = b"existing C17 output\n"
        dest.write_bytes(sentinel)
        before = dest.stat()
        result = run([CLI, "--emit-c17-to", dest, source(name)], success=False)
        after = dest.stat()
        assert not result.stdout and diagnostic in result.stderr
        assert dest.read_bytes() == sentinel
        assert (before.st_ino, before.st_mtime_ns) == (after.st_ino, after.st_mtime_ns)
        assert not list(WORK.glob(dest.name + ".tmp*"))
        print(f"{name} {case['diagnostic']} no-clobber PASS")


if MODE == "positive":
    positive()
elif MODE == "negative":
    negative()
elif MODE == "authenticity":
    report = run([PROBE, MODE, source("call-reuse.nl")])
    assert not report.stderr and b"foreign receipt and equal-value wrong result rejected" in report.stdout
    print(report.stdout.decode(), end="")
elif MODE in {"second-seal", "world-seal", "staging"}:
    command = [PROBE, MODE, source("call-reuse.nl")]
    charge("oom_cases", command, amount=2)  # Injection and pristine retry.
    report = run(command)
    assert b"OOM hit=1 no-clobber=PASS retry=PASS" in report.stdout
    if MODE != "staging":
        assert b"P3-OUT-OF-MEMORY" in report.stderr
    else:
        assert not report.stderr
    print(report.stdout.decode(), end="")
else:
    raise AssertionError(MODE)
