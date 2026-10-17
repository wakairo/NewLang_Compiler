"""P541 actual source/public owned → ordinary C17 → passive original storage.
The observer writes only its own I/O/address records, never guest values."""
import hashlib
import json
from pathlib import Path
import signal
import subprocess
import sys
import tempfile


def run(args):
    return subprocess.run(args, capture_output=True, text=True, check=False)


def require(ok, reason):
    if not ok:
        raise SystemExit(reason)


def observed(text, proof):
    records = proof['records']
    require(len(records) == 2 and proof['record_count'] == 2
            and proof['public_validator'] and proof['worlds_distinct'],
            'no two actual source-issued public records')
    option, array = records[0]['element_type'], records[0]['array_type']
    locals_ = [f'nl_local_{p["symbol"]}' for p in records]
    require(locals_[0] != locals_[1], 'same original symbol')
    require(text.count(f'typedef nl_option_{option} nl_array_{array}[4];') == 1
            and all(text.count(f'nl_array_{array} {s} = {{') == 1 for s in locals_)
            and text.count('{ .tag = NL_ARRAY4_NONE }') == 8
            and 'memset' not in text,
            'missing genuine two-carrier typed construction')
    first, second = locals_
    code = f'''
    const uintptr_t p541_first = (uintptr_t)(const void *)&{first}[0];
    const uintptr_t p541_second = (uintptr_t)(const void *)&{second}[0];
    if ((const void *)&{first} == (const void *)&{second} ||
        p541_first == p541_second ||
        sizeof({first}) != 4 * sizeof(nl_option_{option}) ||
        sizeof({second}) != 4 * sizeof(nl_option_{option}) ||
        !((p541_first + sizeof({first}) <= p541_second) ||
          (p541_second + sizeof({second}) <= p541_first))) abort();
'''
    for index, local in enumerate(locals_):
        base = 'p541_first' if index == 0 else 'p541_second'
        code += f'''
    for (size_t i = 0; i < 4; ++i) {{
        const uintptr_t here = (uintptr_t)(const void *)&{local}[i];
        if (here - {base} != i * sizeof(nl_option_{option}) ||
            {local}[i].tag != NL_ARRAY4_NONE) abort();
        printf("array={index} element=%zu base=%" PRIuPTR
               " address=%" PRIuPTR " stride=%zu tag=%u\\n",
               i, {base}, here, sizeof(nl_option_{option}),
               (unsigned){local}[i].tag);
    }}
'''
    marker = f'    (void){second};'
    require(text.count(marker) == 1, 'no observation point after both live roots')
    return text.replace('#include <stdint.h>',
                        '#include <stdint.h>\n#include <stddef.h>\n'
                        '#include <inttypes.h>\n#include <stdio.h>\n#include <stdlib.h>'
                        ).replace(marker, code + marker)


def execute(root, cc, name, text, good, sanitizer, observation=False):
    c = root / (name + '.c')
    c.write_text(text)
    flags = [] if sanitizer == 'none' else [
        f'-fsanitize={sanitizer}', '-fno-sanitize-recover=all']
    results = []
    for opt in ['-O0', '-O2']:
        exe = root / (name + opt)
        build = run([cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic',
                     '-Werror', opt, *flags, str(c), '-o', str(exe)])
        require(build.returncode == 0, f'{name}/{opt} C build: {build.stderr}')
        result = run([str(exe)])
        require(result.returncode == 0 and not result.stderr if good
                else result.returncode == -signal.SIGABRT,
                f'{name}/{opt}: unexpected native result {result}')
        if good:
            require(len(result.stdout.splitlines()) == 8 if observation
                    else not result.stdout, 'wrong original observation output')
            if observation:
                rows = [dict(item.split('=') for item in line.split())
                        for line in result.stdout.splitlines()]
                require([int(r['array']) for r in rows] == [0]*4 + [1]*4
                        and [int(r['element']) for r in rows] == list(range(4))*2,
                        'missing physical subobjects')
                require(rows[0]['base'] != rows[4]['base'], 'coalesced originals')
                for row in rows:
                    require(int(row['stride']) > 0 and
                            int(row['address']) - int(row['base']) ==
                            int(row['element']) * int(row['stride']),
                            'not actual sizeof(Option) contiguous storage')
        results.append({'opt': opt, 'exit': result.returncode,
                        'stdout': result.stdout,
                        'stderr': result.stderr if good else 'expected observer SIGABRT'})
    return results


def main():
    compiler, regular, probe, gate, cc, directory, sanitizer = sys.argv[1:]
    directory = Path(directory)
    manifest = json.loads((directory / 'frozen.json').read_text())
    for name, case in manifest['cases'].items():
        data = (directory / (name + '.nl')).read_bytes()
        require(hashlib.sha256(data).hexdigest() == case['sha256'],
                'pre-execution frozen source changed')
    for name, case in manifest['cases'].items():
        source = directory / (name + '.nl')
        default = run([regular, '--check', str(source)])
        require(default.returncode == 3 and not default.stdout and
                'P5-EXPRESSION-UNSUPPORTED' in default.stderr,
                'default-OFF isolation lost')
        expected = case['expected']
        if expected['check']:
            for args in [['--check'], []]:
                result = run([compiler, *args, str(source)])
                require(result.returncode == 3 and not result.stdout and
                        expected['diagnostic'] in result.stderr and
                        'error(semantic)' in result.stderr,
                        f'{name}: wrong semantic rejection {result}')
            rejection = run([gate, 'reject', str(source)])
            require(rejection.returncode == 0 and
                    'phase=registration' in rejection.stdout and
                    'rollback=PASS artifact=NULL' in rejection.stdout,
                    f'{name}: no semantic rollback {rejection}')
            print(json.dumps({'case': name, 'parser': 0, 'check': 3,
                              'diagnostic': result.stderr,
                              'rollback': rejection.stdout.strip()}))
            continue
        check = run([compiler, '--check', str(source)])
        emitted = run([compiler, str(source)])
        require(check.returncode == emitted.returncode == 0 and
                not check.stdout and not check.stderr and not emitted.stderr,
                f'{name}: source/C17 failed')
        public = run([probe, str(source)])
        require(public.returncode == 0 and not public.stderr,
                f'{name}: no public owned {public}')
        proof = json.loads(public.stdout)
        require(proof['view_mutants_rejected'] == 100, 'public refusal coverage')
        observed_c = observed(emitted.stdout, proof)
        with tempfile.TemporaryDirectory(prefix='p541-native-') as tmp:
            root = Path(tmp)
            plain = execute(root, cc, 'plain', emitted.stdout, True, sanitizer)
            physical = execute(root, cc, 'observed', observed_c, True, sanitizer, True)
            a, b = proof['records']
            start = observed_c.index(f'    nl_array_{b["array_type"]} nl_local_{b["symbol"]} = {{')
            end = observed_c.index('    };', start) + len('    };')
            bads = {
                'aliased_second_carrier': observed_c[:start] +
                f'    nl_option_{b["element_type"]} *nl_local_{b["symbol"]} = nl_local_{a["symbol"]};' +
                observed_c[end:],
                'wrong_second_None': observed_c[:start] +
                observed_c[start:].replace('{ .tag = NL_ARRAY4_NONE }', '{ .tag = 0 }', 1),
                'wrong_extent': observed_c.replace(f'nl_array_{a["array_type"]}[4]',
                                                   f'nl_array_{a["array_type"]}[5]')}
            refused = {key: execute(root, cc, key, text, False, sanitizer)
                       for key, text in bads.items()}
        print(json.dumps({'case': name, 'source_sha256': case['sha256'],
                          'check': 0, 'c17': 0, 'public_owned': proof,
                          'c17_sha256': hashlib.sha256(emitted.stdout.encode()).hexdigest(),
                          'observed_C_sha256': hashlib.sha256(observed_c.encode()).hexdigest(),
                          'plain_native': plain, 'physical_observed_native': physical,
                          'bad_C_refused': refused}))
    print('P541 CALIBRATED TWO ORIGINAL ARRAY SOURCE/OWNED/C17/NATIVE PASS; no Ring/product claim')


if __name__ == '__main__':
    main()
