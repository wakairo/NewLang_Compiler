"""Current source + archived pre-execution harness + adopted history anchors."""
from pathlib import Path
import hashlib
import json
import sys
import zipfile
root = Path(__file__).resolve().parents[2]
data = root / 'tests/p1098'
sha = lambda b: hashlib.sha256(b).hexdigest()
for name, expected in json.loads((data / 'source-freeze-v1.json').read_bytes())['files'].items():
    assert sha((root / name).read_bytes()) == expected, name
for freeze in sorted(data.glob('harness-freeze-v*.json')):
    doc = json.loads(freeze.read_bytes())
    with zipfile.ZipFile(data / 'evidence' / (freeze.stem.replace('harness-freeze', 'harness') + '-inputs.zip')) as z:
        assert set(z.namelist()) == set(doc['files'])
        for name, expected in doc['files'].items():
            assert sha(z.read(name)) == expected, (freeze.name, name)
old = (root / 'tests/p1092/cli.py').read_bytes()
assert sha(old) == 'febf3c55dbf97c6b743fc49ecc43b3e4bd1e775e109ea6287336419d937bf1f0'
# The new forward runner adjusts precisely one prediction expression at runtime.
forward = (data / 'p1092_forward.py').read_text()
assert 'expected = 0 if number == 0 else 4 if number in positive else 3' in forward
assert 'expected = 0 if number in {0, 2, 3, 6, 16, 17} else 4 if number in positive else 3' in forward
sys.path.insert(0, str(data))
from history_v20 import verify
verify(root, 'tests/p990/current-freeze-v20.json')
print('P1098 full pre-run inputs, original FIRST, fixed adopted-v19, inherited history: PASS')
