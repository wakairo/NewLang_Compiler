"""Forward C276 apparatus history; published v57 and its CI RED stay fixed."""
from pathlib import Path
import sys
import zipfile

V57 = 'tests/p990/current-freeze-v57.json'
SHA = 'a48a7cb7f0ea98c35c33bd8e59febeb87d01f5204c9f1cad041bcdcd9c777bcd'
OID = '12f7a75c553d92f0deffd05b5be07a6c03655e7b'
ARCHIVE_SHA = '126d201b6ab531aaf9c4eafd44ba05a39c8daf17a3d8e612f2be227e6d3aea5c'


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / 'tests/p1217'))
    from history_v57 import verify as previous
    from versioned_freeze import digest, git_oid, json_data, local, manifest_data, require
    count = previous(root, current)
    raw = local(root, V57).read_bytes()
    require(digest(raw) == SHA and git_oid('blob', raw) == OID,
            'C276 fixed published c992 v57 SHA/blob')
    old = json_data(raw)
    value, _ = manifest_data(root, current)
    require(old['generation'] == 57 and len(old['files']) == 4047,
            'C276 original v57 inventory')
    require(value['generation'] >= 58 and old['files'].keys() <= value['files'].keys(),
            'C276 inherited v57 coverage')
    for name, expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json', '.nl'))):
            require(digest(local(root, name).read_bytes()) == expected,
                    'C276 original source/oracle/FIRST: ' + name)
    data = local(root, 'tests/p1217/evidence/c276-history-predecessor-inputs-v1.zip').read_bytes()
    require(digest(data) == ARCHIVE_SHA, 'C276 original mutable predecessor sources')
    import io
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        require(set(z.namelist()) == {'CMakeLists.txt', 'tests/p1217/CMakeLists.txt',
                                     'tests/p982/freeze_test.py', 'tests/p1208/c268/inputs.py'},
                'C276 predecessor source archive coverage')
        for name in z.namelist():
            require(digest(z.read(name)) == old['files'][name],
                    'C276 published v57 source bytes: ' + name)
    return count
