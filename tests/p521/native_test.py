"""Frozen actual-source ladder. Observers read guest carriers only; C mutants
are negative observer controls, never alternate success implementations."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile


def run(args):
    return subprocess.run(args, capture_output=True, text=True, check=False)


def need(ok, why):
    if not ok:
        raise SystemExit(why)


def observe(text, oracle, fields):
    text = text.replace('#include <stdint.h>', '#include <stdint.h>\n#include <stdlib.h>\nstatic unsigned p521_calls;')
    calls = list(re.finditer(r'const uint8_t (nl_call_\d+) = (nl_fn_\d+)\(([^\n()]*)\);', text))
    if not calls:
        # Unchanged direct IF→Pair baseline is already observed by P517's test.
        return None, {}
    need(len(calls) == 1, 'not one actual runtime relay call')
    call, fn, actual = calls[0].groups()
    expected = oracle['forwarded']
    definition = re.search(rf'static uint8_t {fn}\(uint8_t (nl_local_\d+)\)\n\{{', text)
    need(definition, 'missing genuine typed callee definition')
    text = text.replace(definition[0], definition[0] + '\n    ++p521_calls;')
    text = text.replace(calls[0][0], calls[0][0] + f'\n    if ({call} != {expected} || p521_calls != 1) abort();')
    receiver = re.search(rf'const uint8_t (nl_local_\d+) = {call};', text)
    need(receiver, 'missing source call return receiver')
    forwarded = receiver[1]
    text = text.replace(receiver[0], receiver[0] + f'\n    if ({forwarded} != {expected}) abort();')
    if fields is None:
        return text, {'return': f'return {definition[1]};', 'call': calls[0][0]}
    e = fields
    root = f"nl_local_{e['original_symbol']}"
    selected = f"nl_local_{e['selected_symbol']}"
    ref = f"nl_local_{e['ref_symbol']}"
    need(actual == selected, 'actual is not source-selected runtime carrier')
    source_root = re.search(rf'(nl_type_\d+) {root} = \{{ .f0 = 37, .f1 = 211, \}};', text)
    need(source_root, 'missing ORIGINAL pre-IF Pair')
    choice = re.search(rf'const uint8_t {selected} = \([^\n]* \? 108 : 173\);', text)
    need(choice and text.index(source_root[0]) < text.index(choice[0]) < text.index(calls[0][0]), 'wrong choice/call/root order')
    text = text.replace(source_root[0], source_root[0] + f'\n    const void *const p521_root = &{root};\n    uint8_t *const p521_left = &{root}.f0;\n    uint8_t *const p521_right = &{root}.f1;')
    text = text.replace(choice[0], choice[0] + f'\n    if ({selected} != {expected}) abort();')
    pointer = f'uint8_t *const {ref} = &{root}.f0;'
    need(pointer in text, 'missing original scoped ref')
    text = text.replace(pointer, pointer + f'\n        if ({ref} != p521_left || p521_root != &{root} || *{ref} != 37) abort();')
    write = f'*{ref} = {forwarded};'
    need(write in text, 'call result not used by physical write')
    text = text.replace(write, write + f'\n        if ({root}.f0 != {expected} || {root}.f1 != 211 || p521_right != &{root}.f1) abort();')
    for key, value in [('old',37),('after',expected),('sibling',211)]:
        variable = f"nl_local_{e[key+'_symbol']}"
        binding = re.search(rf'const uint8_t {variable} = [^;]+;', text)
        need(binding, f'missing {key} observation')
        text = text.replace(binding[0], binding[0] + f'\n    if ({variable} != {value}) abort();')
    return text, {'return':f'return {definition[1]};', 'choice':choice[0], 'pointer':pointer,
                  'root':root, 'root_decl':source_root[0], 'root_type':source_root[1], 'ref':ref, 'write':write}


def execute(root, cc, stem, text, expected, sanitizer):
    c = root / (stem+'.c')
    c.write_text(text)
    results=[]
    flags=[] if sanitizer=='none' else [f'-fsanitize={sanitizer}', '-fno-sanitize-recover=all']
    for opt in ['-O0','-O2']:
        exe=root/(stem+opt)
        built=run([cc,'-std=c17','-Wall','-Wextra','-Wpedantic','-Werror',opt,*flags,str(c),'-o',str(exe)])
        need(built.returncode==0, f'{stem} C17: {built.stderr}')
        result=run([str(exe)])
        need((result.returncode==0 and not result.stdout and not result.stderr) if expected else result.returncode!=0,
             f'{stem} observer exit mismatch {result}')
        results.append(result.returncode)
    return results


def main():
    compiler, owned_probe, field_probe, call_gate, semantic_gate, cc, directory, sanitizer=sys.argv[1:]
    directory=Path(directory)
    manifest=json.loads((directory/'frozen.json').read_text())
    with tempfile.TemporaryDirectory(prefix='p521-native-') as tmp:
        root=Path(tmp)
        for name, case in manifest['cases'].items():
            source=directory/case['source']
            need(hashlib.sha256(source.read_bytes()).hexdigest()==case['sha256'], 'frozen source changed')
            checked=run([compiler,'--check',str(source)])
            generated=run([compiler,str(source)])
            if name.startswith('negative_'):
                code=case['expectation']['diagnostic']
                need(checked.returncode==3 and generated.returncode==3 and not checked.stdout and not generated.stdout and code in checked.stderr, f'{name}: diagnostic changed')
                rejection=run([semantic_gate,'reject',str(source)])
                need(rejection.returncode==0 and 'rollback=PASS artifact=NULL' in rejection.stdout,f'{name}: rollback {rejection}')
                print(json.dumps({'case':name,'cli':3,'diagnostic':checked.stderr.strip(),'rollback':rejection.stdout.strip()}))
                continue
            need(checked.returncode==0 and not checked.stdout and not checked.stderr and generated.returncode==0 and not generated.stderr,f'{name}: source pipeline failed {generated}')
            second=run([compiler,str(source)])
            need(second.stdout==generated.stdout and second.returncode==0,'nondeterministic C17')
            fields=None
            if 'pair' in name or name.startswith(('combined','adopted')):
                evidence=run([field_probe,str(source)])
                need(evidence.returncode==0,f'{name}: public field observations {evidence}')
                fields=json.loads(evidence.stdout)
                need(fields['joined_unknown'] and fields['scope_ended'] and fields['old']==37 and fields['sibling']==211,'wrong public field evidence')
            if name.startswith(('combined','call_choice','scalar')):
                evidence=run([owned_probe,str(source)])
                need(evidence.returncode==0 and 'PUBLIC source-issued PASS' in evidence.stdout,f'{name}: owned call evidence {evidence}')
                if not name.startswith('scalar'):
                    need('known=0' in evidence.stdout,'unknown call result forced known')
                    gate=run([call_gate,str(source)])
                    need(gate.returncode==0 and 'rejects=19' in gate.stdout, f'{name}: call controls {gate}')
            plain=execute(root,cc,name+'_plain',generated.stdout,True,sanitizer)
            observed,sites=observe(generated.stdout,case['oracle'],fields)
            observed_exits=execute(root,cc,name+'_observed',observed,True,sanitizer) if observed else None
            mutants={}
            if observed:
                mutants['wrong_callee']=observed.replace(sites['return'],'return 0;')
                if sites.get('choice'):
                    mutants['wrong_condition']=observed.replace('nl_local_1 == nl_local_2','nl_local_1 != nl_local_2')
                    mutants['wrong_arm']=observed.replace('? 108 : 173','? 173 : 108')
                    mutants['wrong_field']=observed.replace(sites['pointer'],sites['pointer'].replace('.f0','.f1'))
                    mutants['wrong_root']=observed.replace(sites['root_decl'],sites['root_decl']+f"\n    {sites['root_type']} p521_other = {sites['root']};").replace(sites['pointer'],sites['pointer'].replace(sites['root']+'.f0','p521_other.f0'))
                    mutants['wrong_sibling']=observed.replace(sites['write'],sites['write']+f"\n        {sites['root']}.f1 = 0;")
                for mutation,text in mutants.items():
                    need(text!=observed,'mutant did not change generated output')
                    mutants[mutation]=execute(root,cc,name+'_'+mutation,text,False,sanitizer)
            print(json.dumps({'case':name,'sha256':case['sha256'],'check':0,'c17':0,'plain_native':plain,'observed_native':observed_exits,'physical_oracle':case['oracle'] if observed else 'existing P517 native test','public_field_facts':fields,'bad_C_refused':mutants}))
    print('P521 frozen source -> owned -> actual C17/native call composition PASS; negative controls reject')


if __name__=='__main__':
    main()
