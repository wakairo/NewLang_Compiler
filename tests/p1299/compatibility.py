"""Live C17 functional compatibility against adopted-main output, no history scan."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

cli, destination = sys.argv[1:]
here = Path(__file__).resolve().parent
root = here.parent.parent
parent = Path(destination)
parent.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix="compat-", dir=parent))
oracle = json.loads((here / "compatibility.json").read_text())
rows = []
for i, case in enumerate(oracle["cases"]):
    source = root / case["source"]
    assert hashlib.sha256(source.read_bytes()).hexdigest() == case["source_sha256"]
    p = subprocess.run([cli, str(source)], capture_output=True)
    (out / (str(i) + ".stdout")).write_bytes(p.stdout)
    (out / (str(i) + ".stderr")).write_bytes(p.stderr)
    same = len(p.stdout) == case["C_bytes"] and \
        hashlib.sha256(p.stdout).hexdigest() == case["C_sha256"]
    rows.append({"source": case["source"], "status": p.returncode,
                 "C_bytes_identical": same})
    (out / "results.json").write_text(json.dumps(rows, indent=2) + "\n")
    assert p.returncode == 0 and same, (case, p.returncode, out)
print("P1299 old unit/typed-result/direct C17 bytes unchanged PASS", out)
