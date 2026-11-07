"""C283: authentic v7 inputs/checks, independently derived CURRENT v8.

Historical checker PASS authenticates original bytes, never the bad compiler.
"""
from pathlib import Path
import base64
import json
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/support'))
from versioned_freeze import FreezeError, digest, git_oid, json_data, local, require
import evidence_current_v7 as previous

FIXED = 'c2bbb2dc5047677ed38d80968ed9a2bacce9b374'
TREE = 'e1ce870645ba3ba37d2927221a1d31223ed45742'
CAPSULE = 'tests/p1227/c283/evidence/fixed-c2bbb-capsule-v1.json'
CAPSULE_SHA = 'd3beb2d826954f49f4c9ded4e798ef6c92360e164f8e8152a21dcf35f9f3a268'
CMAKE = 'tests/p1217/CMakeLists.txt'
CONTRACT = 'tests/p1217/execution-freeze-v8.json'
REGISTRATION = b'\ninclude(tests/p1227/c283/CMakeLists.txt)\n'
MATERIAL_FIRST = 'tests/p1227/c283/evidence/material-FIRST-c2bbb-v1.zip'
MATERIAL_SHA = '3e19d629a827db6eb0e2ffa14878f71b939a87f854839f85d99d26d7f982b89f'
CURRENT_FILES = set([
    "tests/p1217/CMakeLists.txt",
    "tests/p1217/evidence_current_v8.py",
    "tests/p1227/c283/CMakeLists.txt",
    "tests/p1227/c283/README.md",
    "tests/p1227/c283/alias_oom.py",
    "tests/p1227/c283/evidence/ACTUAL-START-v1.json",
    "tests/p1227/c283/evidence/fixed-c2bbb-capsule-v1.json",
    "tests/p1227/c283/evidence/harness-pre-execution-v1.json",
    "tests/p1227/c283/evidence/harness-pre-execution-v2.json",
    "tests/p1227/c283/evidence/history-predecessors-v1.zip",
    "tests/p1227/c283/evidence/material-FIRST-c2bbb-v1.zip",
    "tests/p1227/c283/evidence/production-draft1-before-build.json",
    "tests/p1227/c283/evidence/production-draft2-before-build.json",
    "tests/p1227/c283/evidence_negative.py",
    "tests/p1227/c283/history_negative.py",
    "tests/p1227/c283/history_v61.py",
    "tests/p1227/c283/observe.c",
    "tests/p1227/c283/pre-execution-v3.json",
    "tests/p1227/c283/return_safety.c",
    "tests/p1227/c283/source-oracle-freeze-v2.json",
    "tests/p1227/c283/source_native.py",
    "tests/p1227/c283/sources/A-alias-postH.nl",
    "tests/p1227/c283/sources/A-alias-preH.nl",
    "tests/p1227/c283/sources/A-different-tail-postH.nl",
    "tests/p1227/c283/sources/A-different-tail-preH.nl",
    "tests/p1227/c283/sources/A-equal-bits-tail-postH.nl",
    "tests/p1227/c283/sources/A-equal-bits-tail-preH.nl",
    "tests/p1227/c283/sources/A-explicit-final-postH.nl",
    "tests/p1227/c283/sources/A-explicit-final-preH.nl",
    "tests/p1227/c283/sources/A-old-material-preH.nl",
    "tests/p1227/c283/sources/A-unrelated-alias-postH.nl",
    "tests/p1227/c283/sources/A-unrelated-alias-preH.nl"
])


def authenticate(root):
    root = Path(root).resolve()
    require(sys.flags.optimize == 0, 'assertions enabled')
    raw = local(root, CAPSULE).read_bytes()
    require(digest(raw) == CAPSULE_SHA, 'fixed-c2bbb capsule digest')
    capsule = json_data(raw)
    data = base64.b64decode(capsule['commit_base64'], validate=True)
    require(capsule['commit'] == FIXED and capsule['tree'] == TREE and
            git_oid('commit', data) == FIXED and
            data.splitlines()[0] == ('tree ' + TREE).encode(), 'fixed-c2bbb commit/tree')
    entries, subtree, lookup = previous.old.entries, previous.old.subtree, previous.old.lookup
    tests_mode, tests = entries(capsule, TREE)['tests']
    require(tests_mode == '40000', 'fixed-c2bbb tests mode')
    expected = {}
    for name in ['p1217', 'p1227']:
        mode, own = entries(capsule, tests)[name]
        require(mode == '40000', 'fixed-c2bbb subtree mode')
        expected.update(subtree(capsule, own, 'tests/' + name + '/'))
    for name in ['CMakeLists.txt', 'tests/support/versioned_freeze.py']:
        expected[name] = lookup(capsule, TREE, name.split('/'))
    require(capsule['files'].keys() == expected.keys(), 'fixed-c2bbb complete original inventory')
    payload = {}
    for name, oid in expected.items():
        pin = capsule['files'][name]
        require(pin['oid'] == oid, 'fixed-c2bbb blob membership: ' + name)
        data = (base64.b64decode(capsule['retained_blobs_base64'][oid], validate=True)
                if oid in capsule['retained_blobs_base64'] else local(root, name).read_bytes())
        require(git_oid('blob', data) == oid and digest(data) == pin['sha256'],
                'fixed-c2bbb blob: ' + name)
        payload[name] = data
    before = payload[CMAKE]
    require(before.count(b'evidence_current_v7.py') == 1 and
            before.count(b'../p1227/evidence_negative.py') == 1, 'fixed-c2bbb selectors')
    wanted = before.replace(b'evidence_current_v7.py', b'evidence_current_v8.py')
    wanted = wanted.replace(b'../p1227/evidence_negative.py', b'../p1227/c283/evidence_negative.py')
    require(local(root, CMAKE).read_bytes() == wanted + REGISTRATION,
            'CURRENT v8 registration independently derived from c2bbb')
    require(digest(local(root, MATERIAL_FIRST).read_bytes()) == MATERIAL_SHA,
            'original MATERIAL FIRST packet')
    contract = json_data(local(root, CONTRACT).read_bytes())
    require(contract['version'] == 8 and contract['before_execution'] is True and
            contract['fixed_head'] == FIXED and contract['fixed_tree'] == TREE, 'CURRENT v8 authority')
    require(contract['files'].keys() == CURRENT_FILES, 'CURRENT v8 exact dependency keys')
    for name, pin in contract['files'].items():
        require(digest(local(root, name).read_bytes()) == pin, 'CURRENT v8 source digest: ' + name)
    return payload


def verify(root):
    root = Path(root).resolve()
    payload = authenticate(root)
    with tempfile.TemporaryDirectory(prefix='p1227-authentic-v7-') as directory:
        view = Path(directory)
        for name, data in payload.items():
            p = view / name
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(data)
        old_check = previous.old.original_run(view / 'tests/p1217/evidence_current_v7.py')
        require(old_check.returncode == 0, 'unchanged v7/authentic input: ' + old_check.stderr.decode(errors='replace'))
        old_negative = previous.old.original_run(view / 'tests/p1227/evidence_negative.py')
        require(old_negative.returncode == 0, 'unchanged v7 negative suite: ' + old_negative.stderr.decode(errors='replace'))
    current_old = previous.old.original_run(root / 'tests/p1217/evidence_current_v7.py')
    require(current_old.returncode == 1 and b'CURRENT registration independently derived' in current_old.stderr,
            'unchanged v7 must reject new registration')
    return dict(authentic_historical_head=FIXED, historical_files=len(payload),
                old_v7_checker=0, old_v7_negatives=0, old_v7_on_current=1,
                current=8, historical_semantic_pass_claim=False)


if __name__ == '__main__':
    try:
        print('C283 CURRENT v8 original-v7/history/current PASS', json.dumps(verify(ROOT), sort_keys=True))
    except (FreezeError, OSError, ValueError, KeyError, TypeError) as error:
        sys.exit('C283 CURRENT v8 REFUSE: ' + str(error))
