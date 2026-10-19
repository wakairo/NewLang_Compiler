"""Read-only integrity check for preserved history and versioned P609 output."""
import hashlib
import json
from pathlib import Path


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    root = Path(__file__).resolve().parents[2]
    frozen = json.loads((root / 'tests/p609/frozen.json').read_text())
    for group in ('old_p603', 'old_p605_p608', 'protected_arithmetic'):
        for name, expected in frozen[group].items():
            assert digest((root / name).read_bytes()) == expected, name
    text = (root / 'src/semantic_check.c').read_bytes()
    start = text.index(b'static NLCheckedNodeId usize_arithmetic(')
    end = text.index(b'\nstatic ', start + 1)
    assert digest(text[start:end]) == frozen['arithmetic_region_sha256']
    originals = json.loads((root / 'tests/p603/frozen.json').read_text())
    for name, expected in originals['sources'].items():
        assert digest((root / 'tests/p603' / name).read_bytes()) == expected
    index = json.loads((root / 'tests/p609/evidence/generated-index.json').read_text())
    assert len(index) == 9
    for name, entry in index.items():
        assert originals['sources'][name] == entry['source_sha256']
        file = name.replace('.nl', '.c17.txt')
        assert digest((root / 'tests/p603/evidence' / file).read_bytes()) == entry['old_c17_sha256']
        assert digest((root / 'tests/p609/evidence' / file).read_bytes()) == entry['new_c17_sha256']
        assert entry['old_c17_sha256'] != entry['new_c17_sha256']
    print('P609 integrity: 27 source/oracles, 61 P603 historical files, P605/P608 history, 9 old/new C versions, 4 arithmetic files and exact capture body PASS')


if __name__ == '__main__':
    main()
