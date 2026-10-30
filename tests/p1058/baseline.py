"""C148 parent measurements; immutable source/oracle, no candidate execution."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

cli, cc, directory = sys.argv[1:]
cli = Path(cli).resolve()
data = Path(__file__).resolve().parent
repo = data.parents[1]
out = Path(directory)
out.mkdir(parents=True, exist_ok=False)
oracle = json.loads((data / 'oracle-v1.json').read_bytes())
sha = lambda b: hashlib.sha256(b).hexdigest()
assert sha(cli.read_bytes()) == oracle['parent_compiler_SHA256']
commands, results = [], []


def run(label, args):
    args = list(map(str, args))
    r = subprocess.run(args, capture_output=True, timeout=90)
    for stream in ['stdout', 'stderr']:
        (out / (label + '.' + stream)).write_bytes(getattr(r, stream))
    commands.append(dict(label=label, command=args, exit=r.returncode,
                         stdout_SHA256=sha(r.stdout), stderr_SHA256=sha(r.stderr)))
    (out / 'commands-FIRST.json').write_text(json.dumps(commands, indent=2) + '\n')
    assert r.returncode >= 0, ('unexpected signal STOP', label, r.returncode)
    return r


libs = [cli.parent / ('libnewlang_' + n + '.a') for n in
        ['semantic', 'parser', 'lexer', 'checked', 'syntax', 'source', 'diagnostic']]
flags = ['-std=c17', '-Wall', '-Wextra', '-Werror', '-pedantic-errors', '-g',
         '-O0', '-fno-pie', '-no-pie', '-I' + str(repo / 'include')]
r = run('build-public', [cc, *flags, data / 'observe.c', '-Wl,--start-group',
                         *libs, '-Wl,--end-group', '-o', out / 'observe'])
assert not r.returncode and not r.stderr, (r.returncode, r.stdout, r.stderr)
for positive, group in [(True, 'positive'), (False, 'negative')]:
    for name, expected in oracle[group].items():
        source = data / 'sources' / (name + '.nl')
        r = run(name + '-CHECK', [cli, '--check', source])
        allowed = [0] if positive else expected['CHECK_allowed']
        assert r.returncode in allowed and not r.stdout, (name, r.returncode, r.stderr)
        public = None
        if not r.returncode:
            v = run(name + '-public', [out / 'observe', source, str(expected['calls'])])
            assert not v.returncode and not v.stderr, (name, v.returncode, v.stdout, v.stderr)
            public = [json.loads(s) for s in v.stdout.decode().splitlines() if s.startswith('{')]
            currents = [s for s in public if s['P1058'] == 'current']
            assert len(currents) == expected['calls']
            assert all(s['GET_body'] and s['GET_parameters'] and s['body'] == s['parameters'] == 0 for s in currents)
            if positive:
                origins = [s for s in public if s['P1058'] == 'origin']
                assert len(origins) == 2 and len({s['root'] for s in origins}) == 2
                assert len({s['function'] for s in currents}) == 2
                assert all(s['event_count'] == 1 and s['returned_event'] == 0 and s['old'] == s['returned'] and s['old_forward_source'] for s in origins)
                assert [s['old'] for s in origins] == expected['helper_old']
                assert [s['next'] for s in origins] == expected['helper_next']
                assert [s['sibling'] for s in origins] == expected['helper_sibling']
        status = expected['parent_C17'] if positive else expected['C17']
        if status == 'same-as-CHECK':
            status = r.returncode
        named = out / 'sentinel.c'
        sentinel = b'P1058 parent output must survive\n'
        named.write_bytes(sentinel)
        before = named.stat()
        c = run(name + '-C17', [cli, '--emit-c17-to', named, source])
        assert c.returncode == status and not c.stdout, (name, c.returncode, c.stderr)
        assert named.read_bytes() == sentinel and (before.st_dev, before.st_ino) == (named.stat().st_dev, named.stat().st_ino)
        assert not list(out.glob('*.tmp*'))
        value = dict(source=name, source_SHA256=sha(source.read_bytes()),
                     CHECK=r.returncode, public=public, C17=c.returncode,
                     diagnostic=c.stderr.decode(), native='NOT EXECUTED',
                     first_private_runtime_predicate='UNKNOWN in uninstrumented fixed parent')
        results.append(value)
        (out / 'results-FIRST.json').write_text(json.dumps(results, indent=2) + '\n')
        print(json.dumps(value), flush=True)
print('P1058 parent FIRST source/current/CLI evidence preserved', flush=True)
