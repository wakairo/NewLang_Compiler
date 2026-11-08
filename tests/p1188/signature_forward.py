"""Exercise original C246a safety behavior without SHA-history gates."""
from pathlib import Path
import json
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
old = root / "tests/p1184/c246a"
oracle = json.loads((old / 'oracle-v1.json').read_bytes())
assert sys.argv[2] in oracle['sources']
result = subprocess.run([sys.argv[1], str(root / sys.argv[2])], capture_output=True, text=True)
sys.stdout.write(result.stdout)
sys.stderr.write(result.stderr)
assert result.returncode == 0, result.returncode
assert result.stderr == ''
assert result.stdout.count('reject/out/sink/retry=PASS') == oracle['trials_per_source']
assert 'C246a trials=120 B-survive/cleanup/fresh-CHECK-retry=PASS\n' in result.stdout
