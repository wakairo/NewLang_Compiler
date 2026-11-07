"""Authenticate the original failures and each forward apparatus/source generation."""
from pathlib import Path
import hashlib
import json
import zipfile

here = Path(__file__).resolve().parent
sha = lambda b: hashlib.sha256(b).hexdigest()
for version in [1, 2, 3]:
    manifest = json.loads((here / f'source-freeze-v{version}.json').read_bytes())
    for name, expected in manifest['files'].items():
        assert sha((here / name).read_bytes()) == expected, name
current = json.loads((here / 'execution-freeze-v3.json').read_bytes())
assert sha((here / current['prior_apparatus_archive']).read_bytes()) == current['prior_apparatus_sha256']
with zipfile.ZipFile(here / current['prior_apparatus_archive']) as old:
    previous = json.loads(old.read('execution-freeze-v2.json'))
    assert old.read('execution-freeze-v2.json') == (here / 'execution-freeze-v2.json').read_bytes()
    for name, expected in previous['files'].items():
        assert sha(old.read(name)) == expected, name
for name, expected in current['files'].items():
    assert sha((here / name).read_bytes()) == expected, name
assert sha((here / 'evidence/P1214-Gate0-source-oracle-FIRST-raw-v1.zip').read_bytes()) == '4db34028375317e70ab49bce76339ca30f656bb49c5a49b7ef30ab5e9aad3eb5'
with zipfile.ZipFile(here / 'evidence/P1214-Gate0-source-oracle-FIRST-raw-v1.zip') as original:
    assert sha(original.read('freeze/source-freeze-v1.json')) == '0da6401d694e6b0118463908c27dd776faac0f879125c1050a3ad0f09ebdcfa7'
    assert sha(original.read('freeze/oracle-v1.json')) == '1c05bcb2e289d1d4a2dd216e6e73d1f93928eef12d92ae09b467a944c2b4e4e8'
    before = json.loads(original.read('records/Gate0-FIRST-summary.json'))
    for row in before:
        if row['source'].startswith(('03-', '04-')):
            assert row['check'] == 0 and row['emit'] == 4
    for name in ['03-postheap-current-loop.nl', '04-preheap-original-current-loop.nl']:
        assert original.read('freeze/sources/' + name) == (here / 'sources' / name).read_bytes()
print('P1217 original P1214 / new source / every FIRST / forward equipment PASS')
more = json.loads((here / 'execution-freeze-v4.json').read_bytes())
for name, expected in more['files'].items():
    assert sha((here / name).read_bytes()) == expected, name
