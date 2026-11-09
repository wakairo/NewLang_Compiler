"""Independent C314 source/owned/native oracle; no historical SHA generation."""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess
import sys

compiler, observer, cc, sanitizer, output = sys.argv[1:]
here = Path(__file__).resolve().parent
oracle = json.loads((here / 'oracle.json').read_bytes())
out = Path(output)
out.mkdir()
rows = []


def run(args, label, expected=0):
    args = list(map(str, args))
    env = os.environ.copy()
    env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    result = subprocess.run(args, capture_output=True, env=env, timeout=90)
    (out / (label + '.stdout')).write_bytes(result.stdout)
    (out / (label + '.stderr')).write_bytes(result.stderr)
    rows.append(dict(label=label, argv=args, status=result.returncode))
    (out / 'FIRST-results.json').write_text(json.dumps(rows, indent=2) + '\n')
    assert result.returncode == expected, (label, result.stderr.decode(errors='replace'))
    return result


positive = list(oracle['cases'])[:2]
for name, item in oracle['cases'].items():
    source = here / 'sources' / name
    assert hashlib.sha256(source.read_bytes()).hexdigest() == item['SHA256']
    run([compiler, '--check', source], source.stem + '-CHECK', item['CHECK_target'])
    target = out / (source.stem + '.c')
    target.write_bytes(b'P1260 refusal preserves output\n')
    before = (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns)
    run([compiler, '--emit-c17-to', target, source], source.stem + '-C17', item['C17_target'])
    if item['C17_target']:
        assert before == (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns)

flags = [cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror']
if sanitizer != 'none':
    flags += ['-fsanitize=address,undefined' if sanitizer == 'address' else '-fsanitize=undefined',
              '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
# Execute the unmodified generated guest before installing any observer.
for name in positive:
    stem = Path(name).stem
    for opt in ['O0', 'O2']:
        executable = out / (stem + '-raw-' + opt)
        run(flags + ['-' + opt, out / (stem + '.c'), '-o', executable], executable.name + '-build')
        run([executable], executable.name)

proof = run([observer, *(here / 'sources' / name for name in positive)], 'owned-negative')
assert b'A destroyed unrestored/B survives' in proof.stdout
assert re.search(rb'P1260 REJECT (\d+) observations (\d+)', proof.stdout)
# The first two genuine owned roots have their own source binding tables.
tables = []
for section in proof.stdout.decode().split('SOURCE ')[1:3]:
    tables.append({name: int(symbol) for name, symbol in re.findall(r'^BIND (\w+) (\d+)$', section, re.M)})
assert len(tables) == 2
names = ['p1260_case_0', 'p1260_case_1']
driver = out / 'driver.c'
driver.write_text('#include <stddef.h>\n' + ''.join('void ' + n + '(size_t);\n' for n in names) +
                  'int main(void){for(size_t q=0;q<=2;++q){' + ''.join(n + '(q);' for n in names) + '}return 0;}\n')
for opt in ['O0', 'O2']:
    objects = []
    for i, name in enumerate(positive):
        item = oracle['cases'][name]
        stem = Path(name).stem
        table = tables[i]
        values = {'selected' if not i else 'target': item['loop_new'][-1],
                  'saved_level' if not i else 'saved_target': item['loop_new'][-1],
                  'displaced': item['BREAK_old'], 'audit_copy': item['caller_Copy'],
                  **item['post_result_bindings'],
                  'applied': item['final_T'][item['loop_field']],
                  'history': item['final_T'][1 - item['loop_field']]}
        fields = [item['loop_field']] * len(item['loop_old']) + item['post_fields']
        defines = dict(P314_INDEX=item['loop_field'], P314_LOOP_STORES=len(item['loop_old']),
                       P314_STORES=len(fields), P314_BREAK=item['BREAK_old'])
        arrays = dict(P314_INITIAL=item['initial_T'], P314_FINAL=item['final_T'],
                      P314_FIELDS=fields, P314_OLD=item['loop_old'] + item['post_old'],
                      P314_NEW=item['loop_new'] + item['post_new'],
                      P314_SYMBOLS=[table[k] for k in values], P314_VALUES=list(values.values()))
        text = ''.join('#define ' + k + ' ' + str(v) + '\n' for k, v in defines.items())
        text += ''.join('#define ' + k + ' {' + ','.join(map(str, v)) + '}\n' for k, v in arrays.items())
        text += '#define main ' + names[i] + '_main\n'
        for macro in ['NEWLANG_NODE_OBSERVER', 'NEWLANG_HEAP_OBSERVER', 'NEWLANG_CURRENT_LOOP_OBSERVER']:
            text += '#define ' + macro + ' ' + json.dumps(str(here / 'native_observer.h')) + '\n'
        text += '#include ' + json.dumps(str(out / (stem + '.c'))) + '\n#undef main\n'
        text += 'void ' + names[i] + '(size_t q){p_reset(q);assert(' + names[i] + '_main()==0 && p314.finished);}\n'
        wrapper = out / (stem + '-observer-' + opt + '.c')
        wrapper.write_text(text)
        obj = out / (stem + '-' + opt + '.o')
        run(flags + ['-' + opt, '-c', wrapper, '-o', obj], stem + '-' + opt + '-observer-build')
        objects.append(obj)
    executable = out / ('matrix-' + opt)
    run(flags + ['-' + opt, driver, *objects, '-o', executable], opt + '-observer-link')
    result = run([executable], opt + '-observer-native')
    assert result.stdout.count(b'P1260 live-original') == 6
print('P1260 source/owned/old/current/same-original/post-loan writes/H0-1-2 PASS')
