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
    expression = f'({operands[0]} - {operands[1]})'
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
    mutants = {'wrong_operator': staged.replace(expression, expression.replace(' - ', ' + '), 1)}
    # Each control retains the original read-only passive observer.
    mutants['wrong_result'] = staged.replace(expression, f'((size_t)UINTMAX_C({1 if view["difference"] == 0 else 0}))', 1)
    mutants['swapped_operands'] = staged.replace(expression, f'({operands[1]} - {operands[0]})', 1)
    return staged, mutants, expression


def main():
    compiler, public, gate, rollback, cc, directory, sanitizer = sys.argv[1:]
    root = Path(directory)
    frozen = json.loads((root/'frozen.json').read_text())
    for name,digest in json.loads((root/'historical-hashes.json').read_text()).items():
        require(hashlib.sha256((root.parents[1]/name).read_bytes()).hexdigest()==digest,
                'historical plus evidence/API header changed: '+name)
    for old_c in (root.parent/'p589'/'evidence').glob('*.c17.txt'):
        old_source=root.parent/'p589'/(old_c.name.removesuffix('.c17.txt')+'.nl')
        p=run([compiler,str(old_source)])
        require(p.returncode==0 and p.stdout==old_c.read_text(), 'old plus/equality C17 changed')
    with tempfile.TemporaryDirectory(prefix='p596-native-') as tmp:
        folder = Path(tmp)
        for name, oracle in frozen['cases'].items():
            source = root/(name+'.nl')
            require(hashlib.sha256(source.read_bytes()).hexdigest() == oracle['sha256'], 'frozen source altered')
            checked = run([compiler, '--check', str(source)])
            emitted = run([compiler, str(source)])
            if oracle['candidate_cli']:
                expected = oracle['expected']
                for p in (checked, emitted):
                    require(p.returncode == 3 and not p.stdout and 'P3-INTERNAL' not in p.stderr and
                            (f'[{expected}]' in p.stderr if expected != 'SOURCE' else ('error(unsupported)' in p.stderr or 'error(syntax)' in p.stderr)),
                            f'wrong refusal {name}: {p}')
                state = run([rollback, 'reject', str(source)])
                require(state.returncode == 0 and ('rollback=PASS' in state.stdout or 'state=UNCHANGED' in state.stdout),
                        f'rejection rollback {name}: {state}')
                print(json.dumps(dict(source=name, exit=checked.returncode, diagnostic=checked.stderr.strip(),
                                      c_bytes=0, rollback=state.stdout.strip(),
                                      frozen_prediction=oracle['expected'])))
                continue
            if name == 'function_arrow':
                # P685 advances only this zero-formal u8-call boundary. The
                # original source/hash, frozen oracle and old logs are unchanged.
                require(checked.returncode == emitted.returncode == 0 and not checked.stdout and
                        not checked.stderr and not emitted.stderr, 'arrow call admission failed')
                outcome = native(folder, cc, name, emitted.stdout, 0, sanitizer)
                print(json.dumps(dict(source=name, check=0, c17=0, native=outcome,
                                      calibration='P685 ordinary zero-formal call; unit guest exit0, not numeric output')))
                continue
            require(checked.returncode == emitted.returncode == 0 and not checked.stdout and
                    not checked.stderr and not emitted.stderr, f'admission {name}: {checked} {emitted}')
            if name in ('equality','plus'):
                native(folder, cc, name, emitted.stdout, 0, sanitizer)
                print(json.dumps(dict(source=name, check=0, c17=0, existing_control=name)))
                continue
            p = run([public, str(source)])
            require(p.returncode == 0 and not p.stderr, f'public proof {name}: {p}')
            lines = p.stdout.splitlines()
            view = json.loads(lines[0])
            require(view['difference'] == oracle['expected'] and view['authority'] == 0, 'wrong source-issued difference')
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
            if view['numbers'][0] + view['numbers'][1] != view['difference']:
                outcomes['wrong_operator'] = native(folder, cc, name+'wrong_operator', mutants['wrong_operator'], 91, sanitizer)
            if view['numbers'][0] != view['numbers'][1]:
                outcomes['swapped_operands'] = native(folder, cc, name+'swapped', mutants['swapped_operands'], 91, sanitizer)
            print(json.dumps(dict(source=name, check=0, public=view, private_gate=g.stdout.strip(),
                                  c17=0, expression=expression,
                                  c_sha256=hashlib.sha256(emitted.stdout.encode()).hexdigest(), native=outcomes)))
        supplemental=json.loads((root/'supplemental.json').read_text())
        source=root/supplemental['source']
        require(hashlib.sha256(source.read_bytes()).hexdigest()==supplemental['sha256'], 'supplemental source changed')
        checked=run([compiler,'--check',str(source)])
        emitted=run([compiler,str(source)])
        require(checked.returncode==emitted.returncode==0 and not checked.stdout and not checked.stderr and not emitted.stderr, 'shared sequence source failed')
        pattern=r'const size_t (nl_local_\d+) = ([^\n]+);'
        decls=list(re.finditer(pattern,emitted.stdout))
        require(len(decls)==3 and ['+' in decls[0][2],'-' in decls[1][2],'+' in decls[2][2]]==[True]*3,'no real shared-list arithmetic carriers')
        observed=emitted.stdout.replace('#include <stddef.h>','#include <stddef.h>\n#include <stdlib.h>')
        for m,value in zip(decls,supplemental['values']):
            observed=observed.replace(m[0],m[0]+f'\n    if ({m[1]} != ((size_t)UINTMAX_C({value}))) exit(91);',1)
        outcome=native(folder,cc,'shared-sequence',observed,0,sanitizer)
        wrong=observed.replace(decls[1][0],decls[1][0].replace(' - ',' + '),1)
        refused=native(folder,cc,'shared-sequence-wrong',wrong,91,sanitizer)
        p=run([rollback,'oom',str(source)])
        require(p.returncode==0 and 'rollback=PASS' in p.stdout and 'output=NULL' in p.stdout,'shared-list OOM failed')
        print(json.dumps(dict(source=source.name,classification=supplemental['classification'],values=supplemental['values'],native=outcome,wrong_operator=refused,oom=p.stdout.strip())))
        for name in ('names', 'literals'):
            p = run([rollback, 'oom', str(root/(name+'.nl'))])
            require(p.returncode == 0 and 'rollback=PASS' in p.stdout and 'output=NULL' in p.stdout,
                    f'OOM {name}: {p}')
            print(json.dumps(dict(source=name, oom=p.stdout.strip())))
    print('P596 finite source-issued usize subtraction; no general arithmetic/ownership claim')


if __name__ == '__main__':
    main()
