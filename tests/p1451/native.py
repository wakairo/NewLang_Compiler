"""Actual ordinary CLI output: unchanged raw C and passive H/T observations."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

compiler, cc, root_arg, output_arg = sys.argv[1:]
root = Path(root_arg)
tests = root / "tests/p1451"
base = Path(output_arg)
base.mkdir(parents=True, exist_ok=True)
output = Path(tempfile.mkdtemp(prefix="run-", dir=base))
rows = []
env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")


def run(command, label, guest=False, expected=0):
    row = {"label": label, "command": [str(x) for x in command],
           "guest": guest, "state": "started"}
    rows.append(row)
    (output / "ledger.json").write_text(json.dumps(rows, indent=2) + "\n")
    result = subprocess.run(command, capture_output=True, env=env)
    (output / (label + ".stdout")).write_bytes(result.stdout)
    (output / (label + ".stderr")).write_bytes(result.stderr)
    row.update(state="completed", exit=result.returncode)
    (output / "ledger.json").write_text(json.dumps(rows, indent=2) + "\n")
    if result.stdout:
        print(result.stdout.decode(), end="", flush=True)
    if result.returncode != expected:
        print(result.stderr.decode(), flush=True)
        raise SystemExit(f"{label}: {result.returncode}, expected {expected}")


generated = {}
sources = {"A": tests / "A-front-install.nl",
           "B": tests / "B-back-alias-install.nl",
           "L": tests / "L-literal-install.nl",
           "C": root / "tests/p1441/P-some-read.nl"}
reuse = os.environ.get("P1451_GENERATED")
if reuse:
    prior = Path(reuse)
    manifest = json.loads((prior / "generated.json").read_text())
    for name in sources:
        target = prior / (name + ".raw.c")
        assert hashlib.sha256(target.read_bytes()).hexdigest() == manifest[name]
        generated[name] = target
for name, source in (() if reuse else sources.items()):
    target = output / (name + ".raw.c")
    run([compiler, "--emit-c17-to", str(target), str(source)], "emit-" + name)
    generated[name] = target
    digest = hashlib.sha256(target.read_bytes()).hexdigest()
    rows[-1]["raw_sha256"] = digest
    if name == "C":
        assert digest == "4e45981c33257e1e0b4cfb6183d1d32ff8a03b14b90851444a3ce4059b6d836f"

for source in (() if reuse else ("N-wrong-domain.nl", "N-loan-escape.nl")):
    target = output / (source + ".sentinel")
    sentinel = b"P1451 existing named output\n"
    target.write_bytes(sentinel)
    run([compiler, "--emit-c17-to", str(target), str(root / "tests/p1441" / source)],
        source, expected=3)
    assert target.read_bytes() == sentinel
    rows[-1]["named_no_clobber"] = True

if not reuse:
    (output / "generated.json").write_text(json.dumps({
        name: hashlib.sha256(path.read_bytes()).hexdigest()
        for name, path in generated.items()}, indent=2) + "\n")
if os.environ.get("P1451_PREPARE_ONLY"):
    print("PREPARED (guest NOT_RUN)", output)
    raise SystemExit(0)

compilers = [cc]
extra = os.environ.get("P1451_EXTRA_CC")
if extra and Path(extra).resolve() != Path(cc).resolve():
    compilers.append(extra)
strict = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
for index, native_cc in enumerate(compilers):
    for name in ("A", "B"):
        for optimization in ("O0", "O2"):
            label = f"raw-{index}-{name}-{optimization}"
            exe = output / label
            run([native_cc, *strict, "-" + optimization,
                 str(generated[name]), "-o", str(exe)], label + "-build")
            run([str(exe)], label, guest=True)

for name, fail, native_cc in (("A", 0, cc), ("B", 0, compilers[-1]),
                              ("A", 1, cc), ("A", 2, cc)):
    label = f"passive-{name}-H{2 if not fail else fail - 1}"
    exe = output / label
    raw = generated[name]
    original = hashlib.sha256(raw.read_bytes()).hexdigest()
    flags = [native_cc, *strict, "-O2", "-fno-omit-frame-pointer"]
    if name == "B":
        flags += ["-fsanitize=address,undefined"]
    flags += [f'-DNEWLANG_NODE_OBSERVER="{tests / "passive.h"}"',
              f'-DNEWLANG_HEAP_OBSERVER="{tests / "passive.h"}"',
              f"-DP1451_FAIL_AT={fail}", f"-DP1451_CASE_BACK={int(name == 'B')}"]
    run([*flags, str(raw), str(root / "tests/p1441/guest_wrap.c"),
         f"-DP1441_FAIL_AT={fail}", "-Wl,--wrap=malloc", "-o", str(exe)],
        label + "-build")
    run([str(exe)], label, guest=True)
    assert hashlib.sha256(raw.read_bytes()).hexdigest() == original

(output / "ledger.json").write_text(json.dumps(rows, indent=2) + "\n")
print("PASS ordinary CLI/C17, negative no-clobber, raw/passive native", output)
