"""P-owned C222 CLI and passive native observations; no fixture authorities."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile

p = Path(__file__).resolve().parent
compiler, probe, raw_arg, gcc, clang, sanitizer = sys.argv[1:]
assert sanitizer in ['none', 'address', 'undefined']
for name, expected in json.loads((p/'freeze-v2.json').read_bytes())['files'].items():
    assert hashlib.sha256((p/name).read_bytes()).hexdigest() == expected, name
oracle = json.loads((p/'oracle.json').read_bytes())
Path(raw_arg).mkdir(parents=True, exist_ok=True)
raw = Path(tempfile.mkdtemp(prefix='p1133-', dir=raw_arg))
rows = []


def run(command, name):
    command = list(map(str, command))
    result = subprocess.run(command, capture_output=True, timeout=120)
    label = str(len(rows)+1)+'-'+name
    (raw/(label+'.out')).write_bytes(result.stdout)
    (raw/(label+'.err')).write_bytes(result.stderr)
    rows.append(dict(name=name, command=command, status=result.returncode,
                     stdout_sha256=hashlib.sha256(result.stdout).hexdigest(),
                     stderr_sha256=hashlib.sha256(result.stderr).hexdigest()))
    (raw/'observations.json').write_text(json.dumps(rows, indent=2)+'\n')
    print(name, result.returncode, flush=True)
    return result


for case, expected in oracle['cases'].items():
    source = p/'sources'/(case+'.nl')
    result = run([compiler, '--check', source], case+'-check')
    assert result.returncode == 0 and not result.stdout and not result.stderr
    result = run([probe, source], case+'-owned')
    assert result.returncode == 0 and not result.stderr, result.stderr
    assert b'Unknown quantity has no public known-number receipt PASS' in result.stdout
    assert b'public Copy GET/current0' in result.stdout and b'B survives PASS' in result.stdout
    result = run([compiler, source], case+'-emit')
    assert result.returncode == 0 and not result.stderr
    c = result.stdout.decode()
    assert 'for (;;) {' in c and 'continue;' in c
    assert re.search(r'nl_loop_carry_\d+ = nl_loop_phase_\d+ [+-] ', c)
    named = raw/(case+'-named.c')
    named.write_bytes(b'C222 original named bytes\x00\xff\n')
    result = run([compiler, '--emit-c17-to', named, source], case+'-named')
    assert not result.returncode and not result.stderr and not result.stdout
    assert named.read_bytes() == c.encode()
    # Read-only callbacks next to genuine original stores and the helper call.
    pattern = r'(static uint8_t nl_fn_\d+\(uint8_t \*(nl_local_\d+), uint8_t (nl_local_\d+)\)\n\{\n)(.*?)(\n\})'

    def helper(match):
        body, count = re.subn(r'(^\s*\*(nl_local_\d+) = [^;]+;)$',
                             lambda x: x[1]+'\n    p1133_changed('+x[2]+');',
                             match[4], flags=re.M)
        assert count == 1
        return match[1]+'    p1133_helper('+match[2]+', '+match[3]+');\n'+body+match[5]

    observed, count = re.subn(pattern, helper, c, flags=re.S)
    assert count == 1
    declarations = list(re.finditer(pattern, observed, re.S))
    assert len(declarations) == 1
    start, _ = declarations[0].span()
    prefix, count = re.subn(r'(^\s*\*(nl_local_\d+) = [^;]+;)$',
                           lambda x: x[1]+'\n        p1133_outer_store('+x[2]+');',
                           observed[:start], flags=re.M)
    assert count == 1
    observed = prefix+observed[start:]
    macros = dict(SELECTED=expected['selected'], INDEX=expected['index'],
                  OLD=expected['old'], NEXT=expected['incoming'],
                  F0=expected['entry'][0], F1=expected['entry'][1],
                  ENTRY0=expected['entry'][0], ENTRY1=expected['entry'][1],
                  Q0=expected['ambient'][0], Q1=expected['ambient'][1],
                  CONTINUES=len(expected['trace'])-1, PREFIX=0)
    trace = '{'+','.join('(size_t)UINTMAX_C('+str(x)+')' for x in expected['trace'])+'}'
    for variant, text in [('raw', c), ('observed', observed)]:
        generated = raw/(case+'-'+variant+'.c')
        generated.write_text(text)
        for cc in dict.fromkeys([gcc, clang]):
            for opt in ['-O0', '-O2']:
                label = case+'-'+Path(cc).name+opt+'-'+variant
                executable = raw/label
                flags = ['-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', opt]
                if sanitizer != 'none':
                    flags += ['-fsanitize='+sanitizer, '-fno-sanitize-recover=all',
                              '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
                if variant == 'observed':
                    flags += ['-DNEWLANG_LOOP_OBSERVER="'+str(p/'observe.h')+'"',
                              '-DP1133_TRACE='+trace]
                    flags += ['-DP1133_'+key+'='+str(value) for key, value in macros.items()]
                result = run([cc, *flags, generated, '-o', executable], label+'-build')
                assert not result.returncode, result.stderr
                result = run([executable], label+'-native')
                assert not result.returncode and not result.stderr, result.stderr
                if variant == 'observed':
                    assert b'actual backedge/fresh phase/same original/old/postcurrent/sibling/ambient/caller PASS' in result.stdout
                executable.unlink()
for case in oracle['negative']:
    source = p/'sources'/(case+'.nl')
    result = run([compiler, '--check', source], case+'-check')
    assert result.returncode in [2, 3] and not result.stdout
    named = raw/(case+'.c')
    sentinel = b'C222 rejection named bytes\x00\xff\n'
    named.write_bytes(sentinel)
    before = named.stat()
    result = run([compiler, '--emit-c17-to', named, source], case+'-named')
    assert result.returncode in [2, 3] and not result.stdout
    after = named.stat()
    assert named.read_bytes() == sentinel
    assert (before.st_ino, before.st_mtime_ns) == (after.st_ino, after.st_mtime_ns)
    assert not list(raw.glob('.'+named.name+'*.tmp*'))
for case in ['03-zero-less-control', '04-zero-equality-control']:
    result = run([compiler, p.parent/'p1123/sources'/(case+'.nl')], case+'-unchanged-c17')
    assert not result.returncode and not result.stderr
    assert result.stdout == (p.parent/'p1123/expected-c17'/(case+'.c17')).read_bytes()
print('C222 finite guarded progression cohort PASS; general loop soundness unproved')
