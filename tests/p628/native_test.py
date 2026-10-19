"""Ordinary CLI source-owned less-than IF -> original Pair; passive reads only."""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys
import tempfile


def main():
    compiler, probe, identity_gate, comparison_gate, rollback_gate, cc, directory, sanitizer = sys.argv[1:]
    root = Path(directory)
    spec = importlib.util.spec_from_file_location('native_helpers', root.parent / 'p517/native_test.py')
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    run, require, execute = helper.run, helper.require, helper.execute
    manifest = json.loads((root / 'frozen.json').read_text())
    records = json.loads((root / 'baseline-freeze.json').read_text())
    for name, digest in records['records'].items():
        require(hashlib.sha256((root / name).read_bytes()).hexdigest() == digest, 'baseline evidence changed')
    with tempfile.TemporaryDirectory(prefix='p628-native-') as temp:
        folder = Path(temp)
        for name, oracle in manifest['cases'].items():
            source = root / name
            require(hashlib.sha256(source.read_bytes()).hexdigest() == oracle['sha256'], 'frozen source changed')
            check, emitted = run([compiler, '--check', str(source)]), run([compiler, str(source)])
            if name in ('wrong_quantity.nl', 'readonly_write.nl'):
                category, code = ('semantic', 'USIZE-LESS-THAN-TYPE') if name == 'wrong_quantity.nl' else ('unsupported', 'LOCAL-LOAN-PROFILE')
                require(all(p.returncode == 3 and not p.stdout and f'error({category})[{code}]' in p.stderr
                            for p in (check, emitted)), 'invalid source accepted or first diagnostic changed')
                rollback = run([rollback_gate, 'reject', str(source)])
                marker = 'rollback=PASS artifact=NULL' if name == 'wrong_quantity.nl' else 'state=UNCHANGED semantic=NOT_ENTERED artifact=NULL'
                require(rollback.returncode == 0 and marker in rollback.stdout,
                        f'refusal rollback failed: {rollback}')
                print(json.dumps(dict(source=name, check=3, c17=3, c_bytes=0, diagnostic=check.stderr.strip(), rollback=rollback.stdout.strip())))
                continue
            if name == 'complex_arm.nl':
                require(check.returncode == 0 and not check.stdout and not check.stderr and emitted.returncode == 4 and
                        not emitted.stdout and 'V1-BACKEND-UNSUPPORTED' in emitted.stderr, 'complex field/IF profile broadened')
                print(json.dumps(dict(source=name, check=0, c17=4, c_bytes=0, native=None)))
                continue
            require(check.returncode == emitted.returncode == 0 and not check.stdout and not check.stderr and not emitted.stderr,
                    f'ordinary CLI admission failed: {check} {emitted}')
            repeat = run([compiler, str(source)])
            require(repeat.returncode == 0 and repeat.stdout == emitted.stdout and not repeat.stderr, 'repeat changed C17')
            public = run([probe, str(source)])
            require(public.returncode == 0 and not public.stderr, f'public owned observation failed: {public}')
            comparison, e = map(json.loads, public.stdout.splitlines())
            require(comparison['operands'] == oracle['operands'] and e['bool'] == oracle['condition'] and e['arms'] == [91, 13] and
                    e['old'] == oracle['old'] and e['sibling'] == oracle['sibling'] and e['joined_unknown'] and e['scope_ended'] and
                    comparison['condition_copy_depth'] == (2 if name == 'pair_alias.nl' else 1), 'source-owned identity/value mismatch')
            text = emitted.stdout
            root_name, chosen = f"nl_local_{e['original_symbol']}", f"nl_local_{e['selected_symbol']}"
            ref = f"nl_local_{e['ref_symbol']}"
            physical = re.search(rf'nl_type_\d+ {root_name} = \{{ .f0 = 27, .f1 = 201, \}};', text)
            require(physical is not None, 'original carrier not constructed by guest')
            staged = list(re.finditer(r'const size_t (nl_less_(left|right)_(\d+)) = (nl_local_(\d+));', text))
            require(len(staged) == 2 and [m[2] for m in staged] == ['left', 'right'] and staged[0][3] == staged[1][3] and
                    [int(m[5]) for m in staged] == e['operand_symbols'], 'source operands not staged in order')
            less = f'({staged[0][1]} < {staged[1][1]})'
            truth = f"const _Bool nl_local_{e['equal_symbol']} = {less};"
            select = f"(nl_local_{e['condition_symbol']} ? 91 : 13)"
            choice = f'const uint8_t {chosen} = {select};'
            pointer = f'uint8_t *const {ref} = &{root_name}.f0;'
            capture = f"const uint8_t nl_replace_old_{e['replace_node']} = *{ref};"
            write = f'*{ref} = {chosen};'
            old = f"const uint8_t nl_local_{e['old_symbol']} = nl_loan_result_{e['scope']};"
            after = f"const uint8_t nl_local_{e['after_symbol']} = {root_name}.f0;"
            sibling = f"const uint8_t nl_local_{e['sibling_symbol']} = {root_name}.f1;"
            points = [physical[0], staged[0][0], staged[1][0], truth, choice, pointer, capture, write, old, after, sibling]
            require(all(text.count(p) == 1 for p in points) and [text.index(p) for p in points] == sorted(text.index(p) for p in points) and
                    text.count(' ? ') == 1 and len(re.findall(r'\*nl_local_\d+ = nl_local_\d+;', text)) == 1,
                    'lost source order / one genuine conditional and original field write')
            require('CHAR_BIT == 8' in text and '_Static_assert(SIZE_MAX ==' in text, 'lost target guards')
            watched = text.replace('#include <stdint.h>', '#include <stdint.h>\n#include <stdlib.h>')
            checks = {
                physical[0]: f'{root_name}.f0 != 27 || {root_name}.f1 != 201',
                truth: f"nl_local_{e['equal_symbol']} != {oracle['condition']}",
                choice: f"{chosen} != {oracle['selected']} || &{root_name} != p628_root",
                pointer: f'{ref} != p628_left || &{root_name} != p628_root',
                capture: f"nl_replace_old_{e['replace_node']} != 27",
                write: f"{root_name}.f0 != {oracle['after']} || {root_name}.f1 != 201 || &{root_name}.f1 != p628_right",
                old: f"nl_local_{e['old_symbol']} != 27",
                after: f"nl_local_{e['after_symbol']} != {oracle['after']}",
                sibling: f"nl_local_{e['sibling_symbol']} != 201 || &{root_name} != p628_root",
            }
            # Observer-only addresses point to the actual already-initialized
            # guest carrier. No extra guest object, ownership or branch seed.
            addresses = f'\n    const void *const p628_root = &{root_name};\n    const uint8_t *const p628_left = &{root_name}.f0;\n    const uint8_t *const p628_right = &{root_name}.f1;'
            for point, test in checks.items():
                watched = watched.replace(point, point + (addresses if point == physical[0] else '') + f'\n    if ({test}) exit(91);', 1)
            for m, value in zip(staged, oracle['operands']):
                watched = watched.replace(m[0], m[0] + f'\n    if ({m[1]} != ((size_t)UINTMAX_C({value}))) exit(91);', 1)
            variants = {
                'wrong_comparison': watched.replace(less, less.replace(' < ', ' > '), 1),
                'wrong_direction': watched.replace(select, select.replace('(nl_local_', '(!nl_local_', 1), 1),
                'wrong_field': watched.replace(pointer, pointer.replace('.f0', '.f1'), 1),
                'wrong_old': watched.replace(capture, capture.replace(f'*{ref}', '99'), 1),
                'wrong_sibling': watched.replace(write, write + f'\n    {root_name}.f1 = 99;', 1),
            }
            native = {label: execute(folder, cc, name+label, code, 0, sanitizer) for label, code in [('plain', text), ('passive', watched)]}
            synthetic = {label: execute(folder, cc, name+label, code, 91, sanitizer) for label, code in variants.items()}
            gates = {}
            for tag, gate, marker in [('identity', identity_gate, 'original=PASS worlds=UNCHANGED driver_allocations=0'),
                                      ('comparison', comparison_gate, 'original=PASS snapshot=UNCHANGED')]:
                fault = run([gate, str(source)])
                require(fault.returncode == 0 and not fault.stderr and marker in fault.stdout, f'{tag} mutated evidence gate failed: {fault}')
                gates[tag] = fault.stdout.strip()
            print(json.dumps(dict(source=name, check=0, c17=0, source_sha256=oracle['sha256'], c17_sha256=hashlib.sha256(text.encode()).hexdigest(),
                                  comparison=comparison, owned=e, native=native, synthetic_native=synthetic, gates=gates, sanitizer=sanitizer)))
        baseline = json.loads((root / 'evidence/baseline.json').read_text())
        for name, digest in manifest['controls'].items():
            source = root.parent.parent / name
            require(hashlib.sha256(source.read_bytes()).hexdigest() == digest, 'old source control changed')
            check, emitted = run([compiler, '--check', str(source)]), run([compiler, str(source)])
            require(check.returncode == emitted.returncode == 0 and not check.stdout and not check.stderr and not emitted.stderr and
                    hashlib.sha256(emitted.stdout.encode()).hexdigest() == baseline[name]['c17']['stdout_sha256'], 'old C17 control changed')
            print(json.dumps(dict(control=name, check=0, c17=0, native=execute(folder, cc, source.stem, emitted.stdout, 0, sanitizer))))
    print('P628 actual original Pair write after source-issued less-than Copy IF; no new authority/public validator')


if __name__ == '__main__':
    main()
