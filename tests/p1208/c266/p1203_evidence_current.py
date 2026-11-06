"""Run authenticated P1203 assertions with a prospective current selector."""
from pathlib import Path
import hashlib
import json
import zipfile

root = Path(__file__).resolve().parents[3]
here = root / 'tests/p1203'
current = root / 'tests/p1208/c266'
sha = lambda data: hashlib.sha256(data).hexdigest()
original = here / 'evidence_test.py'
raw = original.read_bytes()
assert sha(raw) == '77afd1222d3bf94f25c0308159c04772aab655dd47adb75491523a60b1f04dda'
old_bytes = (here / 'focused-freeze-v3.json').read_bytes()
assert sha(old_bytes) == 'c0c4fbef92159e5493b9f9a7453314da5a1e92d29171b9391fbbaafc56510133'
old = json.loads(old_bytes)
receipt = json.loads((current / 'P1203-adopted-inputs-v3.json').read_bytes())
assert sha((root / receipt['archive']).read_bytes()) == receipt['sha256']
with zipfile.ZipFile(root / receipt['archive']) as z:
    assert set(z.namelist()) == set(receipt['files'])
    for name, expected in receipt['files'].items():
        assert sha(z.read(name)) == expected, name
    for name, expected in old['files'].items():
        assert sha(z.read(name)) == expected, name
new = json.loads((current / 'p1203-freeze-v4.json').read_bytes())
assert new['previous_sha256'] == sha(old_bytes)
assert new['files'].keys() == old['files'].keys()
changes = {'src/current_loop_check_internal.h', 'src/main.c', 'tests/p1203/CMakeLists.txt'}
for name, expected in old['files'].items():
    if name not in changes:
        assert new['files'][name] == expected, name
before = "'focused-freeze-v3.json'"
source = raw.decode()
assert source.count(before) == 1
source = source.replace(before, "'../p1208/c266/p1203-freeze-v4.json'", 1)
exec(compile(source, str(original) + ':C266-current', 'exec'),
     {'__file__': str(original), '__name__': '__main__'})
print('C266 adopted P1203 v3 archived/authenticated; unchanged assertions + current v4 PASS')
