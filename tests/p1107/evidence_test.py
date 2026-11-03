"""Archived first inputs and forward-only adopted-v20 history remain exact."""
from pathlib import Path
import hashlib
import json
import sys
import zipfile

root = Path(__file__).resolve().parents[2]
data = root / 'tests/p1107'
sha = lambda b: hashlib.sha256(b).hexdigest()
freezes = sorted(data.glob('freeze-v*.json'), key=lambda p:int(p.stem.split('v')[-1]))
for freeze in freezes:
    doc = json.loads(freeze.read_bytes())
    archive = data / 'evidence' / ('harness-inputs-' + freeze.stem.split('-')[-1] + '.zip')
    assert sha(archive.read_bytes()) == doc['archive_sha256']
    with zipfile.ZipFile(archive) as z:
        assert set(z.namelist()) == set(doc['files'])
        for name, expected in doc['files'].items():
            assert sha(z.read(name)) == expected, (freeze.name,name)
            if name.endswith(('.nl','.json')) or name.endswith('generate_sources_v1.py'):
                assert sha((root/name).read_bytes()) == expected
for name, expected in json.loads(freezes[-1].read_bytes())['files'].items():
    assert sha((root/name).read_bytes()) == expected, name
first = json.loads((data/'evidence/fixed-main-FIRST.json').read_bytes())
assert sha((data/'evidence/fixed-main-FIRST.zip').read_bytes()) == first['archive_sha256']
assert sha((data/'sources/01-bound.nl').read_bytes()) == 'cccdc1b51e7fddb230f2a1ee268f69750559cdf7310074b0a63b49ea7f637fa1'
assert sha((data/'sources/08-equal-bits-wrong-return.nl').read_bytes()) == '1099d5cd5ae589f52d571f683390bd83b32c27a1723c0225555b429231e3208c'
sys.path.insert(0,str(data))
from history_v21 import verify
count = verify(root,'tests/p990/current-freeze-v21.json')
print('P1107 original planning/source/oracle/FIRST, all input versions, adopted history: PASS', count)
