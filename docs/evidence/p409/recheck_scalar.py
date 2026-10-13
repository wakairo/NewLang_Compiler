"""Test-only local integration observation using unchanged P404 test helpers.

No Production changes; do not mint arguments/results or modify generated callee.
"""
from pathlib import Path
import hashlib
import json
import sys
import tempfile

repo = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(repo / "tests" / "integration"))
from checked_c_u8_call_test import execute, observe
from checked_c_v0_test import run

compiler, probe, cc = sys.argv[1:]
source = Path(__file__).with_name("scalar_edges.nl")
compiled = run([compiler, str(source)])
assert compiled.returncode == 0 and not compiled.stderr and compiled.stdout
owned = run([probe, str(source)])
assert owned.returncode == 0 and not owned.stderr and "OWNED 4" in owned.stdout
calls = [line.split() for line in owned.stdout.splitlines() if line.startswith("CALL ")]
expected = [0, 255, 0, 255]
assert [int(line[-1]) for line in calls] == expected
assert len({line[3] for line in calls}) == 4
with tempfile.TemporaryDirectory(prefix="newlang-p409-scalar-") as temp:
    root = Path(temp)
    plain = execute(root, cc, "plain", compiled.stdout, True)
    observed = execute(root, cc, "observed", observe(compiled.stdout, expected), True)
print(json.dumps({"source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
                  "CLI": compiled.returncode, "C_bytes": len(compiled.stdout.encode()),
                  "C_sha256": hashlib.sha256(compiled.stdout.encode()).hexdigest(),
                  "host_cc": cc, "native_O0_O2": {"plain": plain, "observed": observed},
                  "actual_return_observations": expected, "owned": owned.stdout.splitlines()}, indent=2))
