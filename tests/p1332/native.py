"""Strict untouched C first; passive hooks read real callee/caller values."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

p = argparse.ArgumentParser()
p.add_argument("compiler")
p.add_argument("cc")
p.add_argument("parent")
p.add_argument("--mode", choices=["both", "raw", "passive", "sanitize"], default="both")
p.add_argument("--prepare-only", action="store_true")
a = p.parse_args()
root = Path(__file__).resolve().parent
Path(a.parent).mkdir(parents=True, exist_ok=True)
work = Path(tempfile.mkdtemp(prefix="native-", dir=a.parent))
commands = []


def run(label, command):
    result = subprocess.run(list(map(str, command)), capture_output=True, timeout=60)
    (work / (label + ".stdout")).write_bytes(result.stdout)
    (work / (label + ".stderr")).write_bytes(result.stderr)
    assert result.returncode == 0, (command, result.stderr.decode())
    return result


for profile, name in enumerate(["A-callee-local", "B-index1"]):
    source = root / (name + ".nl")
    generated = work / (name + ".c")
    emitted = run(name + ".emit", [a.compiler, "--emit-c17-to", generated, source])
    assert not emitted.stdout and not emitted.stderr
    data = generated.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    assert b"#include <stddef.h>" in data and b"NewLang usize target mismatch" in data
    assert b"#define NL_CURRENT_CALL_RESULT(...) ((void)0)" in data
    modes = [False, True] if a.mode == "both" else [a.mode != "raw"]
    for passive in modes:
        for level in [0, 2]:
            label = name + ("-passive" if passive else "-raw") + "-O" + str(level)
            binary = work / label
            command = [a.cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                       "-UNDEBUG", "-O" + str(level)]
            if passive:
                command += [f"-DP1332_PROFILE={profile}",
                            f'-DNEWLANG_CURRENT_LOOP_OBSERVER="{root / "observer.h"}"']
            if a.mode == "sanitize":
                command += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                            "-fno-omit-frame-pointer"]
            command += [str(generated), "-o", str(binary)]
            run(label + ".build", command)
            assert generated.read_bytes() == data
            commands.append({"label": label, "binary": str(binary), "compile": command,
                             "generated": str(generated), "sha256": digest,
                             "passive": passive, "profile": profile})
(work / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
print(work, flush=True)
if not a.prepare_only:
    # In local matrix runs the outer controller records budget before exec.
    for row in commands:
        result = run(row["label"] + ".run", [row["binary"]])
        assert not result.stderr
        if row["passive"]:
            assert b"old->tail->call->caller PASS" in result.stdout
            expected = b"value95" if row["profile"] == 0 else b"value107"
            assert result.stdout.count(expected) == 2
        assert hashlib.sha256(Path(row["generated"]).read_bytes()).hexdigest() == row["sha256"]
        print(row["label"], "PASS", flush=True)
