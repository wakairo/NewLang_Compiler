"""Frozen source hashes, semantic admission and unchanged C17 fail-closed."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile

compiler, root = sys.argv[1:]
root = Path(root)
old = json.loads((root / 'evidence/p814/freeze.json').read_text())
fresh = json.loads((root / 'evidence/p817/freeze.json').read_text())

def run(*args):
    return subprocess.run([str(x) for x in (compiler, *args)], capture_output=True)

with tempfile.TemporaryDirectory(prefix='p817-cli-') as directory:
    target = Path(directory) / 'saved.c'
    for name, digest in old['sources'].items():
        source = root / 'tests/p813/sources' / name
        assert hashlib.sha256(source.read_bytes()).hexdigest() == digest
    new = root / fresh['new_negative']['path']
    assert hashlib.sha256(new.read_bytes()).hexdigest() == fresh['new_negative']['sha256']
    for name in ('pump_true', 'pump_false', 'charge_true', 'charge_false'):
        source = root / 'tests/p813/sources' / (name + '.nl')
        checked = run('--check', source)
        assert checked.returncode == 0 and not checked.stdout and not checked.stderr
        emitted = run(source)
        assert emitted.returncode == 4 and not emitted.stdout
        assert b'V1-BACKEND-UNSUPPORTED' in emitted.stderr
        target.write_bytes(b'previous correct output')
        named = run('--emit-c17-to', target, source)
        assert named.returncode == 4 and not named.stdout
        assert target.read_bytes() == b'previous correct output'
        assert sorted(x.name for x in Path(directory).iterdir()) == ['saved.c']
        print(name, 'CHECK0 C174 zero-output named-sentinel-preserved; no native claim')
    negative = [(new, b'error(semantic)[P3-TYPE-MISMATCH]')]
    for name, code in [('wrong_field', b'FIELD-UNKNOWN-FIELD'),
                       ('wrong_return', b'P9-RETURN-TYPE'),
                       ('escape', b'P8-EXIT-DEPENDENCY')]:
        negative.append((root / 'tests/p813/sources' / (name + '.nl'),
                         b'error(semantic)[' + code + b']'))
    # Historical mismatch is preserved and explicitly NOT a semantic type proof.
    negative.append((root / 'tests/p813/sources/read_only.nl',
                     b'error(unsupported)[LOCAL-LOAN-PROFILE]'))
    for source, diagnostic in negative:
        checked = run('--check', source)
        emitted = run(source)
        assert checked.returncode == emitted.returncode == 3
        assert not checked.stdout and not emitted.stdout
        assert diagnostic in checked.stderr and diagnostic in emitted.stderr
        target.write_bytes(b'previous correct output')
        named = run('--emit-c17-to', target, source)
        assert named.returncode == 3 and not named.stdout
        assert diagnostic in named.stderr
        assert target.read_bytes() == b'previous correct output'
        assert sorted(x.name for x in Path(directory).iterdir()) == ['saved.c']
        print(source.name, checked.stderr.decode().strip())
