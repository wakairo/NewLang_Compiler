"""P1208 finite guest cohort: source/owned/current/CFG gates before execution."""
from pathlib import Path
import hashlib,json,os,re,subprocess,sys
sys.dont_write_bytecode=True
here=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(here.parent/'p1193'/'production'))
from mapping import roles,inspect
from native_observer import observer

compiler,probe,cc,sanitizer,destination=sys.argv[1:]
raw=Path(destination);raw.mkdir(parents=True,exist_ok=False)
from inputs import verify
verify()
oracle=json.loads((here/'oracle-v2.json').read_text())
normal_phases={'01-left-normal':[2,3],'02-right-normal':[2],
    '03-left-final-return':[3],'04-right-final-return':[1],
    '05-left-discard-normal':[2,3],'82-both-return-control':[]}
native_count=0;rows=[];accepted=[]
def run(args,label):
    args=list(map(str,args))
    r=subprocess.run(args,capture_output=True,timeout=120,
        env={**os.environ,'ASAN_OPTIONS':'detect_leaks=1:abort_on_error=1',
             'UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'})
    (raw/(label+'.out')).write_bytes(r.stdout);(raw/(label+'.err')).write_bytes(r.stderr)
    rows.append({'label':label,'args':args,'status':r.returncode})
    (raw/'observations.json').write_text(json.dumps(rows,indent=2)+'\n')
    assert r.returncode==0,rows[-1]
    return r

for name in ('01-left-normal', '04-right-final-return'):
    m=oracle['profiles'][name]
    source=here/'sources'/f'{name}.nl'
    run([compiler,'--check',source],name+'-check')
    evidence=run([probe,source],name+'-owned').stdout.decode()
    text=run([compiler,source],name+'-c17').stdout.decode()
    view=roles(evidence)
    index=int(m['selected_field']=='kept')
    assert view['index']==index
    meta={'initial':m['initial_field'],'peer':m['sibling'],'index':index,
        'headers':[0,1,2,3,4],'incoming':m['values'],
        'old':[m['initial_field'],*m['values'][:-1]],'final':m['values'][-1],
        'prefix_field_reads':0,'new_capture_use':False,'captured':0,
        'locals':{'actual':'phase','incoming':'incoming','old':'old',
                  'old_copy':'old','during':'incoming','beside':'peer'}}
    mapped=inspect(text,view,meta)
    h=re.search(r'C256 HELPER formal=(\d+) selection=(\d+) normal=(\d+) returns=(\d+)',evidence);assert h
    formal,selection,normal,exits=map(int,h.groups())
    plan=re.search(r'C263 PLAN normal_arm=(\d+) final_return=(\d+) body_returns=(\d+)',evidence);assert plan
    normal_arm,final_return,body_returns=map(int,plan.groups())
    assert exits==body_returns==(2 if name=='82-both-return-control' or final_return else 1)
    assert (normal_arm<2)==(normal==1)
    assert bool(final_return)==('final-return' in name)
    actuals={v['name']:s for s,v in view['locals'].items()}
    helper=f'nl_fn_{view["helper"]}'
    phase=f'nl_loop_phase_{view["loop"]}'
    assert f'const size_t nl_local_{actuals["actual"]} = {phase};' in mapped['body']
    assert f'const uint8_t nl_local_{actuals["incoming"]} = {helper}(nl_local_{actuals["actual"]});' in mapped['body']
    assert len(re.findall(r'\*nl_local_\d+ = nl_local_'+str(actuals['incoming'])+r';',mapped['body']))==1
    pattern=r'(static uint8_t '+helper+r'\(size_t (nl_local_\d+)\)\n\{\n)(.*?)(^\})'
    f=re.search(pattern,text,flags=re.S|re.M);assert f
    assert f[2]==f'nl_local_{formal}'
    assert len(re.findall(r'<|==',f[3]))==1,'condition must be evaluated once'
    bindings={n:int(s) for s,n,_ in re.findall(r'C263 HELPER_BIND symbol=(\d+) name=(\w+) initializer=(\d+)',evidence)}
    selected=None
    if normal==1:
        selected=f'nl_local_{bindings["answer"]}'
        branch=re.search(r'    uint8_t '+selected+r';\n    if \(([^\n]+)\) \{\n(.*?)    \} else \{\n(.*?)    \}\n',f[3],flags=re.S);assert branch
        arms=[int(v) for _,v in re.findall(r'C256 LITERAL arm=(\d+) value=(\d+)',evidence)]
        for a in range(2):
            expected=(f'        {selected} = {arms[a]};\n' if a==normal_arm else f'        return {arms[a]};\n')
            assert branch[a+2]==expected,(name,a,branch[a+2],expected)
        before=f[3][:branch.start()]
        after=f[3][branch.end():]
        initialized={f[2]}
        def check_lines(code):
            for line in code.splitlines():
                d=re.match(r'\s*const (?:size_t|uint8_t) (nl_\w+) = ([^;]+);',line)
                expression=d[2] if d else line
                used=set(re.findall(r'\bnl_(?:local|less_left|less_right)_\d+\b',expression))
                assert used<=initialized,(name,'uninitialized/wrong carrier',line,used,initialized)
                if d:initialized.add(d[1])
        check_lines(before)
        assert set(re.findall(r'\bnl_(?:local|less_left|less_right)_\d+\b',branch[1]))<=initialized
        initialized.add(selected) # only the actual normal predecessor reaches here
        check_lines(after)
        assert f[3].count('return ')==2
    else:
        assert normal==0 and normal_arm==2 and f[3].count('return ')==1
    (raw/(name+'-structural.json')).write_text(json.dumps({'roles':view,'normal_arm':normal_arm,'final_return':final_return,'body_returns':body_returns},indent=2)+'\n')
    header=observer(meta,view,mapped).replace('C256 physical','C263 physical')
    header=header.replace('static size_t phases,','static size_t normal_entries,normal_discards;\nstatic size_t phases,')
    count=len(normal_phases[name]);discard_count=count if name=='05-left-discard-normal' else 0
    header=header.replace('static void finish(void){','static void finish(void){\n assert(normal_entries=='+str(count)+' && normal_discards=='+str(discard_count)+');')
    if selected:
        flags=','.join('1' if x in normal_phases[name] else '0' for x in range(4))
        header+=f'''\nstatic const unsigned normal_flags[]={{ {flags} }};
static void normal_continuation(uint8_t value){{
 assert(calls==returns+1 && calls<=4 && normal_flags[calls-1]);
 assert(value==incoming[calls-1]);++normal_entries;
}}
'''
    if discard_count:
        header+='''static void normal_discard(uint8_t value){
 assert(calls==returns+1 && normal_flags[calls-1] && value==incoming[calls-1]);++normal_discards;
}
'''
    def passive(match):
        body,n=re.subn(r'\breturn ([^;\n]+);',r'return helper_return(\1);',match[3]);assert n==(2 if selected else 1)
        if selected:
            anchor=f'    (void){selected};\n';assert body.count(anchor)==1
            body=body.replace(anchor,anchor+f'    normal_continuation({selected});\n')
        if discard_count:
            relay=f'nl_local_{bindings["relay"]}'
            anchor=f'    (void){relay};\n'
            assert body.count(anchor)==2,'implicit unused suppression then actual language discard'
            first=body.index(anchor);second=body.index(anchor,first+len(anchor))
            body=body[:second]+f'    normal_discard({relay});\n'+body[second+len(anchor):]
        return match[1]+'    helper_input('+match[2]+');\n'+body+match[4]
    instrumented,n=re.subn(pattern,passive,text,flags=re.S|re.M);assert n==1
    plain=raw/(name+'.c');plain.write_text(text)
    observed=raw/(name+'-passive.c');observed.write_text(instrumented)
    head=raw/(name+'-observer.h');head.write_text(header)
    accepted.append((name,plain,observed,head))
print('C266 selected2 CHECK/owned/current/CFG/carrier gates PASS; native0',flush=True)
for name,plain,observed,header in accepted:
    for opt in ('O0','O2'):
        for active in (False,True):
            assert native_count<8
            label=name+'-'+opt+('-passive' if active else '-raw');exe=raw/label
            flags=['-std=c17','-Wall','-Wextra','-Wpedantic','-Werror','-UNDEBUG','-'+opt]
            if active:flags+=['-include',str(header)]
            if sanitizer!='none':flags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
            run([cc,*flags,observed if active else plain,'-o',exe],label+'-build')
            result=run([exe],label+'-native');native_count+=1
            (raw/'native-count.json').write_text(json.dumps({'native':native_count,'configured_cap':8,'C266_total_cap':24})+'\n')
            assert not result.stderr
            assert (b'C263 physical-original/old/current/helper/phase PASS' in result.stdout)==active
            exe.unlink()
    print(name,'strict O0/O2 raw/passive PASS4',flush=True)
assert native_count==8
print('C266 mixed guest executions PASS8',flush=True)
