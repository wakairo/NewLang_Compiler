"""C272 successor: external adopted v56 and original source/FIRST remain exact."""
from pathlib import Path
import hashlib
import json
import sys
import zipfile

V56 = 'tests/p990/current-freeze-v56.json'
SHA = 'e2b88c8c0194f53b2d9d81b685ebea0a3c28e57806ea417a16e3d897cd348419'
OID = 'a0f34677a1713f79b89ad38d2e574ad20e3464ac'
V8_ARCHIVE_SHA = '6e56660f648ac634c1ff9fcbb5191f32c58892abad38b42131a1d8e071c53500'
P1214_SHA = '4db34028375317e70ab49bce76339ca30f656bb49c5a49b7ef30ab5e9aad3eb5'


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / 'tests/p1208/c268'))
    from history_v56 import verify as previous
    from versioned_freeze import digest, git_oid, json_data, local, manifest_data, require
    count = previous(root, current)
    raw = local(root, V56).read_bytes()
    require(digest(raw) == SHA and git_oid('blob', raw) == OID,
            'P1217 externally fixed adopted-v56 SHA/blob')
    old = json_data(raw)
    value, _ = manifest_data(root, current)
    require(old['generation'] == 56 and len(old['files']) == 3963,
            'P1217 adopted-v56 inventory')
    require(value['generation'] >= 57 and old['files'].keys() <= value['files'].keys(),
            'P1217 inherited adopted-v56 coverage')
    for name, expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json', '.nl'))):
            require(digest(local(root, name).read_bytes()) == expected,
                    'P1217 immutable adopted-v56 source/oracle/FIRST: ' + name)
    receipt = json.loads(local(root, 'tests/p1217/c268-v8-history.json').read_bytes())
    archive = local(root, receipt['archive'])
    require(digest(archive.read_bytes()) == V8_ARCHIVE_SHA == receipt['sha256'],
            'P1217 adopted C268 v8 history archive')
    with zipfile.ZipFile(archive) as z:
        require(set(z.namelist()) == set(receipt['files']), 'P1217 v8 archive inventory')
        for name, expected in receipt['files'].items():
            data = z.read(name)
            require(digest(data) == expected, 'P1217 original v8 bytes: ' + name)
            if name in old['files']:
                require(expected == old['files'][name], 'P1217 v8 external inherited anchor: ' + name)
        require(z.read(receipt['manifest']) == local(root, receipt['manifest']).read_bytes(),
                'P1217 original v8 manifest unchanged')
    require(digest(local(root, 'tests/p1217/evidence/P1214-Gate0-source-oracle-FIRST-raw-v1.zip').read_bytes()) == P1214_SHA,
            'P1217 original P1214 FIRST packet')
    return count
