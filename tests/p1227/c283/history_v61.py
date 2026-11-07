"""C283 inherits immutable published wrong-code v60 (history only) and genuine original mutable inputs."""
from pathlib import Path
import io
import sys
import zipfile

V60 = 'tests/p990/current-freeze-v60.json'
SHA = '6e254a1b74a004e1b6c010da0e990420595c86efa8b68a0479107805aea8039c'
OID = '49ad206ac8fc6c51ae6ef922d00baa019f3c5574'
ARCHIVE_SHA = '27eef1912eb8a900add35b6b162b8590f00f2ff4c34d7f7c41b7650a8cc97ef9'


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / 'tests/p1227'))
    from history_v60 import verify as previous
    from versioned_freeze import digest, git_oid, json_data, local, manifest_data, require
    count = previous(root, current)
    raw = local(root, V60).read_bytes()
    require(digest(raw) == SHA and git_oid('blob', raw) == OID,
            'C283 fixed-c2bbb v60 SHA/blob')
    old = json_data(raw)
    value, _ = manifest_data(root, current)
    require(old['generation'] == 60 and len(old['files']) == 4114,
            'C283 original v60 inventory')
    require(value['generation'] >= 61 and old['files'].keys() <= value['files'].keys(),
            'C283 inherited v60 coverage')
    for name, expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json', '.nl'))):
            require(digest(local(root, name).read_bytes()) == expected,
                    'C283 original source/oracle/FIRST: ' + name)
    raw = local(root, 'tests/p1227/c283/evidence/history-predecessors-v1.zip').read_bytes()
    require(digest(raw) == ARCHIVE_SHA, 'C283 original current-selector inputs')
    with zipfile.ZipFile(io.BytesIO(raw)) as z:
        require(set(z.namelist()) == {'tests/p1217/CMakeLists.txt',
                                     'tests/p982/freeze_test.py', 'tests/p1208/c268/inputs.py'},
                'C283 predecessor input coverage')
        for name in z.namelist():
            require(digest(z.read(name)) == old['files'][name],
                    'C283 original fixed-c2bbb input: ' + name)
    return count
