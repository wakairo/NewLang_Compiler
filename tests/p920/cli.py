"""New P-owned fixed-source CLI custody. No R source or oracle."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile

compiler, root = sys.argv[1], Path(sys.argv[2])
for name, digest in json.loads((root / 'freeze.json').read_text())['source_sha256'].items():
    source = root / 'sources' / name
    assert hashlib.sha256(source.read_bytes()).hexdigest() == digest
    for args in [['--check', str(source)], [str(source)]]:
        result = subprocess.run([compiler, *args], capture_output=True)
        assert result.returncode == 3 and not result.stdout
        assert b'P6-JOIN-PRECISION' in result.stderr
    with tempfile.TemporaryDirectory() as temp:
        output = Path(temp) / 'out.c'
        output.write_bytes(b'existing correct C17\n')
        result = subprocess.run([compiler, '--emit-c17-to', str(output), str(source)], capture_output=True)
        assert result.returncode == 3 and not result.stdout
        assert output.read_bytes() == b'existing correct C17\n'
        assert list(Path(temp).iterdir()) == [output]
    print(name, 'CHECK3/P6 / zero C17 / named output preserved')
