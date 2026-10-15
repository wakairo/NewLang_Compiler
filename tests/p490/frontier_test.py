"""Frozen P490 C1 physical read and C2 precision stop; no source rewrite.
Public source-issued proof precedes unmodified C17 and read-only native hooks.
C2 is an expected analysis gap, never a successful reloan or safety negative.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

TESTS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TESTS / 'integration'))
sys.path.insert(0, str(TESTS / 'p471'))
from node_join_native_test import native
from importlib.util import spec_from_file_location, module_from_spec
_spec = spec_from_file_location('p471_frontier', TESTS / 'p471/frontier_test.py')
_control = module_from_spec(_spec)
_spec.loader.exec_module(_control)
control_header = _control.header


def run(argv):
    return subprocess.run(argv, capture_output=True, text=True, timeout=60)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read_header(manifest, template):
    lines = [line.split() for line in manifest.splitlines()]
    b = [list(map(int, x[1:])) for x in lines if x[0] == 'B']
    assert len(b) == 8 and [x[2] for x in b] == [1, 2, 1, 3, 3, 3, 3, 3]
    assert [x[0] for x in b] == [0, 0, 0, 0, 0, 1, 0, 0]
    tail, pointer, head, old_none, observed, old_tail, joined, old_some = [x[1] for x in b]
    assert b[0][3:] == [2, 0, 0] and b[2][3:] == [1, 0, 0]
    assert b[1][3] == tail
    assert [x[4:] for x in b[3:]] == [[0, 0], [1, tail], [0, 0], [1, tail], [1, tail]]
    writes = [list(map(int, x[1:])) for x in lines if x[0] == 'W']
    assert writes == [[head, 0, 0, 1, tail], [tail, 0, 0, 1, tail], [head, 1, tail, 0, 0]]
    assert [x for x in lines if x[0] == 'M'] == [['M', '1', str(tail)]]
    roles = dict(TAIL=tail, PTR=pointer, HEAD=head, OLD_NONE=old_none,
                 OBSERVED=observed, OLD_TAIL=old_tail, JOINED=joined, OLD_SOME=old_some)
    return ''.join(f'#define P490_{key} {value}\n' for key, value in roles.items()) + template


def measure(compiler, probe, runtime_probe, cc, repo, source, name, root, sanitizer):
    checked = run([compiler, '--check', str(source)])
    assert checked.returncode == 0 and not checked.stdout and not checked.stderr, checked
    public = run([probe, str(source)])
    assert public.returncode == 0 and not public.stderr, public
    if name == 'C1':
        assert 'variant=UNKNOWN' in public.stdout and 'PUBLIC VALID' in public.stdout
    manifest = run([runtime_probe, str(source)])
    assert manifest.returncode == 0 and not manifest.stderr, manifest
    if name == 'C1':
        passive = read_header(manifest.stdout, (repo / 'tests/p490/native_observer.h').read_text())
    else:
        passive = control_header(manifest.stdout, (repo / 'tests/p471/native_observer.h').read_text())
    emitted = run([compiler, str(source)])
    assert emitted.returncode == 0 and not emitted.stderr and emitted.stdout, emitted
    assert 'switch (' in emitted.stdout and 'case 0:' in emitted.stdout and 'case 1:' in emitted.stdout
    h = root / (name + '.h')
    h.write_text(passive)
    runs = [native(root, cc, name + opt + str(observed), emitted.stdout,
                   h if observed else None, opt, sanitizer)
            for opt in ('-O0', '-O2') for observed in (False, True)]
    if name == 'C1':
        assert all('post_join_read=Some(' in x['stdout'] for x in runs if 'True' in x['name'])
    return dict(case=name, check_exit=0, public_exit=0, public=public.stdout,
                manifest=manifest.stdout, C17_exit=0, C17_bytes=len(emitted.stdout.encode()),
                C17_sha256=sha(emitted.stdout.encode()), runs=runs)


def main():
    compiler, probe, control_probe, runtime_probe, cc, repo, sanitizer = sys.argv[1:]
    repo = Path(repo)
    frozen = json.loads((repo / 'tests/p490/frozen.json').read_text())
    control = repo / frozen['control']
    assert sha(control.read_bytes()) == frozen['control_sha256']
    for c in frozen['cases']:
        assert sha((repo / c['source']).read_bytes()) == c['sha256'], 'STOP source hash mismatch'
    c1, c2 = [repo / x['source'] for x in frozen['cases']]
    refused = run([compiler, '--check', str(c2)])
    assert refused.returncode == 3 and not refused.stdout
    assert 'error(precision)[NODE-JOIN-POST-PRECISION]' in refused.stderr
    assert ':16:5-19:6:' in refused.stderr
    with tempfile.TemporaryDirectory(prefix='newlang-p490-') as tmp:
        root = Path(tmp)
        records = [measure(compiler, control_probe, runtime_probe, cc, repo,
                           control, 'control', root, sanitizer),
                   measure(compiler, probe, runtime_probe, cc, repo,
                           c1, 'C1', root, sanitizer)]
    print(json.dumps(dict(host=cc, sanitizer=sanitizer, cases=records,
                          C2=dict(check_exit=3, diagnostic=refused.stderr,
                                  first_barrier='NODE-JOIN-POST-PRECISION',
                                  owned='NOT REACHED', C17='NOT REACHED', native='NOT REACHED')), indent=2))
    print('P490 C1 PHYSICAL READ PASS; C2 EXPECTED PRECISION STOP (RELOAN NOT REACHED)')


if __name__ == '__main__':
    main()
