"""P930 independent frozen input custody and unchanged ordinary CLI boundaries."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile

compiler, root = sys.argv[1], Path(sys.argv[2])
freeze = json.loads((root / 'freeze.json').read_text())
assert hashlib.sha256((root / 'evidence/p929-original-freeze.json').read_bytes()).hexdigest() == freeze['preserve_p929_freeze_sha256']
for name, digest in freeze['sources'].items():
    source = root / 'sources' / name
    assert hashlib.sha256(source.read_bytes()).hexdigest() == digest
    negative = freeze['common']['semantic_control_codes'].get(name)
    expected = 3 if negative else 0
    check = subprocess.run([compiler, '--check', str(source)], capture_output=True)
    assert check.returncode == expected and not check.stdout
    assert (negative.encode() in check.stderr) if negative else not check.stderr
    expected_emit = 3 if negative else 4
    diagnostic = negative or 'V1-BACKEND-UNSUPPORTED'
    emit = subprocess.run([compiler, str(source)], capture_output=True)
    assert emit.returncode == expected_emit and not emit.stdout and diagnostic.encode() in emit.stderr
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / 'existing.c'
        out.write_bytes(b'original correct output retained\n')
        emit = subprocess.run([compiler, '--emit-c17-to', str(out), str(source)], capture_output=True)
        assert emit.returncode == expected_emit and not emit.stdout and diagnostic.encode() in emit.stderr
        assert out.read_bytes() == b'original correct output retained\n'
        assert list(Path(tmp).iterdir()) == [out]
    print(name, 'CHECK', expected, 'C17', expected_emit, diagnostic, 'sentinel/no-temp preserved')
