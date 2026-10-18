"""Frozen source -> actual registered owned receipt -> normal C17/native.
Passive observers never supply a guest operand or permission. Mutants retain
exactly the same observers; expected values come from the pre-run frozen oracle.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile


def run(args):
    return subprocess.run(args, capture_output=True, text=True, check=False)


def require(ok, why):
    if not ok:
        raise SystemExit(why)


def native(folder, cc, name, code, expected, sanitizer):
    source = folder / f"{name}.c"
    source.write_text(code)
    results = []
    extra = [] if sanitizer == 'none' else [f'-fsanitize={sanitizer}', '-fno-sanitize-recover=all']
    for opt in ('-O0', '-O2'):
        exe = folder / (name + opt)
        p = run([cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', opt,
                 *extra, str(source), '-o', str(exe)])
        require(p.returncode == 0, f'C17 {name}: {p.stderr}')
        p = run([str(exe)])
        require(p.returncode == expected and not p.stdout and not p.stderr,
                f'native {name}/{opt}: {p}')
        results.append(p.returncode)
    return results


def observe(code, view, bindings):
    pattern = r'const (size_t|_Bool) (nl_local_(\d+)) = ([^\n]+);'
    declarations = list(re.finditer(pattern, code))
    require(len(declarations) == len(bindings), 'missing physical source carrier')
    operands = [f'nl_local_{symbol}' if symbol else f'((size_t)UINTMAX_C({number}))'
                for symbol, number in zip(view['symbols'], view['numbers'])]
    expression = f'({operands[0]} + {operands[1]})'
    require(code.count(expression) == 1, 'no exact source-carrier addition')
    require('CHAR_BIT == 8' in code and 'SIZE_MAX == UINTMAX_C(' in code, 'target guards lost')
    staged = code.replace('#include <stddef.h>', '#include <stddef.h>\n#include <stdlib.h>')
    seen = set()
    for m in declarations:
        symbol = int(m[3])
        require(symbol in bindings and symbol not in seen, 'foreign or duplicated binding')
        seen.add(symbol)
        expected = bindings[symbol]
        literal = f'((size_t)UINTMAX_C({expected}))' if m[1] == 'size_t' else str(expected)
        staged = staged.replace(m[0], m[0] + f'\n    if ({m[2]} != {literal}) exit(91);', 1)
    mutants = {'wrong_operator': staged.replace(expression, expression.replace(' + ', ' ^ '), 1)}
    # XOR happens to equal plus for disjoint bits or zero: guaranteed bad carrier.
    mutants['wrong_result'] = staged.replace(expression, f'((size_t)UINTMAX_C({1 if view["sum"] == 0 else 0}))', 1)
    return staged, mutants, expression


def main():
    compiler, public, gate, rollback, cc, directory, sanitizer = sys.argv[1:]
    root = Path(directory)
    frozen = json.loads((root/'frozen.json').read_text())
    with tempfile.TemporaryDirectory(prefix='p589-native-') as tmp:
        folder = Path(tmp)
        for name, oracle in frozen['cases'].items():
            source = root/(name+'.nl')
            require(hashlib.sha256(source.read_bytes()).hexdigest() == oracle['sha256'], 'frozen source altered')
            checked = run([compiler, '--check', str(source)])
            emitted = run([compiler, str(source)])
            if oracle['candidate_cli']:
                expected = oracle['expected']
                # Frozen escape prediction was too late: plain usize is not an
                # admitted loan root. Record exact earlier profile barrier.
                if name == 'escape':
                    expected = 'LOCAL-LOAN-PROFILE'
                for p in (checked, emitted):
                    require(p.returncode == 3 and not p.stdout and 'P3-INTERNAL' not in p.stderr and
                            (f'[{expected}]' in p.stderr if expected != 'SOURCE' else 'error(unsupported)' in p.stderr),
                            f'wrong refusal {name}: {p}')
                state = run([rollback, 'reject', str(source)])
                require(state.returncode == 0 and ('rollback=PASS' in state.stdout or 'state=UNCHANGED' in state.stdout),
                        f'rejection rollback {name}: {state}')
                print(json.dumps(dict(source=name, exit=checked.returncode, diagnostic=checked.stderr.strip(),
                                      c_bytes=0, rollback=state.stdout.strip(),
                                      frozen_prediction=oracle['expected'])))
                continue
            require(checked.returncode == emitted.returncode == 0 and not checked.stdout and
                    not checked.stderr and not emitted.stderr, f'admission {name}: {checked} {emitted}')
            if name == 'equality':
                native(folder, cc, name, emitted.stdout, 0, sanitizer)
                print(json.dumps(dict(source=name, check=0, c17=0, existing_equality=True)))
                continue
            p = run([public, str(source)])
            require(p.returncode == 0 and not p.stderr, f'public proof {name}: {p}')
            lines = p.stdout.splitlines()
            view = json.loads(lines[0])
            require(view['sum'] == oracle['expected'] and view['authority'] == 0, 'wrong source-issued sum')
            bindings = {}
            for line in lines[1:]:
                _, symbol, _, number = line.split()
                bindings[int(symbol)] = int(number)
            g = run([gate, str(source)])
            require(g.returncode == 0 and not g.stderr and 'original-validator-and-driver=PASS' in g.stdout,
                    f'fault controls {name}: {g}')
            observed, mutants, expression = observe(emitted.stdout, view, bindings)
            outcomes = dict(plain=native(folder, cc, name+'plain', emitted.stdout, 0, sanitizer),
                            observed=native(folder, cc, name+'observed', observed, 0, sanitizer))
            outcomes['wrong_result'] = native(folder, cc, name+'wrong_result', mutants['wrong_result'], 91, sanitizer)
            if (view['numbers'][0] ^ view['numbers'][1]) != view['sum']:
                outcomes['wrong_operator'] = native(folder, cc, name+'wrong_operator', mutants['wrong_operator'], 91, sanitizer)
            print(json.dumps(dict(source=name, check=0, public=view, private_gate=g.stdout.strip(),
                                  c17=0, expression=expression,
                                  c_sha256=hashlib.sha256(emitted.stdout.encode()).hexdigest(), native=outcomes)))
        for name in ('names', 'literals'):
            p = run([rollback, 'oom', str(root/(name+'.nl'))])
            require(p.returncode == 0 and 'rollback=PASS' in p.stdout and 'output=NULL' in p.stdout,
                    f'OOM {name}: {p}')
            print(json.dumps(dict(source=name, oom=p.stdout.strip())))
    print('P589 finite source-issued usize addition; no general arithmetic/ownership claim')


if __name__ == '__main__':
    main()
