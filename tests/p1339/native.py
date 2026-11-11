"""Strict raw C17 first, then passive physical observations for A and B."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile


parser = argparse.ArgumentParser()
parser.add_argument("compiler")
parser.add_argument("cc")
parser.add_argument("parent")
parser.add_argument("sanitizer", choices=["none", "address", "undefined"])
args = parser.parse_args()
root = Path(__file__).resolve().parent
Path(args.parent).mkdir(parents=True, exist_ok=True)
work = Path(tempfile.mkdtemp(prefix="native-", dir=args.parent))
commands = []


def run(label, command, env=None):
    result = subprocess.run(list(map(str, command)), capture_output=True,
                            timeout=60, env=env)
    (work / (label + ".stdout")).write_bytes(result.stdout)
    (work / (label + ".stderr")).write_bytes(result.stderr)
    assert result.returncode == 0, (command, result.stderr.decode())
    return result


for profile, name in enumerate(["A-input67", "B-input109"]):
    source = root / (name + ".nl")
    generated = work / (name + ".c")
    emitted = run(name + ".emit",
                  [args.compiler, "--emit-c17-to", generated, source])
    assert not emitted.stdout and not emitted.stderr
    c_bytes = generated.read_bytes()
    digest = hashlib.sha256(c_bytes).hexdigest()
    signature = re.search(
        rb"static uint8_t nl_fn_(\d+)\(uint8_t nl_local_(\d+)\)", c_bytes)
    assert signature and re.search(rb"\*nl_local_\d+ = nl_local_" +
                                   signature.group(2) + rb";", c_bytes)
    assert b"NL_CURRENT_CALL_RESULT" in c_bytes
    modes = [False, True] if args.sanitizer == "none" else [True]
    levels = [0, 2] if args.sanitizer == "none" else [0]
    for passive in modes:
        for level in levels:
            label = name + ("-passive" if passive else "-raw") + "-O" + str(level)
            binary = work / label
            command = [args.cc, "-std=c17", "-Wall", "-Wextra",
                       "-Wpedantic", "-Werror", "-UNDEBUG",
                       "-O" + str(level)]
            if passive:
                command += ["-DP1339_PROFILE=" + str(profile),
                            '-DNEWLANG_CURRENT_LOOP_OBSERVER="' +
                            str(root / "observer.h") + '"']
            if args.sanitizer != "none":
                command += ["-fsanitize=" + args.sanitizer,
                            "-fno-sanitize-recover=all",
                            "-fno-omit-frame-pointer"]
            command += [generated, "-o", binary]
            run(label + ".build", command)
            assert generated.read_bytes() == c_bytes
            env = dict(os.environ,
                       ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
            result = run(label + ".run", [binary], env)
            assert not result.stderr
            if passive:
                expected = ("input67" if profile == 0 else "input109").encode()
                assert b"P1339 physical original " + expected in result.stdout
                assert b"current233 sibling181 PASS" in result.stdout
            else:
                assert not result.stdout
            commands.append({"label": label, "command": list(map(str, command)),
                             "C_SHA256": digest, "passive": passive,
                             "sanitizer": args.sanitizer})
            print(label, "PASS", flush=True)

(work / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
print(work, flush=True)
