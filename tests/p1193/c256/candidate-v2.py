"""First RETURN candidate: real CLI and owned-loop checks before any native."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
here = Path(__file__).resolve().parent
repo = here.parents[2]
compiler, probe, dest = map(Path, sys.argv[1:])
dest.mkdir(parents=True, exist_ok=False)
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
frozen = json.loads((here / 'candidate-freeze-v2.json').read_text())
for name, digest in frozen['files'].items():
    assert sha(repo / name) == digest, name
old = json.loads((here / 'old-oracle-v1.json').read_text())
rows = []

def run(args, label, expected):
    r = subprocess.run(list(map(str, args)), capture_output=True, timeout=180)
    (dest / (label + '.out')).write_bytes(r.stdout)
    (dest / (label + '.err')).write_bytes(r.stderr)
    rows.append(dict(label=label, status=r.returncode, expected=expected,
                     args=list(map(str,args))))
    (dest / 'observations.json').write_text(json.dumps(rows, indent=2) + '\n')
    assert r.returncode == expected, rows[-1]
    return r

for name, meta in old['cases'].items():
    source = here / 'sources' / (name + '.nl')
    expected = 3 if meta['classification'] == 'invalid' else 0
    run([compiler, '--check', source], name + '-check', expected)
    c = run([compiler, source], name + '-emit', expected)
    named = dest / (name + '.named.c')
    named.write_bytes(b'C256 preexisting output\n')
    before = (named.read_bytes(), named.stat().st_ino, named.stat().st_mtime_ns)
    n = run([compiler, '--emit-c17-to', named, source], name + '-named', expected)
    if expected:
        assert not c.stdout and not n.stdout
        assert before == (named.read_bytes(), named.stat().st_ino, named.stat().st_mtime_ns)
    else:
        assert c.stdout == named.read_bytes() and not n.stdout
        if name == '80-normal-control':
            assert c.stdout == (here / '80-normal-control.c17').read_bytes()
    actuals = meta['isolated_helper_actuals'] or {'0':'-', '3':'-'}
    args = [str(v) for pair in actuals.items() for v in pair]
    observed = run([probe, source, *args], name + '-owned', 0).stdout.decode()
    if not expected:
        assert 'P1193 AUTH stage=full owned=1 body_GET=1' in observed
        assert 'LOOP stage=full current_validate=0' in observed
    print(name, 'CLI/C17/owned-current PASS', flush=True)
print('C256 candidate seven-case vertical PASS; guest native0')
