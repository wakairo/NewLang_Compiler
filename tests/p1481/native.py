"""Ordinary C17 bytes, strict raw and passive physical-identity native."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

compiler, cc, tests_arg, output_arg, sanitizer, *existing = sys.argv[1:]
tests = Path(tests_arg).resolve()
base = Path(output_arg)
base.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix="run-", dir=base))
rows = []
env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")


def digest(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def run(label, argv, guest=False):
    row = dict(label=label, argv=[str(x) for x in argv], guest=guest,
               state="started")
    rows.append(row)
    (out / "ledger.json").write_text(json.dumps(rows, indent=2) + "\n")
    r = subprocess.run(row["argv"], capture_output=True, env=env)
    for key, data in [("stdout", r.stdout), ("stderr", r.stderr)]:
        (out / (label + "." + key)).write_bytes(data)
        row[key + "_sha256"] = hashlib.sha256(data).hexdigest()
    row.update(state="completed", exit=r.returncode)
    (out / "ledger.json").write_text(json.dumps(rows, indent=2) + "\n")
    if r.returncode:
        raise SystemExit(f"{label}: exit{r.returncode}\n" +
                         r.stderr.decode(errors="replace"))
    return r


oracle = json.loads((tests / "oracle.json").read_text())
for name, expected in oracle["inputs"].items():
    p = tests / name
    assert p.stat().st_size == expected["bytes"]
    assert digest(p) == expected["sha256"]
names = ["P1468-B2-after-cleanup-in-success-arm.nl", "Z-success-zero.nl",
         "I-field-index-one.nl"]
if existing:
    assert len(existing) == 3
    raws = [Path(p).resolve() for p in existing]
else:
    raws = []
    for i, name in enumerate(names):
        p = out / (name + ".c")
        p.write_bytes(b"P1481 prospective no-clobber sentinel\n")
        run(f"emit-{i}", [compiler, "--emit-c17-to", p, tests / name])
        raws.append(p)
hashes = [digest(p) for p in raws]
strict = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
flags = [] if sanitizer == "none" else [f"-fsanitize={sanitizer}",
          "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
exe = out / "raw-O2"
run("raw-compile", [cc, *strict, "-O2", *flags, raws[0], "-o", exe])
run("raw-native", [exe], True)
# Both optimizations exercise all allocation worlds. The independent zero
# sample and declaration-index variant exercise genuine H2, not a sentinel.
profiles = [(0, opt, fail) for opt in [0, 2] for fail in [1, 2, 0]]
profiles += [(variant, opt, 0) for variant in [1, 2] for opt in [0, 2]]
for variant, opt, fail in profiles:
    name = f"passive-v{variant}-O{opt}-fail{fail}"
    exe = out / name
    run(name + "-compile", [cc, *strict, f"-O{opt}", *flags,
        f'-DNEWLANG_NODE_OBSERVER="{tests / "passive.h"}"',
        f'-DNEWLANG_HEAP_OBSERVER="{tests / "passive.h"}"',
        f"-DP1463_FAIL_AT={fail}",
        f"-DP1481_SAMPLE={0 if variant == 1 else 181}",
        f"-DP1463_FIELD_INDEX={1 if variant == 2 else 0}",
        raws[variant], tests / "../p1463/guest_wrap.c", "-Wl,--wrap=malloc",
        "-Wl,--wrap=free", "-o", exe])
    r = run(name, [exe], True)
    assert b"P1481" in r.stdout and b"PASS" in r.stdout
    assert [digest(p) for p in raws] == hashes
print("PASS P1481 raw/passive normal H0/H1/H2, zero, index1:", out)
