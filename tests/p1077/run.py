"""P1077 finite scoped direct-write contract, ordinary CLI and actual carriers."""
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

cli, cc, sanitizer, selection = sys.argv[1:]
cli = Path(cli).resolve()
data = Path(__file__).resolve().parent
repo = data.parents[1]
oracle = json.loads((data/'oracle-v2.json').read_bytes())
sha = lambda b: hashlib.sha256(b).hexdigest()
for manifest in ('source-freeze-v1.json', 'source-freeze-v2.json', 'harness-freeze-v1.json'):
    for path, digest in json.loads((data/manifest).read_bytes())['files'].items():
        assert sha((repo/path).read_bytes()) == digest, path
for row in json.loads((data/'evidence/parent-adapters/relocations-v1.json').read_bytes())['files']:
    assert sha((repo/row['preserved_path']).read_bytes()) == row['sha256']
assert selection in ('0', '1')
profile = oracle['profiles'][int(selection)]
name = profile['name']
source = repo/profile['source']
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
evidence = os.environ.get('P1077_EVIDENCE_DIR')
archive = None
if evidence:
    Path(evidence).mkdir(parents=True, exist_ok=True)
    archive = Path(tempfile.mkdtemp(prefix='P1077-'+name+'-', dir=evidence))
commands = []


def run(label, args, **kwargs):
    args = list(map(str, args))
    r = subprocess.run(args, capture_output=True, timeout=180, **kwargs)
    commands.append(dict(label=label, command=args, exit=r.returncode,
                         stdout_SHA256=sha(r.stdout), stderr_SHA256=sha(r.stderr)))
    if archive:
        (archive/(label+'.stdout')).write_bytes(r.stdout)
        (archive/(label+'.stderr')).write_bytes(r.stderr)
        (archive/'commands-FIRST.json').write_text(json.dumps(commands, indent=2)+'\n')
    assert r.returncode >= 0, ('unexpected signal STOP', label, r.returncode, r.stderr)
    return r


def ok(r):
    assert r.returncode == 0 and not r.stderr, (r.returncode, r.stdout, r.stderr)


def preserved(path, sentinel, before):
    now = path.stat()
    assert path.read_bytes() == sentinel
    assert (now.st_dev, now.st_ino) == (before.st_dev, before.st_ino)
    assert not list(path.parent.glob('*.tmp*'))


def quota():
    signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
    resource.setrlimit(resource.RLIMIT_FSIZE, (128, 128))


flags = ['-std=c17', '-Wall', '-Wextra', '-Werror', '-pedantic-errors', '-fno-pie', '-no-pie']
if sanitizer != 'none':
    flags += ['-fsanitize='+sanitizer, '-fno-omit-frame-pointer', '-fno-sanitize-recover=all']
with tempfile.TemporaryDirectory(prefix='p1077-') as directory:
    temp = Path(directory)
    libraries = [cli.parent/('libnewlang_'+n+'.a') for n in
                 ('semantic', 'parser', 'checked_c_node', 'lexer', 'checked', 'syntax', 'source', 'diagnostic')]
    for tool, path in [('observe', data/'observe-current.c'), ('mutants', data/'batch_mutants.c'),
                       ('consumer', repo/'tests/p1062/consumer.c'), ('oom', repo/'tests/p1062/oom.c')]:
        wraps = ['-Wl,--wrap='+n for n in ('malloc', 'calloc', 'realloc')] if tool == 'oom' else []
        ok(run('build-'+tool, [cc, *flags, '-g', '-O0', '-I'+str(repo/'include'), path,
                              '-Wl,--start-group', *libraries, '-Wl,--end-group', *wraps, '-o', temp/tool]))
    if selection == '0':
        for negative in oracle['negative'].values():
            path = repo/negative['source']
            r = run(path.stem+'-CHECK', [cli, '--check', path])
            assert r.returncode == negative['CHECK'] and not r.stdout
            named = temp/'rejected.c'
            sentinel = b'P1077 preserve complete rejected output\n'
            named.write_bytes(sentinel)
            before = named.stat()
            for mode in ('stdout', 'fresh', 'existing'):
                target = temp/'never.c' if mode == 'fresh' else named
                args = [cli]+([] if mode == 'stdout' else ['--emit-c17-to', target])+[path]
                r = run(path.stem+'-'+mode, args)
                assert r.returncode == negative['C17'] and not r.stdout
                if mode == 'fresh':
                    assert not target.exists()
                preserved(named, sentinel, before)
            print('P1077 SOURCE REJECT', path.name, 'CHECK', negative['CHECK'],
                  'C17', negative['C17'], 'no_clobber=YES', flush=True)
        # Preserve the first source failure as a real rejection; never retrograde its old prediction.
        for first in ('batch_hot', 'batch_cold'):
            r = run(first+'-original-FIRST-control', [cli, '--check', data/'sources'/(first+'.nl')])
            assert r.returncode == 3 and b'P5-DUPLICATE-BINDING' in r.stderr
    ok(run('CHECK', [cli, '--check', source]))
    public = run('current', [temp/'observe', source])
    ok(public)
    rows = [json.loads(s) for s in public.stdout.splitlines() if s.startswith(b'{')]
    calls = [r for r in rows if r['kind'] == 'call']
    directs = [r for r in rows if r['kind'] == 'direct']
    bindings = {r['name']:r for r in rows if r['kind'] == 'binding'}
    assert len(calls) == 2 and len({r['root'] for r in calls}) == 2
    assert all(r['body_current'] == r['parameters_current'] == 0 for r in calls)
    for key in ('old', 'next', 'sibling', 'index'):
        assert [r[key] for r in calls] == profile['helper_'+key]
    assert len(directs) == len(profile['direct_changes']) == 3
    for actual, wanted in zip(directs, profile['direct_changes']):
        assert actual['root'] == calls[wanted['root']]['root']
        assert all(actual[k] == wanted[k] for k in ('index', 'old', 'next'))
        assert actual['old_fact'] != actual['new_fact']
    result_names = ('old_a', 'old_b', 'batch_result', 'final_old')
    assert [bindings[k]['value'] for k in result_names] == profile['copy_results']
    assert rows[-1] == dict(kind='ownership', origin=True, custody=True)
    print(public.stdout.decode(), end='', flush=True)
    emitted = run('C17', [cli, source])
    ok(emitted)
    assert emitted.stdout
    c = temp/'main.c'
    c.write_bytes(emitted.stdout)
    if archive:
        (archive/'main.c.data').write_bytes(emitted.stdout)
    control = run('split-control-C17', [cli, repo/profile['control']])
    ok(control)
    original = json.loads((data/'control-C-parent-v1.json').read_bytes())['split_'+name]
    assert sha(control.stdout) == original['SHA256'] and len(control.stdout) == original['bytes']
    frames = re.findall(rb'NL_MIXED_COPY_BIND\((\d+),\d+,&nl_b_\d+_\d+\.f0', emitted.stdout)
    assert len(frames) == 2 and frames[0] == frames[1]
    values = {prefix+str(i)+str(j): matrix[i][j]
              for prefix, matrix in [('T', profile['initial_fields']), ('F', profile['final_fields'])]
              for i in range(2) for j in range(2)}
    for prefix, key in [('OLD', 'helper_old'), ('NEXT', 'helper_next'), ('SIB', 'helper_sibling'), ('I', 'helper_index')]:
        values.update({prefix+str(i): v for i, v in enumerate(profile[key])})
    for i, change in enumerate(profile['direct_changes']):
        values.update({'D'+str(i)+'_'+key.upper(): value for key, value in change.items()})
    values.update({'H0':profile['H_payload'][0], 'H1':profile['H_payload'][1],
                   'LOCAL_PACKETS':1, 'HEAD_READS':0, 'CALLER_READS':1, 'RECEIVER_READS':0,
                   'RESULT2':profile['copy_results'][2], 'RESULT3':profile['copy_results'][3]})
    values.update({'RET'+str(i): 'nl_b_'+frames[0].decode()+'_'+str(bindings[k]['symbol'])
                   for i,k in enumerate(result_names)})
    definitions = ['-DP1077_'+key+'='+str(value) for key,value in values.items()]
    observer = '-DNEWLANG_HEAP_OBSERVER="'+str(data/'observer.h')+'"'
    named = temp/'named.c'
    ok(run('named-publish', [cli, '--emit-c17-to', named, source]))
    assert named.read_bytes() == emitted.stdout
    sentinel = b'P1077 complete previous output\n'
    named.write_bytes(sentinel)
    before = named.stat()
    r = run('named-output-error', [cli, '--emit-c17-to', named, source], preexec_fn=quota)
    assert r.returncode == 1 and not r.stdout and b'OUTPUT' in r.stderr
    preserved(named, sentinel, before)
    ok(run('named-exact-retry', [cli, '--emit-c17-to', named, source]))
    assert named.read_bytes() == emitted.stdout
    for opt in ('-O0', '-O2'):
        raw = temp/'raw'
        ok(run(opt+'-raw-build', [cc, *flags, opt, c, '-o', raw]))
        r = run(opt+'-raw', [raw])
        ok(r)
        assert not r.stdout
        observed = temp/'observed'
        ok(run(opt+'-physical-build', [cc, *flags, opt, c, repo/'tests/integration/two_heap_platform.c',
               '-Wl,--wrap=malloc', '-Wl,--wrap=free', observer, *definitions, '-o', observed]))
        for fail, successes in [('1',0), ('2',1), ('0',2)]:
            r = run(opt+'-guest'+str(successes), [observed],
                    env=dict(os.environ, NEWLANG_TEST_FAIL_CALL=fail,
                             ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                             UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
            ok(r)
            text = r.stdout.decode().strip()
            assert 'T_originals=2 typed_live_fields=YES no_alias=YES calls=2 direct_writes=3' in text
            assert 'all_H_closed_before_T=YES actual_caller_returns=YES' in text
            assert f'allocations={successes} frees={successes} trials={2 if successes else 1} live_tail={int(successes==2)} original_H=YES teardown=YES' in text
            print(json.dumps(dict(P1077='physical', source=name, opt=opt, sanitizer=sanitizer,
                                  C_SHA256=sha(emitted.stdout), observations=text)), flush=True)
    for label, tool in [('batch-mutants','mutants'), ('current-owner-negative','consumer'), ('reached-OOM','oom')]:
        r = run(label, [temp/tool, source])
        ok(r)
        if tool == 'mutants':
            assert r.stdout.count(b'P1077 BATCH REJECT') == 7
        elif tool == 'consumer':
            assert r.stdout.count(b'P1062 ACTUAL REJECT') == 14
            assert r.stdout.count(b'P1062 H REJECT') == 4
            assert r.stdout.count(b'P1062 WORLD REJECT') == 7
            assert b'A_destroyed_unrestored=YES B_current_exact_C_destroy=YES' in r.stdout
            assert b'foreign_arm_A_destroyed_unrestored=YES B_exact_C_destroy=YES' in r.stdout
        else:
            assert b'P1062 OOM SUMMARY reached=' in r.stdout
        print(r.stdout.decode(), end='', flush=True)
print('P1077', name, 'current/scoped-writes/native/physical/owner/OOM/no-clobber PASS', flush=True)

