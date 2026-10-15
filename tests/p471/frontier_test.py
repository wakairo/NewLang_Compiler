"""Frozen S/A first-barrier map. Runner success does not imply candidate PASS.
No source rewriting or invented semantic authority; source-owned public probes
precede ordinary C emission. Native physical expectations predate execution.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'integration'))
from node_topology_test import observer as control_observer
from node_join_native_test import native


def run(args):
    return subprocess.run(args, capture_output=True, text=True, timeout=60)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def header(manifest, template):
    lines = [line.split() for line in manifest.splitlines()]
    b = [list(map(int, line[1:])) for line in lines if line[0] == 'B']
    assert len(b) == 7
    assert [x[2] for x in b] == [1, 2, 1, 3, 3, 3, 3]
    tail, pointer, head, old_none, observed, old_tail, old_some = [x[1] for x in b]
    assert [x[0] for x in b] == [0, 0, 0, 0, 0, 1, 0]
    assert b[0][3:] == [2, 0, 0] and b[2][3:] == [1, 0, 0]
    assert b[1][3] == tail
    assert [x[4:] for x in b[3:]] == [[0, 0], [1, tail], [0, 0], [1, tail]]
    writes = [list(map(int, x[1:])) for x in lines if x[0] == 'W']
    assert writes == [[head, 0, 0, 1, tail], [tail, 0, 0, 1, tail], [head, 1, tail, 0, 0]]
    assert [x for x in lines if x[0] == 'M'] == [['M', '1', str(tail)]]
    roles = dict(TAIL=tail, PTR=pointer, HEAD=head, OLD_NONE=old_none,
                 OBSERVED=observed, OLD_TAIL=old_tail, OLD_SOME=old_some)
    return ''.join(f'#define P471_{name} {symbol}\n' for name, symbol in roles.items()) + template


def measure(compiler, probe, runtime_probe, source, expected, name, root, cc, sanitizer, folder):
    raw = source.read_bytes()
    assert digest(raw) == expected['sha256'], 'STOP source hash mismatch'
    result = dict(case=name, source_sha256=digest(raw))
    checked = run([compiler, '--check', str(source)])
    result.update(check_exit=checked.returncode, check_stdout=checked.stdout, diagnostic=checked.stderr)
    if checked.returncode:
        assert checked.returncode == 3 and not checked.stdout and checked.stderr
        result.update(first_barrier='SOURCE/CHECK', owned='NOT REACHED', C17='NOT REACHED', native='NOT REACHED')
        return result
    assert not checked.stdout and not checked.stderr
    owned = run([runtime_probe if name == 'control' else probe, str(source)])
    result.update(public_probe_exit=owned.returncode, public_evidence=owned.stdout, public_diagnostic=owned.stderr)
    if owned.returncode:
        result.update(first_barrier='PUBLIC EVIDENCE NOT ESTABLISHED', C17='NOT REACHED', native='NOT REACHED')
        return result
    assert not owned.stderr
    emitted = run([compiler, str(source)])
    result.update(backend_exit=emitted.returncode, C_bytes=len(emitted.stdout.encode()),
                  C_sha256=digest(emitted.stdout.encode()), backend_diagnostic=emitted.stderr)
    if emitted.returncode:
        assert emitted.returncode == 4 and not emitted.stdout and 'V1-BACKEND-UNSUPPORTED' in emitted.stderr
        result.update(first_barrier='BACKEND', native='NOT REACHED')
        return result
    assert emitted.stdout and not emitted.stderr
    if name == 'control':
        passive = control_observer(owned.stdout)
        result['public_join'] = 'N/A normal_frame_unchanged control'
    else:
        runtime = run([runtime_probe, str(source)])
        assert runtime.returncode == 0 and not runtime.stderr, runtime
        result['public_runtime_manifest'] = runtime.stdout
        passive = header(runtime.stdout, (folder / 'native_observer.h').read_text())
    h = root / (name + '.h')
    h.write_text(passive)
    result['runs'] = [native(root, cc, name + opt + str(observed), emitted.stdout,
                             h if observed else None, opt, sanitizer)
                      for opt in ('-O0', '-O2') for observed in (False, True)]
    result['first_barrier'] = None
    result['observation'] = 'PASS original physical topology (not heap authority/product)'
    return result


def main():
    compiler, probe, control_probe, runtime_probe, cc, repo, sanitizer = sys.argv[1:]
    repo = Path(repo)
    folder = repo / 'tests/p471'
    frozen = json.loads((folder / 'frozen.json').read_text())
    records = []
    with tempfile.TemporaryDirectory(prefix='newlang-p471-') as tmp:
        root = Path(tmp)
        control = measure(compiler, probe, control_probe, repo / frozen['control']['path'],
                          frozen['control'], 'control', root, cc, sanitizer, folder)
        assert control['first_barrier'] is None, ('control failure', control)
        records.append(control)
        for name, expected in frozen['candidates'].items():
            records.append(measure(compiler, probe, runtime_probe, folder / (name + '.nl'),
                                   expected, name, root, cc, sanitizer, folder))
    print(json.dumps(dict(compiler_authority=frozen['compiler'], host=cc, sanitizer=sanitizer,
                          contract='FIRST-BARRIER MAP; runner exit0 is not candidate PASS', cases=records), indent=2))
    if all(r['first_barrier'] is None for r in records):
        print('P471 PASS FINITE CROSSROOT: control and both frozen cases observed')
    else:
        print('P471 FIRST BARRIER RECORDED: no native PASS inferred')


if __name__ == '__main__':
    main()
