"""C268 safety successor; immutable v55 and all original evidence retained."""
from pathlib import Path
import importlib.util
import sys

V55 = 'tests/p990/current-freeze-v55.json'
SHA = '12c727fe18158acc289bc3a7c151e7c41eeef1edfab295b09be6a165135bef58'
OID = '31c48e71e72904e0ae5eef615453c28a35dc8e42'


def verify(root, current):
    root = Path(root)
    location = root / 'tests/p1208/c268/inputs.py'
    spec = importlib.util.spec_from_file_location('c268_current_inputs', location)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    sys.modules['inputs'] = module
    sys.path.insert(0, str(root / 'tests/p1208/c266'))
    sys.path.insert(0, str(root / 'tests/support'))
    from history_v55 import verify as previous
    from versioned_freeze import digest, git_oid, json_data, local, manifest_data, require
    count = previous(root, current)
    raw = local(root, V55).read_bytes()
    require(digest(raw) == SHA and git_oid('blob', raw) == OID, 'C268 fixed b5 v55 SHA/blob')
    old = json_data(raw)
    value, _ = manifest_data(root, current)
    require(old['generation'] == 55 and len(old['files']) == 3937, 'C268 v55 coverage')
    require(value['generation'] >= 56 and old['files'].keys() <= value['files'].keys(),
            'C268 inherited v55 coverage')
    for name, expected in old['files'].items():
        if name.startswith('tests/') and (
            '/evidence/' in name or '/lineage/' in name or name.endswith(('.json', '.nl'))
        ):
            require(digest(local(root, name).read_bytes()) == expected,
                    'C268 preserved b5 evidence: ' + name)
    return count
