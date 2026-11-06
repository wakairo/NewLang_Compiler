"""Read-only P-owned source/oracle/equipment/FIRST custody checks."""
from pathlib import Path
import hashlib,json,zipfile
here=Path(__file__).resolve().parent;repo=here.parents[1]
sha=lambda b:hashlib.sha256(b).hexdigest()
for version in (1,2):
    freeze=json.loads((here/f'source-freeze-v{version}.json').read_bytes())
    for name,digest in freeze['files'].items():assert sha((here/name).read_bytes())==digest,name
for freeze_name in ('probe-freeze-v1.json','validation-freeze-v1.json'):
    freeze=json.loads((here/freeze_name).read_bytes())
    prefix=here if freeze_name.startswith('probe') else repo
    for name,digest in freeze['files'].items():assert sha((prefix/name).read_bytes())==digest,name
receipt=json.loads((here/'evidence-receipt-v1.json').read_text())
archive=repo/receipt['archive'];assert sha(archive.read_bytes())==receipt['sha256']
with zipfile.ZipFile(archive) as z:
    assert set(z.namelist())==set(receipt['files'])
    for name,digest in receipt['files'].items():assert sha(z.read(name))==digest,name
    assert b'P14-LOOP-PRECISION' in z.read('gate0/01-left-normal-check.err')
    assert b'P14-LOOP-PRECISION' in z.read('prior/preserved-95-P1193-prior-LastTest-section.log')
print('C263 original source/oracle/freeze/FIRST and prior boundary95 custody PASS')
