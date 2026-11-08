"""P-owned runtime terminal-u8 transfer; actual source/owner/native oracle."""
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
out.mkdir()  # A fresh apparatus never overwrites an older FIRST.
rows = []


def run(args, label, expected=0, env=None):
    args = list(map(str, args))
    p = subprocess.run(args, capture_output=True, env=env, timeout=90)
    (out / (label + '.stdout')).write_bytes(p.stdout)
    (out / (label + '.stderr')).write_bytes(p.stderr)
    rows.append(dict(argv=args, label=label, status=p.returncode,
                     stdout_sha256=sha(p.stdout), stderr_sha256=sha(p.stderr)))
    (out / 'FIRST-results.json').write_text(json.dumps(rows, indent=2) + '\n')
    assert p.returncode == expected, (label, p.returncode, p.stderr.decode(errors='replace'))
    return p


cases = oracle['cases']
for name, item in cases.items():
    assert sha((here / 'sources' / name).read_bytes()) == item['sha256'], name
positives = ['A-precomputed-independent-byte.nl', 'B-other-field-other-policy-arm.nl',
             'C-same-precomputation-literal-terminal.nl', 'L-adopted-literal-calibration.nl',
             'D-zero-Continue-entry-at-bound.nl', 'E-one-Continue.nl',
             'F-equal-bits-distinct-package.nl', 'G-Copy-quantity-argument.nl']
proof = run([observer, *(here / 'sources' / n for n in positives)], 'owned-current-negative')
roles = re.findall(rb'ROLES (\d+) (\d+) (\d+) (\d+) (\d+) CALLER (\d+) (\d+) (\d+) (\d+)', proof.stdout)
assert len(roles) >= len(positives)
assert b'NEGATIVE 17 genuine' in proof.stdout
prefixes = re.findall(rb'PREFIX (\d+) (\d+)', proof.stdout)
assert len(prefixes) >= len(positives)
controls = re.search(rb'P1253 PREFIX NEGATIVE (\d+) PASS', proof.stdout)
assert controls and int(controls[1]) >= 30
for name, item in cases.items():
    source = here / 'sources' / name
    wanted = item['CHECK_expected']
    run([compiler, '--check', source], source.stem + '-CHECK', wanted)
    target = out / (source.stem + '.c')
    if wanted:
        target.write_bytes(b'P1253 refusal must preserve named output\n')
        before = (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns)
    p = run([compiler, '--emit-c17-to', target, source], source.stem + '-C17', item['C17_expected'])
    assert p.stdout == b''
    if wanted:
        assert before == (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns)

assert sha((out / 'L-adopted-literal-calibration.c').read_bytes()) == 'f58bd932a1e39895aabd28cc33c2249023ac815a2dda476f16f2e1813ba56ab0'
# Inspect the emitted selected-original load/store shape before native use.
for name in positives:
    generated = (out / (Path(name).stem + '.c')).read_text()
    assert generated.count('*nl_current_write;') == 2
    assert generated.count('*nl_current_write=') == 2
    assert generated.count('NL_MIXED_COPY_STORE(nl_current_write,') == 2
    loads = list(re.finditer(r'nl_v_\d+=\*nl_current_write;', generated))
    stores = list(re.finditer(r'\*nl_current_write=nl_v_\d+;', generated))
    assert len(loads) == len(stores) == 2
    assert all(load.end() < store.start() for load, store in zip(loads, stores))
header = here / 'native_observer.h'
names = ['p1253_case_' + str(i) for i in range(len(positives))]
driver = out / 'driver.c'
driver.write_text('#include <stddef.h>\n#include <stdio.h>\n' + ''.join(
    'void ' + n + '(size_t);\n' for n in names) +
    'int main(void){for(size_t quota=0;quota<=2;++quota){' +
    ''.join(n + '(quota);' for n in names) + '}puts("P1253 native matrix PASS");return 0;}\n')
env = os.environ.copy()
env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
for opt in ['O0', 'O2']:
    flags = [cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-' + opt]
    if sanitizer != 'none':
        flags += ['-fsanitize=address,undefined' if sanitizer == 'address' else '-fsanitize=undefined',
                  '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    objects = []
    for i, name in enumerate(positives):
        item = cases[name]
        stem = Path(name).stem
        wrapper = out / (stem + '-' + opt + '.c')
        entry = names[i] + '_main'
        defines = '#define main ' + entry + '\n'
        defines += ''.join('#define ' + selector + ' ' + json.dumps(str(header)) + '\n'
                           for selector in ['NEWLANG_NODE_OBSERVER', 'NEWLANG_HEAP_OBSERVER', 'NEWLANG_CURRENT_LOOP_OBSERVER'])
        values = dict(P299_PREHEAP=1, P299_INDEX=item['field_index'],
                      P299_LIMIT=item['phase_limit'], P299_TERMINAL=item['postcurrent'],
                      P299_OLD=item['BREAK_old'], P299_INITIAL=item['initial_phase'],
                      P299_STORES=len(item['store_old']), P299_HAS_PREFIX=int(name != 'L-adopted-literal-calibration.nl'),
                      P299_POLICY=167 if name.startswith('F-') else (229 if name.startswith(('B-','E-','G-')) else 43))
        defines += ''.join('#define ' + k + ' ' + str(v) + '\n' for k, v in values.items())
        for key, value in [('P299_OLD_TRACE', item['store_old']), ('P299_NEW_TRACE', item['store_new'])]:
            defines += '#define ' + key + ' {' + ','.join(map(str, value)) + '}\n'
        defines += '#define P299_ROLES {' + ','.join(x.decode() for x in roles[i][:5]) + '}\n'
        defines += '#define P299_CALLER_ROLES {' + ','.join(x.decode() for x in roles[i][5:]) + '}\n'
        defines += '#define P299_PREFIX_ROLES {' + ','.join(x.decode() for x in prefixes[i]) + '}\n'
        wrapper.write_text(defines + '#include ' + json.dumps(str(out / (stem + '.c'))) +
                           '\n#undef main\nvoid ' + names[i] + '(size_t quota){p_reset(quota);int result=' +
                           entry + '();assert(result==0 && p299.finished);}\n')
        obj = out / (stem + '-' + opt + '.o')
        run(flags + ['-c', wrapper, '-o', obj], stem + '-' + opt + '-build')
        objects.append(obj)
    executable = out / ('matrix-' + opt)
    run(flags + [driver, *objects, '-o', executable], opt + '-link')
    p = run([executable], opt + '-native', env=env)
    assert p.stderr == b'' and p.stdout.count(b'live-original') == 3 * len(positives)
print('P1253 source/owned/current/pre-loop Copy/actual old BREAK/native H0-1-2, host refusal/no-clobber and literal C17 equality PASS; 2 native processes/48 entries')
