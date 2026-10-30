"""C150 immutable parent observations; positive coverage failure stays RED."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

cli, cc, directory = sys.argv[1:]
cli = Path(cli).resolve()
data = Path(__file__).resolve().parent
repo = data.parents[1]
oracle = json.loads((data / 'oracle-v1.json').read_bytes())
out = Path(directory)
out.mkdir(parents=True, exist_ok=False)
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
    assert r.returncode >= 0, ('unsafe signal STOP', label, r.returncode, r.stderr)
    return r


libs = [cli.parent / ('libnewlang_' + n + '.a') for n in
        ['semantic', 'parser', 'lexer', 'checked', 'syntax', 'source', 'diagnostic']]
r = run('build-public', [cc, '-std=c17', '-Wall', '-Wextra', '-Werror',
                         '-pedantic-errors', '-g', '-O0', '-fno-pie', '-no-pie',
                         '-I' + str(repo / 'include'), data / 'observe.c',
                         '-Wl,--start-group', *libs, '-Wl,--end-group', '-o', out / 'observe'])
assert not r.returncode and not r.stderr, (r.returncode, r.stdout, r.stderr)
for positive, group in [(True, 'positive'), (False, 'negative')]:
    for name, expected in oracle[group].items():
        source = data / 'sources' / (name + '.nl')
        check = run(name + '-CHECK', [cli, '--check', source])
        assert not check.stdout
        public, observed = None, None
        if not check.returncode:
            count = expected['genuine_Copy_calls'] if positive else expected['calls']
            observation = run(name + '-public', [out / 'observe', source, str(count)])
            observed = observation.returncode
            if not observed:
                assert not observation.stderr
                public = [json.loads(s) for s in observation.stdout.decode().splitlines()
                          if s.startswith('{')]
        named = out / 'sentinel.c'
        sentinel = b'P1062 preserve complete prior output\n'
        named.write_bytes(sentinel)
        before = named.stat()
        c = run(name + '-C17', [cli, '--emit-c17-to', named, source])
        assert not c.stdout
        emitted = None
        if not c.returncode:
            assert not c.stderr and named.read_bytes() != sentinel
            emitted = named.read_bytes()
            (out / (name + '.c.data')).write_bytes(emitted)
        else:
            assert named.read_bytes() == sentinel
            assert (before.st_dev, before.st_ino) == (named.stat().st_dev, named.stat().st_ino)
        assert not list(out.glob('*.tmp*'))
        current = [v for v in public or [] if v['P1062'] == 'current']
        h = [v for v in public or [] if v['P1062'].startswith('H_')]
        authentic = (public is not None and observed == 0 and
                     all(v['GET_body'] and v['GET_parameters'] and
                         v['body'] == v['parameters'] == 0 for v in current))
        target = bool(positive and check.returncode == 0 and authentic and
                      c.returncode == expected['expected_C17'])
        result = dict(source=name, source_SHA256=sha(source.read_bytes()),
                      positive=positive, CHECK=check.returncode, observer_exit=observed,
                      public=public, authentic_Copy_current=authentic, H_observations=h,
                      C17=c.returncode, C_SHA256=sha(emitted) if emitted else None,
                      diagnostic=c.stderr.decode(), positive_target_reached=target,
                      first_private_runtime_predicate='UNKNOWN in uninstrumented fixed parent',
                      native='NOT EXECUTED')
        results.append(result)
        (out / 'results-FIRST.json').write_text(json.dumps(results, indent=2) + '\n')
        print(json.dumps(result), flush=True)
        if positive:
            if name.startswith('regression_'):
                assert target, ('unchanged P1058 regression STOP', result)
        else:
            assert check.returncode in expected['CHECK_allowed'], result
            status = check.returncode if expected['C17'] == 'same-as-CHECK' else expected['C17']
            assert c.returncode == status, ('unsafe false admission STOP', result)
print('P1062 parent FIRST observations preserved; unmet positive expectations remain RED', flush=True)
