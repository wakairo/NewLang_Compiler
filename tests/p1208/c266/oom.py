"""Fresh reached census; <=128 targeted rollback/no-clobber trials/config."""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess
import sys

sys.dont_write_bytecode = True
from inputs import HERE, sha, verify

verify()
exe, destination = map(Path, sys.argv[1:])
raw = destination
raw.mkdir(parents=True, exist_ok=False)
source = HERE / 'sources/03-left-final-return.nl'
env = {**os.environ, 'ASAN_OPTIONS': 'detect_leaks=1:abort_on_error=1',
       'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}
census = subprocess.run([exe, source, 'census'], capture_output=True, env=env, timeout=120)
(raw / 'census.out').write_bytes(census.stdout)
(raw / 'census.err').write_bytes(census.stderr)
assert census.returncode == 0, 'census FIRST; injection not started'
table = subprocess.check_output(['nm', '-S', '--defined-only', exe], text=True)
(raw / 'symbols.txt').write_text(table)
functions = []
for line in table.splitlines():
    m = re.fullmatch(r'([0-9a-f]+) ([0-9a-f]+) ([tTwW]) (.+)', line)
    if m:
        functions.append((int(m[1], 16), int(m[2], 16), m[4]))
assert sum(name == 'append' for _, _, name in functions) == 1
events = []
for stage, ordinal, kind, pc in re.findall(
    r'C263 SITE stage=(\w+) ordinal=(\d+) kind=(\w) pc=(0x[0-9a-f]+)', census.stdout.decode()
):
    address = int(pc, 16) - 1
    names = [name for start, size, name in functions if start <= address < start + size]
    assert len(names) == 1, (pc, names)
    events.append(dict(stage=stage, ordinal=int(ordinal), kind=kind, pc=pc, function=names[0]))
plans = {}
total = 0
for stage in ('registration', 'actual', 'named'):
    rows = [e for e in events if e['stage'] == stage]
    assert {e['ordinal'] for e in rows} == set(range(1, len(rows) + 1))
    by_function = {}
    for event in rows:
        by_function.setdefault(event['function'], []).append(event['ordinal'])
    selected = set()
    for name in ('nl_control_state_create', 'nl_control_state_capture', 'append', 'nl_copy_state_anchor'):
        assert by_function.get(name), (stage, name)
        selected.add(by_function[name][0])
    windows = []
    if stage != 'registration':
        starts = by_function['seal_current_cycle']
        signatures = by_function['current_signature_seal']
        assert len(starts) == len(signatures) == 1
        begin = starts[0]
        end = next(e['ordinal'] for e in rows if e['ordinal'] > begin and e['function'] == 'linear_seal_graph')
        assert end > begin + 20, 'real RETURN array/clone/anchor prefix must exist'
        windows = [[begin, end - 1]]
        if stage == 'actual':
            selected.update(range(begin, end))  # full partial-prefix rollback
        else:
            selected.update([begin, begin + 1, end - 2, end - 1])
            for name in ('nl_control_state_clone', 'nl_control_state_anchor_clone'):
                reached = [e['ordinal'] for e in rows if begin <= e['ordinal'] < end and e['function'] == name]
                if reached:
                    selected.update([reached[0], reached[-1]])
        selected.update(signatures)
    points = raw / (stage + '.points')
    points.write_text(''.join(f'{p}\n' for p in sorted(selected)))
    plans[stage] = dict(full_reached=len(rows), selected=len(selected),
                       return_windows_inclusive=windows, points_sha256=sha(points.read_bytes()))
    total += len(selected)
assert 0 < total <= 128, ('C266 injection cap; no trial started', total)
plan = dict(version=1, before_injected_execution=True, binary_sha256=sha(exe.read_bytes()),
            source_sha256=sha(source.read_bytes()), selector_sha256=sha(Path(__file__).read_bytes()),
            census_sha256=sha(census.stdout), stages=plans, selected_total=total,
            oracle='Exact OOM/rollback/no publication/retry; named bytes/inode/mtime/temp no-clobber; original host assertions unchanged')
(raw / 'pre-injection-plan-v1.json').write_text(json.dumps(plan, indent=2) + '\n')
for stage, meta in plans.items():
    assert sha(exe.read_bytes()) == plan['binary_sha256']
    points = raw / (stage + '.points')
    assert sha(points.read_bytes()) == meta['points_sha256']
    result = subprocess.run([exe, source, stage, points], capture_output=True, env=env, timeout=180)
    (raw / (stage + '.out')).write_bytes(result.stdout)
    (raw / (stage + '.err')).write_bytes(result.stderr)
    assert result.returncode == 0, (stage, 'FIRST trial failure')
    text = result.stdout.decode()
    reached = [int(p) for p in re.findall(r'C263 reached stage=' + stage + r' ordinal=(\d+) ', text)]
    assert reached == [int(p) for p in points.read_text().split()]
    assert f'C263 selected OOM stage={stage} trials={meta["selected"]} PASS' in text
    assert not re.search(r'ERROR: (?:AddressSanitizer|LeakSanitizer)|runtime error:', result.stderr.decode())
    print('C266 OOM', stage, 'reached', len(reached), 'rollback/retry/no-clobber PASS', flush=True)
print('C266 selected reached OOM PASS', total, 'cap128/config')
