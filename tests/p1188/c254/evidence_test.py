"""Retain original failed predictions/equipment, not just the final passing run."""
from pathlib import Path
import hashlib
import json
import zipfile

here = Path(__file__).resolve().parent
root = here.parents[2]
sha = lambda b: hashlib.sha256(b).hexdigest()
assert sha((here / 'source-freeze-v1.json').read_bytes()) == (
    'ce8a3bf39d11e53f8af3ea98ac277574adb3cb8526bec6e8f8d5d55ff0be656f')
assert sha((here / 'oracle-v1.json').read_bytes()) == (
    '1183b2b07d2f0f50fb77fcb016cc1c08a808cfe895075df637728b08d98c32da')
originals = here / 'evidence/capture-equipment-originals.zip'
# Original .c harness attempts keep their exact bytes here and in forward Git
# history. They are not active translation units (one intentionally did not
# compile). The frozen paths and every rejection assertion remain verifiable.
with zipfile.ZipFile(originals) as archive:
    for pattern in ['source-freeze-*.json', 'equipment-freeze-*.json',
                    'safety-freeze-*.json']:
        for freeze in sorted(here.glob(pattern)):
            for name, expected in json.loads(freeze.read_bytes())['files'].items():
                path = root / name
                data = path.read_bytes() if path.is_file() else archive.read(name)
                assert sha(data) == expected, (freeze.name, name)
for packet in (here / 'evidence').glob('*.zip'):
    with zipfile.ZipFile(packet) as archive:
        assert archive.testzip() is None, packet
receipt = json.loads((here / 'local-receipt-v1.json').read_bytes())
assert sha((root / receipt['archive']).read_bytes()) == receipt['archive_sha256']
print('C254 independent sources/oracles/FIRST/equipment and finite receipts PASS')
