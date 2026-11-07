"""C280: keep authentic v6 executable, independently admit CURRENT v7 registration.

Private test provenance only; this does not create source or memory authority.
"""
from pathlib import Path
import base64
import json
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/support'))
from versioned_freeze import FreezeError, digest, git_oid, json_data, local, require
import evidence_current_v6 as old

FIXED = 'aa8dce8935f44d7b0732d24695f1c0f7c305efb1'
TREE = '1e231eee06332990adf840885eca39d10065a8e3'
CAPSULE = 'tests/p1227/evidence/fixed-aa-capsule-v1.json'
CAPSULE_SHA = '4f8e63a3651a9c7632e623d76e392e6ec7b1f5ef1715fe611ada46c020a66116'
CMAKE = 'tests/p1217/CMakeLists.txt'
REGISTRATION = b'\ninclude(tests/p1227/CMakeLists.txt)\n'
CONTRACT = 'tests/p1217/execution-freeze-v7.json'
# EXACT_CURRENT_FILES is filled once before the first execution, not from a manifest.
EXACT_CURRENT_FILES = set([
    "tests/p1217/CMakeLists.txt",
    "tests/p1217/evidence_current_v7.py",
    "tests/p1227/CMakeLists.txt",
    "tests/p1227/README.md",
    "tests/p1227/evidence/fixed-aa-capsule-v1.json",
    "tests/p1227/evidence/history-predecessors-v1.zip",
    "tests/p1227/evidence/native-freeze-v1.json",
    "tests/p1227/evidence/oracle-v1.json",
    "tests/p1227/evidence/owned-probe-FIRST-actual-counter-clarification.json",
    "tests/p1227/evidence/owned-probe-freeze-v1.json",
    "tests/p1227/evidence/owned-probe-freeze-v2.json",
    "tests/p1227/evidence/phase0-FIRST-v1.zip",
    "tests/p1227/evidence/phase0-freeze-v1.json",
    "tests/p1227/evidence/phase1-apparatus-FIRST-v1.zip",
    "tests/p1227/evidence/phase1-build-FIRST-result.json",
    "tests/p1227/evidence/phase1-pre-format-v1.zip",
    "tests/p1227/evidence/phase1-production-draft1-freeze.json",
    "tests/p1227/evidence/phase1-production-draft2-freeze.json",
    "tests/p1227/evidence_negative.py",
    "tests/p1227/history_v60.py",
    "tests/p1227/native_observer.h",
    "tests/p1227/observe.c",
    "tests/p1227/oom.c",
    "tests/p1227/oom_gate.py",
    "tests/p1227/oracle-v1.json",
    "tests/p1227/pre-execution-v1.json",
    "tests/p1227/pre-execution-v2.json",
    "tests/p1227/pre-execution-v3.json",
    "tests/p1227/safety.c",
    "tests/p1227/source_native.py",
    "tests/p1227/source_native_v2.py",
    "tests/p1227/sources/A-inverse-postH.nl",
    "tests/p1227/sources/A-inverse-preH.nl",
    "tests/p1227/sources/A-postH.nl",
    "tests/p1227/sources/A-preH.nl",
    "tests/p1227/sources/B-postH.nl",
    "tests/p1227/sources/B-preH.nl",
    "tests/p1227/sources/N-foreign-owner.nl",
    "tests/p1227/sources/N-loan-escape.nl",
    "tests/p1227/sources/N-return-type.nl"
])


def authenticate(root):
    root = Path(root).resolve()
    require(sys.flags.optimize == 0, 'assertions enabled')
    raw = local(root, CAPSULE).read_bytes()
    require(digest(raw) == CAPSULE_SHA, 'fixed-aa capsule digest')
    capsule = json_data(raw)
    data = base64.b64decode(capsule['commit_base64'], validate=True)
    require(capsule['commit'] == FIXED and capsule['tree'] == TREE and
            git_oid('commit', data) == FIXED and data.splitlines()[0] == ('tree ' + TREE).encode(),
            'fixed-aa commit/tree')
    tests_mode, tests = old.entries(capsule, TREE)['tests']
    own_mode, own = old.entries(capsule, tests)['p1217']
    require(tests_mode == own_mode == '40000', 'fixed-aa tree mode')
    expected = old.subtree(capsule, own, 'tests/p1217/')
    for name in ['CMakeLists.txt', 'tests/support/versioned_freeze.py']:
        expected[name] = old.lookup(capsule, TREE, name.split('/'))
    require(capsule['files'].keys() == expected.keys(), 'fixed-aa complete original inventory')
    payload = {}
    for name, oid in expected.items():
        pin = capsule['files'][name]
        require(pin['oid'] == oid, 'fixed-aa blob membership: ' + name)
        data = (base64.b64decode(capsule['retained_blobs_base64'][oid], validate=True)
                if oid in capsule['retained_blobs_base64'] else local(root, name).read_bytes())
        require(git_oid('blob', data) == oid and digest(data) == pin['sha256'],
                'fixed-aa blob: ' + name)
        payload[name] = data
    expected_current = payload[CMAKE]
    require(expected_current.count(b'evidence_current_v6.py') == 1 and
            expected_current.count(b'c277/negative_test.py') == 1,
            'fixed-aa registration selectors')
    expected_current = expected_current.replace(b'evidence_current_v6.py', b'evidence_current_v7.py')
    expected_current = expected_current.replace(b'c277/negative_test.py', b'../p1227/evidence_negative.py')
    expected_current += REGISTRATION
    require(local(root, CMAKE).read_bytes() == expected_current,
            'CURRENT registration independently derived from authentic aa')
    contract = json_data(local(root, CONTRACT).read_bytes())
    require(contract['version'] == 7 and contract['before_execution'] is True and
            contract['fixed_head'] == FIXED and contract['fixed_tree'] == TREE,
            'CURRENT v7 authority')
    require(contract['files'].keys() == EXACT_CURRENT_FILES, 'CURRENT exact dependency keys')
    for name, pin in contract['files'].items():
        require(digest(local(root, name).read_bytes()) == pin, 'CURRENT source digest: ' + name)
    return payload


def verify(root):
    root = Path(root).resolve()
    payload = authenticate(root)
    # Authenticate and materialize the complete OLD input tree, never patch an
    # old checker/manifest or feed it current CMake pretending that it is old.
    with tempfile.TemporaryDirectory(prefix='p1227-genuine-aa-') as directory:
        view = Path(directory)
        for name, data in payload.items():
            target = view / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        genuine = old.original_run(view / 'tests/p1217/evidence_current_v6.py')
        require(genuine.returncode == 0, 'authentic old v6 checker: ' + genuine.stderr.decode(errors='replace'))
        negatives = old.original_run(view / 'tests/p1217/c277/negative_test.py')
        require(negatives.returncode == 0, 'authentic old v6 negative suite: ' + negatives.stderr.decode(errors='replace'))
    rejected = old.original_run(root / 'tests/p1217/evidence_current_v6.py')
    require(rejected.returncode == 1 and b'CURRENT registration independently derived' in rejected.stderr,
            'unchanged old v6 must still reject CURRENT registration')
    return dict(original_head=FIXED, original_files=len(payload), old_v6=0,
                old_v6_negatives=0, old_v6_on_current=1, current=7)


if __name__ == '__main__':
    try:
        print('P1217 CURRENT v7 authentic v6/history/current PASS', json.dumps(verify(ROOT), sort_keys=True))
    except (FreezeError, OSError, ValueError, KeyError, TypeError) as error:
        sys.exit('P1217 CURRENT v7 REFUSE: ' + str(error))
