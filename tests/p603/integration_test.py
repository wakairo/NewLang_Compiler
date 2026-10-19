"""Real source-issued bool observations and same-original passive C17 observers.

These public views are observations, not an untrusted-IR proof/authority issuer.
Mutant C is a synthetic negative; only the actual generated guest creates values.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile


def run(args):
    return subprocess.run(args, text=True, capture_output=True, check=False)


def require(ok, message):
    if not ok:
        raise SystemExit(message)


def execute(root, cc, name, text, expected, sanitizer, *, mutant_kind=None):
    require(mutant_kind in (None, "TRUTH_CHANGING", "SAME_TRUTH"),
            "unknown synthetic mutation category")
    if mutant_kind is not None:
        require(expected == (91 if mutant_kind == "TRUTH_CHANGING" else 0),
                "synthetic category/result mismatch")
    source = root / (name + '.c')
    source.write_text(text)
    statuses = []
    extra = [] if sanitizer == 'none' else [f'-fsanitize={sanitizer}', '-fno-sanitize-recover=all']
    for opt in ('-O0', '-O2'):
        exe = root / (name + opt)
        build = run([cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', opt,
                     *extra, str(source), '-o', str(exe)])
        if build.returncode != 0 and mutant_kind is not None:
            statuses.append(dict(category=mutant_kind, optimization=opt,
                                 outcome='MUTANT_COMPILE_REFUSED',
                                 compile_exit=build.returncode,
                                 diagnostic=build.stderr, native_exit=None))
            continue
        # Original positives must strictly compile. Either synthetic math class
        # may be compile-refused, but this supplies no runtime falsifier.
        # Refusing mutated C is neither a CHECK result nor a native observation.
        require(build.returncode == 0, f'strict C17: {build.stderr}')
        result = run([str(exe)])
        require(result.returncode == expected and not result.stdout and not result.stderr,
                f'native {name}/{opt}: {result}')
        if mutant_kind is None:
            statuses.append(result.returncode)
        else:
            statuses.append(dict(category=mutant_kind, optimization=opt,
                                 outcome='NATIVE_REJECTED' if expected == 91 else 'NATIVE_SAME_TRUTH',
                                 compile_exit=0, native_exit=result.returncode))
    return statuses


def require_runtime_falsifiers(witnesses):
    require(all(witnesses.get(family) for family in ('<=', '>', 'copy')),
            'missing non-boundary compiled/runtime91 synthetic family')


def observe(text, evidence, bindings):
    pattern = r'const (size_t|_Bool) (nl_local_(\d+)) = ([^\n]+);'
    declarations = list(re.finditer(pattern, text))
    require(len(declarations) == len(bindings), 'actual source receiver count')
    require(len({int(m[3]) for m in declarations}) == len(bindings), 'duplicate receiver')
    comparisons = []
    for m in declarations:
        require(int(m[3]) in bindings, 'foreign receiver')
        require(m[1] == ('_Bool' if bindings[int(m[3])][0] == 'bool' else 'size_t'),
                'physical type')
        if ' < ' in m[4]:
            comparisons.append(m)
    require(len(comparisons) == 1, 'one real original guest comparison required')
    a, b = evidence['left'], evidence['right']
    operands = [f'nl_local_{s}' if s else f'((size_t)UINTMAX_C({v}))'
                for s, v in zip(evidence['symbols'], (a, b))]
    temporaries = list(re.finditer(
        r'const size_t (nl_less_(left|right)_(\d+)) = ([^\n]+);', text))
    if temporaries:
        require(len(temporaries) == 2 and
                [m[2] for m in temporaries] == ['left', 'right'] and
                temporaries[0][3] == temporaries[1][3] and
                [m[4] for m in temporaries] == operands and
                temporaries[0].end() < temporaries[1].start() and
                temporaries[1].end() < comparisons[0].start(),
                'sequenced original guest operand carriers')
        expression = f'({temporaries[0][1]} < {temporaries[1][1]})'
    else:
        expression = f'({operands[0]} < {operands[1]})'
    require(comparisons[0][4] == expression and
            int(comparisons[0][3]) == evidence['bool_symbol'], 'original typed operands')
    require('_Static_assert(SIZE_MAX == UINTMAX_C(' in text and 'CHAR_BIT == 8' in text,
            'target model guards')
    observed = text.replace('#include <stddef.h>', '#include <stddef.h>\n#include <stdlib.h>')
    observed = re.sub(pattern, lambda m: m[0] +
                      f'\n    if ({m[2]} != {bindings[int(m[3])][1]}) exit(91);', observed)
    for temporary, value in zip(temporaries, (a, b)):
        observed = observed.replace(temporary[0], temporary[0] +
                                    f'\n    if ({temporary[1]} != ((size_t)UINTMAX_C({value}))) exit(91);', 1)
    mutants = {}
    for operator, truth in (('<=', a <= b), ('>', a > b)):
        mutants[operator] = (observed.replace(expression, expression.replace(' < ', f' {operator} '), 1),
                              91 if truth != bool(evidence['bool']) else 0)
    copies = [m for m in declarations if m[1] == '_Bool' and re.fullmatch(r'nl_local_\d+', m[4])]
    require(len(copies) == 1, 'exactly one bool Copy')
    copy = copies[0]
    mutants['copy'] = (observed.replace(copy[0], copy[0].replace(' = ', ' = !', 1), 1), 91)
    return observed, mutants


def main():
    compiler, probe, gate, cc, directory, sanitizer = sys.argv[1:]
    root = Path(directory)
    frozen = json.loads((root / 'frozen.json').read_text())
    candidate = json.loads((root / 'evidence/candidate-initial.json').read_text())
    portable = json.loads((root.parent / 'p609/evidence/generated-index.json').read_text())
    for path, digest in frozen['unchanged_controls'].items():
        require(hashlib.sha256((root.parent.parent / path).read_bytes()).hexdigest() == digest,
                f'historical source changed {path}')
    baseline = json.loads((root / 'evidence/baseline.json').read_text())
    for path in frozen['unchanged_controls']:
        source = root.parent.parent / path
        for mode, args in (('check', ['--check']), ('c17', [])):
            result = run([compiler, *args, str(source)])
            old = baseline['cases'][path][mode]
            require(result.returncode == old['exit'] and
                    hashlib.sha256(result.stdout.encode()).hexdigest() == old['sha256'],
                    f'legacy status/C-byte regression {path}/{mode}')
            if result.returncode:
                old_code = re.search(r'error\([^)]*\)\[[^]]*\]', old['diagnostic'])[0]
                require(old_code in result.stderr, f'legacy diagnostic regression {path}')
    boundary = max(value for case in frozen['oracles'].values()
                   if case['stage'] == 'native' for value in case['operands'])
    witnesses = {family: [] for family in ('<=', '>', 'copy')}
    with tempfile.TemporaryDirectory(prefix='p603-') as temp:
        for name, digest in frozen['sources'].items():
            source = root / name
            require(hashlib.sha256(source.read_bytes()).hexdigest() == digest, f'frozen {name}')
            check = run([compiler, '--check', str(source)])
            emitted = run([compiler, str(source)])
            oracle = frozen['oracles'][name]
            if oracle['stage'] == 'native':
                require(check.returncode == emitted.returncode == 0 and not check.stdout and
                        not check.stderr and not emitted.stderr, f'admission {name}: {check} {emitted}')
                snapshot = portable[name]
                require(snapshot['source_sha256'] == digest and
                        hashlib.sha256(emitted.stdout.encode()).hexdigest() == snapshot['new_c17_sha256'] and
                        hashlib.sha256((root / 'evidence' / name.replace('.nl', '.c17.txt')).read_bytes()).hexdigest() == snapshot['old_c17_sha256'],
                        f'versioned original/portable C17 snapshot changed {name}')
                public = run([probe, str(source)])
                require(public.returncode == 0 and not public.stderr, f'public observation {public}')
                lines = public.stdout.splitlines()
                evidence = json.loads(lines[0])
                require([evidence['left'], evidence['right']] == oracle['operands'] and
                        evidence['bool'] == oracle['truth'] and evidence['authority'] == 0 and
                        evidence['if_checks'] == 0, 'wrong observed source values')
                bindings = {}
                for line in lines[1:]:
                    tag, symbol, kind, value = line.split()
                    require(tag == 'BIND', 'unexpected observation record')
                    bindings[int(symbol)] = (kind, int(value))
                fault = run([gate, str(source)])
                require(fault.returncode == 0 and not fault.stderr and 'original=PASS snapshot=UNCHANGED' in fault.stdout,
                        f'private gate {fault}')
                observed, mutants = observe(emitted.stdout, evidence, bindings)
                statuses = {'plain': execute(Path(temp), cc, name+'plain', emitted.stdout, 0, sanitizer),
                            'observed': execute(Path(temp), cc, name+'observed', observed, 0, sanitizer)}
                mutation_statuses = {}
                for mutant, (text, expected) in mutants.items():
                    category = 'TRUTH_CHANGING' if expected == 91 else 'SAME_TRUTH'
                    mutation_statuses[mutant] = execute(Path(temp), cc, name+('le' if mutant=='<=' else 'gt' if mutant=='>' else mutant), text, expected, sanitizer, mutant_kind=category)
                    # Both operands are positive and away from the target-width
                    # boundary; these runtime witnesses cannot be replaced by a
                    # compile refusal on the zero/MAX instrumentation variants.
                    if all(0 < value < boundary for value in oracle['operands']):
                        for row in mutation_statuses[mutant]:
                            if row['outcome'] == 'NATIVE_REJECTED' and row['native_exit'] == 91:
                                witnesses[mutant].append(dict(source=name, optimization=row['optimization']))
                print(json.dumps(dict(source=name, sha256=digest, check=0, public=evidence,
                                      gate=fault.stdout.strip(), bindings=bindings, c17=0,
                                      c_sha256=hashlib.sha256(emitted.stdout.encode()).hexdigest(),
                                      native=statuses, mutations=mutation_statuses, sanitizer=sanitizer)))
            else:
                for result in (check, emitted):
                    require(result.returncode == 3 and not result.stdout, f'negative accepted {name}')
                    if oracle['stage'] == 'source':
                        require('error(syntax)' in result.stderr or 'error(unsupported)' in result.stderr,
                                f'not SOURCE {name}: {result.stderr}')
                    elif name == 'uintptr.nl':
                        # Original expectation stays frozen. Exact uintptr type resolution
                        # stops before operand comparison in both contexts, not a TYPE pass.
                        require('error(semantic)[P3-UNKNOWN-TYPE]' in result.stderr, 'uintptr phase calibration')
                    else:
                        require(f"error({oracle['stage']})[{oracle['code']}]" in result.stderr,
                                f'wrong phase {name}: {result.stderr}')
                require(check.returncode == candidate[name]['check'], 'unrecorded candidate change')
                print(json.dumps(dict(source=name, exit=3, diagnostic=check.stderr.strip(), c_bytes=0)))
    require_runtime_falsifiers(witnesses)
    print(json.dumps(dict(runtime_falsifiers=witnesses)))
    print('P603 finite source-owned Copy comparator; no privileged public validator or Product GO')


if __name__ == '__main__':
    main()
