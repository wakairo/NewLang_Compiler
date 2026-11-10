"""Frozen P1291 source CHECK and real negative diagnostics."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

compiler, destination = sys.argv[1:]
here = Path(__file__).resolve().parent
oracle = json.loads((here / "oracle.json").read_text())
parent = Path(destination)
parent.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix="check-", dir=parent))
cases = dict(oracle["Stage1"]["CHECK_expected"])
extra = json.loads((here / "typed-result-negative.json").read_text())
cases[extra["source"]] = extra["CHECK"]
hashes = dict(oracle["Stage1"]["sources"])
hashes[extra["source"]] = extra["sha256"]
for name, expected in cases.items():
    source = here / "sources" / name
    assert hashlib.sha256(source.read_bytes()).hexdigest() == hashes[name]
    result = subprocess.run([compiler, "--check", str(source)],
                            capture_output=True)
    (out / (name + ".stdout")).write_bytes(result.stdout)
    (out / (name + ".stderr")).write_bytes(result.stderr)
    assert result.returncode == expected and not result.stdout, (
        name, result.returncode, expected, result.stderr, out)
    print(name, result.returncode, result.stderr.decode().strip())
print("P1291 source CHECK PASS", out)
