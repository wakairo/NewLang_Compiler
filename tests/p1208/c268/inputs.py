"""Prospective current selector; original v7 source/algorithm stays exact."""
from pathlib import Path
import hashlib
import json
import zipfile

ROOT = Path(__file__).resolve().parents[3]
HERE = ROOT / 'tests/p1208'
sha = lambda data: hashlib.sha256(data).hexdigest()


def verify():
    receipt = json.loads((HERE / 'c268/legacy-source-v7.json').read_bytes())
    assert sha((ROOT / receipt['archive']).read_bytes()) == receipt['sha256']
    with zipfile.ZipFile(ROOT / receipt['archive']) as z:
        assert set(z.namelist()) == set(receipt['files'])
        for name, expected in receipt['files'].items():
            assert sha(z.read(name)) == expected, name
        frozen = json.loads(z.read('tests/p1208/c266/validation-freeze-v7.json'))
        for name, expected in frozen['files'].items():
            if name in receipt['files']:
                data = z.read(name)
            else:
                assert name.endswith(tuple(receipt['immutable_live_suffixes']))
                data = (ROOT / name).read_bytes()
            assert sha(data) == expected, name
    original = HERE / 'c266/inputs.py'
    raw = original.read_bytes()
    assert sha(raw) == '785090d51ebe0cabce0a1735a203ffe8114702f66b88200ac0d6b5a4b2d7bd58'
    source = raw.decode()
    before = 'c266/validation-freeze-v7.json'
    assert source.count(before) == 1
    source = source.replace(before, 'c268/validation-freeze-v14.json', 1)
    scope = {'__file__': str(original), '__name__': 'c268_authenticated_v7_current'}
    exec(compile(source, str(original) + ':C268-current', 'exec'), scope)
    scope['verify']()
    for name, expected in json.loads((HERE / 'c268/source-freeze-v1.json').read_bytes())['files'].items():
        assert sha((HERE / 'c268' / name).read_bytes()) == expected, name


if __name__ == '__main__':
    verify()
    print('C268 original v7 source/algorithm + prospective current v14 PASS')
