"""C238 bounded independent source/current/native/no-clobber cohort."""
from pathlib import Path
import hashlib,json,re,subprocess,sys

p=Path(__file__).resolve().parent
compiler,probe,gcc,clang,sanitizer,output=sys.argv[1:]
raw=Path(output);raw.mkdir(exist_ok=False,parents=True)
oracle=json.loads((p/'oracle.json').read_text())
rows=[]

def run(command,label,expected=0):
    command=list(map(str,command))
    r=subprocess.run(command,capture_output=True,timeout=90)
    (raw/(label+'.out')).write_bytes(r.stdout)
    (raw/(label+'.err')).write_bytes(r.stderr)
    rows.append(dict(label=label,command=command,status=r.returncode))
    (raw/'observations.json').write_text(json.dumps(rows,indent=2)+'\n')
    print(label,r.returncode,flush=True)
    if r.returncode!=expected:
        (raw/'FIRST.json').write_text(json.dumps(rows[-1],indent=2)+'\n')
        raise AssertionError((label,r.returncode,r.stderr.decode()))
    return r

def observer(meta, symbols):
    headers=','.join('SIZE_C('+str(x)+')' for x in meta['headers'])
    olds=','.join(map(str,meta['old_values'])) or '0'
    roles={name:(1 if name in ('old','old_copy','alias') else 2 if name=='during' else 3)
           for name in symbols}
    cases='\n'.join('case '+str(symbols[name])+': assert(value=='+
        ('olds[phases-1]' if roles[name]==1 else str(meta['iteration_current_values'][0])
         if roles[name]==2 and meta['iteration_current_values'] else str(meta['final_child'])
         if roles[name]==2 else str(meta['sibling']))+'); ++locals['+str(i)+']; break;'
        for i,name in enumerate(symbols))
    initial=meta['old_values'][0] if meta['old_values'] else meta['final_child']
    fields=[initial,meta['sibling']] if meta['target_index']==0 else [meta['sibling'],initial]
    final=[meta['final_child'],meta['sibling']] if meta['target_index']==0 else [meta['sibling'],meta['final_child']]
    header_child=('(phases<'+str(meta['backedges'])+'?olds[phases]:'+str(meta['final_child'])+')'
                  if meta['backedges'] else str(meta['final_child']))
    return '''#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#define SIZE_C(n) ((size_t)UINTMAX_C(n))
static uint8_t *a0,*a1;
static size_t phases,results,reads;
static size_t locals['''+str(len(symbols))+'''];
static const size_t headers[]={'''+headers+'''};
static const uint8_t olds[]={'''+olds+'''};
static void finish(void) {
 assert(phases=='''+str(len(meta['headers']))+''' && results==1 && reads==2);
 for(size_t i=0;i<sizeof locals/sizeof locals[0];++i) assert(locals[i]=='''+str(meta['backedges'])+''');
 puts("C238 original-address/old-copy/current/sibling/phase/result PASS");
}
static void bind(uint8_t *a,uint8_t *b) {
 assert(!a0 && !a1 && a!=b && *a=='''+str(fields[0])+''' && *b=='''+str(fields[1])+''');
 a0=a; a1=b; assert(atexit(finish)==0);
}
static void phase(size_t n,uint8_t *a,uint8_t *b) {
 assert(a==a0 && b==a1 && phases<sizeof headers/sizeof headers[0]);
 assert(n==headers[phases]);
 assert(*(('''+str(meta['target_index'])+''')?b:a)=='''+header_child+''');
 assert(*(('''+str(meta['target_index'])+''')?a:b)=='''+str(meta['sibling'])+'''); ++phases;
}
static void local(size_t symbol,uint8_t value) {
 assert(phases && phases<='''+str(meta['backedges'])+''');
 switch(symbol){'''+cases+''' default: assert(0); }
}
static void result(size_t n) {
 assert(n==SIZE_C('''+str(meta['loop_result'])+'''));
 assert(*a0=='''+str(final[0])+''' && *a1=='''+str(final[1])+'''); ++results;
}
static void read_value(uint8_t *a,uint8_t v) {
 assert((a==a0 || a==a1) && v==*a);
 assert(v==(a==a0?'''+str(final[0])+''':'''+str(final[1])+''')); ++reads;
}
#define NL_LOOP_BIND(a,b) bind(a,b)
#define NL_LOOP_READ(a,v) read_value(a,v)
#define NL_CURRENT_PHASE(n,a,b) phase(n,a,b)
#define NL_CURRENT_LOCAL(s,v) local(s,v)
#define NL_CURRENT_RESULT(n) result(n)
'''

for name,meta in oracle['cases'].items():
    src=p/'sources'/(name+'.nl')
    cat=meta['category'];expected=3 if cat=='negative' else 0
    check=run([compiler,'--check',src],name+'-check',expected)
    emitted=run([compiler,src],name+'-emit',expected)
    directory=raw/(name+'-named');directory.mkdir()
    target=directory/'out.c';sentinel=b'C238 named-output sentinel\n'
    target.write_bytes(sentinel);before=target.stat()
    run([compiler,'--emit-c17-to',target,src],name+'-named',expected)
    assert sorted(x.name for x in directory.iterdir())==['out.c']
    if expected:
        assert not check.stdout and not emitted.stdout
        assert target.read_bytes()==sentinel
        assert (target.stat().st_ino,target.stat().st_mtime_ns)==(before.st_ino,before.st_mtime_ns)
        continue
    assert not check.stdout and not check.stderr and not emitted.stderr
    assert target.read_bytes()==emitted.stdout
    if cat=='control':
        assert emitted.stdout==(p/'evidence'/(name+'.c17')).read_bytes()
        continue
    owned=run([probe,src],name+'-current')
    symbols={n:int(s) for n,s in re.findall(r'C238 LOCAL name=(\w+) symbol=(\d+)',owned.stdout.decode())}
    assert {'old','old_copy','during','beside'}<=symbols.keys()
    header=raw/(name+'-observer.h');header.write_text(observer(meta,symbols))
    cfile=raw/(name+'.c');cfile.write_bytes(emitted.stdout)
    compilers=[('gcc',gcc)] + ([] if gcc==clang else [('clang',clang)])
    for tag,cc in compilers:
        for opt in ('O0','O2'):
            for active in (False,True):
                label=name+'-'+tag+'-'+opt+('-observe' if active else '-plain')
                flags=['-std=c17','-Wall','-Wextra','-Wpedantic','-Werror','-UNDEBUG','-'+opt]
                if active:flags+=['-include',str(header)]
                if sanitizer!='none':flags+=['-fsanitize='+sanitizer,'-fno-sanitize-recover=all','-fno-omit-frame-pointer']
                exe=raw/label
                run([cc,*flags,cfile,'-o',exe],label+'-build')
                native=run([exe],label+'-native')
                assert not native.stderr
                assert (b'C238 original-address' in native.stdout)==active
print('C238 bounded source/current/native/no-clobber cohort PASS',flush=True)
