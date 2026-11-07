"""Original P733/P738 assertions and measured FIRST remain historical truth."""
from pathlib import Path
import hashlib
import json
import zipfile

root = Path(__file__).resolve().parents[3]
here = Path(__file__).resolve().parent
sha = lambda b: hashlib.sha256(b).hexdigest()
frozen = json.loads((here / 'current-freeze-v2.json').read_bytes())
OLD_PINS = {'tests/unit/live_return_backend_test.c': ('d87cf2a04938687c4774db5484a12f5c540fcd1096f3e3f32c99802a0a0427b6', '1e75111852c4e28bfb75818d891870bb1822445f'), 'tests/p733/gate_test.c': ('89a74a19a907acb29629adf7bf63013c7c027d4eb2d3a467b4ac893435382eea', '2607f6bee4178dc22def50518c25c26737d45232'), 'tests/p738/gate_test.c': ('3dcbbff223bbe1d20728926c3162d0430d35de25657a8e5659a49c7e35cc37cf', '8214c14a1c0855919deef9139f79b5f65c5d73cd'), 'tests/p738/sprout.nl': ('a9e24ba9ba0436f417118fbee3bd1706ab8f20faa29ef195f690c87b3ea2ca5e', '1328f5b7dd67512837181125fe4d93916f61f409'), 'tests/p738/ticket.nl': ('752c120c7b17fb12a9c94d2853d85d758fd7d159754170ca0259df51e1aa2ac9', '32d03b2f2bcf3d4f4429c5bc358a9abf6139d616')}
assert frozen['old_inputs'] == {n: pair[0] for n, pair in OLD_PINS.items()}
assert frozen['old_git_blobs'] == {n: pair[1] for n, pair in OLD_PINS.items()}
for name, expected in frozen['old_inputs'].items():
    data = (root / name).read_bytes()
    assert sha(data) == expected, name
    assert hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest() == frozen['old_git_blobs'][name], name
for name, expected in frozen['current_files'].items():
    assert sha((root / name).read_bytes()) == expected, name
archive = root / frozen['archive']
assert sha(archive.read_bytes()) == frozen['archive_sha256']
with zipfile.ZipFile(archive) as z:
    for name, expected in frozen['old_inputs'].items():
        assert sha(z.read(name)) == expected, name
    assert b'at < 32 && staging_failures == 3 && validation_failures != 0' in z.read('tests/unit/live_return_backend_test.c')
    assert b'p738_local_return_existing_controls.unit' in z.read('C274-original-p738-FIRST-regression.log')
    for config in ['gcc', 'asan']:
        actual = json.loads(z.read('diagnostic/records/' + config + '-diagnostic-1.json'))
        assert actual['final'] == dict(at=19, calls=19, staging_failures=19, validation_failures=0, conjuncts=[True, False, False])
        assert actual['inferior_exit'] == 1
        failed = [x for x in actual['trials'] if x['status']]
        assert len(failed) == 19
        for row in failed:
            assert row['status'] == 2 and row['out_null'] and row['length'] == 37 and row['root_world_fields_unchanged'] and row['fault']
            assert len([x for x in actual['retries'] if x['at'] == row['at'] and x['success']]) == 2
        assert all(x['actual_return'] == 4 for x in actual['causes'])
        assert actual['trials'][-1]['status'] == 0 and actual['trials'][-1]['fault'] is None
print('C275 original source/oracle/RED + exact measured classification + versioned CURRENT PASS')

proposal = root / 'tests/p1217/evidence/c275-current-proposal-v1.zip'
assert sha(proposal.read_bytes()) == frozen['prior_proposal_sha256']
with zipfile.ZipFile(proposal) as z:
    old = json.loads(z.read('current-freeze-v1.json'))
    for name, expected in old['current_files'].items():
        assert sha(z.read(name)) == expected, name
print('C275 unexecuted v1 proposal retained; literal fixed-main SHA/Git anchors PASS')
