"""Untouched C17, raw first, then passive actual-H/current observations."""
import hashlib
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

compiler, cc = sys.argv[1:3]
root = Path(__file__).resolve().parent
parent = Path(sys.argv[3])
parent.mkdir(parents=True, exist_ok=True)
work = Path(tempfile.mkdtemp(prefix="run-", dir=parent))
smoke = len(sys.argv) == 5 and sys.argv[4] == "--smoke"
generated = work / "original.c"
emit = subprocess.run([compiler, "--emit-c17-to", str(generated), str(root / "positive.nl")],
                      capture_output=True)
(work / "emit.stdout").write_bytes(emit.stdout)
(work / "emit.stderr").write_bytes(emit.stderr)
assert emit.returncode == 0 and not emit.stdout and not emit.stderr, emit
original = generated.read_bytes()
digest = hashlib.sha256(original).hexdigest()
print("actual generated C SHA256", digest)


def build(level, passive):
    name = ("passive" if passive else "raw") + "-O" + str(level)
    output = work / name
    command = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    command += shlex.split(os.environ.get("CFLAGS", ""))
    command += ["-UNDEBUG", "-O" + str(level), str(generated)]
    if passive:
        command += [str(root / "allocator.c"), "-Wl,--wrap=malloc,--wrap=free"]
        for flag in ["NEWLANG_NODE_OBSERVER", "NEWLANG_HEAP_OBSERVER",
                     "NEWLANG_CURRENT_LOOP_OBSERVER"]:
            command.append(f'-D{flag}="{root / "observer.h"}"')
    command += ["-o", str(output)]
    built = subprocess.run(command, capture_output=True)
    (work / (name + ".build.stdout")).write_bytes(built.stdout)
    (work / (name + ".build.stderr")).write_bytes(built.stderr)
    assert built.returncode == 0, built.stderr.decode()
    assert generated.read_bytes() == original
    return output


def run(binary, world, passive):
    env = os.environ.copy()
    env["P1312_WORLD"] = str(world)
    result = subprocess.run([str(binary)], env=env, capture_output=True, timeout=15)
    key = binary.name + "-H" + str(world)
    (work / (key + ".stdout")).write_bytes(result.stdout)
    (work / (key + ".stderr")).write_bytes(result.stderr)
    assert result.returncode == 0 and not result.stderr, (key, result)
    if passive:
        assert f"PASSIVE H{world} ".encode() in result.stdout
        print(result.stdout.decode(), end="")
    assert generated.read_bytes() == original


if smoke:
    # Local apparatus registration check only. Normal CTest always runs the
    # complete two-optimization / all-allocation-world family below.
    run(build(2, True), 2, True)
else:
    for level in [0, 2]:
        run(build(level, False), 2, False)
    for level in [0, 2]:
        binary = build(level, True)
        for world in [0, 1, 2]:
            run(binary, world, True)
