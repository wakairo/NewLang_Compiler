"""Preserve P930 input/history; P934 prospectively adds pure-unit C17 output."""
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
    # The frozen initial EXIT4 remains historical data, not a current oracle.
    # P934's separately frozen backend candidate now supports these positives.
    expected_emit = 3 if negative else 0
    diagnostic = negative
    emit = subprocess.run([compiler, str(source)], capture_output=True)
    assert emit.returncode == expected_emit
    if negative:
        assert not emit.stdout and diagnostic.encode() in emit.stderr
    else:
        assert emit.stdout.startswith(b'/*') and not emit.stderr
    generated = emit.stdout
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / 'existing.c'
        out.write_bytes(b'original correct output retained\n')
        emit = subprocess.run([compiler, '--emit-c17-to', str(out), str(source)], capture_output=True)
        assert emit.returncode == expected_emit and not emit.stdout
        assert diagnostic.encode() in emit.stderr if negative else not emit.stderr
        assert out.read_bytes() == (b'original correct output retained\n' if negative else generated)
        assert list(Path(tmp).iterdir()) == [out]
    print(name, 'CHECK', expected, 'C17', expected_emit, diagnostic, 'output/no-temp checked; historical oracle retained')

# Same registered CTest, actual configured compiler/sanitizer and existing
# output-fault executable. No alternate compiler or historical false-green.
subprocess.run([sys.executable, str(root.parent / 'p934/native.py'),
                compiler, str(root.parents[1])], check=True)
