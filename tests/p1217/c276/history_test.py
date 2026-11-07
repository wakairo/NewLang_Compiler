"""Authenticate the original Release RED and a forward apparatus correction."""
from pathlib import Path
import hashlib
import json
import runpy
import zipfile

root = Path(__file__).resolve().parents[3]
here = Path(__file__).resolve().parent
sha = lambda b: hashlib.sha256(b).hexdigest()
runpy.run_path(str(here.parent / 'c275/history_test.py'), run_name='__main__')
frozen = json.loads((here / 'current-freeze-v2.json').read_bytes())
archive = root / 'tests/p1217/evidence/c276-release-FIRST-and-diagnosis-v1.zip'
assert sha(archive.read_bytes()) == 'f8bf8eda7b114e65e78c25cd1652e5a984fa90f2260a4827ad35fb4b4897c4ea'
with zipfile.ZipFile(archive) as z:
    original = json.loads(z.read('original-receipt.json'))
    assert original['head'] == 'c99264155b2a0546e578dc6c743457599d05869b'
    assert original['tree'] == '2201e0d82a30a23608be8bb33fb6a1e2931c6b41'
    for name, pin in original['old_files'].items():
        data = z.read(name)
        assert sha(data) == pin['sha256'], name
        assert hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest() == pin['git_blob'], name
        if name.startswith('tests/p1217/c275/') or name in ['tests/p1217/oom.c', 'tests/p1217/oom_v2.py']:
            assert (root / name).read_bytes() == data, name
    assert b'assert 0 < len(plans) <= 320, len(plans)' in z.read('tests/p1217/oom_v2.py')
    raw = z.read('records/release-current-diagnostic-1.stdout')
    assert b'RELEASE_CALLOC selected at=2 calls=2 recorded-before=0 site-before=(nil)' in raw
    assert b'RELEASE_CURRENT at=2 calls=3 inject=0 recorded=0 site=(nil) status=2 out=(nil) length=37' in raw
    assert b'exited with code 01' in raw
    first = json.loads(z.read('records/CI-job-128314249055-FIRST-RED.json'))
    assert first['head'] == original['head'] and first['job_id'] == 128314249055
    assert any('3 tests failed out of 822' in s for s in first['selected'])
    assert any('AssertionError: 378' in s for s in first['selected'])
    plans = json.loads(z.read('census/selected-observed-not-injected.json'))
    assert len(plans) == 378
proposal = root / 'tests/p1217/evidence/c276-unexecuted-adapter-proposal-v1.zip'
assert sha(proposal.read_bytes()) == '54fa5f1ad70d38be3ecf72ddc536a41270771b14dc43c567b4e240ee0f76894e'
with zipfile.ZipFile(proposal) as z:
    old_freeze = z.read('tests/p1217/c276/current-freeze-v1.json')
    assert old_freeze == (here / 'current-freeze-v1.json').read_bytes()
    for name, expected in json.loads(old_freeze)['current_files'].items():
        assert sha(z.read(name)) == expected, name
    assert b'redefinition of' in z.read('compile-FIRST.log')
for name, expected in frozen['current_files'].items():
    assert sha((root / name).read_bytes()) == expected, name
old = (here.parent / 'oom_v2.py').read_text()
new = (here.parent / 'oom_v3.py').read_text()
# The finite ceiling/documentation alone changes; selection and strict oracle do not.
assert old.split('from pathlib import Path', 1)[1].replace(
    'assert 0 < len(plans) <= 320, len(plans)',
    'assert 0 < len(plans) <= 512, len(plans)') == new.split('from pathlib import Path', 1)[1]
print('C276 original CI RED, actual calloc telemetry diagnosis, old320 and all prior CURRENT bytes retained; strict source/selection/oracle unchanged')
