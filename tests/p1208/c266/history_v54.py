"""C266 successor of adopted v53; preserve sibling old C263 evidence too."""
from pathlib import Path
import sys

V53 = 'tests/p990/current-freeze-v53.json'
SHA256 = '6ba85c17e6b003938ce24217c4d251fa1926a73263fab8607efca8122cf23bdf'
OID = '8383c7475007d160c5cb420c5ddb5ecaa4e4a86a'
OLD_LOCAL = 'tests/p1208/current-freeze-v53.json'
OLD_SHA = '747ba3dc7f991fab662aeb7bf44b96a87ab7af80e318e263008b29585a415cdb'
V36 = {
    'tests/p1107/freeze-v36.json': ('4383d754e615b2a689fe0eaa716108bf38e14ab7b2f42404596e67d9ea4853f1', '3296d64b84c05a72cc34f8a4d04b2a8c26332e4d'),
    'tests/p1107/evidence/harness-inputs-v36.zip': ('8c5099a1da03cea6c2afda05e7cbded2d6be6fdc2c059077329af581f8c53898', '43cd781b55c0afcd95e5d4fcdd778c7fe4472bf9'),
}


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / 'tests/p1203'))
    sys.path.insert(0, str(root / 'tests/support'))
    from history_v53 import verify as previous_verify
    from versioned_freeze import digest, git_oid, json_data, local, manifest_data, require
    count = previous_verify(root, current)
    raw = local(root, V53).read_bytes()
    require(digest(raw) == SHA256 and git_oid('blob', raw) == OID,
            'C266 externally fixed adopted v53 bytes/blob')
    old = json_data(raw)
    require(old['generation'] == 53 and len(old['files']) == 3855, 'C266 adopted v53 coverage')
    value, _ = manifest_data(root, current)
    require(value['generation'] >= 54 and old['files'].keys() <= value['files'].keys(),
            'C266 inherited adopted v53 coverage')
    for name, expected in old['files'].items():
        if name.startswith('tests/') and (
            '/evidence/' in name or '/lineage/' in name or name.endswith(('.json', '.nl'))
        ):
            require(digest(local(root, name).read_bytes()) == expected,
                    'C266 preserved adopted predecessor evidence: ' + name)
    require(digest(local(root, OLD_LOCAL).read_bytes()) == OLD_SHA, 'C266 old C263 sibling v53')
    for name, (sha, oid) in V36.items():
        data = local(root, name).read_bytes()
        require(digest(data) == sha and git_oid('blob', data) == oid,
                'C266 preserved adopted v36 bytes/blob: ' + name)
    sys.path.insert(0, str(root / 'tests/p1208/c266'))
    from inputs import verify as verify_inputs
    verify_inputs()
    return count
