"""Ordinary emitted C bytes, passive original/receiver observer, real failures."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

compiler, cc, tests_arg, output_arg, sanitizer = sys.argv[1:]
tests = Path(tests_arg).resolve()
Path(output_arg).mkdir(parents=True, exist_ok=True)
output = Path(tempfile.mkdtemp(prefix="p1517-", dir=output_arg))
ledger = []


def run(label, argv, guest=False, fail=0):
    env = dict(os.environ, P1517_FAIL_AT=str(fail),
               ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    result = subprocess.run([str(x) for x in argv], env=env, capture_output=True)
    for suffix, data in [("stdout", result.stdout), ("stderr", result.stderr)]:
        (output / (label + "." + suffix)).write_bytes(data)
    ledger.append(dict(label=label, argv=[str(x) for x in argv], guest=guest,
                       fail_at=fail, exit=result.returncode))
    (output / "ledger.json").write_text(json.dumps(ledger, indent=2) + "\n")
    assert result.returncode == 0, (ledger[-1], result.stdout, result.stderr)
    if guest:
        assert b"receiver-exact PASS" in result.stdout


strict = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
instrument = [] if sanitizer == "none" else [f"-fsanitize={sanitizer}",
    "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
for variant, sample in [("zero", 0), ("nonzero", 42)]:
    raw = output / (variant + ".c")
    run("emit-" + variant, [compiler, "--emit-c17-to", raw,
                            tests / ("P-" + variant + ".nl")])
    digest = hashlib.sha256(raw.read_bytes()).hexdigest()
    for opt in [0, 2]:
        exe = output / f"{variant}-O{opt}"
        run(exe.name + "-cc", [cc, *strict, f"-O{opt}", *instrument,
            f"-DP1517_SAMPLE={sample}",
            f'-DNEWLANG_NODE_OBSERVER="{tests / "passive-v2.h"}"',
            f'-DNEWLANG_HEAP_OBSERVER="{tests / "passive-v2.h"}"',
            raw, tests / "guest_wrap.c", "-Wl,--wrap=malloc",
            "-Wl,--wrap=free", "-o", exe])
        for fail in [1, 2, 0]:
            run(exe.name + f"-fail{fail}", [exe], True, fail)
    assert hashlib.sha256(raw.read_bytes()).hexdigest() == digest
print("P1517 native two sources x O0/O2 x H0/H1/H2 PASS", output)
