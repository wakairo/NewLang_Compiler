from pathlib import Path
import hashlib
import json

root = Path(__file__).resolve().parents[2]
manifest = json.loads((root / 'tests/p965/freeze.json').read_text())
for path, expected in manifest['sha256'].items():
    actual = hashlib.sha256((root / path).read_bytes()).hexdigest()
    assert actual == expected, (path, actual, expected)
print('P965 preregistered full source/harness/oracle SHA256 and protected historical files unchanged')
