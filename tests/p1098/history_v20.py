"""Prospective v20 inherits the externally fixed, adopted main v19 inventory."""
from pathlib import Path
import sys

V19 = 'tests/p990/current-freeze-v19.json'
SHA256 = '4a87323836c1c285ceffe8e1c1677f788592dc12598a8a010d8f886efc6c324b'
OID = 'deccb5584dcb3050163109a3f8e2da161f79ad64'

def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / 'tests/p1062'))
    sys.path.insert(0, str(root / 'tests/support'))
    from history_v12 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V19).read_bytes()
    require(digest(raw) == SHA256, 'P1098 fixed adopted-v19 SHA256')
    require(git_oid('blob', raw) == OID, 'P1098 fixed adopted-v19 Git blob')
    old = json_data(raw)
    require(old['generation'] == 19 and len(old['files']) == 2952, 'P1098 adopted-v19 coverage')
    value, _ = manifest_data(root, current)
    require(value['generation'] >= 20 and old['files'].keys() <= value['files'].keys(), 'P1098 inherited-v19 coverage')
    for name, expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json', '.nl'))):
            require(digest(local(root, name).read_bytes()) == expected, 'P1098 preserved adopted-v19 data: ' + name)
    return count
