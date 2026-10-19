"""Passive observations of actual emitted local IF, reused only by old complex cases.
No guest initialization, selected result or permission is supplied by this host.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import re

_spec = importlib.util.spec_from_file_location('p633_native_helpers', Path(__file__).parents[1]/'p517/native_test.py')
h = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(h)


def observe_case(compiler, source, cc, sanitizer, expected, probe=None, local_probe=None):
    source = Path(source)
    check, emitted = h.run([compiler, '--check', str(source)]), h.run([compiler, str(source)])
    h.require(check.returncode == emitted.returncode == 0 and not check.stdout and not check.stderr and not emitted.stderr,
              f'ordinary local CHECK/C17 failed: {check} {emitted}')
    again = h.run([compiler,str(source)])
    h.require(again.returncode == 0 and again.stdout == emitted.stdout and not again.stderr,'nonrepeatable C17')
    text = emitted.stdout
    choice = re.search(r'const uint8_t (nl_local_(\d+)) = (nl_if_result_(\d+));',text)
    h.require(choice and text.count('if (') == 1 and text.count('} else {') == 1 and ' ? ' not in text,
              'missing actual IF/ELSE arm-local lowering')
    h.require('CHAR_BIT == 8' in text and '_Static_assert(SIZE_MAX ==' in text,'lost target guards')
    truth = re.search(r'const _Bool (nl_local_\d+) = \((nl_less_left_\d+) < (nl_less_right_\d+)\);',text)
    h.require(truth is not None,'missing original comparison')
    tests = {choice[0]:f"{choice[1]} != {expected['selected']}", truth[0]:f"{truth[1]} != {expected['bool']}"}
    additions = {}
    public = None
    if local_probe:
        p = h.run([local_probe,str(source)])
        h.require(p.returncode == 0 and not p.stderr,'local public owned failed')
        public = json.loads(p.stdout)
        h.require(public['bool'] == expected['bool'] and public['selected_symbol'] == int(choice[2]) and
                  public['authority'] == 0 and public['joined_unknown'] and public['arm_locals_ended'] and public['worlds_distinct'],
                  'local owned relation mismatch')
    fields = None
    if probe:
        p = h.run([probe,str(source)])
        h.require(p.returncode == 0 and not p.stderr,f'Pair public owned failed: {p}')
        _,fields = map(json.loads,p.stdout.splitlines())
        h.require(fields['bool'] == expected['bool'] and fields['old'] == expected['old'] and fields['sibling'] == expected['sibling'] and
                  fields['joined_unknown'] and fields['scope_ended'] and fields['selected_symbol'] == int(choice[2]),'Pair owned mismatch')
        owner,ref = f"nl_local_{fields['original_symbol']}",f"nl_local_{fields['ref_symbol']}"
        physical = re.search(rf"nl_type_\d+ {owner} = \{{ .f0 = {expected['old']}, .f1 = {expected['sibling']}, \}};",text)
        h.require(physical is not None,'missing original guest Pair')
        additions[physical[0]] = f'\n    const void *const p633_root = &{owner};\n    const uint8_t *const p633_left = &{owner}.f0;\n    const uint8_t *const p633_right = &{owner}.f1;'
        tests[physical[0]] = f"{owner}.f0 != {expected['old']} || {owner}.f1 != {expected['sibling']}"
        pointer = f'uint8_t *const {ref} = &{owner}.f0;'
        capture = f"const uint8_t nl_replace_old_{fields['replace_node']} = *{ref};"
        write = f'*{ref} = {choice[1]};'
        tests[pointer] = f'{ref} != p633_left || &{owner} != p633_root'
        tests[capture] = f"nl_replace_old_{fields['replace_node']} != {expected['old']}"
        tests[write] = f"{owner}.f0 != {expected['selected']} || {owner}.f1 != {expected['sibling']} || &{owner} != p633_root || &{owner}.f1 != p633_right"
        for key,expr,val in [('old_symbol',f"nl_loan_result_{fields['scope']}",expected['old']),('after_symbol',f'{owner}.f0',expected['selected']),('sibling_symbol',f'{owner}.f1',expected['sibling'])]:
            tests[f"const uint8_t nl_local_{fields[key]} = {expr};"] = f"nl_local_{fields[key]} != {val}"
        h.require(text.index(physical[0]) < text.index(truth[0]) < text.index(choice[0]) < text.index(pointer) < text.index(capture) < text.index(write),
                  'lost original root/IF/scoped replace order')
    watched = text.replace('#include <stdint.h>','#include <stdint.h>\n#include <stdlib.h>')
    for point,test in tests.items():
        h.require(text.count(point)==1,f'nonunique physical observation {point}')
        watched = watched.replace(point,point+additions.get(point,'')+f'\n    if ({test}) exit(91);',1)
    # Check correct branch execution, not just an equal cached selected value.
    watched = watched.replace(f'if ({truth[1]}) {{',f'if ({truth[1]}) {{\n    if ({truth[1]} != 1) exit(91);',1)
    watched = watched.replace('} else {','} else {\n    if ('+truth[1]+' != 0) exit(91);',1)
    mutants = {'wrong_compare':watched.replace(f'({truth[2]} < {truth[3]})',f'({truth[2]} > {truth[3]})',1)}
    if probe:
        mutants['wrong_field'] = watched.replace(pointer,pointer.replace('.f0','.f1'),1)
        mutants['wrong_old'] = watched.replace(capture,capture.replace(f'*{ref}','99'),1)
    # Numeric expectations only appear in passive assertions. Untouched source
    # and uninstrumented native remain separate required executions.
    import tempfile
    with tempfile.TemporaryDirectory(prefix='p633-native-') as temp:
        folder = Path(temp)
        native = {name:h.execute(folder,cc,source.stem+name,code,0,sanitizer) for name,code in [('plain',text),('passive',watched)]}
        synthetic = {name:h.execute(folder,cc,source.stem+name,code,91,sanitizer) for name,code in mutants.items()}
    return dict(source=str(source),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),check=0,c17=0,c17_sha256=hashlib.sha256(text.encode()).hexdigest(),
                public=public,pair_owned=fields,native=native,synthetic_native=synthetic,sanitizer=sanitizer)
