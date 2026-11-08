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
# C274: preserve accepted v4 bytes and its full input set before selecting v5.
accepted_bytes = (current / 'p1203-freeze-v4.json').read_bytes()
next_current = json.loads((root / 'tests/p1217/c274/p1203-freeze-v5.json').read_bytes())
assert next_current['previous_sha256'] == sha(accepted_bytes)
assert next_current['files'].keys() == new['files'].keys()
assert sha((root / next_current['archive']).read_bytes()) == next_current['archive_sha256']
with zipfile.ZipFile(root / next_current['archive']) as z:
    assert len(z.namelist()) == len(set(z.namelist())) and set(z.namelist()) == set(new['files'])
    for name, expected in new['files'].items():
        assert sha(z.read(name)) == expected, name
        if name not in {'src/current_loop_check_internal.h', 'src/linear_break_internal.h'}:
            assert next_current['files'][name] == expected, name
# C288: v5 itself is immutable and authenticated independently of v6.
v5_raw = (root / 'tests/p1217/c274/p1203-freeze-v5.json').read_bytes()
assert sha(v5_raw) == '503f40e874bde597ee899d74a5ea545e92559bdc1d6420ddd7bd5f95a836a61c'
assert hashlib.sha1(b'blob ' + str(len(v5_raw)).encode() + b'\0' + v5_raw).hexdigest() == '6bb845ff9e1e7910795607c9912a3691d0e54595'
v6 = json.loads((root / 'tests/p1236/p1203-freeze-v6.json').read_bytes())
assert v6['previous_sha256'] == sha(v5_raw)
assert v6['files'].keys() == next_current['files'].keys()
assert sha((root / v6['archive']).read_bytes()) == v6['archive_sha256'] == '5be77f91591e01e705fc2a2ee819d8e82b4720fddc7727c42070cbc41678f605'
with zipfile.ZipFile(root / v6['archive']) as z:
    assert set(z.namelist()) == set(next_current['files']) | {'tests/p1217/c274/p1203-freeze-v5.json'}
    assert z.read('tests/p1217/c274/p1203-freeze-v5.json') == v5_raw
    for name, expected in next_current['files'].items():
        assert sha(z.read(name)) == expected, name
        if name not in {'src/current_loop_check_internal.h', 'src/linear_break_internal.h'}:
            assert v6['files'][name] == expected, name
source = source.replace(before, "'../p1236/p1203-freeze-v6.json'", 1)
exec(compile(source, str(original) + ':C266-current', 'exec'),
     {'__file__': str(original), '__name__': '__main__'})
print('C266 adopted P1203 v3 archived/authenticated; unchanged assertions + current v6 PASS')
