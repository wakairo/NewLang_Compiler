from pathlib import Path
import json,re,subprocess,sys

p=Path(__file__).resolve().parent
compiler,probe,gcc,clang,sanitizer,output=sys.argv[1:]
oracle=json.loads((p/'oracle.json').read_text())
raw=Path(output);raw.mkdir(exist_ok=False,parents=True);rows=[];native_runs=0
def run(cmd,label,allowed=(0,)):
    r=subprocess.run(list(map(str,cmd)),capture_output=True,timeout=90)
    (raw/(label+'.out')).write_bytes(r.stdout);(raw/(label+'.err')).write_bytes(r.stderr)
    row=dict(label=label,command=list(map(str,cmd)),status=r.returncode)
    rows.append(row);(raw/'observations.json').write_text(json.dumps(rows,indent=2)+'\n')
    if r.returncode not in allowed:
        (raw/'FIRST-unexpected.json').write_text(json.dumps(row,indent=2)+'\n')
        raise AssertionError(row)
    return r
def observer(meta,symbols):
    fields=[meta['initial'],meta['sibling']] if meta['index']==0 else [meta['sibling'],meta['initial']]
    final=[meta['final'],meta['sibling']] if meta['index']==0 else [meta['sibling'],meta['final']]
    array=lambda xs:','.join('((size_t)UINTMAX_C('+str(x)+'))' for x in xs) or '0'
    expected={'old':'c243_olds[c243_phases-1]','old_copy':'c243_olds[c243_phases-1]',
              'incoming':'c243_incoming[c243_phases-1]', 'chosen':'c243_incoming[c243_phases-1]', 'forwarded':'c243_incoming[c243_phases-1]', 'during':'c243_incoming[c243_phases-1]',
              'beside':str(meta['sibling'])}
    cases='\n'.join('case '+str(symbols[n])+': assert(v=='+expected[n]+'); ++c243_locals['+str(i)+']; break;' for i,n in enumerate(symbols))
    return '''#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
static uint8_t *c243_a,*c243_b;
static size_t c243_phases,c243_results,c243_reads;
static size_t c243_locals['''+str(len(symbols))+'''];
static const size_t c243_headers[]={'''+array(meta['headers'])+'''};
static const size_t c243_header_child[]={'''+array(meta['header_child'])+'''};
static const size_t c243_olds[]={'''+array(meta['olds'])+'''};
static const size_t c243_incoming[]={'''+array(meta['incoming'])+'''};
static void c243_finish(void){
 assert(c243_phases=='''+str(len(meta['headers']))+''' && c243_results==1 && c243_reads==2);
 for(size_t i=0;i<sizeof c243_locals/sizeof c243_locals[0];++i)assert(c243_locals[i]=='''+str(len(meta['olds']))+''');
 puts("C243 actual-original/phase/old/current/sibling/result PASS");
}
static void c243_bind(uint8_t *a,uint8_t *b){
 assert(!c243_a && !c243_b && a!=b && *a=='''+str(fields[0])+''' && *b=='''+str(fields[1])+''');
 c243_a=a;c243_b=b;assert(atexit(c243_finish)==0);
}
static void c243_phase(size_t n,uint8_t *a,uint8_t *b){
 assert(a==c243_a && b==c243_b && c243_phases<sizeof c243_headers/sizeof c243_headers[0]);
 assert(n==c243_headers[c243_phases]);
 assert(*(('''+str(meta['index'])+''')?b:a)==c243_header_child[c243_phases]);
 assert(*(('''+str(meta['index'])+''')?a:b)=='''+str(meta['sibling'])+''');++c243_phases;
}
static void c243_local(size_t symbol,uint8_t v){
 assert(c243_phases && c243_phases<=sizeof c243_olds/sizeof c243_olds[0]);
 switch(symbol){'''+cases+''' default: assert(0);}
}
static void c243_result(size_t n){
 assert(n==((size_t)UINTMAX_C('''+str(meta['result'])+''')));
 assert(*c243_a=='''+str(final[0])+''' && *c243_b=='''+str(final[1])+''');++c243_results;
}
static void c243_read(uint8_t *a,uint8_t v){
 assert((a==c243_a || a==c243_b) && v==*a);
 assert(v==(a==c243_a?'''+str(final[0])+''':'''+str(final[1])+'''));++c243_reads;
}
#define NL_LOOP_BIND(a,b) c243_bind(a,b)
#define NL_CURRENT_PHASE(n,a,b) c243_phase(n,a,b)
#define NL_CURRENT_LOCAL(s,v) c243_local(s,v)
#define NL_CURRENT_RESULT(n) c243_result(n)
#define NL_LOOP_READ(a,v) c243_read(a,v)
'''

summary={}
for name,meta in oracle['cases'].items():
    src=p/'sources'/(name+'.nl');category=meta['category']
    check=run([compiler,'--check',src],name+'-check',(meta['check'],))
    if category=='negative' and check.returncode==0:raise AssertionError(('STOP negative CHECK accepted',name))
    proof=None
    if category=='positive':proof=run([probe,src],name+'-owned-trace')
    emitted=run([compiler,src],name+'-emit',(meta['emit'],))
    target=raw/(name+'-named.c');sentinel=b'C243 named-output sentinel\n';target.write_bytes(sentinel);before=target.stat()
    named=run([compiler,'--emit-c17-to',target,src],name+'-named',(meta['emit'],))
    assert emitted.returncode==named.returncode
    if emitted.returncode:
        assert not emitted.stdout and target.read_bytes()==sentinel
        assert (target.stat().st_ino,target.stat().st_mtime_ns)==(before.st_ino,before.st_mtime_ns)
    else:assert target.read_bytes()==emitted.stdout
    summary[name]=dict(check=check.returncode,emit=emitted.returncode,named=named.returncode,
                       expected=meta['check'],native_runs=0)
    (raw/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    if category=='control':
        assert check.returncode==0 and emitted.returncode==0
        assert emitted.stdout==(p/'evidence'/(name+'.c17')).read_bytes()
        continue
    if emitted.returncode==0:
        assert check.returncode==0 and 'C243 public-owned=YES private-current=0' in proof.stdout.decode()
        locals={n:int(i) for n,i in re.findall(r'C243 LOCAL name=(\w+) symbol=(\d+)',proof.stdout.decode())}
        assert {'old','old_copy','during','beside'}<=locals.keys()
        header=raw/(name+'-observer.h');header.write_text(observer(meta,locals))
        file=raw/(name+'.c');file.write_bytes(emitted.stdout)
        for tag,cc in [('gcc',gcc)]+([] if gcc==clang else [('clang',clang)]):
            for opt in ['O0','O2']:
                for active in (False,True):
                    assert native_runs<48
                    label=name+'-'+tag+'-'+opt+('-observe' if active else '-plain');exe=raw/label
                    flags=['-std=c17','-Wall','-Wextra','-Wpedantic','-Werror','-UNDEBUG','-'+opt]
                    if active:flags+=['-include',str(header)]
                    if sanitizer!='none':flags+=['-fsanitize='+sanitizer,'-fno-sanitize-recover=all','-fno-omit-frame-pointer']
                    run([cc,*flags,file,'-o',exe],label+'-build')
                    result=run([exe],label+'-native');native_runs+=1;summary[name]['native_runs']+=1
                    assert not result.stderr
                    assert (b'C243 actual-original' in result.stdout)==active
    (raw/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
assert len(summary)==20
print('C243 finite Gate0 observations complete; native runs',native_runs)
