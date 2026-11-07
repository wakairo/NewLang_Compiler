"""C280 inherits immutable adopted v59 and genuine original mutable inputs."""
from pathlib import Path
import io
import sys
import zipfile

V59 = 'tests/p990/current-freeze-v59.json'
SHA = '228d16101791f78c6e7ad2011df7bdb2fe4db4611bb2bd9b570f6e9f32426f68'
OID = '78a04a10d509b137d8e906590e6cf4d72072ec4b'
ARCHIVE_SHA = 'a889ba84e0c8e3296790667fc4147d666916dec916b7bd2ef2ac6a90e6b09658'


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / 'tests/p1217'))
    from history_v59 import verify as previous
    from versioned_freeze import digest, git_oid, json_data, local, manifest_data, require
    count = previous(root, current)
    raw = local(root, V59).read_bytes()
    require(digest(raw) == SHA and git_oid('blob', raw) == OID,
            'C280 fixed-aa v59 SHA/blob')
    old = json_data(raw)
    value, _ = manifest_data(root, current)
    require(old['generation'] == 59 and len(old['files']) == 4072,
            'C280 original v59 inventory')
    require(value['generation'] >= 60 and old['files'].keys() <= value['files'].keys(),
            'C280 inherited v59 coverage')
    for name, expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json', '.nl'))):
            require(digest(local(root, name).read_bytes()) == expected,
                    'C280 original source/oracle/FIRST: ' + name)
    raw = local(root, 'tests/p1227/evidence/history-predecessors-v1.zip').read_bytes()
    require(digest(raw) == ARCHIVE_SHA, 'C280 original current-selector inputs')
    with zipfile.ZipFile(io.BytesIO(raw)) as z:
        require(set(z.namelist()) == {'tests/p1217/CMakeLists.txt',
                                     'tests/p982/freeze_test.py', 'tests/p1208/c268/inputs.py'},
                'C280 predecessor input coverage')
        for name in z.namelist():
            require(digest(z.read(name)) == old['files'][name],
                    'C280 original fixed-aa input: ' + name)
    return count
