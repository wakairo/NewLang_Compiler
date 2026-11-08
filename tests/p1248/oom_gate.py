"""Forward finite census: same selection/oracle, including optimized Release PCs.

The original v2/320 limit and its CI RED remain unchanged. This version admits
at most512 trials (fresh C288 census required), without dropping any selected site.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

exe, output, *reuse = sys.argv[1:]
here = Path(__file__).resolve().parent
out = Path(output)
out.mkdir()
symbols = subprocess.check_output(['nm', '-S', '--defined-only', exe], text=True)
if reuse:
    assert symbols == (Path(reuse[0]) / 'symbols.txt').read_text()
(out / 'symbols.txt').write_text(symbols)
functions = []
for line in symbols.splitlines():
    m = re.fullmatch(r'([0-9a-f]+) ([0-9a-f]+) [tTwW] (.+)', line)
    if m:
        functions.append((int(m[1], 16), int(m[2], 16), m[3]))
plans = []
for name in ['A-helper-binding.nl', 'B-changed-phase-direct.nl']:
    source = here / 'sources' / name
    raw = out / source.stem
    raw.mkdir()
    if reuse:
        original = Path(reuse[0]) / source.stem
        census = subprocess.CompletedProcess([], 0, (original / 'census-FIRST.stdout').read_bytes(), (original / 'census-FIRST.stderr').read_bytes())
    else:
        census = subprocess.run([exe, 'census', str(source)], capture_output=True)
    (raw / 'census-FIRST.stdout').write_bytes(census.stdout)
    (raw / 'census-FIRST.stderr').write_bytes(census.stderr)
    assert census.returncode == 0, census.stderr.decode()
    events = []
    for stage, ordinal, kind, pc in re.findall(r'P1217 SITE stage=(\w+) ordinal=(\d+) kind=(\w) pc=(0x[0-9a-f]+)', census.stdout.decode()):
        address = int(pc, 16) - 1
        matches = [n for start, size, n in functions if start <= address < start + size]
        assert len(matches) == 1, (pc, matches)
        events.append(dict(stage=stage, ordinal=int(ordinal), kind=kind, pc=pc, function=matches[0]))
    (raw / 'reached-sites.json').write_text(json.dumps(events, indent=2) + '\n')
    for stage in ['registration', 'actual', 'emit', 'named']:
        rows = [e for e in events if e['stage'] == stage]
        assert rows and [r['ordinal'] for r in rows] == list(range(1, len(rows) + 1))
        selected = set()
        if stage == 'emit':
            selected.update(r['ordinal'] for r in rows)  # every emitter allocation
        elif stage == 'named':
            # Emit is the exact last subsequence: reuse all new borrowed-path allocations.
            emission = [(e['kind'], e['pc']) for e in events if e['stage'] == 'emit']
            trace = [(e['kind'], e['pc']) for e in rows]
            offsets = [i for i in range(len(trace) - len(emission) + 1) if trace[i:i + len(emission)] == emission]
            assert len(offsets) == 1, offsets
            selected.update([1, len(rows)])
            selected.update(offsets[0] + i + 1 for i in range(len(emission)))
        else:
            first = {}
            for e in rows:
                first.setdefault((e['kind'], e['pc']), e['ordinal'])
            selected.update(first.values())
        for point in sorted(selected):
            plans.append(dict(source=str(source), stage=stage, point=point,
                              function=rows[point - 1]['function'], pc=rows[point - 1]['pc'],
                              full_stage_reached=len(rows), raw=str(raw)))
# Freeze the actual static-site census and selected ordinals BEFORE injection.
assert 0 < len(plans) <= 512, len(plans)
(out / 'selected-before-injection.json').write_text(json.dumps(plans, indent=2) + '\n')
(out / 'binary-before-injection.sha256').write_text(hashlib.sha256(Path(exe).read_bytes()).hexdigest() + '\n')
print('P1248 fresh/recovered exact census complete; selected trials', len(plans), flush=True)
for i, plan in enumerate(plans):
    result = subprocess.run([exe, plan['stage'], plan['source'], str(plan['point'])], capture_output=True)
    raw = Path(plan['raw'])
    prefix = plan['stage'] + '-' + str(plan['point'])
    (raw / (prefix + '.stdout')).write_bytes(result.stdout)
    (raw / (prefix + '.stderr')).write_bytes(result.stderr)
    assert result.returncode == 0, (plan, result.stderr.decode())
    assert b'P1217 REACHED' in result.stdout
    (out / 'completed.json').write_text(json.dumps(dict(completed=i+1, total=len(plans))) + '\n')
print('P1248 reached exact-OOM/rollback/retry/named-no-clobber PASS', len(plans), flush=True)
