"""Keep C242 FIRST and every original C243 source/equipment version exact."""
from pathlib import Path
import hashlib,json,zipfile

p=Path(__file__).resolve().parent
sha=lambda b:hashlib.sha256(b).hexdigest()
source=json.loads((p/'source-freeze-v1.json').read_text())
for name,entry in source['files'].items():
    assert sha((p/name).read_bytes())==entry['sha256'],name
for number in (1,2):
    file=p/('equipment-freeze-v'+str(number)+'.json')
    freeze=json.loads(file.read_bytes())
    with zipfile.ZipFile(p/('evidence/equipment-v'+str(number)+'.zip')) as z:
        assert z.read(file.name)==file.read_bytes()
        for name,expected in freeze['files'].items():assert sha(z.read(name))==expected,name
for name,expected in json.loads((p/'equipment-freeze-v3.json').read_bytes())['files'].items():
    assert sha((p/name).read_bytes())==expected,name
old=p/'evidence/p1174-checkpoint.zip'
assert sha(old.read_bytes())=='bc02b244cccccdfe5d882b3a820d629f484c74bc58c245d465ebfb47001da02a'
with zipfile.ZipFile(old) as z:
    assert z.read('oracle.json')==(p/'predecessor-oracle.json').read_bytes()
    for name in ['01-phase-left','02-phase-right','03-one-edge','04-zero-edge',
                 '90-incoming-usize','91-phase-shadow','92-inner-effect','93-overflow']:
        assert z.read('sources/'+name+'.nl')==(p/'sources'/(name+'.nl')).read_bytes(),name
version=json.loads((p/'oracle-v2.json').read_text())
assert sha((p/version['previous_oracle']).read_bytes())==version['previous_sha256']
for name,expected in version['sources'].items():assert sha((p/name).read_bytes())==expected,name
print('C243 original source/oracle/FIRST/equipment v1-v3 and external C242 checkpoint PASS')
