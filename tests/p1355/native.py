"""Strict unedited raw C17, then passive observations of live originals."""
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
attempts = []


def run(label, command, env=None):
    command = list(map(str, command))
    guest = label.endswith(".run")
    if guest:
        attempts.append({"label": label, "command": command, "exit_status": None})
        (work / "guest-attempts.json").write_text(json.dumps(attempts, indent=2) + "\n")
    r = subprocess.run(command, capture_output=True, timeout=60, env=env)
    if guest:
        attempts[-1]["exit_status"] = r.returncode
        (work / "guest-attempts.json").write_text(json.dumps(attempts, indent=2) + "\n")
    (work / (label + ".stdout")).write_bytes(r.stdout)
    (work / (label + ".stderr")).write_bytes(r.stderr)
    assert r.returncode == 0, (command, r.returncode, r.stderr.decode())
    return r


for profile, name in enumerate(["A-input83", "B-input137-index1"]):
    generated = work / (name + ".c")
    r = run(name + ".emit", [args.compiler, "--emit-c17-to", generated,
                             root / (name + ".nl")])
    assert not r.stdout and not r.stderr
    c_bytes = generated.read_bytes()
    functions = re.findall(rb"static uint8_t nl_fn_(\d+)\(uint8_t nl_local_(\d+)\)\n\{\n(.*?)\n\}\n", c_bytes, re.S)
    assert len(functions) == 2
    inner = [f for f in functions if b"for (;;)" in f[2]]
    wrapper = [f for f in functions if b"for (;;)" not in f[2]]
    assert len(inner) == len(wrapper) == 1
    function, formal, inner_body = inner[0]
    assert re.search(rb"\*nl_local_\d+ = nl_local_" + formal + rb";", inner_body)
    assert re.search(rb"\.f[01] = nl_local_" + wrapper[0][1], wrapper[0][2])
    assert len(re.findall(rb"= nl_fn_" + function + rb"\(", wrapper[0][2])) == 1
    assert c_bytes.count(b"NL_LOOP_BIND(&") == 2
    assert c_bytes.count(b"NL_CURRENT_CALL_RESULT(") == 3
    assert c_bytes.count(b"NL_CURRENT_CALLER_COPY(") == 5
    modes = [False, True] if args.sanitizer == "none" else [True]
    levels = [0, 2] if args.sanitizer == "none" else [0]
    for passive in modes:
        for level in levels:
            label = name + ("-passive" if passive else "-raw") + "-O" + str(level)
            binary = work / label
            command = [args.cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic",
                       "-Werror", "-UNDEBUG", "-O" + str(level)]
            if passive:
                command += ["-DP1355_PROFILE=" + str(profile),
                            '-DNEWLANG_CURRENT_LOOP_OBSERVER="' +
                            str(root / "observer.h") + '"']
            if args.sanitizer != "none":
                command += ["-fsanitize=" + args.sanitizer,
                            "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
            command += [generated, "-o", binary]
            run(label + ".build", command)
            assert generated.read_bytes() == c_bytes
            env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
            r = run(label + ".run", [binary], env)
            assert not r.stderr
            if passive:
                assert r.stdout == b"P1355 original wrapper -> inner old -> outer Copy PASS\n"
            else:
                assert not r.stdout
            commands.append({"label": label, "command": list(map(str, command)),
                             "C_SHA256": hashlib.sha256(c_bytes).hexdigest(),
                             "passive": passive, "sanitizer": args.sanitizer})
            (work / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
            print(label, "PASS", flush=True)
print(work, flush=True)
