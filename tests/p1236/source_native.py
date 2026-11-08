"""C288 source-issued typed terminal transfer; P-owned exact native oracle."""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess
import sys

compiler, observer, cc, sanitizer, output = sys.argv[1:]
here = Path(__file__).resolve().parent
oracle = json.loads((here / 'oracle-v2.json').read_bytes())
sha = lambda b: hashlib.sha256(b).hexdigest()
out = Path(output)
out.mkdir()  # Preserve every older FIRST, never overwrite an apparatus.
rows = []

def run(args, label, expected=0, env=None):
    p = subprocess.run(list(map(str, args)), capture_output=True, env=env, timeout=90)
    (out / (label + '.stdout')).write_bytes(p.stdout)
    (out / (label + '.stderr')).write_bytes(p.stderr)
    rows.append(dict(argv=list(map(str, args)), label=label, status=p.returncode,
                     stdout_sha256=sha(p.stdout), stderr_sha256=sha(p.stderr)))
    (out / 'FIRST-results.json').write_text(json.dumps(rows, indent=2) + '\n')
    assert p.returncode == expected, (label, p.returncode, p.stderr.decode(errors='replace'))
    return p

cases = oracle['cases']
for name, item in cases.items():
    assert sha((here / 'sources' / name).read_bytes()) == item['sha256'], name
positives = [n for n, v in cases.items() if v['CHECK_expected'] == 0]
proof = run([observer, *(here / 'sources' / n for n in positives)], 'owned-current')
roles = re.findall(rb'ROLES (\d+) (\d+) (\d+) (\d+) (\d+) CALLER (\d+) (\d+) (\d+) (\d+)', proof.stdout)
assert len(roles) >= len(positives)
for name, item in cases.items():
    source = here / 'sources' / name
    wanted = item['CHECK_expected']
    run([compiler, '--check', source], source.stem + '-CHECK', wanted)
    target = out / (source.stem + '.c')
    if wanted:
        target.write_bytes(b'P1236 rejected terminal proof must preserve output\n')
        before = (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns)
    p = run([compiler, '--emit-c17-to', target, source], source.stem + '-C17', wanted)
    assert p.stdout == b''
    if wanted:
        assert before == (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns)

header = here / 'native_observer.h'
names = ['p1236_case_' + str(i) for i in range(len(positives))]
driver = out / 'driver.c'
driver.write_text('#include <stddef.h>\n#include <stdio.h>\n' + ''.join(
    'void ' + n + '(size_t);\n' for n in names) +
    'int main(void){for(size_t quota=0;quota<=2;++quota){' +
    ''.join(n + '(quota);' for n in names) + '}puts("P1236 native matrix PASS");return 0;}\n')
env = os.environ.copy()
env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
for opt in ['O0', 'O2']:
    flags = [cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-' + opt]
    if sanitizer != 'none':
        flags += ['-fsanitize=address,undefined' if sanitizer == 'address' else '-fsanitize=undefined',
                  '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    objects = []
    for i, name in enumerate(positives):
        stem = Path(name).stem
        wrapper = out / (stem + '-' + opt + '.c')
        entry = names[i] + '_main'
        defines = '#define main ' + entry + '\n'
        defines += ''.join('#define ' + selector + ' ' + json.dumps(str(header)) + '\n'
                           for selector in ['NEWLANG_NODE_OBSERVER', 'NEWLANG_HEAP_OBSERVER', 'NEWLANG_CURRENT_LOOP_OBSERVER'])
        defines += '#define P288_PREHEAP ' + str(int(not name.startswith('D-'))) + '\n'
        defines += '#define P288_INDEX ' + str(int(name.startswith('C-'))) + '\n'
        defines += '#define P288_TERMINAL ' + ('157' if name.startswith('E-') else '239') + '\n'
        defines += '#define P288_ROLES {' + ','.join(x.decode() for x in roles[i][:5]) + '}\n'
        defines += '#define P288_CALLER_ROLES {' + ','.join(x.decode() for x in roles[i][5:]) + '}\n'
        wrapper.write_text(defines + '#include ' + json.dumps(str(out / (stem + '.c'))) +
                           '\n#undef main\nvoid ' + names[i] + '(size_t quota){p_reset(quota);int result=' +
                           entry + '();assert(result==0 && p288.finished);}\n')
        obj = out / (stem + '-' + opt + '.o')
        run(flags + ['-c', wrapper, '-o', obj], stem + '-' + opt + '-build')
        objects.append(obj)
    executable = out / ('matrix-' + opt)
    run(flags + [driver, *objects, '-o', executable], opt + '-link')
    p = run([executable], opt + '-native', env=env)
    assert p.stderr == b'' and p.stdout.count(b'live-original') == 3 * len(positives)
print('P1236 source/current/typed old BREAK/C17/native and source refusal/no-clobber PASS; 2 native processes /30 entry calls')
