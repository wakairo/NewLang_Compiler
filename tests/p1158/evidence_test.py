"""C232 original independent source/oracle/FIRST and current harness stay exact."""
from pathlib import Path
import hashlib
import json
import zipfile

p=Path(__file__).resolve().parent
sha=lambda b:hashlib.sha256(b).hexdigest()
for name,expected in {
    'inputs-prefreeze-v1.zip':'5d13018319bc9dc6c9bbbe9955e36990fddf37b41037c99f460b271e58f33738',
    'first-gate0.zip':'48255916c97f899b9bef5f69f0ff7c8f563bb404a829509277c46f599d00fbf1'
}.items(): assert sha((p/'evidence'/name).read_bytes())==expected
with zipfile.ZipFile(p/'evidence/inputs-prefreeze-v1.zip') as z:
    for name,expected in json.loads((p/'gate0-freeze.json').read_bytes())['files'].items():
        assert sha(z.read(name))==expected
        current=p/('evidence/probe.c.source' if name=='probe.c' else name)
        assert sha(current.read_bytes())==expected,name
for name,expected in json.loads((p/'freeze-v1.json').read_bytes()).items():
    assert sha((p/name).read_bytes())==expected,name
for name,expected in json.loads((p/'reused-equipment.json').read_bytes()).items():
    assert sha((p.parents[1]/name).read_bytes())==expected,name
print('C232 original source/oracle/FIRST and unchanged reused P OOM machinery PASS')
