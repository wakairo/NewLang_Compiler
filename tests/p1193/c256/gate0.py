"""New-target Gate0; every source/oracle/FIRST from the old target stays exact."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

here = Path(__file__).resolve().parent
repo = here.parents[2]
compiler, probe, destination = map(Path, sys.argv[1:])
destination.mkdir(parents=True, exist_ok=False)
sha = lambda b: hashlib.sha256(b).hexdigest()
freeze = json.loads((here / 'new-target-freeze-v1.json').read_bytes())
for name, digest in freeze['files'].items():
    assert sha((repo / name).read_bytes()) == digest, name
assert sha(compiler.read_bytes()) == freeze['baseline_ELF_sha256']
oracle = json.loads((here / 'old-oracle-v1.json').read_bytes())
observations = []
results = {}


def run(args, label, expected):
    args = list(map(str, args))
    r = subprocess.run(args, capture_output=True, timeout=120)
    (destination / (label + '.out')).write_bytes(r.stdout)
    (destination / (label + '.err')).write_bytes(r.stderr)
    row = dict(label=label, args=args, status=r.returncode, expected=expected)
    observations.append(row)
    (destination / 'observations.json').write_text(json.dumps(observations, indent=2) + '\n')
    assert r.returncode == expected, row
    return r


for name, meta in oracle['cases'].items():
    source = here / 'sources' / (name + '.nl')
    expected = 0 if meta['classification'] == 'existing-control' else 3
    checked = run([compiler, '--check', source], name + '-check', expected)
    emitted = run([compiler, source], name + '-emit', expected)
    target = destination / (name + '.named.c')
    target.write_bytes(b'C256 original named output\n')
    before = (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns)
    named = run([compiler, '--emit-c17-to', target, source], name + '-named', expected)
    if expected:
        assert not emitted.stdout and not named.stdout
        assert before == (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns)
    else:
        assert emitted.stdout == (here / '80-normal-control.c17').read_bytes() == target.read_bytes()
    actuals = meta['isolated_helper_actuals'] or {'0': '-', '3': '-'}
    arguments = [str(v) for pair in actuals.items() for v in pair]
    observed = run([probe, source, *arguments], name + '-owned', 0)
    text = observed.stdout.decode()
    span = re.search(r'EXACT-HELPER bytes=(\d+) full_span=(\d+):(\d+)', text)
    assert span
    size, start, end = map(int, span.groups())
    assert end - start == size
    assert sha(source.read_bytes()[start:end]) == meta['helper_declaration_sha256']
    if meta['classification'] == 'canonical-positive':
        assert 'P14-LOOP-PRECISION' in checked.stderr.decode()
        assert 'ownership=NOT_ISSUED rollback=YES' in text
        assert len(re.findall(r'OWNED stage=helper\w+ .*known=0 value=0', text)) == 2
    elif meta['classification'] == 'invalid':
        assert 'P1193 OWNED' not in text
    else:
        assert 'LOOP stage=full current_validate=0' in text
    results[name] = dict(CLI=expected, observations=text,
                         old_numeric_prediction=meta['expected_CLI_check'],
                         native='NOT_RUN')
    (destination / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    print(name, 'CHECK/owned/C17-to-first-boundary', expected, flush=True)
print('C256 new fixed-target Gate07 profiles, CLI21/host7/native0/build0 PASS')
