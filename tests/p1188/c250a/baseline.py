"""Fixed-8585 CHECK/emission only; no guest native compilation or execution."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
from mapping import roles, inspect

here = Path(__file__).resolve().parent
root = here.parents[2]
compiler, probe, output = sys.argv[1:]
raw = Path(output)
raw.mkdir(parents=True, exist_ok=False)
freeze = json.loads((here / 'source-freeze-v1.json').read_bytes())
for name, expected in freeze['files'].items():
    assert hashlib.sha256((root / name).read_bytes()).hexdigest() == expected, name
oracle = json.loads((here / 'oracle-v1.json').read_bytes())
rows = []
for name, meta in oracle['cases'].items():
    source = here / 'sources' / (name + '.nl')
    values = {}
    for label, command in [('check', [compiler, '--check', source]), ('emit', [compiler, source]), ('roles', [probe, source])]:
        result = subprocess.run(list(map(str, command)), capture_output=True, timeout=90)
        (raw / (name + '-' + label + '.out')).write_bytes(result.stdout)
        (raw / (name + '-' + label + '.err')).write_bytes(result.stderr)
        row = dict(source=name, stage=label, status=result.returncode)
        rows.append(row)
        (raw / 'observations.json').write_text(json.dumps(rows, indent=2) + '\n')
        assert result.returncode == 0, row
        values[label] = result.stdout.decode()
    view = roles(values['roles'])
    mapping = inspect(values['emit'], view, meta['expected_discard_statements'], False)
    assert mapping['bad_phase_reads'] == meta['baseline_bad_phase_reads'], (name, mapping)
    (raw / (name + '-mapping.json')).write_text(json.dumps({'roles': view, 'mapping': mapping}, indent=2) + '\n')
    if meta['category'] == 'control':
        assert values['emit'].encode() == (here / 'evidence' / (name + '.c17')).read_bytes()
    print(name, 'CHECK=0 C17=0 pre-Break-result-reads=' + str(mapping['bad_phase_reads']), flush=True)
print('C250a baseline observed; no emitted guest was compiled or executed')
