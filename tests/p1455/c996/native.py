"""C996 noncoincident-prefix D and original B: actual unmodified CLI C17."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

compiler, cc, root_arg, output_arg = sys.argv[1:]
root = Path(root_arg)
tests = root / "tests/p1455"
base = Path(output_arg)
base.mkdir(parents=True, exist_ok=True)
output = Path(tempfile.mkdtemp(prefix="run-", dir=base))
env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
rows = []


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, label, guest=False):
    row = {"command": [str(x) for x in command], "label": label,
           "state": "started", "guest": guest}
    rows.append(row)
    (output / "ledger.json").write_text(json.dumps(rows, indent=2) + "\n")
    result = subprocess.run(command, capture_output=True, env=env)
    (output / (label + ".stdout")).write_bytes(result.stdout)
    (output / (label + ".stderr")).write_bytes(result.stderr)
    row.update(state="completed", exit=result.returncode)
    (output / "ledger.json").write_text(json.dumps(rows, indent=2) + "\n")
    if result.stdout:
        print(result.stdout.decode(), end="", flush=True)
    if result.returncode:
        print(result.stderr.decode(), flush=True)
        raise SystemExit(label + ": " + str(result.returncode))


sources = {"D": tests / "c996/D-noncoincident-prefix.nl",
           "A": tests / "A-enabled.nl",
           "B": tests / "B-disabled-alias.nl",
           "L": root / "tests/p1451/A-front-install.nl"}
expected = {"A": "2b1d0ded08205c7e859df59f774c01e6bf885ae96c8a6be4ff824d3a98fec395",
            "B": "5914c59b48f898ebbb3dd253e277ecc0fa93b58016b9a82273ec8df227812dc7",
            "L": "2b39d20bdb811ba96b6351a22faaa5161c06592fd8dce05441042c2a3a60e592"}
reuse = os.environ.get("P1455_C996_GENERATED")
prior = Path(reuse) if reuse else output
if reuse:
    manifest = json.loads((prior / "generated.json").read_text())
    inputs = json.loads((prior / "sources.json").read_text())
    for name, path in sources.items():
        assert digest(path) == inputs[name]
        assert digest(prior / (name + ".raw.c")) == manifest[name]
else:
    for name, source in sources.items():
        raw = output / (name + ".raw.c")
        run([compiler, "--emit-c17-to", str(raw), str(source)], "emit-" + name)
        if name in expected:
            assert digest(raw) == expected[name]
    (output / "generated.json").write_text(json.dumps({
        name: digest(output / (name + ".raw.c")) for name in sources}, indent=2) + "\n")
    (output / "sources.json").write_text(json.dumps({
        name: digest(path) for name, path in sources.items()}, indent=2) + "\n")
if os.environ.get("P1455_C996_PREPARE_ONLY"):
    print("PREPARED, guest NOT_RUN", output)
    raise SystemExit(0)

compilers = [cc]
extra = os.environ.get("P1455_C996_EXTRA_CC")
if extra and Path(extra).resolve() != Path(cc).resolve():
    compilers.append(extra)
strict = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2"]
for index, native_cc in enumerate(compilers):
    for name in ("D", "B"):
        raw = prior / (name + ".raw.c")
        original = digest(raw)
        header = tests / "c996/passive.h" if name == "D" else tests / "passive.h"
        label = f"{name}-{index}-physical-O2"
        exe = output / label
        flags = [native_cc, *strict, "-fno-omit-frame-pointer"]
        if "clang" in Path(native_cc).name:
            flags += ["-fsanitize=address,undefined"]
        flags += [f'-DNEWLANG_NODE_OBSERVER="{header}"',
                  f'-DNEWLANG_HEAP_OBSERVER="{header}"',
                  "-DP1455_FAIL_AT=0", f"-DP1455_CASE_BACK={int(name == 'B')}"]
        run([*flags, str(raw), str(root / "tests/p1441/guest_wrap.c"),
             "-DP1441_FAIL_AT=0", "-Wl,--wrap=malloc", "-o", str(exe)], label + "-build")
        run([str(exe)], label, guest=True)
        assert original == digest(raw)
print("PASS C996 exact raw/physical native", output)
