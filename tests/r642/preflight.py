#!/usr/bin/env python3
"""R642 fixed-main remote pre-freeze eligibility gate (not a candidate verdict)."""
import pathlib
import subprocess
import sys
import tempfile

compiler, probe, cc, directory = sys.argv[1:]
root = pathlib.Path(directory)
for name in ("true_branch", "false_branch", "equal_arms"):
    path = root / (name + ".nl")
    check = subprocess.run([compiler, "--check", str(path)],
                           capture_output=True, text=True)
    assert check.returncode == 0 and not check.stdout and not check.stderr, (
        name, check.returncode, check.stdout, check.stderr)
    public = subprocess.run([probe, "positive", str(path)],
                            capture_output=True, text=True)
    assert public.returncode == 0 and "public-owned" in public.stdout, (
        name, public.returncode, public.stdout, public.stderr)
    print(f"STAGE-A valid {name}: source CHECK0 / registered public-owned OK",
          flush=True)

for name in ("reject_range", "reject_type"):
    path = root / (name + ".nl")
    semantic = subprocess.run([probe, "negative", str(path)],
                              capture_output=True, text=True)
    assert semantic.returncode == 0 and "semantic-reject" in semantic.stdout, (
        name, semantic.returncode, semantic.stdout, semantic.stderr)
    check = subprocess.run([compiler, "--check", str(path)],
                           capture_output=True, text=True)
    assert check.returncode == 3 and not check.stdout and check.stderr, (
        name, check.returncode, check.stdout, check.stderr)
    print(f"STAGE-A negative {name}: parsed / semantic rejected / no C",
          flush=True)

with tempfile.TemporaryDirectory(prefix="r642-main-") as tmp:
    temp = pathlib.Path(tmp)
    guest = subprocess.run([compiler, str(root / "cli_smoke.nl")],
                           capture_output=True, text=True)
    assert guest.returncode == 0 and guest.stdout and not guest.stderr, (
        guest.returncode, guest.stderr)
    assert "int main(void)" in guest.stdout
    source_file = temp / "guest.c"
    source_file.write_text(guest.stdout)
    for optimization in ("-O0", "-O2"):
        executable = temp / ("guest-" + optimization[1:])
        build = subprocess.run([cc, "-std=c17", "-Wall", "-Wextra",
                                "-Wpedantic", "-Werror", optimization,
                                str(source_file), "-o", str(executable)],
                               capture_output=True, text=True)
        assert build.returncode == 0, (optimization, build.stderr)
        run = subprocess.run([str(executable)], capture_output=True, text=True)
        assert run.returncode == 0, (optimization, run.stderr)
        print(f"STAGE-A strict C17/native {optimization}: OK", flush=True)
print("STAGE-A REMOTE PRE-FREEZE ELIGIBLE", flush=True)
