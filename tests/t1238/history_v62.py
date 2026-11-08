"""T1238/C288: original source/FIRST/history authenticated before current v62.

Compiler CI provenance only; no language, public issuer, or ABI change.
"""
from pathlib import Path
import sys

V61 = 'tests/p990/current-freeze-v61.json'
SHA61 = '0f8b4ee26b4d3f6dd9caac7daec7fec4bbe73e822cac3fae97d3f974df99013d'
OID61 = '0222e61cfc8bb4f3201f51522ae2553e6398bb06'
V13 = 'tests/p1208/c268/validation-freeze-v13.json'
SHA13 = 'd1350c881fdd36d5bb803a42a3cd7c9087dd5fd28ebe09cff17438eced9b3e2a'
OID13 = 'e5251608b06043ac5f97c504e40b8957cb327840'
V14 = 'tests/p1208/c268/validation-freeze-v14.json'
ROOT_SHA = '04b2628ef1f636e0c28f535c8496ec499b5491c697f8a244ae82ca37113e5429'
ROOT_OID = 'ffdc9cb6dfcc9dabe5dd8fabddde15edb41bbcbf'
CHANGING = {'tests/p1208/CMakeLists.txt',
            'tests/p1208/c268/inputs.py',
            'tests/p982/freeze_test.py'}
NEW = {V61, V14, 'tests/t1238/history_v62.py'}


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / 'tests/p1227/c283'))
    from history_v61 import verify as prior
    from versioned_freeze import digest, git_oid, json_data, local, manifest_data, require
    count = prior(root, current)

    def anchored(name, sha, oid):
        raw = local(root, name).read_bytes()
        require(digest(raw) == sha and git_oid('blob', raw) == oid,
                'T1238 immutable external SHA/Git anchor: ' + name)
        return json_data(raw)

    parent = anchored(V61, SHA61, OID61)
    old = anchored(V13, SHA13, OID13)
    root_bytes = local(root, 'CMakeLists.txt').read_bytes()
    require(digest(root_bytes) == ROOT_SHA and git_oid('blob', root_bytes) == ROOT_OID,
            'T1238 original root CMake SHA/Git')
    value, _ = manifest_data(root, current)
    require(value['generation'] >= 62 and
            value['previous'] == {'path': V61, 'sha256': SHA61},
            'T1238 current must inherit actual P v61')
    require(parent['generation'] == 61 and len(parent['files']) == 4148,
            'T1238 adopted P v61 inventory')
    require(set(value['files']) - set(parent['files']) == NEW,
            'T1238 three precise new current inputs')
    require({name for name in parent['files']
             if parent['files'][name] != value['files'].get(name)} == CHANGING,
            'T1238 unrelated previously current digest changed')
    require(value['files']['CMakeLists.txt'] == ROOT_SHA and
            value['files'][V61] == SHA61 and value['files'][V13] == SHA13,
            'T1238 retained original root and predecessors')
    new = json_data(local(root, V14).read_bytes())
    require(old['version'] == 13 and new['version'] == 14 and
            new['previous'] == {'path': V13, 'sha256': SHA13, 'git_blob': OID13},
            'T1238 authentic C268 v13-to-v14 successor')
    require(set(new['files']) - set(old['files']) == {V13} and
            new['files'][V13] == SHA13,
            'T1238 retain old C268 v13 digest')
    require({name for name in old['files']
             if old['files'][name] != new['files'].get(name)} == CHANGING,
            'T1238 C268 unrelated earlier hash changed')
    require(old['files']['CMakeLists.txt'] == new['files']['CMakeLists.txt'] == ROOT_SHA,
            'T1238 unchanged root in C268 revisions')
    for name in CHANGING:
        require(new['files'][name] == value['files'][name] ==
                digest(local(root, name).read_bytes()),
                'T1238 current/source/v14 identity: ' + name)
    require(value['files'][V14] == digest(local(root, V14).read_bytes()),
            'T1238 C268 v14 in global v62')
    return count
