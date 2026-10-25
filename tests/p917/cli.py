"""P-owned fixed-source/ordinary-CLI custody; no R harness or oracle."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile

compiler, root = sys.argv[1], Path(sys.argv[2])
for case in json.loads((root / 'freeze.json').read_text())['sources']:
    source = root / 'sources' / case['file']
    assert hashlib.sha256(source.read_bytes()).hexdigest() == case['sha256']
    for args in [['--check', str(source)], [str(source)]]:
        result = subprocess.run([compiler, *args], capture_output=True)
        assert result.returncode == 3 and not result.stdout
        assert b'P6-JOIN-PRECISION' in result.stderr
    with tempfile.TemporaryDirectory() as temp:
        output = Path(temp) / 'out.c'
        output.write_bytes(b'existing valid output\n')
        result = subprocess.run([compiler, '--emit-c17-to', str(output), str(source)], capture_output=True)
        assert result.returncode == 3 and not result.stdout
        assert output.read_bytes() == b'existing valid output\n'
        assert list(Path(temp).iterdir()) == [output]
    print(source.name, 'CHECK3 / no public IF / zero C / named output preserved')
