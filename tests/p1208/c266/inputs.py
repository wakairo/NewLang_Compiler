"""Current C266 equipment and immutable C263 custody, without Git/network."""
from pathlib import Path
import hashlib
import json
import zipfile

ROOT = Path(__file__).resolve().parents[3]
HERE = ROOT / 'tests/p1208'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def verify():
    receipt = json.loads((HERE / 'c266/old-local-inputs-v1.json').read_bytes())
    archive = ROOT / receipt['archive']
    assert sha(archive.read_bytes()) == receipt['sha256']
    with zipfile.ZipFile(archive) as old:
        assert set(old.namelist()) == set(receipt['files'])
        for name, expected in receipt['files'].items():
            data = old.read(name)
            assert sha(data) == expected, name
            oid = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
            assert oid == receipt['git_blob_oids'][name], name
        for freeze_name in ('probe-freeze-v1.json', 'validation-freeze-v1.json'):
            frozen = json.loads(old.read('tests/p1208/' + freeze_name))
            prefix = 'tests/p1208/' if freeze_name.startswith('probe') else ''
            for name, expected in frozen['files'].items():
                assert sha(old.read(prefix + name)) == expected, (freeze_name, name)
    for version in (1, 2):
        frozen = json.loads((HERE / f'source-freeze-v{version}.json').read_bytes())
        for name, expected in frozen['files'].items():
            assert sha((HERE / name).read_bytes()) == expected, name
    first_equipment = json.loads((HERE / 'c266/equipment-v2-FIRST.json').read_bytes())
    assert sha((ROOT / first_equipment['archive']).read_bytes()) == first_equipment['sha256']
    with zipfile.ZipFile(ROOT / first_equipment['archive']) as z:
        assert set(z.namelist()) == set(first_equipment['files'])
        for name, expected in first_equipment['files'].items():
            assert sha(z.read(name)) == expected, name
        v2 = json.loads(z.read('tests/p1208/c266/validation-freeze-v2.json'))
        for name, expected in v2['files'].items():
            assert sha(z.read(name)) == expected, name
    first_gcc = json.loads((HERE / 'c266/GCC-first-RED-v1.json').read_bytes())
    assert sha((ROOT / first_gcc['archive']).read_bytes()) == first_gcc['sha256']
    with zipfile.ZipFile(ROOT / first_gcc['archive']) as z:
        assert set(z.namelist()) == set(first_gcc['files'])
        for name, expected in first_gcc['files'].items():
            assert sha(z.read(name)) == expected, name
        v3 = json.loads(z.read('tests/p1208/c266/validation-freeze-v3.json'))
        for name, expected in v3['files'].items():
            assert sha(z.read(name)) == expected, name
    current = json.loads((HERE / 'c266/validation-freeze-v4.json').read_bytes())
    for name, expected in current['files'].items():
        assert sha((ROOT / name).read_bytes()) == expected, name
    first = json.loads((HERE / 'evidence-receipt-v1.json').read_bytes())
    assert sha((ROOT / first['archive']).read_bytes()) == first['sha256']
    with zipfile.ZipFile(ROOT / first['archive']) as z:
        assert set(z.namelist()) == set(first['files'])
        for name, expected in first['files'].items():
            assert sha(z.read(name)) == expected, name
        assert b'P14-LOOP-PRECISION' in z.read('gate0/01-left-normal-check.err')
        assert b'P14-LOOP-PRECISION' in z.read('prior/preserved-95-P1193-prior-LastTest-section.log')


if __name__ == '__main__':
    verify()
    print('C266 original C263 input/source/oracle/FIRST and current v4 equipment PASS')
