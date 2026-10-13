#!/usr/bin/env python3
"""Independent R386 probe collection. Capture candidate outputs without assumptions."""
import hashlib
import json
import pathlib
import subprocess
import sys

root = pathlib.Path(__file__).resolve().parent
out = root / "evidence"
out.mkdir(exist_ok=True)
compiler = pathlib.Path(sys.argv[1]).resolve()
digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
print("R386 exact candidate ancestor 8e80af60e8c9c8b29acb0ea6a7aad4ab5d1c8bd8", flush=True)
print("R386 compiler SHA256", digest(compiler), flush=True)
report = {}
for name in ("positive_a", "positive_b", "invalid_escape", "invalid_actual_type", "invalid_read_ref", "invalid_field"):
    source = root / (name + ".nl")
    proc = subprocess.run([str(compiler), str(source)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=30)
    (out / (name + ".c")).write_bytes(proc.stdout)
    (out / (name + ".stderr")).write_bytes(proc.stderr)
    v = dict(source_sha256=digest(source), exit=proc.returncode, c_sha256=digest(out / (name + ".c")),
             stderr=proc.stderr.decode("utf-8", "replace")[:3000], c_bytes=len(proc.stdout))
    if proc.returncode == 0 and proc.stdout:
        for cc in ("gcc-13", "clang"):
            exe = out / (name + "." + cc)
            cmd = [cc, "-std=c17", "-Wall", "-Wextra", "-Werror", "-pedantic",
                   str(out / (name + ".c")), "-o", str(exe)]
            build = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            native = subprocess.run([str(exe)], capture_output=True, timeout=10) if build.returncode == 0 else None
            v[cc] = dict(compile_exit=build.returncode, compile_stderr=build.stderr.decode("utf-8", "replace")[:1500],
                         native_exit=native.returncode if native else None,
                         native_stdout=native.stdout.decode("utf-8", "replace") if native else None,
                         native_stderr=native.stderr.decode("utf-8", "replace") if native else None)
        print("=== GENERATED C", name, "===\n", proc.stdout.decode("utf-8", "replace")[:22000], flush=True)
    print("R386_RESULT", name, json.dumps(v, sort_keys=True), flush=True)
    report[name] = v
(out / "report.json").write_text(json.dumps(report, indent=2))
# This is calibration / artifact capture, not a semantic PASS.
if any(report[n]["exit"] != 0 for n in ("positive_a", "positive_b")):
    sys.exit(2)
