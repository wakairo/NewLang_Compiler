"""Actual source → speculative Production compiler → real C Array carrier.
Observers write only their own I/O/counters, never guest values/authority."""
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


def observed(text, p):
    array, option, symbol = p['array_type'], p['element_type'], p['symbol']
    local = f'nl_local_{symbol}'
    declaration = f'nl_array_{array} {local} = {{'
    require(f'typedef nl_option_{option} nl_array_{array}[4];' in text and text.count(declaration)==1,
            'not one actual typed C Array declaration')
    require(text.count('{ .tag = NL_ARRAY4_NONE }')==4 and 'memset' not in text,
            'None representation invented by zero fill or missing typed values')
    code = f'''
    const void *const p534_original = &{local};
    const uintptr_t p534_base = (uintptr_t)(const void *)&{local}[0];
    if (sizeof({local}) != 4 * sizeof(nl_option_{option})) abort();
    for (size_t i=0; i<4; ++i) {{
        const uintptr_t here = (uintptr_t)(const void *)&{local}[i];
        if (&{local} != p534_original || here-p534_base != i*sizeof(nl_option_{option}) ||
            {local}[i].tag != NL_ARRAY4_NONE) abort();
        printf("element=%zu address=%" PRIuPTR " stride=%zu tag=%u\\n", i, here,
               sizeof(nl_option_{option}), (unsigned){local}[i].tag);
    }}
'''
    marker=f'    (void){local};'
    require(text.count(marker)==1, 'no source-local lifetime observation point')
    return text.replace('#include <stdint.h>','#include <stdint.h>\n#include <stddef.h>\n#include <inttypes.h>\n#include <stdio.h>\n#include <stdlib.h>').replace(marker,code+marker)


def execute(root, cc, name, text, expect, sanitizer, observation=False):
    c=root/(name+'.c'); c.write_text(text)
    flags=[] if sanitizer=='none' else [f'-fsanitize={sanitizer}','-fno-sanitize-recover=all']
    records=[]
    for opt in ['-O0','-O2']:
        exe=root/(name+opt)
        built=run([cc,'-std=c17','-Wall','-Wextra','-Wpedantic','-Werror',opt,*flags,str(c),'-o',str(exe)])
        require(built.returncode==0,f'{name} build: {built.stderr}')
        result=run([str(exe)])
        require((result.returncode==0 and not result.stderr) if expect else result.returncode!=0,
                f'{name}/{opt}: wrong native result {result}')
        if expect:
            require(len(result.stdout.splitlines())==4 if observation else not result.stdout, 'wrong physical observation output')
        records.append({'opt':opt,'exit':result.returncode,'stdout':result.stdout,'stderr':result.stderr if expect else 'expected observer refusal'})
    return records


def main():
    compiler, regular, probe, semantic_gate, cc, directory, sanitizer=sys.argv[1:]
    directory=Path(directory); manifest=json.loads((directory/'frozen.json').read_text())
    expected={'arity3':'ARRAY4-INITIALIZER-ARITY','arity5':'ARRAY4-INITIALIZER-ARITY',
              'wrong_type':'ARRAY4-ELEMENT-TYPE','payloadless_some':'P6-CONSTRUCTOR-SHAPE',
              'wrong_variant':'ARRAY4-OPTION-VARIANT'}
    source=directory/'original.nl'
    c=run([compiler,str(source)]); check=run([compiler,'--check',str(source)])
    require(c.returncode==0 and not c.stderr and check.returncode==0 and not check.stdout and not check.stderr, 'original source not accepted')
    require(run([compiler,str(source)]).stdout==c.stdout,'C17 not deterministic')
    evidence=run([probe,str(source)]); require(evidence.returncode==0 and not evidence.stderr,f'public proof failed {evidence}')
    p=json.loads(evidence.stdout)
    require(p['live_checkpoint_places']==5 and p['independent_roots']==1 and p['public_validator'] and p['separate_view_mutants_rejected']==40,'wrong public checkpoint')
    observed_c=observed(c.stdout,p)
    with tempfile.TemporaryDirectory(prefix='p534-native-') as tmp:
        root=Path(tmp)
        plain=execute(root,cc,'plain',c.stdout,True,sanitizer)
        physical=execute(root,cc,'observed',observed_c,True,sanitizer,True)
        bads={
            'wrong_None_tag':observed_c.replace('{ .tag = NL_ARRAY4_NONE }','{ .tag = 0 }',1),
            'missing_typed_initializer':observed_c.replace('        { .tag = NL_ARRAY4_NONE },\n','',1),
            'wrong_extent':observed_c.replace(f'nl_array_{p["array_type"]}[4]',f'nl_array_{p["array_type"]}[5]')}
        refused={}
        for name,text in bads.items():
            require(text!=observed_c,'C corruption did not change output')
            refused[name]=execute(root,cc,name,text,False,sanitizer)
    print(json.dumps({'case':'original','check':0,'c17':0,'public_owned':p,'plain_native':plain,'physical_observed_native':physical,'bad_C_refused':refused}))
    for name,case in manifest['cases'].items():
        file=directory/(name+'.nl')
        require(hashlib.sha256(file.read_bytes()).hexdigest()==case['sha256'],'frozen source changed')
        default=run([regular,'--check',str(file)])
        require(default.returncode==3 and 'REC-TYPE-PROFILE' in default.stderr and not default.stdout,'default-OFF source isolation failed')
        if name=='original': continue
        for args in [['--check'],[]]:
            neg=run([compiler,*args,str(file)])
            require(neg.returncode==3 and not neg.stdout and expected[name] in neg.stderr,f'{name} wrong semantic refusal {neg}')
        rejection=run([semantic_gate,'reject',str(file)])
        require(rejection.returncode==0 and 'phase=registration' in rejection.stdout and 'rollback=PASS artifact=NULL' in rejection.stdout, f'{name} not a semantic negative {rejection}')
        print(json.dumps({'case':name,'sha256':case['sha256'],'parser':0,'check':3,'diagnostic':neg.stderr,'rollback':rejection.stdout.strip()}))
    print('P534 SPECULATIVE SOURCE/OWNED/C17/NATIVE original Option Array4 PASS; no indexing/Ring/product claim')


if __name__=='__main__':
    main()
