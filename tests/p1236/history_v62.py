"""C288 inherits immutable adopted v61 and its original FIRSTs and genuine original mutable inputs."""
from pathlib import Path
import io
import sys
import zipfile

V61 = 'tests/p990/current-freeze-v61.json'
SHA = '0f8b4ee26b4d3f6dd9caac7daec7fec4bbe73e822cac3fae97d3f974df99013d'
OID = '0222e61cfc8bb4f3201f51522ae2553e6398bb06'
ARCHIVE_SHA = '486b143940867e99e039b2cfe43e8ec0edf56fe95c4c7ecd8bb92b4dd9addb2d'


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / 'tests/p1227/c283'))
    from history_v61 import verify as previous
    from versioned_freeze import digest, git_oid, json_data, local, manifest_data, require
    count = previous(root, current)
    raw = local(root, V61).read_bytes()
    require(digest(raw) == SHA and git_oid('blob', raw) == OID,
            'C288 fixed-main2931 v61 SHA/blob')
    old = json_data(raw)
    value, _ = manifest_data(root, current)
    require(old['generation'] == 61 and len(old['files']) == 4148,
            'C288 original v61 inventory')
    require(value['generation'] >= 62 and old['files'].keys() <= value['files'].keys(),
            'C288 inherited v61 coverage')
    for name, expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json', '.nl'))):
            require(digest(local(root, name).read_bytes()) == expected,
                    'C288 original source/oracle/FIRST: ' + name)
    raw = local(root, 'tests/p1236/evidence/history-predecessors-v3.zip').read_bytes()
    require(digest(raw) == ARCHIVE_SHA, 'C288 original current-selector inputs')
    with zipfile.ZipFile(io.BytesIO(raw)) as z:
        require(set(z.namelist()) == {'tests/p1227/c283/CMakeLists.txt', 'src/checked_c_current_emit_internal.h', 'tests/p1208/c266/p1203_evidence_current.py', 'tests/p982/freeze_test.py', 'src/linear_break_internal.h', 'tests/p1217/CMakeLists.txt', 'tests/p1208/c268/inputs.py', 'src/checked_c_current_internal.h', 'src/current_loop_check_internal.h'},
                'C288 predecessor input coverage')
        for name in z.namelist():
            require(digest(z.read(name)) == old['files'][name],
                    'C288 original fixed-main2931 input: ' + name)
    return count
