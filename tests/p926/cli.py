"""New P raw-source/oracle custody and unchanged ordinary CLI boundaries."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile

compiler, root = sys.argv[1], Path(sys.argv[2])
sources = [(root / 'sources' / name, digest) for name, digest in json.loads((root / 'freeze.json').read_text())['source_sha256'].items()]
sources += [(root / 'negatives' / name, info['sha256']) for name, info in json.loads((root / 'negative-freeze.json').read_text())['negative_sources'].items()]
for source, digest in sources:
    assert hashlib.sha256(source.read_bytes()).hexdigest() == digest
    for args in [['--check', str(source)], [str(source)]]:
        r = subprocess.run([compiler, *args], capture_output=True)
        assert r.returncode == 3 and not r.stdout and b'P6-JOIN-PRECISION' in r.stderr
    with tempfile.TemporaryDirectory() as temp:
        out = Path(temp) / 'existing.c'
        out.write_bytes(b'existing correct output\n')
        r = subprocess.run([compiler, '--emit-c17-to', str(out), str(source)], capture_output=True)
        assert r.returncode == 3 and not r.stdout
        assert out.read_bytes() == b'existing correct output\n'
        assert list(Path(temp).iterdir()) == [out]
    print(source.name, 'ordinary CHECK3/P6, no C17, existing output preserved')
