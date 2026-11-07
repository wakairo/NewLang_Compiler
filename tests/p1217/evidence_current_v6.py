"""Authenticate the genuine v5 tree and independently admit CURRENT registration."""
from pathlib import Path
import base64
import json
import os
import subprocess
import sys
import tempfile
import zipfile

CODE_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(CODE_ROOT / 'tests/support'))
from versioned_freeze import FreezeError, digest, git_oid, json_data, local, require

PREFIX = 'tests/p1217/'
CMAKE = PREFIX + 'CMakeLists.txt'
CAPSULE = PREFIX + 'c277/git-snapshots-v1.json'
CAPSULE_SHA = 'fc57cbdb1f99228fd35550236d8fb1ef9089415769ba7e4f1147fd43644f1ee1'
OLD_ARCHIVE = PREFIX + 'evidence/c276-history-predecessor-inputs-v1.zip'
OLD_ARCHIVE_SHA = '126d201b6ab531aaf9c4eafd44ba05a39c8daf17a3d8e612f2be227e6d3aea5c'
OLD_CMAKE_SHA = '521b51f6ddd5340811fa34091ae210babc0bfe7455aa81ee185a58a6caf705c5'
FIXED = {
    'v5': ('c99264155b2a0546e578dc6c743457599d05869b',
           '2201e0d82a30a23608be8bb33fb6a1e2931c6b41'),
    'baseline': ('1588176f8378c2377651b344f7c3f0075ee005db',
                 'f9e8ad259330a6ad014cd47b58fd6018a4a521aa'),
}
REGISTRATION = '''
add_test(NAME p1217_evidence_tamper.unit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/p1217/c277/negative_test.py")
'''
CURRENT_FILES = {
    CMAKE, PREFIX + 'evidence_current_v6.py', CAPSULE,
    PREFIX + 'c277/negative_test.py', PREFIX + 'c277/oracle-v1.json',
    PREFIX + 'c277/preflight-inventory-v1.json',
}


def entries(capsule, oid):
    raw = base64.b64decode(capsule['trees_base64'][oid], validate=True)
    require(git_oid('tree', raw) == oid, 'Git tree identity')
    result = {}
    offset = 0
    while offset < len(raw):
        space = raw.index(b' ', offset)
        zero = raw.index(b'\0', space)
        require(zero + 21 <= len(raw), 'truncated Git tree')
        mode, name = raw[offset:space].decode(), raw[space + 1:zero].decode()
        require(name and '/' not in name and name not in result, 'Git tree entry')
        result[name] = (mode, raw[zero + 1:zero + 21].hex())
        offset = zero + 21
    return result


def lookup(capsule, tree, parts):
    for i, part in enumerate(parts):
        mode, tree = entries(capsule, tree)[part]
        require(mode == ('100644' if i == len(parts) - 1 else '40000'),
                'Git membership mode')
    return tree


def subtree(capsule, tree, prefix):
    result = {}
    for name, (mode, oid) in entries(capsule, tree).items():
        path = prefix + name
        if mode == '40000':
            result.update(subtree(capsule, oid, path + '/'))
        else:
            require(mode == '100644', 'nonregular original Git input')
            result[path] = oid
    return result


def snapshot(root, capsule, role):
    record = capsule['snapshots'][role]
    commit, tree = FIXED[role]
    raw = base64.b64decode(record['commit_base64'], validate=True)
    require(record['commit'] == commit and record['tree'] == tree and
            git_oid('commit', raw) == commit and
            raw.splitlines()[0] == ('tree ' + tree).encode(),
            role + ' fixed Git commit/tree')
    if role == 'baseline':
        require(('parent ' + FIXED['v5'][0]).encode() in raw.splitlines(),
                'fixed baseline parent')
    tests_mode, tests = entries(capsule, tree)['tests']
    own_mode, own = entries(capsule, tests)['p1217']
    require(tests_mode == own_mode == '40000', 'original P subtree membership')
    expected = subtree(capsule, own, PREFIX)
    if role == 'baseline':
        expected['CMakeLists.txt'] = lookup(capsule, tree, ['CMakeLists.txt'])
    require(record['files'].keys() == expected.keys(), role + ' complete Git inventory')
    payloads = {}
    for name, oid in expected.items():
        pin = record['files'][name]
        require(pin['oid'] == oid, role + ' recorded blob membership: ' + name)
        if oid in capsule['retained_blobs_base64']:
            data = base64.b64decode(capsule['retained_blobs_base64'][oid], validate=True)
        else:
            data = local(root, name).read_bytes()
        require(git_oid('blob', data) == oid and digest(data) == pin['sha256'],
                role + ' blob: ' + name)
        payloads[name] = data
    return payloads


def original_run(path):
    env = os.environ.copy()
    env.update(PYTHONDONTWRITEBYTECODE='1', PYTHONOPTIMIZE='0',
               PYTHONHASHSEED='0', LC_ALL='C')
    return subprocess.run([sys.executable, str(path)], stdin=subprocess.DEVNULL,
                          capture_output=True, env=env)


def verify(root):
    root = Path(root).resolve()
    require(sys.flags.optimize == 0, 'original assertions must be enabled')
    data = local(root, CAPSULE).read_bytes()
    require(digest(data) == CAPSULE_SHA, 'snapshot capsule digest')
    capsule = json_data(data)
    require(type(capsule['version']) is int and capsule['version'] == 1,
            'snapshot capsule version')
    old = snapshot(root, capsule, 'v5')
    baseline = snapshot(root, capsule, 'baseline')
    archive = local(root, OLD_ARCHIVE).read_bytes()
    require(digest(archive) == OLD_ARCHIVE_SHA, 'original source archive')
    import io
    with zipfile.ZipFile(io.BytesIO(archive)) as z:
        require(z.read(CMAKE) == old[CMAKE] and digest(old[CMAKE]) == OLD_CMAKE_SHA,
                'independent original v5 CMake bytes')
    v5 = json_data(old[PREFIX + 'execution-freeze-v5.json'])
    require(v5['version'] == 5 and v5['files']['CMakeLists.txt'] == OLD_CMAKE_SHA,
            'original v5 contract')
    for name, expected in v5['files'].items():
        require(digest(old[PREFIX + name]) == expected, 'original v5 input: ' + name)
    # Authenticate the two distinct registrations; no old input is rewritten.
    before = b'tests/p1217/oom_v2.py'
    require(old[CMAKE].count(before) == 1 and baseline[CMAKE] == old[CMAKE].replace(
        before, b'tests/p1217/oom_v3.py', 1), 'fixed158 OOM registration')
    before = b'tests/p1217/evidence_test.py'
    require(baseline[CMAKE].count(before) == 1, 'original evidence registration')
    expected = baseline[CMAKE].replace(before, b'tests/p1217/evidence_current_v6.py', 1)
    expected += REGISTRATION.encode()
    current = local(root, CMAKE).read_bytes()
    require(current == expected and digest(current) != OLD_CMAKE_SHA,
            'CURRENT registration independently derived from fixed originals')
    current_contract = json_data(local(root, PREFIX + 'execution-freeze-v6.json').read_bytes())
    require(type(current_contract['version']) is int and current_contract['version'] == 6 and
            current_contract['authority'] == '491f54adfa64ae7308344923ed14a9fcad08a8e9' and
            current_contract['fixed_head'] == FIXED['baseline'][0], 'CURRENT v6 authority')
    require(current_contract['files'].keys() == CURRENT_FILES, 'CURRENT exact dependency keys')
    for name, pin in current_contract['files'].items():
        require(digest(local(root, name).read_bytes()) == pin, 'CURRENT source digest: ' + name)
    # Materialize the complete genuine old subtree, not a patched current tree.
    with tempfile.TemporaryDirectory(prefix='p1217-genuine-v5-') as directory:
        view = Path(directory)
        for name, contents in old.items():
            target = view / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(contents)
        historical = original_run(view / PREFIX / 'evidence_test.py')
        require(historical.returncode == 0, 'original checker/historical tree: ' +
                historical.stderr.decode(errors='replace'))
    rejected_current = original_run(local(root, PREFIX + 'evidence_test.py'))
    require(rejected_current.returncode == 1 and rejected_current.stderr.rstrip().endswith(
        b'AssertionError: CMakeLists.txt'), 'old v5 still rejects current tree')
    return dict(historical_commit=FIXED['v5'][0], historical_files=len(old),
                historical_original_checker_status=historical.returncode,
                historical_stdout_sha256=digest(historical.stdout),
                unchanged_old_checker_current_status=rejected_current.returncode,
                unchanged_old_checker_current_failure='AssertionError: CMakeLists.txt',
                current_registration_sha256=digest(current),
                current_fixed_components=len(baseline), current_dependency_keys=len(CURRENT_FILES))


if __name__ == '__main__':
    try:
        result = verify(CODE_ROOT)
    except (FreezeError, OSError, ValueError, KeyError, TypeError) as error:
        sys.exit('P1217 CURRENT v6 REFUSE: ' + str(error))
    print('P1217 CURRENT v6 original-history / independent-current PASS ' + json.dumps(result, sort_keys=True))
