"""Frozen source refusals preserve existing named-output bytes and identity."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile

here = Path(__file__).resolve().parent
compiler = sys.argv[1]
for name, digest in json.loads((here / 'source-freeze-v1.json').read_text())['files'].items():
    assert hashlib.sha256((here / name).read_bytes()).hexdigest() == digest
with tempfile.TemporaryDirectory(prefix='p1203-negative-') as directory:
    target = Path(directory) / 'out.c'
    target.write_bytes(b'P1203 retained named output\n')
    before = (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns)
    for source in sorted((here / 'sources').glob('9*.nl')):
        for command in [[compiler, '--check', source], [compiler, source],
                        [compiler, '--emit-c17-to', target, source]]:
            response = subprocess.run(list(map(str, command)), capture_output=True, timeout=30)
            assert response.returncode == 3 and not response.stdout
            assert b'precision' in response.stderr or b'error' in response.stderr
            assert (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns) == before
            assert list(Path(directory).iterdir()) == [target]
        print(source.stem, 'rejection/named-no-clobber PASS')
