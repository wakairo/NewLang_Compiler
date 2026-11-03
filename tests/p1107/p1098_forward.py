"""Preserve P1098 assertions while selecting the prospective current freeze."""
from pathlib import Path
import hashlib

root = Path(__file__).resolve().parents[2]
old = root / 'tests/p1098/evidence_test.py'
raw = old.read_bytes()
# The fixed adopted original remains unchanged; this is a version selection,
# not a replacement of its source/oracle/archive/history assertions.
PIN = 'a22eeaf64dbe29e7cd4b58babc0f380ef650e3919194e94d7161b928d5c4b7d4'
assert hashlib.sha256(raw).hexdigest() == PIN
source = raw.decode()
before = "verify(root, 'tests/p990/current-freeze-v20.json')"
assert source.count(before) == 1
source = source.replace(before,"verify(root, 'tests/p990/current-freeze-v21.json')",1)
exec(compile(source,str(old),'exec'),{'__file__':str(old),'__name__':'__main__'})
