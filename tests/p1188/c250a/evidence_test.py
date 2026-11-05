"""Preserve the unsafe-output FIRST without ever compiling its guest C."""
from pathlib import Path
import hashlib
import json
import zipfile

here = Path(__file__).resolve().parent
root = here.parents[2]
sha = lambda data: hashlib.sha256(data).hexdigest()
assert sha((here / 'source-freeze-v1.json').read_bytes()) == (
    '0307be2349d9ce583b2948fed39fdfc76114a9f33ccc4d465f66b7b18d964902')
for name in ['source-freeze-v1.json', 'equipment-freeze-v1.json',
             'equipment-freeze-v2.json']:
    for path, expected in json.loads((here / name).read_bytes())['files'].items():
        assert sha((root / path).read_bytes()) == expected, (name, path)
first = here / 'evidence/FIRST-fixed8585-v1.zip'
assert sha(first.read_bytes()) == (
    '0e62a9f7be4e94751c28c805a750a8fa9ccf8aacf7657649669f453096b23b6a')
with zipfile.ZipFile(first) as archive:
    assert archive.testzip() is None
print('C250a original source/oracle/equipment and fixed8585 CHECK/C17 FIRST PASS')
