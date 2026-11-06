"""One mechanical CI successor; published v54 remains byte/blob-exact."""
from pathlib import Path
import sys

V54 = 'tests/p990/current-freeze-v54.json'
SHA256 = '6bc642eeb7fe72550caf3056aec903939c196f112b6e446ec8a77ed7f168d093'
OID = '379b08acc39e7e02e4c37e4f9b92e3e8c22aae6b'


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / 'tests/p1208/c266'))
    sys.path.insert(0, str(root / 'tests/support'))
    from history_v54 import verify as previous_verify
    from versioned_freeze import digest, git_oid, json_data, local, manifest_data, require
    count = previous_verify(root, current)
    raw = local(root, V54).read_bytes()
    require(digest(raw) == SHA256 and git_oid('blob', raw) == OID,
            'C266 externally fixed initial published v54 bytes/blob')
    old = json_data(raw)
    require(old['generation'] == 54 and len(old['files']) == 3921,
            'C266 initial published v54 coverage')
    value, _ = manifest_data(root, current)
    require(value['generation'] >= 55 and old['files'].keys() <= value['files'].keys(),
            'C266 inherited published v54 coverage')
    for name, expected in old['files'].items():
        if name.startswith('tests/') and (
            '/evidence/' in name or '/lineage/' in name or name.endswith(('.json', '.nl'))
        ):
            require(digest(local(root, name).read_bytes()) == expected,
                    'C266 preserved initial published evidence: ' + name)
    return count
