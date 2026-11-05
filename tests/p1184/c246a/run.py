"""Verify the separate C246a freeze before running the repaired-code probe."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

root = Path(__file__).resolve().parents[3]
here = Path(__file__).resolve().parent
previous = here / "freeze-v1.json"
assert hashlib.sha256(previous.read_bytes()).hexdigest() == \
    "33a3febd5760810ca25f8346ed70b0fe5975e2f4c22b8d64be5e3bfb3371bb34"
frozen = json.loads((here / "freeze-v2.json").read_bytes())
for name, expected in frozen["files"].items():
    assert hashlib.sha256((root / name).read_bytes()).hexdigest() == expected, name
oracle = json.loads((here / "oracle-v1.json").read_bytes())
source = root / sys.argv[2]
assert sys.argv[2] in oracle["sources"]
result = subprocess.run([sys.argv[1], str(source)], capture_output=True, text=True)
sys.stdout.write(result.stdout)
sys.stderr.write(result.stderr)
assert result.returncode == 0, result.returncode
assert result.stderr == ""
assert result.stdout.count("reject/out/sink/retry=PASS") == oracle["trials_per_source"]
assert "C246a trials=120 B-survive/cleanup/fresh-CHECK-retry=PASS\n" in result.stdout
