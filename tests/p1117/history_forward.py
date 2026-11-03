"""Keep every old evidence assertion; select the legitimate successor only."""
from pathlib import Path
import hashlib
import sys

root = Path(__file__).resolve().parents[2]
family = sys.argv[1]
pins = {'p1098': 'a22eeaf64dbe29e7cd4b58babc0f380ef650e3919194e94d7161b928d5c4b7d4',
        'p1107': 'e778afc9fd22da945f00eed50dc93bde3b86c1d34bb743c03d641b204e58831d'}
old = root/'tests'/family/'evidence_test.py'
raw = old.read_bytes()
assert hashlib.sha256(raw).hexdigest() == pins[family]
source = raw.decode()
before = "verify(root, 'tests/p990/current-freeze-v20.json')" if family=='p1098' else 'verify(root,\'tests/p990/current-freeze-v21.json\')'
assert source.count(before)==1
source=source.replace(before,"verify(root, 'tests/p990/current-freeze-v22.json')",1)
exec(compile(source,str(old)+':C212-current','exec'),{'__file__':str(old),'__name__':'__main__'})
