"""Shared-H resource cause stays private; the original strict OOM oracle runs."""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess
import sys

compiler, probe, oom, cc, sanitizer, directory = sys.argv[1:]
here = Path(__file__).resolve().parent
out = Path(directory)
out.mkdir()
sha = lambda b: hashlib.sha256(b).hexdigest()
for name, digest in json.loads((here / 'apparatus-freeze-v1.json').read_bytes())['files'].items():
    assert sha((here / name).read_bytes()) == digest, name
env = os.environ.copy()
env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
           UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')


def run(label, args, success=True):
    (out / (label + '.argv.json')).write_text(json.dumps(args) + '\n')
    result = subprocess.run(args, capture_output=True, env=env, timeout=45)
    (out / (label + '.stdout')).write_bytes(result.stdout)
    (out / (label + '.stderr')).write_bytes(result.stderr)
    assert (result.returncode == 0) == success, (label, result.returncode, result.stderr.decode())
    if success:
        assert not result.stderr, (label, result.stderr.decode())
    return result


source = here / '01-independent-h-only.nl'
for name, path in [('H-only', source),
                   ('B03', here.parent / 'sources/03-postheap-current-loop.nl'),
                   ('B04', here.parent / 'sources/04-preheap-original-current-loop.nl')]:
    result = run(name + '-status', [probe, str(path), '2'])
    assert b'emit ordinal=2 status=2 out-null=1 length=98765 root-same=1' in result.stdout
    assert b'logical-invalid producer status=1' in result.stdout
    assert b'rollback/retry/canonical-destroy PASS' in result.stdout

negative = here / '90-wrong-domain-h-only.nl'
target = out / 'negative.c'
target.write_bytes(b'C274 named output must stay unchanged\n')
before = (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns)
run('negative-check', [compiler, '--check', str(negative)], False)
run('negative-emit', [compiler, '--emit-c17-to', str(target), str(negative)], False)
assert (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns) == before
run('positive-check', [compiler, '--check', str(source)])
generated = out / 'positive.c'
run('positive-emit', [compiler, '--emit-c17-to', str(generated), str(source)])

# Every reached emitter allocation, including each repeated producer check,
# plus every corresponding named-output allocation and first/last endpoint.
census = run('h-only-census', [oom, 'census', str(source)])
events = [(stage, int(n), kind, pc) for stage, n, kind, pc in re.findall(
    r'P1217 SITE stage=(\w+) ordinal=(\d+) kind=(\w) pc=(0x[0-9a-f]+)', census.stdout.decode())]
emit = [(kind, pc) for stage, _, kind, pc in events if stage == 'emit']
named = [(kind, pc) for stage, _, kind, pc in events if stage == 'named']
offsets = [i for i in range(len(named) - len(emit) + 1) if named[i:i + len(emit)] == emit]
assert emit and len(offsets) == 1
plans = [('emit', n + 1) for n in range(len(emit))]
plans += [('named', n) for n in sorted({1, len(named)} | {
    offsets[0] + n + 1 for n in range(len(emit))})]
assert 0 < len(plans) <= 48
(out / 'h-only-selected-before-injection.json').write_text(json.dumps({
    'binary_sha256': sha(Path(oom).read_bytes()), 'events': events, 'plans': plans
}, indent=2) + '\n')
for stage, point in plans:
    trial = run(f'h-only-{stage}-{point}', [oom, stage, str(source), str(point)])
    assert b'P1217 REACHED' in trial.stdout

summary = []
for optimization in (['-O0', '-O2'] if sanitizer == 'none' else ['-O2']):
    executable = out / ('guest-' + optimization[1:])
    args = [cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', optimization]
    args += [f'-D{key}="{here / "heap_observer.h"}"' for key in
             ['NEWLANG_NODE_OBSERVER', 'NEWLANG_HEAP_OBSERVER']]
    if sanitizer != 'none':
        args += ['-fsanitize=address,undefined' if sanitizer == 'address' else '-fsanitize=undefined',
                 '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    run('build-' + optimization[1:], args + [str(generated), '-o', str(executable)])
    for budget in [0, 1, 2]:
        env['P1217_HEAP_BUDGET'] = str(budget)
        result = run(f'guest-{optimization[1:]}-{budget}', [str(executable)])
        assert b'original H identity/field/owner/cleanup PASS' in result.stdout
        summary.append({'optimization': optimization, 'guest_heap': budget, 'exit': result.returncode})
(out / 'results.json').write_text(json.dumps({
    'h_only_strict_oom': len(plans), 'native': summary,
    'generated_sha256': sha(generated.read_bytes())
}, indent=2) + '\n')
print('C274 H-only/shared status/logical rejection/no-clobber/OOM/native PASS', len(plans), len(summary))
