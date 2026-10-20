"""Real ordinary source, public readback, and generated physical C carriers."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def run(cmd):
    return subprocess.run(cmd, capture_output=True, text=True, timeout=120)


def require(ok, detail):
    if not ok:
        raise SystemExit(str(detail))


def native(folder, cc, name, text, expected, sanitizer):
    c = folder / (name + '.c')
    c.write_text(text)
    out = []
    for opt in ['-O0', '-O2']:
        exe = folder / (name + opt)
        flags = [] if sanitizer == 'none' else [f'-fsanitize={sanitizer}', '-fno-sanitize-recover=all']
        built = run([cc, '-std=c17', '-pedantic-errors', '-Wall', '-Wextra', '-Werror', opt, *flags, str(c), '-o', str(exe)])
        require(built.returncode == 0, built.stderr)
        result = run([str(exe)])
        require(result.returncode == expected and not result.stdout and not result.stderr, result)
        out.append(result.returncode)
    return out


def observe(text, data, oracle):
    watched = text.replace('#include <stdint.h>', '#include <stdint.h>\n#include <stdlib.h>', 1)
    roots = data['roots']
    state = [list(x) for x in oracle['initial_pairs']]
    require(len(re.findall(r'\*nl_local_\d+ = nl_local_\d+;', text)) == 2, 'not two actual writes')
    require(f"if (nl_local_{data['condition_symbol']})" in text, 'actual branch missing')
    for k, row in enumerate(roots):
        var = f"nl_local_{row['symbol']}"
        pattern = rf'nl_type_\d+ {var} = \{{ .f0 = {state[k][0]}, .f1 = {state[k][1]}, \}};'
        ctor = re.search(pattern, text)
        require(ctor is not None, 'original constructor missing')
        addresses = (f'\n    const void *const p651_root_{k} = &{var};'
                     f'\n    const volatile uint8_t *const p651_left_{k} = &{var}.f0;'
                     f'\n    const volatile uint8_t *const p651_right_{k} = &{var}.f1;')
        if k == 1:
            addresses += f'\n    if (p651_root_0 == &{var}) exit(91);'
        watched = watched.replace(ctor[0], ctor[0] + addresses, 1)
    def check_state():
        conditions = []
        for k, row in enumerate(roots):
            var = f"nl_local_{row['symbol']}"
            conditions += [f'&{var} != p651_root_{k}', f'&{var}.f0 != p651_left_{k}',
                           f'&{var}.f1 != p651_right_{k}', f'{var}.f0 != {state[k][0]}',
                           f'{var}.f1 != {state[k][1]}', f'*p651_left_{k} != {state[k][0]}', f'*p651_right_{k} != {state[k][1]}']
        return ' || '.join(conditions)
    first_bad = None
    for k, row in enumerate(roots):
        var = f"nl_local_{row['symbol']}"
        ref = f"nl_local_{row['ref_symbol']}"
        pointer = f'uint8_t *const {ref} = &{var}.f0;'
        old = re.search(rf'const uint8_t (nl_replace_old_\d+) = \*{ref};', text)
        write = re.search(rf'\*{ref} = (nl_local_\d+);', text)
        require(pointer in text and old and write, 'source write path absent')
        watched = watched.replace(pointer, pointer + f'\n        if ({ref} != p651_left_{k}) exit(91);', 1)
        old_condition = f"{old[1]} != {oracle['old_copy_values'][k]}"
        watched = watched.replace(old[0], old[0] + f'\n        if ({old_condition} || {check_state()}) exit(91);', 1)
        first_bad = first_bad or old_condition
        state[k][0] = oracle['final_pairs'][k][0]
        watched = watched.replace(write[0], write[0] + f'\n        if ({check_state()}) exit(91);', 1)
        forwarding = f"const uint8_t nl_local_{row['old_symbol']} = nl_loan_result_{row['scope']};"
        require(forwarding in text, 'lost original old result')
        watched = watched.replace(forwarding, forwarding + f"\n    if (nl_local_{row['old_symbol']} != {oracle['old_copy_values'][k]}) exit(91);", 1)
        for j, symbol in enumerate(row['read_symbols']):
            read = f'const uint8_t nl_local_{symbol} = {var}.f{j};'
            require(read in text, 'read is not the original carrier')
            watched = watched.replace(read, read + f"\n    if (nl_local_{symbol} != {oracle['final_pairs'][k][j]}) exit(91);", 1)
    require('CHAR_BIT == 8' in text and '_Static_assert(SIZE_MAX ==' in text, 'target guards lost')
    poisoned = watched.replace(first_bad, first_bad + ' + 1', 1)
    return watched, poisoned


def main():
    compiler, gate, rollback, cc, directory, sanitizer = sys.argv[1:]
    root = Path(directory)
    manifest = json.loads((root / 'freeze.json').read_text())
    for name, digest in manifest['source_sha256'].items():
        require(hashlib.sha256((root/name).read_bytes()).hexdigest() == digest, 'source freeze changed')
    with tempfile.TemporaryDirectory(prefix='p651-native-') as tmp:
        folder = Path(tmp)
        for name, key in [('one_if_control.nl', 'one_if_control'), ('false_variant.nl', 'false_variant')]:
            oracle = manifest[key]
            source = str(root/name)
            checked, emitted = run([compiler, '--check', source]), run([compiler, source])
            require(checked.returncode == emitted.returncode == 0 and not checked.stdout and
                    not checked.stderr and not emitted.stderr and bool(emitted.stdout), (checked, emitted))
            again = run([compiler, source])
            require(again.returncode == 0 and again.stdout == emitted.stdout, 'C17 nondeterministic')
            proof = run([gate, source])
            require(proof.returncode == 0 and not proof.stderr, proof)
            data, guards = [json.loads(x) for x in proof.stdout.splitlines()]
            require(data['truth'] == oracle['truth'] and data['arms'] == oracle.get('arms', oracle.get('arm_values')),
                    'source arm observations differ')
            require([r['initial'] for r in data['roots']] == oracle['initial_pairs'] and
                    guards['mutants_rejected'] == 15 and guards['worlds_unchanged'] and
                    guards['driver_allocations'] == 0, 'authenticity controls failed')
            passive, poisoned = observe(emitted.stdout, data, oracle)
            outcomes = {label: native(folder, cc, name+label, code, 0, sanitizer)
                        for label, code in [('plain', emitted.stdout), ('passive', passive)]}
            synthetic = native(folder, cc, name+'wrong_observer', poisoned, 91, sanitizer)
            print(json.dumps({'source': name, 'sha256': manifest['source_sha256'][name],
                              'check': 0, 'c17': 0, 'c17_sha256': hashlib.sha256(emitted.stdout.encode()).hexdigest(),
                              'public_readback': data, 'private_mutants': guards,
                              'native': outcomes, 'synthetic_wrong_observer': synthetic, 'sanitizer': sanitizer}))
        for name, code in [('two_if_unsupported.nl', 4), ('extra_arm_local.nl', 4), ('wrong_quantity.nl', 3)]:
            source = str(root/name)
            checked, emitted = run([compiler, '--check', source]), run([compiler, source])
            diagnostic = 'USIZE-LESS-THAN-TYPE' if code == 3 else 'V1-BACKEND-UNSUPPORTED'
            require(checked.returncode == (3 if code == 3 else 0) and emitted.returncode == code and
                    not emitted.stdout and diagnostic in emitted.stderr, (checked, emitted))
            extra = {}
            if code == 3:
                r = run([rollback, 'reject', source])
                require(r.returncode == 0 and 'rollback=PASS artifact=NULL' in r.stdout, r)
                extra['rollback'] = r.stdout.strip()
            print(json.dumps({'source': name, 'check': checked.returncode, 'c17': code,
                              'c_bytes': 0, 'diagnostic': emitted.stderr.strip(), **extra}))


if __name__ == '__main__':
    main()
