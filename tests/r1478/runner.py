"""Independent R1478 V1 stage runner. Never rewrite R1474 originals."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

SOURCE = {
    "s1": (5446, "4069b0fe781598c14c91d4ec1e49b72923877d79002a8257b2bebefc919a7222"),
    "s2": (5447, "2caa736ff599ea800d0f1759a3aa7df77acf206c80d57655d45b53bbfee2419a"),
    "s3": (5448, "a899a94095e900f0b54462508ac0640565bd2fce0838674c81edc5dc8d40db44"),
}
ORACLE = (7470, "8c10aad7ac43877d8f8767e1c339c607d31d9f969b55e84f53f153e6600725fd")
ROOT = Path(__file__).resolve().parent
STAGES = []
ENV = dict(
    os.environ,
    ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
    UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1",
)


def validate(path, expected):
    raw = path.read_bytes()
    length, digest = expected
    if len(raw) != length or hashlib.sha256(raw).hexdigest() != digest:
        raise SystemExit(f"STOP: original frozen source/oracle drift at {path}")
    if not raw.endswith(b"\n") or b"\r" in raw or b"\x00" in raw:
        raise SystemExit(f"STOP: LF/UTF-8 integrity drift at {path}")


def run(name, args, workspace):
    args = [str(v) for v in args]
    row = {"stage": name, "command": args, "state": "started"}
    STAGES.append(row)
    (workspace / "r1478-ledger.json").write_text(
        json.dumps(STAGES, indent=2) + "\n", encoding="utf-8"
    )
    completed = subprocess.run(args, capture_output=True, env=ENV, check=False)
    (workspace / (name + ".stdout")).write_bytes(completed.stdout)
    (workspace / (name + ".stderr")).write_bytes(completed.stderr)
    row.update(state="finished", exit=completed.returncode)
    (workspace / "r1478-ledger.json").write_text(
        json.dumps(STAGES, indent=2) + "\n", encoding="utf-8"
    )
    print(
        f"R1478 {name}: exit={completed.returncode},"
        f" stderr={completed.stderr.decode(errors='replace')[:500]}"
    )
    return completed


def stop_or_skip(name, result, skip_allowed):
    if result.returncode == 0:
        return
    if result.returncode == 125 and skip_allowed:
        print(f"R1478 {name}: NOT_REACHED (CTest SKIP, never PASS)")
        raise SystemExit(125)
    if skip_allowed and name in ("source-check", "emit-c17"):
        print(f"R1478 {name}: SOURCE/CHECK or backend NOT_REACHED, not PASS")
        raise SystemExit(125)
    print(f"R1478 STOP: {name} returned {result.returncode}")
    raise SystemExit(1)


if len(sys.argv) != 7:
    raise SystemExit("usage: runner.py (s1|s2|s3) newlangc public_gate cc sanitizer build_dir")
variant, compiler, probe, cc, sanitizer, output = sys.argv[1:]
if variant not in SOURCE or sanitizer not in ("none", "address", "undefined"):
    raise SystemExit("R1478 STOP: invalid bounded parameters")
for key, expected in SOURCE.items():
    validate(ROOT / (key + ".nl"), expected)
validate(ROOT / "oracle.txt", ORACLE)

workspace = Path(output) / ("R1478-" + variant)
workspace.mkdir(parents=True, exist_ok=True)
source = ROOT / (variant + ".nl")

if variant == "s2":
    negative = run("wrong-domain-public-negative", [probe, "negative", source], workspace)
    stop_or_skip("wrong-domain-public-negative", negative, True)
    print("R1478 S2: actual wrong Domain semantically rejected; guest NOT_RUN")
    raise SystemExit(0)

checked = run("source-check", [compiler, "--check", source], workspace)
stop_or_skip("source-check", checked, True)

proof = run("public-owned-machine", [probe, "positive", source], workspace)
stop_or_skip("public-owned-machine", proof, True)

c_source = workspace / (variant + ".generated.c")
emitted = run("emit-c17", [compiler, "--emit-c17-to", c_source, source], workspace)
stop_or_skip("emit-c17", emitted, True)
if not c_source.is_file():
    raise SystemExit("R1478 STOP: missing emitted C17")
original_c_sha = hashlib.sha256(c_source.read_bytes()).hexdigest()

strict = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
flags = (
    []
    if sanitizer == "none"
    else [f"-fsanitize={sanitizer}", "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
)
raw = workspace / "raw-native"
built = run(
    "strict-raw-build", [cc, *strict, "-O2", *flags, c_source, "-o", raw],
    workspace
)
stop_or_skip("strict-raw-build", built, False)
executed = run("strict-raw-execute", [raw], workspace)
stop_or_skip("strict-raw-execute", executed, False)

observer = ROOT / "passive.h"
wrapper = ROOT / "guest_wrap.c"
sample = 0 if variant == "s1" else 173
for optimization in (0, 2):
    for failure in (1, 2, 0):
        stem = f"H{failure}-O{optimization}"
        executable = workspace / (stem + ".native")
        compiled = run(
            stem + "-build",
            [
                cc, *strict, f"-O{optimization}", *flags,
                f'-DNEWLANG_NODE_OBSERVER="{observer}"',
                f'-DNEWLANG_HEAP_OBSERVER="{observer}"',
                f"-DR1478_SAMPLE={sample}",
                f"-DR1478_FAIL_AT={failure}",
                c_source, wrapper,
                "-Wl,--wrap=malloc", "-Wl,--wrap=free", "-o", executable,
            ],
            workspace
        )
        stop_or_skip(stem + "-build", compiled, False)
        result = run(stem + "-native", [executable], workspace)
        stop_or_skip(stem + "-native", result, False)
        expected_world = "H" + str(0 if failure == 1 else 1 if failure == 2 else 2)
        if b"R1478-NATIVE-VERIFIED " + expected_world.encode() not in result.stdout:
            raise SystemExit(f"R1478 STOP: {stem} no authentic observer evidence")
        if b" PASS" not in result.stdout:
            raise SystemExit(f"R1478 STOP: {stem} missing PASS marker")
        if hashlib.sha256(c_source.read_bytes()).hexdigest() != original_c_sha:
            raise SystemExit("R1478 STOP: emitted source mutated by observer")

print(
    "R1478 POSITIVE STAGE-VERIFIED CHECK+PUBLIC_MACHINE+C17+GCC/Clang "
    "H0/H1/H2 strict native; original R1474 values were not rewritten"
)
