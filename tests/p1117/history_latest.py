"""Keep original evidence assertions and select the trusted current wrapper."""
from pathlib import Path
import hashlib
import json
import runpy
import sys
import zipfile
root=Path(__file__).resolve().parents[2]
data=root/'tests/p1117'
current=runpy.run_path(str(root/'tests/p982/freeze_test.py'))['CURRENT']
family=sys.argv[1]
pins={'p1098':('a22eeaf64dbe29e7cd4b58babc0f380ef650e3919194e94d7161b928d5c4b7d4',"verify(root, 'tests/p990/current-freeze-v20.json')"),
      'p1107':('e778afc9fd22da945f00eed50dc93bde3b86c1d34bb743c03d641b204e58831d',"verify(root,'tests/p990/current-freeze-v21.json')"),
      'p1117':('86e3136fec1ce518c12f5420f056e84811eef81bd932cdfb57fdc1f942a64991',"verify(root,'tests/p990/current-freeze-v22.json')")}
old=root/'tests'/family/'evidence_test.py'
raw=old.read_bytes()
pin,before=pins[family]
assert hashlib.sha256(raw).hexdigest()==pin
source=raw.decode()
assert source.count(before)==1
source=source.replace(before,'verify(root,'+repr(current)+')',1)
exec(compile(source,str(old)+':C212-current','exec'),{'__file__':str(old),'__name__':'__main__'})
for p in sorted(data.glob('freeze-v*.json')):
    freeze=json.loads(p.read_bytes())
    archive=data/'evidence'/('inputs-v'+str(freeze['version'])+'.zip')
    assert hashlib.sha256(archive.read_bytes()).hexdigest()==freeze['archive_sha256']
    with zipfile.ZipFile(archive) as z:
        assert set(z.namelist())==set(freeze['files'])
        for name,digest in freeze['files'].items():
            assert hashlib.sha256(z.read(name)).hexdigest()==digest
            assert hashlib.sha256((data/name).read_bytes()).hexdigest()==digest
print('P1117 all immutable input versions plus trusted current history PASS')
