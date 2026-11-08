"""C288: authentic v8 inputs/checks, independently derived CURRENT v9.

Historical checker PASS authenticates original bytes, never a general compiler-soundness claim.
"""
from pathlib import Path
import base64
import json
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/support'))
from versioned_freeze import FreezeError, digest, git_oid, json_data, local, require
import evidence_current_v8 as previous

FIXED = '2931ba92a72b08a9fdf247cb2f7700078c131b6d'
TREE = '07c423b373e09680093c3423296c66d251a722f1'
CAPSULE = 'tests/p1236/evidence/fixed-main2931-capsule-v2.json'
CAPSULE_SHA = 'e919526637603de966d9bc6d72aa31d11ce5155ce770591059d57d8c3f63e371'
CMAKE = 'tests/p1217/CMakeLists.txt'
CONTRACT = 'tests/p1217/execution-freeze-v9.json'
REGISTRATION = b'\ninclude(tests/p1236/CMakeLists.txt)\n'
MATERIAL_FIRST = 'tests/p1236/evidence/source-FIRST-and-apparatus-v1.zip'
MATERIAL_SHA = '63258b764f3348f91eaf1d87108fd9b61f3ce4a248a35b934940981461200a70'
CURRENT_FILES = set([
    "src/checked_c_current_emit_internal.h",
    "src/checked_c_current_internal.h",
    "src/current_loop_check_internal.h",
    "src/linear_break_internal.h",
    "tests/p1208/c266/p1203_evidence_current.py",
    "tests/p1217/CMakeLists.txt",
    "tests/p1217/evidence_current_v8.py",
    "tests/p1217/evidence_current_v9.py",
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
    "tests/p1227/c283/sources/A-unrelated-alias-preH.nl",
    "tests/p1236/CMakeLists.txt",
    "tests/p1236/CURRENT-v9-keys-before-execution.json",
    "tests/p1236/README.md",
    "tests/p1236/evidence/fixed-main2931-capsule-v1.json",
    "tests/p1236/evidence/fixed-main2931-capsule-v2.json",
    "tests/p1236/evidence/history-predecessors-v1.zip",
    "tests/p1236/evidence/history-predecessors-v2.zip",
    "tests/p1236/evidence/history-predecessors-v3.zip",
    "tests/p1236/evidence/local-history-FIRST-candidate1-v1.zip",
    "tests/p1236/evidence/p1203-adopted-v5-inputs-v1.zip",
    "tests/p1236/evidence/source-FIRST-and-apparatus-v1.zip",
    "tests/p1236/evidence_negative.py",
    "tests/p1236/historical_v61_negative.py",
    "tests/p1236/history_negative.py",
    "tests/p1236/history_v62.py",
    "tests/p1236/native_observer.h",
    "tests/p1236/oom.c",
    "tests/p1236/oom_gate.py",
    "tests/p1236/oracle-v2.json",
    "tests/p1236/owned.c",
    "tests/p1236/p1203-freeze-v6.json",
    "tests/p1236/source_native.py",
    "tests/p1236/sources/A-bound-old-u8.nl",
    "tests/p1236/sources/B-direct-break-operand.nl",
    "tests/p1236/sources/C-other-index.nl",
    "tests/p1236/sources/D-post-H.nl",
    "tests/p1236/sources/E-equal-old-current.nl",
    "tests/p1236/sources/N-H1-cleanup-missing.nl",
    "tests/p1236/sources/N-H2-cleanup-missing.nl",
    "tests/p1236/sources/N-current-as-old.nl",
    "tests/p1236/sources/N-equal-bits-literal.nl",
    "tests/p1236/sources/N-equal-current-as-old.nl",
    "tests/p1236/sources/N-foreign-original.nl",
    "tests/p1236/sources/N-helper-static-type.nl",
    "tests/p1236/sources/N-overflow-backedge.nl",
    "tests/p1236/sources/N-sibling-write.nl"
])


def authenticate(root):
    root = Path(root).resolve()
    require(sys.flags.optimize == 0, 'assertions enabled')
    raw = local(root, CAPSULE).read_bytes()
    require(digest(raw) == CAPSULE_SHA, 'fixed-main2931 capsule digest')
    capsule = json_data(raw)
    data = base64.b64decode(capsule['commit_base64'], validate=True)
    require(capsule['commit'] == FIXED and capsule['tree'] == TREE and
            git_oid('commit', data) == FIXED and
            data.splitlines()[0] == ('tree ' + TREE).encode(), 'fixed-main2931 commit/tree')
    entries, subtree, lookup = previous.previous.old.entries, previous.previous.old.subtree, previous.previous.old.lookup
    tests_mode, tests = entries(capsule, TREE)['tests']
    require(tests_mode == '40000', 'fixed-main2931 tests mode')
    expected = {}
    for name in ['p1217', 'p1227']:
        mode, own = entries(capsule, tests)[name]
        require(mode == '40000', 'fixed-main2931 subtree mode')
        expected.update(subtree(capsule, own, 'tests/' + name + '/'))
    for name in ['CMakeLists.txt', 'tests/support/versioned_freeze.py']:
        expected[name] = lookup(capsule, TREE, name.split('/'))
    require(capsule['files'].keys() == expected.keys(), 'fixed-main2931 complete original inventory')
    payload = {}
    for name, oid in expected.items():
        pin = capsule['files'][name]
        require(pin['oid'] == oid, 'fixed-main2931 blob membership: ' + name)
        data = (base64.b64decode(capsule['retained_blobs_base64'][oid], validate=True)
                if oid in capsule['retained_blobs_base64'] else local(root, name).read_bytes())
        require(git_oid('blob', data) == oid and digest(data) == pin['sha256'],
                'fixed-main2931 blob: ' + name)
        payload[name] = data
    before = payload[CMAKE]
    require(before.count(b'evidence_current_v8.py') == 1 and
            before.count(b'../p1227/c283/evidence_negative.py') == 1, 'fixed-main2931 selectors')
    wanted = before.replace(b'evidence_current_v8.py', b'evidence_current_v9.py')
    wanted = wanted.replace(b'../p1227/c283/evidence_negative.py', b'../p1236/evidence_negative.py')
    require(local(root, CMAKE).read_bytes() == wanted + REGISTRATION,
            'CURRENT v9 registration independently derived from main2931')
    historical = 'tests/p1227/c283/CMakeLists.txt'
    original = payload[historical]
    old_selector = b'tests/p1227/c283/history_negative.py'
    require(original.count(old_selector) == 1 and local(root, historical).read_bytes() ==
            original.replace(old_selector, b'tests/p1236/historical_v61_negative.py'),
            'CURRENT v9 authentic historical-negative registration')
    require(digest(local(root, 'tests/p1236/evidence/local-history-FIRST-candidate1-v1.zip').read_bytes()) == '4cb4a676663694c15aa75d3c94d2c37bce0652cf372e8acc88886d043f4b9fdd',
            'original candidate1 history FIRST')
    require(digest(local(root, MATERIAL_FIRST).read_bytes()) == MATERIAL_SHA,
            'original MATERIAL FIRST packet')
    contract = json_data(local(root, CONTRACT).read_bytes())
    require(contract['version'] == 9 and contract['before_execution'] is True and
            contract['fixed_head'] == FIXED and contract['fixed_tree'] == TREE, 'CURRENT v9 authority')
    require(contract['files'].keys() == CURRENT_FILES, 'CURRENT v9 exact dependency keys')
    for name, pin in contract['files'].items():
        require(digest(local(root, name).read_bytes()) == pin, 'CURRENT v9 source digest: ' + name)
    oracle_name = 'tests/p1236/oracle-v2.json'
    oracle_raw = local(root, oracle_name).read_bytes()
    require(digest(oracle_raw) == 'a3dd4315d33baf308ed7296be553c30043c7d7bb9f5e38da4492c5dcafbbeaa4',
            'C288 externally frozen source oracle')
    for name, case in json_data(oracle_raw)['cases'].items():
        require(digest(local(root, 'tests/p1236/sources/' + name).read_bytes()) == case['sha256'],
                'C288 externally frozen complete source: ' + name)
    return payload


def verify(root):
    root = Path(root).resolve()
    payload = authenticate(root)
    with tempfile.TemporaryDirectory(prefix='p1236-authentic-v8-') as directory:
        view = Path(directory)
        for name, data in payload.items():
            p = view / name
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(data)
        old_check = previous.previous.old.original_run(view / 'tests/p1217/evidence_current_v8.py')
        require(old_check.returncode == 0, 'unchanged v8/authentic input: ' + old_check.stderr.decode(errors='replace'))
        old_negative = previous.previous.old.original_run(view / 'tests/p1227/c283/evidence_negative.py')
        require(old_negative.returncode == 0, 'unchanged v8 negative suite: ' + old_negative.stderr.decode(errors='replace'))
    current_old = previous.previous.old.original_run(root / 'tests/p1217/evidence_current_v8.py')
    require(current_old.returncode == 1 and b'CURRENT v8 registration independently derived' in current_old.stderr,
            'unchanged v8 must reject new registration')
    return dict(authentic_historical_head=FIXED, historical_files=len(payload),
                old_v8_checker=0, old_v8_negatives=0, old_v8_on_current=1,
                current=9, historical_semantic_pass_claim=False)


if __name__ == '__main__':
    try:
        print('C288 CURRENT v9 original-v8/history/current PASS', json.dumps(verify(ROOT), sort_keys=True))
    except (FreezeError, OSError, ValueError, KeyError, TypeError) as error:
        sys.exit('C288 CURRENT v9 REFUSE: ' + str(error))
