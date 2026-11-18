"""Unchanged ordinary CLI bytes; passive observations and allocation faults only."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

compiler, cc, tests_arg, base_arg, sanitizer, *existing = sys.argv[1:]
tests = Path(tests_arg).resolve()
base = Path(base_arg).resolve()
base.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix="run-", dir=base))
rows = []
env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")


def run(label, argv, guest=False, fail=0, expected=0):
    row = dict(label=label, argv=[str(x) for x in argv], guest=guest,
               fail_at=fail, expected=expected, state="started")
    rows.append(row)
    (out / "ledger.json").write_text(json.dumps(rows, indent=2) + "\n")
    command = row["argv"]
    if not guest and os.environ.get("P1507_LOCAL_LEDGER"):
        command = [sys.executable, "/workspace/p1507-evidence/run.py",
                   out.name + "-" + label, *command]
    r = subprocess.run(command, capture_output=True,
                       env=dict(env, P1507_FAIL_AT=str(fail)))
    for suffix, data in [("stdout", r.stdout), ("stderr", r.stderr)]:
        (out / (label + "." + suffix)).write_bytes(data)
    row.update(state="completed", exit=r.returncode)
    (out / "ledger.json").write_text(json.dumps(rows, indent=2) + "\n")
    assert r.returncode == expected, (row, r.stdout, r.stderr)
    return r


names = ["A-direct-zero.nl", "B-direct-index-one.nl"]
raws = [Path(x).resolve() for x in existing]
if not raws:
    for name in names:
        raw = out / (name + ".c")
        raw.write_bytes(b"P1507 named output sentinel\n")
        run("emit-" + name, [compiler, "--emit-c17-to", raw, tests / name])
        raws.append(raw)
assert len(raws) == 2
hashes = [hashlib.sha256(p.read_bytes()).hexdigest() for p in raws]
strict = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
flags = [] if sanitizer == "none" else [f"-fsanitize={sanitizer}",
          "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
if sanitizer == "none" and not os.environ.get("P1507_RESUME_GCC"):
    raw_exe = out / "raw"
    run("raw-compile", [cc, *strict, "-O2", raws[0], "-o", raw_exe])
    run("raw-native", [raw_exe], True)
profiles = [(i, o) for i in range(2) for o in [0, 2]] if sanitizer == "none" else [(0, 2)]
if os.environ.get("P1507_RESUME_GCC"):
    profiles = [(0, 2), (1, 0), (1, 2)]
for index, opt in profiles:
    exe = out / f"v{index}-O{opt}"
    run(exe.name + "-compile", [cc, *strict, f"-O{opt}", *flags,
        f'-DNEWLANG_NODE_OBSERVER="{tests / "passive.h"}"',
        f'-DNEWLANG_HEAP_OBSERVER="{tests / "passive.h"}"',
        f"-DP1507_INDEX={index}", raws[index], tests / "guest_wrap.c",
        "-Wl,--wrap=malloc", "-Wl,--wrap=free", "-o", exe])
    failures = [0] if os.environ.get("P1507_RESUME_GCC") and index == 0 else [1, 2, 0]
    for fail in failures:
        r = run(exe.name + f"-fail{fail}", [exe], True, fail)
        assert b"P1507" in r.stdout and b"PASS" in r.stdout
assert [hashlib.sha256(p.read_bytes()).hexdigest() for p in raws] == hashes
print("P1507 strict/raw/passive native PASS:", out)
