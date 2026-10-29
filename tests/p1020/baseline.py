"""C118 source-only baseline; no Production edit or synthetic checked graph."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

repo, cli, probe, destination = map(Path, sys.argv[1:])
data = repo / 'tests/p1020'
oracle = json.loads((data / 'oracle-v1.json').read_bytes())
freeze = json.loads((data / 'source-freeze-v1.json').read_bytes())
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
assert all(sha(repo / p) == value for p, value in freeze['files'].items())
assert not destination.exists()
destination.mkdir()
commands = []

def invoke(label, args):
    p = subprocess.run(list(map(str, args)), capture_output=True, timeout=30)
    (destination / (label + '.stdout')).write_bytes(p.stdout)
    (destination / (label + '.stderr')).write_bytes(p.stderr)
    commands.append({'label': label, 'command': list(map(str, args)), 'exit': p.returncode,
                     'stdout_SHA256': hashlib.sha256(p.stdout).hexdigest(),
                     'stderr_SHA256': hashlib.sha256(p.stderr).hexdigest()})
    (destination / 'commands-FIRST.json').write_text(json.dumps(commands, indent=2) + '\n')
    assert p.returncode >= 0, ('signal: STOP', label, p.returncode)
    return p

results = []
for source in sorted((data / 'sources').glob('*.nl')):
    check = invoke(source.stem + '-check', [cli, '--check', source])
    if source.name in oracle['source_negatives'] and check.returncode == 0:
        raise AssertionError(('possible false semantic acceptance STOP before emission', source.name))
    target = destination / (source.stem + '.c')
    emitted = invoke(source.stem + '-emit', [cli, '--emit-c17-to', target, source])
    assert (emitted.returncode == 0) == target.exists()
    sentinel = destination / (source.stem + '-existing.c')
    original = b'P1020 preserve original named output\n'
    sentinel.write_bytes(original)
    before = sentinel.stat()
    named = invoke(source.stem + '-named', [cli, '--emit-c17-to', sentinel, source])
    if named.returncode:
        assert sentinel.read_bytes() == original
        assert (before.st_dev, before.st_ino) == (sentinel.stat().st_dev, sentinel.stat().st_ino)
        assert not list(destination.glob('*.tmp*'))
    observed = invoke(source.stem + '-public', [probe, source])
    assert observed.returncode == 0
    receipts = [json.loads(line) for line in observed.stdout.decode().splitlines() if line.startswith('{')]
    row = {'source': source.name, 'CHECK': check.returncode, 'C17': emitted.returncode,
           'C_SHA256': sha(target) if target.exists() else None,
           'named_failure_preserved': named.returncode != 0,
           'public': receipts}
    results.append(row)
    (destination / 'results-FIRST.json').write_text(json.dumps(results, indent=2) + '\n')
    print(json.dumps(row), flush=True)
assert all(sha(repo / p) == value for p, value in freeze['files'].items())
print('P1020 frozen parent baseline complete; Production edits0/CI0')
