"""Original Gate0/first prototype and prospective experiment freezes stay exact."""
from pathlib import Path
import hashlib
import json
import zipfile

here = Path(__file__).resolve().parent
root = here.parents[1]
sha = lambda data: hashlib.sha256(data).hexdigest()
with zipfile.ZipFile(here / 'evidence/P1202-gate0-v1.zip') as z:
    manifest = json.loads(z.read('manifest-v1.json'))
    for name, expected in manifest['files'].items():
        data = z.read(name)
        assert len(data) == expected['bytes'] and sha(data) == expected['sha256']
    old = json.loads(z.read('raw/result-v1.json'))
    assert [row['status'] for row in old['queries']] == [0, 0, 0, 0, 3, 3]
    assert z.read('sources/02-independent-break.nl') == (here / 'sources/01-many.nl').read_bytes()
with zipfile.ZipFile(here / 'evidence/C260-first-prototype-v1.zip') as z:
    original = json.loads(z.read('inputs/focused-freeze-v1.json'))
    assert sha(z.read('inputs/safety-v1.c')) == original['files']['tests/p1203/safety.c']
    assert b'cmake: command not found' in z.read('cmake-PATH-FIRST.log')
with zipfile.ZipFile(here / 'evidence/focused-inputs-v2.zip') as z:
    original = json.loads(z.read('focused-freeze-v2.json'))
    for name, wanted in original['files'].items():
        assert sha(z.read(name)) == wanted, name
for freeze in ['source-freeze-v1.json', 'focused-freeze-v3.json']:
    for name, wanted in json.loads((here / freeze).read_text())['files'].items():
        path = root / name if name.startswith(('tests/', 'src/')) else here / name
        assert sha(path.read_bytes()) == wanted, (freeze, name)
print('P1203 P1202 original FIRST/raw, first prototype and prospective freezes PASS')
