"""Ordinary CLI -> unchanged C17 -> strict raw/passive guests, no value injection."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

compiler, cc, tests_arg, output_arg, sanitizer = sys.argv[1:]
tests = Path(tests_arg).resolve()
base = Path(output_arg)
base.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix="run-", dir=base))
rows = []
env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(label, command, expected=0):
    command = [str(arg) for arg in command]
    row = {"label": label, "command": command, "state": "started"}
    rows.append(row)
    (out / "ledger.json").write_text(json.dumps(rows, indent=2) + "\n")
    result = subprocess.run(command, capture_output=True, env=env)
    (out / (label + ".stdout")).write_bytes(result.stdout)
    (out / (label + ".stderr")).write_bytes(result.stderr)
    row.update(state="completed", exit=result.returncode)
    (out / "ledger.json").write_text(json.dumps(rows, indent=2) + "\n")
    if result.returncode != expected:
        raise SystemExit(f"{label}: exit{result.returncode}, expected{expected}\n"
                         + result.stdout.decode(errors="replace")
                         + result.stderr.decode(errors="replace"))
    return result


oracle = json.loads((tests / "oracle.json").read_text())
for name, receipt in oracle["inputs"].items():
    source = tests / name
    assert source.stat().st_size == receipt["bytes"]
    assert digest(source) == receipt["sha256"]

generated = {}
for shape, name in [("A", "A-post-cleanup.nl"),
                    ("B", "B-field-index-one.nl"),
                    ("C", "C-option-bool-true.nl")]:
    target = out / (shape + ".raw.c")
    sentinel = b"P1463 uncommitted output\n"
    target.write_bytes(sentinel)
    run("emit-" + shape, [compiler, "--emit-c17-to", target, tests / name])
    assert target.read_bytes() != sentinel
    generated[shape] = (target, digest(target))

for name, code in [("N-wrong-domain.nl", "ALLOCATED-DOMAIN-MISMATCH"),
                   ("N-escaped-write-loan.nl", "P8-EXIT-DEPENDENCY"),
                   ("N-missing-cleanup.nl", "P5-SCOPE-OBLIGATION")]:
    target = out / (name + ".sentinel")
    sentinel = b"P1463 existing output must remain\n"
    target.write_bytes(sentinel)
    result = run(name, [compiler, "--emit-c17-to", target, tests / name], expected=3)
    assert code.encode() in result.stderr and target.read_bytes() == sentinel

strict = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
flags = [] if sanitizer == "none" else [f"-fsanitize={sanitizer}",
          "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
for optimization in [0, 2]:
    raw, original = generated["A"]
    exe = out / f"raw-A-O{optimization}"
    run(exe.name + "-compile", [cc, *strict, f"-O{optimization}", *flags,
                              raw, "-o", exe])
    run(exe.name, [exe])
    assert digest(raw) == original

profiles = [("A", optimization, fault) for optimization in [0, 2]
            for fault in [1, 2, 0]] + [("B", 2, 0), ("C", 2, 0)]
for shape, optimization, fault in profiles:
    raw, original = generated[shape]
    exe = out / f"passive-{shape}-O{optimization}-fault{fault}"
    defines = [f'-DNEWLANG_NODE_OBSERVER="{tests / "passive.h"}"',
               f'-DNEWLANG_HEAP_OBSERVER="{tests / "passive.h"}"',
               f"-DP1463_FAIL_AT={fault}",
               f"-DP1463_FIELD_INDEX={int(shape == 'B')}",
               f"-DP1463_BOOL_TRUE={int(shape == 'C')}"]
    run(exe.name + "-compile", [cc, *strict, f"-O{optimization}", *flags,
        *defines, raw, tests / "guest_wrap.c", "-Wl,--wrap=malloc",
        "-Wl,--wrap=free", "-o", exe])
    result = run(exe.name, [exe])
    assert b"PASS" in result.stdout and digest(raw) == original
print("PASS CLI/semantic-negative/no-clobber/raw/passive", out)
