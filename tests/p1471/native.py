"""Frozen source -> ordinary emitted C, strict raw/passive native. No C edits."""
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


def run(label, args):
    args = [str(x) for x in args]
    row = dict(label=label, argv=args, state="started")
    rows.append(row)
    (out / "ledger.json").write_text(json.dumps(rows, indent=2) + "\n")
    r = subprocess.run(args, capture_output=True, env=env)
    for name, data in [("stdout", r.stdout), ("stderr", r.stderr)]:
        (out / (label + "." + name)).write_bytes(data)
    row.update(state="completed", exit=r.returncode)
    (out / "ledger.json").write_text(json.dumps(rows, indent=2) + "\n")
    if r.returncode:
        raise SystemExit(f"{label}: exit{r.returncode}\n" + r.stderr.decode(errors="replace"))
    return r


oracle = json.loads((tests / "oracle.json").read_text())
for name, v in oracle["inputs"].items():
    p = tests / name
    assert p.stat().st_size == v["bytes"] and digest(p) == v["sha256"]
raws = []
for i, source in enumerate(["P1468-B1-failure-return.nl", "Z-selected-zero.nl"]):
    if existing:
        assert len(existing) == 2
        raw = Path(existing[i]).resolve()
    else:
        raw = out / (source + ".c")
        raw.write_bytes(b"P1471 uncommitted output\n")
        run("emit-" + str(i), [compiler, "--emit-c17-to", raw, tests / source])
    raws.append((raw, digest(raw)))
strict = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
flags = [] if sanitizer == "none" else [f"-fsanitize={sanitizer}",
          "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
raw, sha = raws[0]
exe = out / "raw-O2"
run("raw-compile", [cc, *strict, "-O2", *flags, raw, "-o", exe])
run("raw-native", [exe])
assert digest(raw) == sha
for variant, opt, fail in [(0, opt, fail) for opt in [0, 2] for fail in [1, 2, 0]] + [(1, 2, 0)]:
    raw, sha = raws[variant]
    name = f"passive-v{variant}-O{opt}-fail{fail}"
    exe = out / name
    run(name + "-compile", [cc, *strict, f"-O{opt}", *flags,
        f'-DNEWLANG_NODE_OBSERVER="{tests / "passive.h"}"',
        f'-DNEWLANG_HEAP_OBSERVER="{tests / "passive.h"}"',
        f"-DP1463_FAIL_AT={fail}", f"-DP1471_SAMPLE={0 if variant else 181}",
        raw, tests / "../p1463/guest_wrap.c", "-Wl,--wrap=malloc",
        "-Wl,--wrap=free", "-o", exe])
    r = run(name, [exe])
    assert b"P1471" in r.stdout and b"PASS" in r.stdout and digest(raw) == sha
print("PASS P1471 strict raw/passive H0/H1/H2 and true zero-result", out)
