"""Keep the original Gate0 refusal and every equipment generation exact."""
from pathlib import Path
import hashlib,json,zipfile

p=Path(__file__).resolve().parent
sha=lambda data:hashlib.sha256(data).hexdigest()
source=json.loads((p/'source-freeze-v1.json').read_text())
for name,digest in source['files'].items():
    assert sha((p/name).read_bytes())==digest,name
old=p/'evidence/p1182-checkpoint.zip'
assert sha(old.read_bytes())=='a6216233676675b3b5f7af7cbf93fc70208c6912a1b235f43a8b3eb701b1fa25'
with zipfile.ZipFile(old) as z:
    oracle=json.loads(z.read('oracle-v1.json'))
    assert oracle['cases']['01-helper-loop']['current_prediction']['check']==3
    assert json.loads(z.read('raw/FIRST.json'))['status']==3
    for before,after in [('01-helper-loop','01-helper-loop'),
                         ('02-helper-alias-loop','02-helper-alias-loop'),
                         ('03-helper-only','81-pure-control'),
                         ('04-constant-helper-loop','99-literal-helper'),
                         ('90-wrong-result','90-wrong-result'),
                         ('91-wrong-actual','91-wrong-actual'),
                         ('92-arm-type','92-arm-type')]:
        assert z.read('sources/'+before+'.nl')==(p/'sources'/(after+'.nl')).read_bytes()
latest={}
for number in (1,2):
    name='equipment-freeze-v'+str(number)+'.json'
    freeze=json.loads((p/name).read_text())
    with zipfile.ZipFile(p/('evidence/equipment-v'+str(number)+'.zip')) as z:
        assert z.read(name)==(p/name).read_bytes()
        for path,digest in freeze['files'].items():assert sha(z.read(path))==digest
    latest.update(freeze['files'])
for number in (3,4,5):
    freeze=json.loads((p/('equipment-freeze-v'+str(number)+'.json')).read_text())
    latest.update(freeze['files'])
    for key,path in [('v2_archive_sha256','equipment-v2.zip'),
                     ('v3_native_archive_sha256','equipment-v3-native.zip'),
                     ('v1_archive_sha256','equipment-v1.zip')]:
        if key in freeze:assert sha((p/'evidence'/path).read_bytes())==freeze[key]
for name,digest in latest.items():assert sha((p/name).read_bytes())==digest,name
print('C245 original Gate0/source/oracle, original equipment/FIRST and current equipment PASS')
