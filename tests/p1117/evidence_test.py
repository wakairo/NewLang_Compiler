"""Check original inputs/FIRST and forward-only current generation."""
from pathlib import Path
import hashlib
import json
import sys
import zipfile

root=Path(__file__).resolve().parents[2]
data=root/'tests/p1117'
sha=lambda b:hashlib.sha256(b).hexdigest()
ancestors={1:('8620806e3527b2ce8a6256dffdd7214041aa5e4a81e249e5341c7dfd29b4d07a',
              'c103d1f7f5ae8d082a61a0fd37d29f7af9a1443ff8f0b1b8d6cb8662dedd769a'),
           2:('77fd0635c7d7852b2d0459e43fbd63ef65132c48c2a4057eb49e6bba288ef3ce',
              'd546f684e6173e465beedbb9b4501ce496ff830f972b5f3b48d6492ae1089a30')}
for version,(archive_sha,manifest_sha) in ancestors.items():
    p=data/'evidence'/f'inputs-v{version}.zip'
    assert sha(p.read_bytes())==archive_sha
    with zipfile.ZipFile(p) as z:
        raw=z.read(f'freeze-v{version}.json')
        assert sha(raw)==manifest_sha
        for name,expected in json.loads(raw)['files'].items():
            assert sha(z.read(name))==expected
            if name.startswith('sources/'):
                assert sha((data/name).read_bytes())==expected
assert sha((data/'evidence/FIRST-baseline-v1.zip').read_bytes())=='29ebb9c77109c435d75b32af4285ca634ef8b6839e8d2d2ed177d0066488dfff'
assert sha((data/'evidence/FIRST-baseline-v1.json').read_bytes())=='2f0e4e53468bea2081535e755798fba28af12fe62796dcbe85093b566d2ff67d'
assert sha((data/'sources/07-p1115-loop-control.nl').read_bytes())=='7d71183a846f28e378974aa4c38013bb87caf86a24778d9f5872da1ced3841b7'
freeze=json.loads((data/'freeze-v3.json').read_bytes())
with zipfile.ZipFile(data/'evidence/inputs-v3.zip') as z:
    assert set(z.namelist())==set(freeze['files'])
    for name,expected in freeze['files'].items():
        assert sha(z.read(name))==expected
        assert sha((data/name).read_bytes())==expected
sys.path.insert(0,str(data))
from history_v22 import verify
print('P1117 immutable input versions/FIRST and adopted-v21 inheritance PASS',verify(root,'tests/p990/current-freeze-v22.json'))
