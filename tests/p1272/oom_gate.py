"""Finite reached allocator census and actual rollback/retry; preserve raw."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

probe, destination = sys.argv[1:]
source = Path(__file__).resolve().parent / "sources/P-template-copy.nl"
parent = Path(destination)
parent.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix="run-", dir=parent))
for mode in ["check", "emit"]:
    command = [probe, str(source), mode]
    with (out / (mode + ".stdout")).open("wb") as stdout, (out / (mode + ".stderr")).open("wb") as stderr:
        p = subprocess.run(command, stdout=stdout, stderr=stderr)
    (out / (mode + ".json")).write_text(json.dumps({"command": command, "status": p.returncode}, indent=2) + "\n")
    assert p.returncode == 0, (mode, p.returncode, out)
    final = [line for line in (out / (mode + ".stdout")).read_text().splitlines() if line.startswith("OOM-FINAL")]
    assert len(final) == 1
    print(final[0])
print("P1272 actually reached OOM/rollback/retry PASS", out)
