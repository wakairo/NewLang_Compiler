"""Finite P-native evidence: genuine CHECK roles, original T and H0/1/2."""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess
import sys

compiler, observer, cc, sanitizer, output = sys.argv[1:]
here = Path(__file__).resolve().parent
out = Path(output)
out.mkdir()
oracle = json.loads((here / 'oracle-v3.json').read_bytes())
summary = []
for name, expected in oracle['positive'].items():
    source = here / 'sources' / name
    stem = source.stem
    raw = out / stem
    raw.mkdir()
    roles = subprocess.run([observer, str(source)], capture_output=True)
    (raw / 'roles.stdout').write_bytes(roles.stdout)
    (raw / 'roles.stderr').write_bytes(roles.stderr)
    assert roles.returncode == 0, roles.stderr.decode()
    found = re.findall(r'ROLES incoming=(\d+) old=(\d+) old_copy=(\d+) current=(\d+) sibling=(\d+) index=(\d+)', roles.stdout.decode())
    no_write = name.startswith('08-')
    assert len(found) == (0 if no_write else 1)
    ids = list(map(int, found[0])) if found else [0, 0, 0, 0, 0, 0]
    generated = raw / 'source.c'
    emitted = subprocess.run([compiler, '--emit-c17-to', str(generated), str(source)], capture_output=True)
    (raw / 'emit.stdout').write_bytes(emitted.stdout)
    (raw / 'emit.stderr').write_bytes(emitted.stderr)
    assert emitted.returncode == 0
    has_heap = name.startswith(('03-', '04-'))
    first, second = (43, 229) if has_heap else (38, 227) if no_write else (52, 197)
    iterations = expected['loop_result']
    defines = dict(P_HAS_HEAP=int(has_heap), P_PRE_HEAP=int(name.startswith('04-')),
                   P_NO_WRITE=int(no_write), P_ITERATIONS=iterations, P_INDEX=ids[5],
                   P_FIRST_VALUE=first, P_SECOND_VALUE=second, P_FINAL_VALUE=expected['final'],
                   P_SIBLING_VALUE=expected['sibling'],
                   P_OLD_VALUES='{' + ','.join(map(str, expected.get('old', [38] * iterations))) + '}',
                   P_NEXT_VALUES='{' + ','.join(map(str, expected.get('incoming', [38] * iterations))) + '}')
    defines.update(zip(['P_ROLE_INCOMING', 'P_ROLE_OLD', 'P_ROLE_COPY', 'P_ROLE_CURRENT', 'P_ROLE_SIBLING'], ids[:5]))
    header = str(here / 'native.h')
    options = ['-O0', '-O2'] if sanitizer == 'none' else ['-O2']
    for optimization in options:
        executable = raw / ('guest-' + optimization[1:])
        argv = [cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', optimization]
        argv += [f'-D{k}={v}' for k, v in defines.items()]
        argv += [f'-D{selector}="{header}"' for selector in
                 ['NEWLANG_NODE_OBSERVER', 'NEWLANG_HEAP_OBSERVER', 'NEWLANG_CURRENT_LOOP_OBSERVER', 'NEWLANG_LOOP_OBSERVER', 'NEWLANG_SCALAR_LOOP_OBSERVER']]
        if sanitizer != 'none':
            argv += ['-fsanitize=address,undefined' if sanitizer == 'address' else '-fsanitize=undefined',
                     '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
        argv += [str(generated), '-o', str(executable)]
        built = subprocess.run(argv, capture_output=True)
        (raw / (optimization[1:] + '.build.stdout')).write_bytes(built.stdout)
        (raw / (optimization[1:] + '.build.stderr')).write_bytes(built.stderr)
        (raw / (optimization[1:] + '.argv.json')).write_text(json.dumps(argv) + '\n')
        assert built.returncode == 0, built.stderr.decode()
        for quota in ([0, 1, 2] if has_heap else [0]):
            env = os.environ.copy()
            env.update(P1217_HEAP_BUDGET=str(quota), ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
            ran = subprocess.run([str(executable)], capture_output=True, env=env, timeout=15)
            prefix = optimization[1:] + '-heap' + str(quota)
            (raw / (prefix + '.stdout')).write_bytes(ran.stdout)
            (raw / (prefix + '.stderr')).write_bytes(ran.stderr)
            row = dict(source=name, optimization=optimization, quota=quota, exit=ran.returncode,
                       generated_sha256=hashlib.sha256(generated.read_bytes()).hexdigest(), sanitizer=sanitizer)
            summary.append(row)
            (out / 'FIRST-results.json').write_text(json.dumps(summary, indent=2) + '\n')
            assert ran.returncode == 0 and ran.stderr == b'', (row, ran.stderr.decode())
            assert b'P1217 original T/' in ran.stdout
print('P1217 strict native original T / H0-1-2 / real Continue-Break PASS', len(summary))
