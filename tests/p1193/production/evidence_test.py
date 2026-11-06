"""Preserve every phase1 input/false prediction/FIRST; check live successors."""
from pathlib import Path
import hashlib,json,zipfile
here=Path(__file__).resolve().parent;root=here.parents[2]
sha=lambda b:hashlib.sha256(b).hexdigest()
f=json.loads((here/'validation-freeze-v1.json').read_bytes())
for name,digest in f['files'].items():assert sha((root/name).read_bytes())==digest,name
receipt=json.loads((here.parent/'evidence/phase1-receipt-v1.json').read_bytes())
p=root/receipt['archive'];assert sha(p.read_bytes())==receipt['sha256']
with zipfile.ZipFile(p) as z:
 assert set(z.namelist())==set(receipt['files'])
 for name,digest in receipt['files'].items():assert sha(z.read(name))==digest,name
 old=json.loads(z.read('inputs/tests/p1193/c256/old-oracle-v1.json'))
 assert len(old['cases'])==7
 assert old['cases']['01-return-left']['expected_CLI_check']==1
 assert b'P14-LOOP-PRECISION' in z.read('raw/candidate-v1/01-return-left-check.err')
 assert b'P14-LOOP-PRECISION' in z.read('raw/result-copy-FIRST/check.err')
 assert json.loads(z.read('raw/native-v3/native-count.json'))['native']==40
 print('C256 original seven sources/false prediction/all phase1 FIRST/raw/native40 preserved',len(receipt['files']))
print('C256 live source/oracle/equipment freeze PASS',len(f['files']))
