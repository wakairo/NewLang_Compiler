"""Execute every original evidence assertion, selecting only current history."""
from pathlib import Path
import hashlib
import json
import sys
import zipfile
root=Path(__file__).resolve().parents[2]
data=root/'tests/p1117'
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
source=source.replace(before,"verify(root,'tests/p990/current-freeze-v23.json')",1)
exec(compile(source,str(old)+':C212-current','exec'),{'__file__':str(old),'__name__':'__main__'})
sys.path.insert(0,str(data))
from history_v23 import verify
verify(root,'tests/p990/current-freeze-v23.json')
freeze=json.loads((data/'freeze-v4.json').read_bytes())
with zipfile.ZipFile(data/'evidence/inputs-v4.zip') as z:
    assert set(z.namelist())==set(freeze['files'])
    for name,digest in freeze['files'].items():
        assert hashlib.sha256(z.read(name)).hexdigest()==digest
        assert hashlib.sha256((data/name).read_bytes()).hexdigest()==digest
print('P1117 prospective mechanical freeze-v4 and inherited history-v23 PASS')
