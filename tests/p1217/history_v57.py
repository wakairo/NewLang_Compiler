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
V8_ARCHIVE_OID = 'fb85a95a717d4c2d03f03833a84dac509382b865'
V8_RECEIPT_SHA = 'ffac896093da6647b67ef5007b79315044d3206f5265158aa99f0ae2fa900d8f'
V8_RECEIPT_OID = '45bb18d9687541aef5f0adc72ba9b86ab29174a7'
V8_MANIFEST_SHA = '5342b077c32f88add9b5f3a0e80745053375fbcf92f9daa624e9dd3486c41c95'
V8_MANIFEST_OID = 'fdd2859dcbbcdf82f40b60001b52daecfb68b3ad'
COMPACT_SHA = 'fd227e1a509b349e4ae8cb01522a9d4e7f015a38eec9dc4665a35760c3c2adf4'
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
    raw = local(root, 'tests/p1217/c268-v8-history.json').read_bytes()
    require(digest(raw) == V8_RECEIPT_SHA and git_oid('blob', raw) == V8_RECEIPT_OID,
            'P1217 original v8 archive catalogue SHA/blob')
    original = json.loads(raw)
    raw = local(root, original['manifest']).read_bytes()
    require(digest(raw) == V8_MANIFEST_SHA and git_oid('blob', raw) == V8_MANIFEST_OID,
            'P1217 adopted v8 manifest SHA/blob')
    frozen = json.loads(raw)
    require(original['files'] == dict(frozen['files'], **{original['manifest']: V8_MANIFEST_SHA}),
            'P1217 full original v8 member coverage')
    receipt = json.loads(local(root, 'tests/p1217/c274-v8-inheritance.json').read_bytes())
    require(receipt['fixed_main'] == original['fixed_main'] ==
            '491f54adfa64ae7308344923ed14a9fcad08a8e9' and
            receipt['old_exact_zip']['commit'] == '0ebe4bc0d25aa89dacfe8463b60207377a041fd6' and
            receipt['old_exact_zip']['path'] == original['archive'] and
            receipt['old_exact_zip']['sha256'] == original['sha256'] == V8_ARCHIVE_SHA and
            receipt['old_exact_zip']['git_blob'] == V8_ARCHIVE_OID,
            'P1217 exact original v8 ZIP/Git ancestor anchor')
    mutable = receipt['mutable_files']
    live = receipt['immutable_live_files']
    require(not mutable.keys() & live.keys() and dict(mutable, **live) == original['files'],
            'P1217 equivalent complete v8 archive/live partition')
    require(all(name.endswith(('.json', '.nl', '.zip')) for name in live),
            'P1217 only immutable data may use live original bytes')
    for name, expected in original['files'].items():
        if name in old['files']:
            require(expected == old['files'][name], 'P1217 v8 external inherited anchor: ' + name)
    for name, expected in live.items():
        require(digest(local(root, name).read_bytes()) == expected,
                'P1217 immutable original v8 live bytes: ' + name)
    archive = local(root, receipt['archive'])
    require(digest(archive.read_bytes()) == COMPACT_SHA == receipt['sha256'],
            'P1217 compact original v8 mutable bytes')
    with zipfile.ZipFile(archive) as z:
        require(len(z.namelist()) == len(set(z.namelist())) and set(z.namelist()) == set(mutable),
                'P1217 compact v8 inventory without duplicates')
        for name, expected in mutable.items():
            require(digest(z.read(name)) == expected, 'P1217 original v8 mutable bytes: ' + name)
    require(digest(local(root, 'tests/p1217/evidence/P1214-Gate0-source-oracle-FIRST-raw-v1.zip').read_bytes()) == P1214_SHA,
            'P1217 original P1214 FIRST packet')
    return count
