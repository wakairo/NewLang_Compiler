"""C277 inherits immutable published v58 and genuine original mutable inputs."""
from pathlib import Path
import io
import sys
import zipfile

V58 = 'tests/p990/current-freeze-v58.json'
SHA = '0f5aff9140f559e2917197f5ba5da95cd2470bc19646f13b19e9c59189a91a9b'
OID = '521403e1e6964172d0b403c72fe30a451edcda8a'
ARCHIVE_SHA = 'f6de8910efd1e7de7deb026df4ac70d94c9c35f34c28fab4a9e935ad1239a48e'


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / 'tests/p1217'))
    from history_v58 import verify as previous
    from versioned_freeze import digest, git_oid, json_data, local, manifest_data, require
    count = previous(root, current)
    raw = local(root, V58).read_bytes()
    require(digest(raw) == SHA and git_oid('blob', raw) == OID,
            'C277 fixed158 v58 SHA/blob')
    old = json_data(raw)
    value, _ = manifest_data(root, current)
    require(old['generation'] == 58 and len(old['files']) == 4062,
            'C277 original v58 inventory')
    require(value['generation'] >= 59 and old['files'].keys() <= value['files'].keys(),
            'C277 inherited v58 coverage')
    for name, expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json', '.nl'))):
            require(digest(local(root, name).read_bytes()) == expected,
                    'C277 original source/oracle/FIRST: ' + name)
    raw = local(root, 'tests/p1217/evidence/c277-history-predecessors-v1.zip').read_bytes()
    require(digest(raw) == ARCHIVE_SHA, 'C277 original current-selector inputs')
    with zipfile.ZipFile(io.BytesIO(raw)) as z:
        require(set(z.namelist()) == {'tests/p1217/CMakeLists.txt',
                                     'tests/p982/freeze_test.py', 'tests/p1208/c268/inputs.py'},
                'C277 predecessor input coverage')
        for name in z.namelist():
            require(digest(z.read(name)) == old['files'][name],
                    'C277 original fixed158 input: ' + name)
    return count
