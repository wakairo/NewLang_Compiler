"""Frozen real sources, public return receipts and separate passive C observations."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def run(args):
    return subprocess.run(list(map(str,args)),capture_output=True)


def require(ok, message):
    if not ok:
        raise AssertionError(message)


def passive(text, evidence, oracle):
    base,index=evidence['base'],evidence['index']
    normal,terminal=oracle['normal_arm'],1-oracle['normal_arm']
    require(evidence['normal']==normal and evidence['index']==index==oracle['index'], 'exact original/normal edge')
    require(evidence['old']==oracle['old'] and evidence['current']==oracle['normal_post'][index], 'owned old/current')
    additions={}
    def after(match,extra,offset=0):
        pos=match.end()+offset
        additions[pos]=additions.get(pos,'')+extra
    def state(values):
        return ' || '.join(f'*observer_{i} != {value}' for i,value in enumerate(values))
    ctor=list(re.finditer(rf'(?:const )?nl_type_\d+ nl_local_{base} = \{{[^;]+;',text))
    require(len(ctor)==1,'one original physical carrier')
    after(ctor[0],''.join(f'\n    const volatile uint8_t *const observer_{i} = &nl_local_{base}.f{i};' for i in range(2))+f'\n    if ({state(oracle["initial"])}) exit(91);\n    observer_phase=1;')
    headers=list(re.finditer(r'if \(\(nl_less_left_\d+ < nl_less_right_\d+\)\) \{',text))
    require(len(headers)==1,'real operand comparison and both C branches')
    header=headers[0];split=text.index('    } else {',header.end());end=text.index('\n    }\n',split)+len('\n    }')
    bounds=[(header.end(),split),(split+len('    } else {'),end)]
    for a in range(2):
        lo,hi=bounds[a];additions[lo]=f'\n        observer_arm={a};'
        events=evidence['streams'][a]['events'];changes=[e for e in events if e['change']]
        incoming=oracle['normal_post'][index] if a==normal else oracle['terminal_store']
        require(len(changes)==(1 if incoming is not None else 0),'actual store count per edge')
        for e in changes:
            ref=f'nl_local_{e["ref"]}'
            part=text[lo:hi]
            pointer=list(re.finditer(rf'uint8_t \*const {ref} = &nl_local_{base}.f{index};',part))
            capture=list(re.finditer(rf'const uint8_t nl_replace_old_{e["node"]} = \*{ref};',part))
            store=list(re.finditer(rf'\*{ref} = {incoming};',part))
            require(len(pointer)==len(capture)==len(store)==1,'real scoped ref/capture/store')
            after(pointer[0],f'\n            if ({ref} != observer_{index} || {state(oracle["initial"])}) exit(91);',lo)
            after(capture[0],f'\n            if (nl_replace_old_{e["node"]} != {oracle["old"]}) exit(91);',lo)
            expected=list(oracle['initial']);expected[index]=incoming
            after(store[0],f'\n            ++observer_changes;\n            if ({state(expected)}) exit(91);',lo)
        returns=list(re.finditer(r'\breturn;',text[lo:hi]))
        if a==terminal:
            require(len(returns)==1,'terminal edge must really return')
            pos=returns[0].start()+lo
            expected=list(oracle['initial'])
            if incoming is not None:expected[index]=incoming
            additions[pos]=f'if (observer_phase != 1 || observer_arm != {terminal} || {state(expected)}) exit(91);\n        observer_phase=2;\n        '
        else:
            require(not returns,'normal arm must not return')
    additions[end]=f'\n    if (observer_phase != 1 || observer_arm != {normal} || {state(oracle["normal_post"])}) exit(91);\n    observer_phase=3;'
    binding=evidence['binding']
    result=list(re.finditer(rf'const uint8_t nl_local_{binding} = nl_if_result_{evidence["if"]};',text))
    require(len(result)==1,'genuine normal Copy forwarding')
    after(result[0],f'\n    if (nl_local_{binding} != {oracle["old"]} || observer_phase != 3) exit(91);')
    require(len(evidence['reads'])==2,'normal current and sibling reads')
    for i,read in enumerate(evidence['reads']):
        decl=list(re.finditer(rf'const uint8_t nl_local_{read["symbol"]} = nl_local_{base}.f{read["index"]};',text))
        require(len(decl)==1,'source current field read')
        after(decl[0],f'\n    if (nl_local_{read["symbol"]} != {oracle["normal_post"][read["index"]]} || observer_phase != 3) exit(91);'+('\n    observer_phase=4;' if i==1 else ''))
    calls=list(re.finditer(r'    nl_fn_\d+\(\);',text))
    require(len(calls)==1,'real source callee from native entry')
    taken=oracle['taken'];phase=4 if taken==normal else 2
    changes=1 if taken==normal or oracle['terminal_store'] is not None else 0
    after(calls[0],f'\n    if (observer_phase != {phase} || observer_arm != {taken} || observer_changes != {changes}) exit(91);')
    for pos in sorted(additions,reverse=True):
        text=text[:pos]+additions[pos]+text[pos:]
    return '#include <stdlib.h>\nstatic unsigned observer_phase,observer_changes,observer_arm=99;\n'+text


def one(compiler,probe,cc,sanitizer,source,oracle):
    check=run([compiler,'--check',source]);emitted=run([compiler,source]);public=run([probe,source])
    require(check.returncode==0 and not check.stdout and not check.stderr,check)
    require(emitted.returncode==0 and emitted.stdout and not emitted.stderr,emitted)
    require(public.returncode==0 and not public.stderr,public)
    evidence=json.loads(public.stdout);code=emitted.stdout.decode();observed=passive(code,evidence,oracle)
    mutants={'wrong_condition':re.sub(r'(nl_less_left_\d+) < (nl_less_right_\d+)',r'\1 >= \2',observed)}
    normal=oracle['taken']==oracle['normal_arm']
    if normal or oracle['terminal_store'] is not None:
        base,index=evidence['base'],evidence['index']
        mutants['wrong_field']=re.sub(rf'(uint8_t \*const nl_local_\d+ = &nl_local_{base}\.)f{index};',rf'\g<1>f{1-index};',observed)
        mutants['wrong_old']=re.sub(r'(const uint8_t nl_replace_old_\d+ = )\*nl_local_\d+;',r'\g<1>0;',observed)
    if not normal:
        # May be rejected by strict C dataflow diagnostics before runtime.
        mutants['removed_return']=observed.replace('        return;','        (void)0;')
    rows={}
    with tempfile.TemporaryDirectory() as tmp:
        root=Path(tmp)
        for label,text in [('original',code),('passive',observed),*mutants.items()]:
            statuses=[]
            for opt in ['-O0','-O2']:
                path=root/(label+'.c');path.write_text(text);exe=root/(label+opt)
                args=[cc,'-std=c17','-Wall','-Wextra','-Wpedantic','-Werror',opt,path,'-o',exe]
                if sanitizer!='none':args += [f'-fsanitize={sanitizer}','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
                build=run(args)
                if build.returncode and label=='removed_return':
                    statuses.append({'compile':build.returncode,'native':'NOT RUN','diagnostic':build.stderr.decode()})
                    continue
                require(build.returncode==0,build.stderr.decode())
                guest=run([exe]);expected=91 if label in mutants else 0
                require(guest.returncode==expected and not guest.stdout and not guest.stderr,guest)
                statuses.append({'compile':0,'native':guest.returncode})
            rows[label]=statuses
        output=root/'saved.c';output.write_bytes(b'KEEP\n')
        saved=run([compiler,'--emit-c17-to',output,source])
        require(saved.returncode==0 and not saved.stdout and output.read_bytes()==emitted.stdout,saved)
    return {'source':str(source),'sha256':hashlib.sha256(Path(source).read_bytes()).hexdigest(),'check':0,'C17':0,'C_sha256':hashlib.sha256(emitted.stdout).hexdigest(),'public':evidence,'native':rows,'sanitizer':sanitizer}


def main():
    compiler,probe,cc,sanitizer,directory=sys.argv[1:];folder=Path(directory)
    manifest=json.loads((folder/'freeze.json').read_text());baseline=json.loads((folder/'evidence/baseline.json').read_text());rows=[]
    for name,oracle in manifest['positive'].items():
        source=folder/(name+'.nl');require(hashlib.sha256(source.read_bytes()).hexdigest()==oracle['sha256'],'frozen positive')
        rows.append(one(compiler,probe,cc,sanitizer,source,oracle))
    for name,oracle in manifest['negative'].items():
        source=folder/(name+'.nl');require(hashlib.sha256(source.read_bytes()).hexdigest()==oracle['sha256'],'frozen negative')
        old=next(row for path,row in baseline.items() if Path(path).name==source.name)['check']
        diagnostic=old['diagnostic'].split('error(',1)[1].split(':',1)[0]
        for flags in [['--check'],[]]:
            r=run([compiler,*flags,source]);require(r.returncode==old['exit'] and not r.stdout and diagnostic.encode() in r.stderr,r)
        rollback=run([Path(compiler).parent/'p718_gate_test','reject',source])
        require(rollback.returncode==0 and b'rollback=PASS artifact=NULL' in rollback.stdout,rollback)
        with tempfile.TemporaryDirectory() as tmp:
            out=Path(tmp)/'preserved.c';out.write_bytes(b'KEEP\n');r=run([compiler,'--emit-c17-to',out,source])
            require(r.returncode==old['exit'] and not r.stdout and out.read_bytes()==b'KEEP\n',r)
    old=folder.parent/'p713/sole_normal.nl'
    require(hashlib.sha256(old.read_bytes()).hexdigest()=='f3bc52b1631e791351c6324a51285f20bb79472a78040f4dfa817e955364ec2b','original P713 source')
    rows.append(one(compiler,probe,cc,sanitizer,old,{'normal_arm':1,'taken':0,'index':1,'initial':[42,111],'normal_post':[42,17],'old':111,'terminal_store':None}))
    print(json.dumps(rows,sort_keys=True))


if __name__=='__main__':main()
