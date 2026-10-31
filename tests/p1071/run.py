"""Finite final-unit RETURN contract, using unchanged P-owned owner witnesses."""
from pathlib import Path
import hashlib
import json
import os
import re
import resource
import signal
import subprocess
import sys
import tempfile

cli, cc, sanitizer, selected = sys.argv[1:]
cli = Path(cli).resolve()
data = Path(__file__).resolve().parent
repo = data.parents[1]
oracle = json.loads((data / 'oracle-v1.json').read_bytes())
old = repo / 'tests/p1062'
physical = json.loads((old / 'oracle-v1.json').read_bytes())['positive']
packets = json.loads((old / 'harness-oracle-v1.json').read_bytes())['producer_local_packets']
sha = lambda b: hashlib.sha256(b).hexdigest()
for name, digest in json.loads((data / 'source-freeze-v1.json').read_bytes())['files'].items():
    assert sha((repo / name).read_bytes()) == digest, name
for name, digest in json.loads((data / 'harness-freeze-v1.json').read_bytes())['files'].items():
    assert sha((repo / name).read_bytes()) == digest, name
assert selected in ('0', '1', '2')
profile = oracle['profiles'][int(selected)]
name = profile['name']
source = repo / profile['source']
commands = []
evidence = os.environ.get('P1071_EVIDENCE_DIR')
if evidence:
    Path(evidence).mkdir(parents=True, exist_ok=True)
archive = Path(tempfile.mkdtemp(prefix='P1071-' + name + '-', dir=evidence)) if evidence else None
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))


def run(label, args, **kw):
    args = list(map(str, args))
    r = subprocess.run(args, capture_output=True, timeout=180, **kw)
    commands.append(dict(label=label, command=args, exit=r.returncode,
                         stdout_SHA256=sha(r.stdout), stderr_SHA256=sha(r.stderr)))
    if archive:
        (archive / (label + '.stdout')).write_bytes(r.stdout)
        (archive / (label + '.stderr')).write_bytes(r.stderr)
        (archive / 'commands-FIRST.json').write_text(json.dumps(commands, indent=2) + '\n')
    assert r.returncode >= 0, ('unexpected signal STOP', label, r.returncode, r.stderr)
    return r


def ok(r):
    assert r.returncode == 0 and not r.stderr, (r.returncode, r.stdout, r.stderr)


def preserved(path, sentinel, before):
    after = path.stat()
    assert path.read_bytes() == sentinel
    assert (before.st_dev, before.st_ino) == (after.st_dev, after.st_ino)
    assert not list(path.parent.glob('*.tmp*'))


def quota():
    signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
    resource.setrlimit(resource.RLIMIT_FSIZE, (128, 128))


flags = ['-std=c17', '-Wall', '-Wextra', '-Werror', '-pedantic-errors']
if sanitizer != 'none':
    flags += ['-fsanitize=' + sanitizer, '-fno-omit-frame-pointer', '-fno-sanitize-recover=all']
with tempfile.TemporaryDirectory(prefix='p1071-') as directory:
    temp = Path(directory)
    libraries = [cli.parent / ('libnewlang_' + n + '.a') for n in
                 ['semantic', 'parser', 'checked_c_node', 'lexer', 'checked', 'syntax', 'source', 'diagnostic']]
    for tool in ('observe', 'consumer', 'oom'):
        wraps = ['-Wl,--wrap=' + n for n in ('malloc', 'calloc', 'realloc')] if tool == 'oom' else []
        ok(run('build-' + tool, [cc, *flags, '-g', '-O0', '-fno-pie', '-no-pie',
            '-I' + str(repo / 'include'), old / (tool + '.c'), '-Wl,--start-group',
            *libraries, '-Wl,--end-group', *wraps, '-o', temp / tool]))
    if selected == '0':
        for negative in oracle['profiles'][3:]:
            path = repo / negative['source']
            spec = negative['negative']
            checked = run(negative['name'] + '-CHECK', [cli, '--check', path])
            assert checked.returncode in spec.get('CHECK_allowed', [spec.get('CHECK')])
            assert not checked.stdout
            wanted = checked.returncode or spec['C17_if_CHECK0']
            named = temp / 'negative-existing.c'
            sentinel = b'P1071 rejected source must preserve complete output\n'
            named.write_bytes(sentinel)
            before = named.stat()
            for mode in ('stdout', 'fresh', 'existing'):
                target = temp / 'never-created.c' if mode == 'fresh' else named
                args = [cli] + ([] if mode == 'stdout' else ['--emit-c17-to', target]) + [path]
                r = run(negative['name'] + '-' + mode, args)
                assert r.returncode == wanted and not r.stdout, (negative['name'], r.returncode, r.stderr)
                if mode == 'fresh':
                    assert not target.exists()
                preserved(named, sentinel, before)
            print('P1071 NEGATIVE', negative['name'], 'CHECK', checked.returncode,
                  'C17', wanted, 'no-clobber=YES', flush=True)
    ok(run('CHECK', [cli, '--check', source]))
    public = run('public-current', [temp / 'observe', source, '2'])
    ok(public)
    rows = [json.loads(s) for s in public.stdout.splitlines() if s.startswith(b'{')]
    currents = [v for v in rows if v['P1062'] == 'current']
    origins = [v for v in rows if v['P1062'] == 'origin']
    events = [v for v in rows if v['P1062'] == 'event']
    h = [v for v in rows if v['P1062'].startswith('H_')]
    expected = physical[name]
    assert len(currents) == len(origins) == len(events) == 2
    assert all(v['GET_body'] and v['GET_parameters'] and v['body'] == v['parameters'] == 0 for v in currents)
    assert all(v['current'] == 0 for v in events)
    assert all(v['event_count'] == 1 and v['returned_event'] == 0 and v['old'] == v['returned'] and v['old_forward_source'] for v in origins)
    for field in ('old', 'next', 'sibling'):
        assert [v[field] for v in origins] == expected['helper_' + field]
    assert len({v['function'] for v in currents}) == len({v['root'] for v in origins}) == 2
    assert len(h) == 3 and h[0]['current'] and h[1]['entry_GET'] and h[1]['valid'] and h[1]['return_proved'] and h[2]['entry_GET'] and h[2]['post_proved']
    print(public.stdout.decode(), end='', flush=True)
    emitted = run('C17', [cli, source])
    ok(emitted)
    control = run('tail-unit-control', [cli, repo / profile['control']])
    ok(control)
    assert emitted.stdout and emitted.stdout == control.stdout
    baseline = json.loads((data / 'evidence/parent-FIRST.json').read_bytes())
    assert sha(emitted.stdout) == baseline['controls'][name]['C_SHA256']
    c = temp / 'main.c'
    c.write_bytes(emitted.stdout)
    if archive:
        (archive / 'main.c.data').write_bytes(emitted.stdout)
    frames = re.findall(rb'NL_MIXED_COPY_BIND\((\d+),\d+,&nl_b_\d+_\d+\.f0', emitted.stdout)
    assert len(frames) == 2 and frames[0] == frames[1]
    direct = [v for v in rows if v['P1062'] == 'direct']
    assert len(direct) == 1 and direct[0]['root'] == origins[1]['root']
    assert all(direct[0][k] == expected['direct_' + k] for k in ('index', 'old', 'next'))
    symbols = [v['result_binding'] for v in origins] + [direct[0]['binding']]
    bindings = ['nl_b_' + frames[0].decode() + '_' + str(v) for v in symbols]
    values = {prefix + str(i) + str(j): matrix[i][j]
              for prefix, matrix in [('T', expected['root_initial']), ('F', expected['final'])]
              for i in range(2) for j in range(2)}
    for prefix, key in [('OLD', 'helper_old'), ('NEXT', 'helper_next'), ('SIB', 'helper_sibling'), ('I', 'helper_indices')]:
        values.update({prefix + str(i): expected[key][i] for i in range(2)})
    values.update({'DIRECT_' + k.upper(): expected['direct_' + k] for k in ('index', 'old', 'next')})
    values.update({'H0': expected['H'][0], 'H1': expected['H'][1], 'LOCAL_PACKETS': packets[name],
                   'HEAD_READS': expected['expected_H_reads']['head_none'],
                   'CALLER_READS': expected['expected_H_reads']['caller_tail'],
                   'RECEIVER_READS': expected['expected_H_reads']['receiver_tail']})
    values.update({'RET' + str(i): v for i, v in enumerate(bindings)})
    definitions = ['-DP1062_' + k + '=' + str(v) for k, v in values.items()]
    observer = '-DNEWLANG_HEAP_OBSERVER="' + str(old / 'observer.h') + '"'
    named = temp / 'named.c'
    ok(run('named-publish', [cli, '--emit-c17-to', named, source]))
    assert named.read_bytes() == emitted.stdout
    sentinel = b'P1071 complete previous output\n'
    named.write_bytes(sentinel)
    before = named.stat()
    failure = run('named-output-error', [cli, '--emit-c17-to', named, source], preexec_fn=quota)
    assert failure.returncode == 1 and not failure.stdout and b'OUTPUT' in failure.stderr
    preserved(named, sentinel, before)
    ok(run('named-exact-retry', [cli, '--emit-c17-to', named, source]))
    assert named.read_bytes() == emitted.stdout
    for opt in ('-O0', '-O2'):
        raw = temp / 'raw'
        ok(run(opt + '-raw-build', [cc, *flags, opt, c, '-o', raw]))
        raw_result = run(opt + '-raw', [raw])
        ok(raw_result)
        assert not raw_result.stdout
        observed = temp / 'observed'
        ok(run(opt + '-physical-build', [cc, *flags, opt, c,
            repo / 'tests/integration/two_heap_platform.c', '-Wl,--wrap=malloc',
            '-Wl,--wrap=free', observer, *definitions, '-o', observed]))
        for fail, successes in [('1', 0), ('2', 1), ('0', 2)]:
            r = run(opt + '-guest' + str(successes), [observed], env=dict(os.environ,
                NEWLANG_TEST_FAIL_CALL=fail, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
            ok(r)
            text = r.stdout.decode().strip()
            assert 'T_originals=2 typed_live_fields=YES no_alias=YES calls=2 direct_writes=1' in text
            assert 'all_H_closed_before_T=YES actual_caller_returns=YES' in text
            assert f'allocations={successes} frees={successes} trials={2 if successes else 1} live_tail={int(successes == 2)} original_H=YES teardown=YES' in text
            print(json.dumps(dict(P1071='physical', source=name, opt=opt, sanitizer=sanitizer,
                                  C_SHA256=sha(emitted.stdout), observations=text)), flush=True)
    consumer = run('current-owner-negative', [temp / 'consumer', source])
    ok(consumer)
    assert consumer.stdout.count(b'P1062 ACTUAL REJECT') == 14 and consumer.stdout.count(b'P1062 H REJECT') == 4
    assert consumer.stdout.count(b'P1062 WORLD REJECT') == 7
    assert b'A_destroyed_unrestored=YES B_current_exact_C_destroy=YES' in consumer.stdout
    assert b'foreign_arm_A_destroyed_unrestored=YES B_exact_C_destroy=YES' in consumer.stdout
    print(consumer.stdout.decode(), end='', flush=True)
    oom = run('reached-OOM', [temp / 'oom', source])
    ok(oom)
    assert b'P1062 OOM SUMMARY reached=' in oom.stdout
    print(oom.stdout.decode(), end='', flush=True)
print('P1071', name, 'current/C17-identical/physical/owner/OOM/no-clobber PASS', flush=True)
