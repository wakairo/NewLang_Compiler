"""Ordinary newlangc output, unchanged strict raw C and passive observations."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

compiler, cc, root_arg, output_arg = sys.argv[1:]
root = Path(root_arg)
tests = root / "tests/p1441"
base = Path(output_arg)
base.mkdir(parents=True, exist_ok=True)
output = Path(tempfile.mkdtemp(prefix="run-", dir=base))
rows = []
ledger = output / "ledger.json"
env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")


def run(command, label, guest=False):
    row = {"label": label, "command": [str(x) for x in command],
           "guest": guest, "state": "started"}
    rows.append(row)
    ledger.write_text(json.dumps(rows, indent=2) + "\n")
    result = subprocess.run(command, capture_output=True, env=env)
    (output / (label + ".stdout")).write_bytes(result.stdout)
    (output / (label + ".stderr")).write_bytes(result.stderr)
    row.update(state="completed", exit=result.returncode)
    ledger.write_text(json.dumps(rows, indent=2) + "\n")
    if result.stdout:
        print(result.stdout.decode(), end="", flush=True)
    if result.returncode:
        print(result.stderr.decode(), flush=True)
        raise SystemExit(result.returncode)


sources = root / "tests/p1441"
generated = {}
for name, source in (("control", "C-direct.nl"),
                     ("some", "P-some-read.nl"),
                     ("none", "P-none-read.nl")):
    target = output / (name + ".raw.c")
    run([compiler, "--emit-c17-to", str(target), str(sources / source)],
        "emit-" + name)
    generated[name] = target
    digest = hashlib.sha256(target.read_bytes()).hexdigest()
    rows[-1]["raw_sha256"] = digest
    if name == "control":
        assert digest == "e42422d7c0a85fc051ab1e0b4281f127cd5b6561ccfe7add0c1e25a738c9a896"

# Named-file refusal uses real invalid source, separate from private graph
# tamper API tests. The old bytes survive semantic rejection unchanged.
for source in ("N-wrong-domain.nl", "N-loan-escape.nl", "N-owner-consumed.nl"):
    target = output / (source + ".sentinel")
    sentinel = b"P1441 existing named output\n"
    target.write_bytes(sentinel)
    command = [compiler, "--emit-c17-to", str(target), str(sources / source)]
    result = subprocess.run(command, capture_output=True, env=env)
    (output / (source + ".stdout")).write_bytes(result.stdout)
    (output / (source + ".stderr")).write_bytes(result.stderr)
    rows.append({"label": source, "command": command, "exit": result.returncode,
                 "named_no_clobber": target.read_bytes() == sentinel})
    ledger.write_text(json.dumps(rows, indent=2) + "\n")
    assert result.returncode == 3 and target.read_bytes() == sentinel

compilers = [cc]
extra = os.environ.get("P1441_EXTRA_CC")
if extra and Path(extra).resolve() != Path(cc).resolve():
    compilers.append(extra)
strict = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
for ci, native_cc in enumerate(compilers):
    for name in ("some", "none"):
        for optimization in ("O0", "O2"):
            label = f"raw-{ci}-{name}-{optimization}"
            exe = output / label
            run([native_cc, *strict, "-" + optimization, str(generated[name]),
                 "-o", str(exe)], label + "-build")
            run([str(exe)], label, guest=True)

profiles = [("some", 0, cc), ("none", 0, compilers[-1]),
            ("some", 1, cc), ("some", 2, cc)]
for name, fail, native_cc in profiles:
    label = f"passive-{name}-fail{fail}"
    exe = output / label
    raw = generated[name]
    original_sha = hashlib.sha256(raw.read_bytes()).hexdigest()
    flags = [native_cc, *strict, "-O2", "-fno-omit-frame-pointer"]
    if name == "none" and fail == 0:
        flags += ["-fsanitize=address,undefined"]
    flags += [f'-DNEWLANG_NODE_OBSERVER="{tests / "passive.h"}"',
              f'-DNEWLANG_HEAP_OBSERVER="{tests / "passive.h"}"',
              f"-DP1441_FAIL_AT={fail}",
              f"-DP1441_CASE_NONE={int(name == 'none')}"]
    run([*flags, str(raw), str(tests / "guest_wrap.c"),
         "-Wl,--wrap=malloc", "-o", str(exe)], label + "-build")
    run([str(exe)], label, guest=True)
    assert original_sha == hashlib.sha256(raw.read_bytes()).hexdigest()

guests = sum(bool(row.get("guest")) for row in rows)
assert guests == 4 * len(compilers) + 4
print(f"P1441 native PASS guest={guests}; raw C unmodified; {output}")
