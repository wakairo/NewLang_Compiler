"""P-owned source/current and live original-address native evidence, C280 Phase1."""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess
import sys

compiler, observer, cc, sanitizer, output = sys.argv[1:]
here = Path(__file__).resolve().parent
oracle = json.loads((here / 'source-oracle-freeze-v2.json').read_bytes())
sha = lambda data: hashlib.sha256(data).hexdigest()
out = Path(output)
out.mkdir()  # Fresh apparatus; never overwrite a FIRST.
rows = []

def run(argv, label, expected=0, env=None):
    p = subprocess.run(list(map(str, argv)), capture_output=True, env=env, timeout=60)
    (out / (label + '.stdout')).write_bytes(p.stdout)
    (out / (label + '.stderr')).write_bytes(p.stderr)
    rows.append(dict(argv=list(map(str, argv)), label=label, status=p.returncode,
                     stdout_sha256=sha(p.stdout), stderr_sha256=sha(p.stderr)))
    (out / 'FIRST-results.json').write_text(json.dumps(rows, indent=2) + '\n')
    assert p.returncode == expected, (label, p.returncode, p.stderr.decode(errors='replace'))
    return p

cases = dict(oracle['positive'], **oracle['negative_semantic_eligible'])
for name, expected in cases.items():
    assert sha((here / 'sources' / name).read_bytes()) == expected['sha256'], name
positives = [here / 'sources' / n for n in oracle['positive']]
all_sources = [here / 'sources' / n for n in cases]
owned = run([observer, *all_sources], 'owned-current')
assert owned.stdout.count(b'OWNED source_custody=1 has_H=1 H_origin=1') == len(cases)
assert owned.stdout.count(b'REJECT unissued_owner=') == len(cases)
roles = re.findall(rb'ROLES incoming=(\d+) old=(\d+) old_copy=(\d+) current=(\d+) sibling=(\d+) index=(\d+)', owned.stdout)
assert len(roles) == len(cases) and all(r[5] == b'0' for r in roles)
for source in positives:
    run([compiler, '--check', source], source.stem + '-CHECK')
    target = out / (source.stem + '.c')
    run([compiler, '--emit-c17-to', target, source], source.stem + '-C17')
for name in oracle['negative_semantic_eligible']:
    source = here / 'sources' / name
    run([compiler, '--check', source], source.stem + '-CHECK')
    target = out / (source.stem + '.c')
    target.write_bytes(b'C283 unproved normal-result output must stay unchanged\n')
    before = (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns)
    p = run([compiler, '--emit-c17-to', target, source], source.stem + '-C17', 4)
    assert p.stdout == b'' and b'V1-BACKEND-UNSUPPORTED' in p.stderr
    assert before == (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns)

header = here.parent / 'native_observer.h'
driver = out / 'driver.c'
names = ['p1227_case_' + str(i) for i in range(len(positives))]
driver.write_text('#include <stddef.h>\n#include <stdio.h>\n' + ''.join(
    'void ' + n + '(size_t);\n' for n in names) +
    'int main(void){for(size_t h=0;h<=2;++h){' + ''.join(n + '(h);' for n in names) +
    '}puts("P1227 C283 true alias chain / H0-1-2 / original-current-old-sibling PASS");return 0;}\n')
env = os.environ.copy()
env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
for opt in ['O0', 'O2']:
    flags = [cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-' + opt]
    if sanitizer != 'none':
        flags += ['-fsanitize=address,undefined' if sanitizer == 'address' else '-fsanitize=undefined',
                  '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    objects = []
    for i, source in enumerate(positives):
        wrapper = out / (source.stem + '-' + opt + '.c')
        entry = names[i] + '_original_main'
        defines = '#define main ' + entry + '\n'
        defines += ''.join('#define ' + selector + ' ' + json.dumps(str(header)) + '\n' for selector in [
            'NEWLANG_NODE_OBSERVER', 'NEWLANG_HEAP_OBSERVER', 'NEWLANG_CURRENT_LOOP_OBSERVER'])
        defines += '#define P280_PREHEAP ' + str(int('preH' in source.name)) + '\n'
        defines += '#define P280_ROLES {' + ','.join(x.decode() for x in roles[i][:5]) + '}\n'
        wrapper.write_text(defines + '#include ' + json.dumps(str(out / (source.stem + '.c'))) + '\n#undef main\nvoid ' + names[i] + '(size_t quota){p_reset(quota);int r=' + entry + '();assert(r==0 && p280.finished);}\n')
        obj = out / (source.stem + '-' + opt + '.o')
        run(flags + ['-c', wrapper, '-o', obj], source.stem + '-' + opt + '-build')
        objects.append(obj)
    executable = out / ('matrix-' + opt)
    run(flags + [driver, *objects, '-o', executable], opt + '-link')
    p = run([executable], opt + '-native', env=env)
    assert p.stderr == b'' and p.stdout.count(b'live-original') == 3 * len(positives)
print('C283 actual normal-tail alias current/C17/native and distinct/equal-bit/final-RETURN refusal PASS; 2 native processes / 12 generated entry calls')
