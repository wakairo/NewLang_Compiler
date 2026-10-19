"""Ordinary CLI Copy '<' -> pure IF: real guest reads, no value injection."""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys
import tempfile


def main():
    compiler, probe, if_gate, less_gate, cc, directory, sanitizer = sys.argv[1:]
    root = Path(directory)
    spec = importlib.util.spec_from_file_location('pure_if_native', root.parent / 'p508/native_test.py')
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    run, require, execute = helper.run, helper.require, helper.execute
    frozen = json.loads((root / 'frozen.json').read_text())
    baseline_freeze = json.loads((root / 'baseline-freeze.json').read_text())
    for name, digest in baseline_freeze['records'].items():
        require(hashlib.sha256((root / name).read_bytes()).hexdigest() == digest, 'baseline record changed')
    with tempfile.TemporaryDirectory(prefix='p626-native-') as temporary:
        folder = Path(temporary)
        for name, oracle in frozen['cases'].items():
            source = root / name
            digest = hashlib.sha256(source.read_bytes()).hexdigest()
            require(digest == oracle['sha256'], 'original frozen source changed')
            check = run([compiler, '--check', str(source)])
            emitted = run([compiler, str(source)])
            if oracle['intent'] == 'semantic_reject':
                for result in (check, emitted):
                    require(result.returncode == 3 and not result.stdout and
                            'error(semantic)[USIZE-LESS-THAN-TYPE]' in result.stderr,
                            'type-invalid input misclassified or emitted C')
                print(json.dumps(dict(source=name, check=3, c17=3, c_bytes=0, diagnostic=check.stderr.strip())))
                continue
            if name == 'complex_arm.nl':
                # Issue633 migrates only this unchanged source's old CLI4 oracle.
                spec = importlib.util.spec_from_file_location('p633_arm_local', root.parent / 'p633/arm_local.py')
                local = importlib.util.module_from_spec(spec)
                spec.loader.exec_module(local)
                print(json.dumps(local.observe_case(compiler, source, cc, sanitizer,
                                  {'bool': 1, 'selected': 42}, probe=None)))
                continue
            require(check.returncode == emitted.returncode == 0 and not check.stdout and
                    not check.stderr and not emitted.stderr, 'ordinary source admission failed')
            public = run([probe, str(source)])
            require(public.returncode == 0 and not public.stderr, 'actual source-owned observations failed')
            lines = public.stdout.splitlines()
            comparison, evidence = json.loads(lines[0]), json.loads(lines[1])
            require(comparison['operands'] == oracle['operands'] and
                    evidence['bool'] == oracle['condition'] and evidence['arms'] == oracle['branches'] and
                    evidence['joined_unknown'] and evidence['authority'] == 0 and
                    comparison['condition_copy_depth'] == (2 if name == 'fits_alias.nl' else 1),
                    'wrong original condition/arm observations')
            bindings = {}
            for line in lines[2:]:
                tag, symbol, kind, known, value = line.split()
                require(tag == 'BIND', 'unexpected public observation')
                symbol, known, value = int(symbol), int(known), int(value)
                if not known:
                    require(symbol in (evidence['selected_symbol'], evidence['copied_symbol']) and kind == 'u8',
                            'unaccounted unknown scalar')
                    # An oracle in a passive assertion, never a guest initializer.
                    value = oracle['result']
                bindings[symbol] = (kind, value)
            text = emitted.stdout
            pattern = r'const (size_t|_Bool|uint8_t) (nl_local_(\d+)) = ([^\n;]+);'
            declarations = list(re.finditer(pattern, text))
            by_symbol = {int(m[3]): m for m in declarations}
            require(len(declarations) == len(by_symbol) == len(bindings), 'missing/duplicate original physical local')
            for symbol, match in by_symbol.items():
                require(symbol in bindings and match[1] == {'usize':'size_t','bool':'_Bool','u8':'uint8_t'}[bindings[symbol][0]],
                        'wrong source-linked carrier')
            staged = list(re.finditer(r'const size_t (nl_less_(left|right)_(\d+)) = (nl_local_(\d+));', text))
            require(len(staged) == 2 and [m[2] for m in staged] == ['left', 'right'] and
                    staged[0][3] == staged[1][3] and [int(m[5]) for m in staged] == evidence['operand_symbols'],
                    'actual source operands not staged LTR')
            expression = f'({staged[0][1]} < {staged[1][1]})'
            condition = by_symbol[evidence['less_symbol']]
            require(condition[4] == expression and staged[0].end() < staged[1].start() and
                    staged[1].end() < condition.start(), 'guest comparison replaced/reordered')
            selection = f"(nl_local_{evidence['condition_symbol']} ? nl_local_{evidence['arm_symbols'][0]} : nl_local_{evidence['arm_symbols'][1]})"
            chosen, copied = by_symbol[evidence['selected_symbol']], by_symbol[evidence['copied_symbol']]
            require(chosen[4] == selection and text.count(' ? ') == 1 and
                    copied[4] == f"nl_local_{evidence['selected_symbol']}", 'guest IF/Copy replaced with static cached result')
            require('CHAR_BIT == 8' in text and '_Static_assert(SIZE_MAX ==' in text, 'lost strict target guards')
            watched = text.replace('#include <stdint.h>', '#include <stdint.h>\n#include <stdlib.h>')
            watched = re.sub(pattern, lambda m: m[0] + f'\n    if ({m[2]} != {bindings[int(m[3])][1]}) exit(91);', watched)
            for m, value in zip(staged, oracle['operands']):
                watched = watched.replace(m[0], m[0] + f'\n    if ({m[1]} != ((size_t)UINTMAX_C({value}))) exit(91);', 1)
            variants = {
                'wrong_comparison': watched.replace(expression, expression.replace(' < ', ' > '), 1),
                'wrong_direction': watched.replace(selection, selection.replace('(nl_local_', '(!nl_local_', 1), 1),
                'wrong_selected': watched.replace(selection, str(oracle['branches'][oracle['condition']]), 1),
                'wrong_copy': watched.replace(copied[0], copied[0].replace(copied[4], '((uint8_t)99)'), 1),
            }
            statuses = {key: execute(folder, cc, name+key, code, 0, sanitizer)
                        for key, code in [('plain', text), ('passive', watched)]}
            # Synthetic C controls exercise the observer; they are not NewLang
            # rejections and never replace a real positive native run.
            synthetic = {key: execute(folder, cc, name+key, code, 91, sanitizer) for key, code in variants.items()}
            gates = {}
            for tag, args, marker in [('if', [if_gate, 'poison', str(source)], 'original=PASS worlds=UNCHANGED driver_allocations=0'),
                                      ('less', [less_gate, str(source)], 'original=PASS snapshot=UNCHANGED')]:
                fault = run(args)
                require(fault.returncode == 0 and not fault.stderr and marker in fault.stdout, f'{tag} source-issued fault/rollback checks failed: {fault}')
                gates[tag] = fault.stdout.strip()
            print(json.dumps(dict(source=name, source_sha256=digest, check=0, c17=0,
                                  c17_sha256=hashlib.sha256(text.encode()).hexdigest(), public=comparison,
                                  if_public=evidence, bindings=bindings, native=statuses, synthetic_native=synthetic,
                                  gates=gates, sanitizer=sanitizer)))
        # Controls keep their original source and exact C17 output unchanged.
        baseline = json.loads((root / 'evidence/baseline.json').read_text())
        for name, digest in frozen['controls'].items():
            source = root.parent.parent / name
            require(hashlib.sha256(source.read_bytes()).hexdigest() == digest, 'control source changed')
            check, emitted = run([compiler, '--check', str(source)]), run([compiler, str(source)])
            require(check.returncode == emitted.returncode == 0 and not check.stdout and not check.stderr and
                    not emitted.stderr and hashlib.sha256(emitted.stdout.encode()).hexdigest() == baseline[name]['c17']['stdout_sha256'],
                    'equality IF / README Node CLI control changed')
            native = execute(folder, cc, source.stem, emitted.stdout, 0, sanitizer)
            print(json.dumps(dict(control=name, check=0, c17=0, native=native)))
    print('P626 finite ordinary CLI less-than Copy condition -> pure u8 IF; no new public authority or general IF effect')


if __name__ == '__main__':
    main()
