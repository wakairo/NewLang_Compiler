"""Migrate only old precision assertions; original sources/oracles stay frozen.

Actual public receipt + normal CLI + unmodified/passively observed native C.
This is a test helper, never a production source-name admission path.
"""
import json
from pathlib import Path
import re
import subprocess
import tempfile


def run(args):
    result = subprocess.run(list(map(str, args)), capture_output=True, text=True)
    assert result.returncode == 0 and not result.stderr, result
    return result.stdout


def exercise(compiler, source, cc, sanitizer):
    source = Path(source)
    taken = 1 if 'false' in source.stem else 0
    expected = 8 if taken else 7
    assert not run([compiler, '--check', source])
    code = run([compiler, source])
    evidence = json.loads(run([Path(compiler).parent / 'p713_public_probe', source]))
    assert evidence['index'] == 0
    assert [a['changes'] for a in evidence['arms']] == ([1, 0] if source.stem == 'branch_effect' else [1, 1])
    assert evidence['parent_before'] != evidence['parent_after']
    base = evidence['base']
    pattern = rf'(?:const )?nl_type_\d+ nl_local_{base} = \{{[^;]+;'
    constructors = list(re.finditer(pattern, code))
    assert len(constructors) == 1
    pointer = f'nl_local_{base}'
    observed = code.replace('#include <stdint.h>', '#include <stdint.h>\n#include <stdlib.h>')
    observed = observed.replace(constructors[0][0], constructors[0][0] +
        f'\n    if ({pointer}.f0 != 0 || {pointer}.f1 != 9) abort();', 1)
    # Check the returned old Copy at each real replace, including P514's later
    # write after the IF. Do not supply a branch decision, value or store.
    captures = list(re.finditer(r'const uint8_t (nl_replace_old_\d+) = \*nl_local_\d+;', observed))
    assert len(captures) == (2 if source.stem == 'branch_effect' else 2)
    for i, match in enumerate(captures):
        old = 7 if source.stem == 'branch_effect' and i == 1 else 0
        observed = observed.replace(match[0], match[0] + f'\n    if ({match[1]} != {old}) abort();', 1)
    for read in evidence['reads']:
        decl = f'const uint8_t nl_local_{read["symbol"]} = {pointer}.f{read["index"]};'
        assert observed.count(decl) == 1
        value = expected if read['index'] == 0 else 9
        observed = observed.replace(decl, decl +
            f'\n    if (nl_local_{read["symbol"]} != {value} || {pointer}.f0 != {expected} || {pointer}.f1 != 9) abort();', 1)
    assert 'if (' in code and '} else {' in code
    with tempfile.TemporaryDirectory() as directory:
        for mode, text in [('original', code), ('passive', observed)]:
            c = Path(directory) / (mode + '.c')
            c.write_text(text)
            for opt in ['-O0', '-O2']:
                exe = Path(directory) / (mode + opt)
                args = [cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', opt, c, '-o', exe]
                if sanitizer != 'none':
                    args += [f'-fsanitize={sanitizer}', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
                run(args)
                assert not run([exe])
    print(json.dumps({'source': str(source), 'P713_frontier': 'CHECK/owned/C17/native PASS', 'owned': evidence}))
