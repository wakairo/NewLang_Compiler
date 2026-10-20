"""HOLD evidence: genuine local prototype C, not integrated branch HEAD."""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import sys
import tempfile

compiler, public, dump, gate, oom, cc, directory = sys.argv[1:]
root = Path(directory)
spec = importlib.util.spec_from_file_location('native', root.parent / 'p517/native_test.py')
h = importlib.util.module_from_spec(spec)
spec.loader.exec_module(h)
cases = json.loads((root/'frozen.json').read_text())['cases'] | json.loads((root/'additional-freeze.json').read_text())['cases']
with tempfile.TemporaryDirectory(prefix='p630-observe-') as tmp:
    folder = Path(tmp)
    for name, oracle in cases.items():
        source = root/name
        h.require(hashlib.sha256(source.read_bytes()).hexdigest() == oracle['sha256'], 'frozen input changed')
        if not oracle['intent'].startswith('native'):
            continue
        emitted = h.run([compiler, str(source)])
        h.require(emitted.returncode == 0 and not emitted.stderr, f'prototype emission failed {emitted}')
        text = emitted.stdout
        original = h.run([dump, str(source)])
        h.require(original.returncode == 0 and not original.stderr, 'real source-owned arm dump failed')
        chosen = re.search(r'const uint8_t (nl_local_\d+) = (nl_if_result_\d+);', text)
        h.require(chosen is not None and text.count('if (') == 1 and text.count('} else {') == 1,
                  'not actual scoped IF result forwarding')
        watched = text.replace('#include <stdint.h>', '#include <stdint.h>\n#include <stdlib.h>')
        tests = {chosen[0]: f"{chosen[1]} != {oracle['selected']}"}
        for m, value in zip(re.finditer(r'const size_t (nl_less_(?:left|right)_\d+) = nl_local_\d+;', text), oracle['operands']):
            tests[m[0]] = f'{m[1]} != ((size_t)UINTMAX_C({value}))'
        truth = re.search(r'const _Bool (nl_local_\d+) = \([^;]+\);', text)
        h.require(truth is not None, 'no original comparison')
        tests[truth[0]] = f"{truth[1]} != {oracle['bool']}"
        fields = None
        poison = None
        if oracle['intent'] != 'native_pure':
            p = h.run([public, str(source)])
            h.require(p.returncode == 0 and not p.stderr, f'public original field observation failed {p}')
            comparison, fields = map(json.loads, p.stdout.splitlines())
            h.require(fields['bool'] == oracle['bool'] and fields['arms'] == [83,24] and fields['old']==18 and fields['sibling']==209 and fields['scope_ended'], 'original owned mismatch')
            owner = f"nl_local_{fields['original_symbol']}"
            ref = f"nl_local_{fields['ref_symbol']}"
            declaration = re.search(rf'nl_type_\d+ {owner} = \{{ .f0 = 18, .f1 = 209, \}};', text)
            h.require(declaration is not None, 'no real original Pair')
            watched = watched.replace(declaration[0], declaration[0] + f'\n    const void *const p630_original = &{owner};\n    const uint8_t *const p630_left = &{owner}.f0;')
            tests[declaration[0]] = f'{owner}.f0 != 18 || {owner}.f1 != 209'
            tests[f'uint8_t *const {ref} = &{owner}.f0;'] = f'{ref} != p630_left || &{owner} != p630_original'
            tests[f"const uint8_t nl_replace_old_{fields['replace_node']} = *{ref};"] = f"nl_replace_old_{fields['replace_node']} != 18"
            tests[f"*{ref} = {chosen[1]};"] = f"{owner}.f0 != {oracle['after']} || {owner}.f1 != 209 || &{owner} != p630_original"
            for key, expr, expected in [('old_symbol',f"nl_loan_result_{fields['scope']}",18),('after_symbol',f'{owner}.f0',oracle['after']),('sibling_symbol',f'{owner}.f1',209)]:
                symbol=f"nl_local_{fields[key]}"
                tests[f'const uint8_t {symbol} = {expr};'] = f'{symbol} != {expected}'
            poison = h.run([gate,str(source)])
            h.require(poison.returncode == 0 and 'worlds=UNCHANGED driver_allocations=0' in poison.stdout, f'legacy gate failure {poison}')
        for point, test in tests.items():
            h.require(text.count(point)==1, f'nonunique guest observation {point}')
            watched = watched.replace(point, point + f'\n    if ({test}) exit(91);',1)
        # Actual source-origin initialization and lexical declarations remain
        # inside the emitted branches. Observer supplies only comparisons.
        statuses={kind:h.execute(folder,cc,name+kind,code,0,'none') for kind,code in [('plain',text),('passive',watched)]}
        print(json.dumps({'source':name,'check':0,'prototype_c17':0,'c17_sha256':hashlib.sha256(text.encode()).hexdigest(),'native':statuses,'owned':fields,'legacy_gate':poison.stdout.strip() if poison else None,'note':'local prototype only; no new local-specific fault suite or integration PASS claimed'}))
