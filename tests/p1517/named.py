"""Semantic/host failures retain the named target and permit a clean retry."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

compiler, fault_compiler, tests_arg, output_arg = sys.argv[1:]
tests = Path(tests_arg).resolve()
Path(output_arg).mkdir(parents=True, exist_ok=True)
output = Path(tempfile.mkdtemp(prefix="p1517-named-", dir=output_arg))
target = output / "result.c"
sentinel = b"P1517 named output sentinel\n"


def run(label, exe, name, expected, setting=None):
    env = dict(os.environ)
    env.pop("P1517_HOST_TEXT_FAIL", None)
    if setting is not None:
        env["P1517_HOST_TEXT_FAIL"] = str(setting)
    result = subprocess.run([exe, "--emit-c17-to", str(target), str(tests / name)],
                            env=env, capture_output=True)
    (output / (label + ".stdout")).write_bytes(result.stdout)
    (output / (label + ".stderr")).write_bytes(result.stderr)
    assert result.returncode == expected, (label, result.returncode, result.stderr)
    assert not list(output.glob("result.c.tmp.*"))
    if expected:
        assert target.read_bytes() == sentinel
    else:
        assert b"static void nl_owner_" in target.read_bytes()


for point in [1, 2]:
    target.write_bytes(sentinel)
    run(f"fault{point}", fault_compiler, "P-zero.nl", 1, point)
    run(f"retry{point}", fault_compiler, "P-zero.nl", 0)
target.write_bytes(sentinel)
run("semantic-failure", compiler, "N-foreign-domain.nl", 3)
run("semantic-retry", compiler, "P-zero.nl", 0)
print("P1517 named semantic+host OOM no-clobber/no-staging/retry PASS", output)
