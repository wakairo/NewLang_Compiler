"""Original inputs and all observed failures remain fixed across successors."""
from pathlib import Path
import hashlib
import json
import zipfile

root = Path(__file__).resolve().parents[2]
here = Path(__file__).resolve().parent
sha = lambda data: hashlib.sha256(data).hexdigest()
for version in range(1, 5):
    freeze = json.loads((here / f'source-freeze-v{version}.json').read_bytes())
    for path, expected in freeze['files'].items():
        assert sha((root / path).read_bytes()) == expected, path
    if 'predecessor' in freeze:
        previous = freeze['predecessor']
        assert sha((root / previous['path']).read_bytes()) == previous['sha256']
latest = json.loads((here / 'equipment-freeze-v3.json').read_bytes())
archive = here / 'evidence/equipment-v2.zip'
assert sha(archive.read_bytes()) == latest['archive_sha256']
with zipfile.ZipFile(archive) as packet:
    for version in [1, 2]:
        freeze = json.loads((here / f'equipment-freeze-v{version}.json').read_bytes())
        for path, expected in freeze['files'].items():
            assert sha(packet.read(path)) == expected, (version, path)
for path, expected in latest['files'].items():
    assert sha((root / path).read_bytes()) == expected, path
failures = {
    'gate0-v1-FIRST.zip': '10368a1df2c1164c2daebe14e31dce42b24841d14334874478ddbca0b8ee610d',
    'candidate-v1-RED.zip': '379eb44027a83e52d04377e8d48505b9e676928fc318344f76d66b6e8e92222c',
    'equipment-v1-build-FIRST.zip': '9056fb2b5fc6b32edacd2feb318a14230e9761cbd0a401eb686dab1e757ccb88',
}
for path, expected in failures.items():
    assert sha((here / 'evidence' / path).read_bytes()) == expected, path
print('C247 original source/oracle/FIRST/RED and versioned equipment PASS')

forward = json.loads((here / 'equipment-freeze-v4.json').read_bytes())
for path, expected in forward['files'].items():
    assert sha((root / path).read_bytes()) == expected, path
print('C247 unchanged C246a oracle/fixed-header archive and forward adapter PASS')
