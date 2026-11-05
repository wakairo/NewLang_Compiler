"""Retain original C236/C238 inputs and first RED; protect current P harness."""
from pathlib import Path
import hashlib,json,zipfile

p=Path(__file__).resolve().parent
sha=lambda data:hashlib.sha256(data).hexdigest()
for name,expected in json.loads((p/'freeze-v2.json').read_bytes()).items():
    assert sha((p/name).read_bytes())==expected,name
with zipfile.ZipFile(p/'evidence/equipment-v1.zip') as z:
    old=json.loads((p/'freeze-v1.json').read_bytes())
    assert z.read('freeze-v1.json')==(p/'freeze-v1.json').read_bytes()
    for name,expected in old.items():
        assert sha(z.read(name))==expected,name
        if name.startswith('sources/') or name.endswith('oracle.json'):
            assert sha((p/name).read_bytes())==expected,name
with zipfile.ZipFile(p/'evidence/inputs-prefreeze-v1.zip') as z:
    manifest=json.loads(z.read('freeze.json'))
    for name,expected in manifest['files'].items():
        assert sha(z.read(name))==expected,name
        if name.startswith('sources/') or name=='oracle.json':
            assert sha((p/name).read_bytes())==expected,name
with zipfile.ZipFile(p/'evidence/p1164-inputs.zip') as z:
    for name in z.namelist():
        if name.startswith('sources/'):
            assert (p/name).read_bytes()==z.read(name),name
print('C238 original C236/C238 source/oracle/FIRST and prospective v39 evidence PASS')
