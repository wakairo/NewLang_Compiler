"""Fresh alias-source census: every emit OOM and any new CHECK allocation site.

The original P1217 strict status/rollback/retry/no-clobber C oracle is unchanged.
Previously covered PCs are reported as such, not claimed as alias ordinal tests.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

exe, previous_dir, directory = sys.argv[1:]
previous, out = Path(previous_dir), Path(directory)
out.mkdir()
here = Path(__file__).resolve().parent
sha = lambda data: hashlib.sha256(data).hexdigest()
assert sha(Path(exe).read_bytes()) == (previous / 'binary-before-injection.sha256').read_text().strip()
symbols = subprocess.check_output(['nm', '-S', '--defined-only', exe], text=True)
assert symbols == (previous / 'symbols.txt').read_text()
functions = []
for line in symbols.splitlines():
    m = re.fullmatch(r'([0-9a-f]+) ([0-9a-f]+) [tTwW] (.+)', line)
    if m:
        functions.append((int(m[1], 16), int(m[2], 16), m[3]))
plans = []
coverage = []
for place in ['preH', 'postH']:
    old = json.loads((previous / ('A-' + place) / 'reached-sites.json').read_bytes())
    old_sites = {(r['stage'], r['kind'], r['pc']) for r in old}
    source = here / 'sources' / ('A-alias-' + place + '.nl')
    raw = out / source.stem
    raw.mkdir()
    p = subprocess.run([exe, 'census', str(source)], capture_output=True)
    (raw / 'census-FIRST.stdout').write_bytes(p.stdout)
    (raw / 'census-FIRST.stderr').write_bytes(p.stderr)
    assert p.returncode == 0, p.stderr.decode()
    events = []
    for stage, n, kind, pc in re.findall(rb'P1217 SITE stage=(\w+) ordinal=(\d+) kind=(\w) pc=(0x[0-9a-f]+)', p.stdout):
        address = int(pc, 16) - 1
        names = [name for start, size, name in functions if start <= address < start + size]
        assert len(names) == 1, (pc, names)
        events.append(dict(stage=stage.decode(), ordinal=int(n), kind=kind.decode(), pc=pc.decode(), function=names[0]))
    (raw / 'all-reached-sites.json').write_text(json.dumps(events, indent=2) + '\n')
    emit = [(x['kind'], x['pc']) for x in events if x['stage'] == 'emit']
    for stage in ['registration', 'actual', 'emit', 'named']:
        rows = [x for x in events if x['stage'] == stage]
        assert rows and [x['ordinal'] for x in rows] == list(range(1, len(rows) + 1))
        selected = set()
        if stage == 'emit':
            selected.update(x['ordinal'] for x in rows)
        elif stage == 'named':
            trace = [(x['kind'], x['pc']) for x in rows]
            offsets = [i for i in range(len(trace) - len(emit) + 1) if trace[i:i + len(emit)] == emit]
            assert len(offsets) == 1
            selected.update([1, len(rows)])
            selected.update(offsets[0] + i + 1 for i in range(len(emit)))
        else:
            first = {}
            for x in rows:
                first.setdefault((x['kind'], x['pc']), x)
            selected.update(x['ordinal'] for x in first.values() if (stage, x['kind'], x['pc']) not in old_sites)
            coverage.append(dict(source=source.name, stage=stage, reached=len(rows),
                                 old_source_covered_static_sites=len(first) - len(selected),
                                 additional_alias_sites=len(selected)))
        for ordinal in sorted(selected):
            plans.append(dict(source=str(source), stage=stage, ordinal=ordinal, raw=str(raw), event=rows[ordinal - 1]))
old_plan = json.loads((previous / 'selected-before-injection.json').read_bytes())
assert 0 < len(plans) <= 128 and len(old_plan) + len(plans) <= 512, (len(old_plan), len(plans))
(out / 'selected-before-injection.json').write_text(json.dumps(dict(binary_sha256=sha(Path(exe).read_bytes()), plans=plans, static_coverage=coverage, old_source_trials=len(old_plan)), indent=2) + '\n')
for i, plan in enumerate(plans):
    p = subprocess.run([exe, plan['stage'], plan['source'], str(plan['ordinal'])], capture_output=True)
    raw = Path(plan['raw'])
    prefix = plan['stage'] + '-' + str(plan['ordinal'])
    (raw / (prefix + '.stdout')).write_bytes(p.stdout)
    (raw / (prefix + '.stderr')).write_bytes(p.stderr)
    assert p.returncode == 0 and b'P1217 REACHED' in p.stdout, (plan, p.stderr.decode())
    (out / 'completed.json').write_text(json.dumps(dict(completed=i + 1, total=len(plans))) + '\n')
print('C283 fresh alias every emit/named OOM + newly reached CHECK static-site OOM PASS', len(plans), 'combined', len(old_plan) + len(plans))
