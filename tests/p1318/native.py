"""Untouched C17: raw first, then passive physical/value observations."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

compiler, cc, parent = sys.argv[1:4]
prepare_only = sys.argv[4:] == ["--prepare-only"]
assert not sys.argv[4:] or prepare_only
root = Path(__file__).resolve().parent
Path(parent).mkdir(parents=True, exist_ok=True)
work = Path(tempfile.mkdtemp(prefix="run-", dir=parent))
commands = []
for profile, name in enumerate(["A-index0.nl", "B-index1.nl"]):
    original = work / (name + ".c")
    emitted = subprocess.run([compiler, "--emit-c17-to", str(original), str(root / name)],
                             capture_output=True, timeout=30)
    (work / (name + ".emit.stdout")).write_bytes(emitted.stdout)
    (work / (name + ".emit.stderr")).write_bytes(emitted.stderr)
    assert emitted.returncode == 0 and not emitted.stdout and not emitted.stderr, emitted
    data = original.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    for passive in [False, True]:
        for level in [0, 2]:
            key = name + ("-passive" if passive else "-raw") + "-O" + str(level)
            binary = work / key
            command = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                       "-UNDEBUG", "-O" + str(level)]
            command += shlex.split(os.environ.get("CFLAGS", ""))
            if passive:
                command += [f"-DP1318_PROFILE={profile}",
                            f'-DNEWLANG_CURRENT_LOOP_OBSERVER="{root / "observer.h"}"']
            command += [str(original), "-o", str(binary)]
            build = subprocess.run(command, capture_output=True, timeout=60)
            (work / (key + ".build.stdout")).write_bytes(build.stdout)
            (work / (key + ".build.stderr")).write_bytes(build.stderr)
            assert build.returncode == 0, build.stderr.decode()
            assert original.read_bytes() == data
            commands.append({"binary": str(binary), "passive": passive,
                             "source": name, "generated": str(original), "sha256": digest})
(work / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
print(work, flush=True)
if not prepare_only:
    for row in commands:
        binary = Path(row["binary"])
        result = subprocess.run([str(binary)], capture_output=True, timeout=15)
        (work / (binary.name + ".stdout")).write_bytes(result.stdout)
        (work / (binary.name + ".stderr")).write_bytes(result.stderr)
        assert result.returncode == 0 and not result.stderr, result
        assert not row["passive"] or b"PASSIVE original1 backedges2 stores3" in result.stdout
        assert hashlib.sha256(Path(row["generated"]).read_bytes()).hexdigest() == row["sha256"]
        print(binary.name, "PASS", flush=True)
