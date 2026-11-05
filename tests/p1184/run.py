from pathlib import Path
import json,re,subprocess,sys

p=Path(__file__).resolve().parent
compiler,probe,gcc,clang,sanitizer,output=sys.argv[1:]
oracle=json.loads((p/'oracle.json').read_text())
version={'source_overrides':{},'expected_diagnostics':{}}
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
    expected={'old':'c245_olds[c245_phases-1]','old_copy':'c245_olds[c245_phases-1]',
              'incoming':'c245_incoming[c245_phases-1]', 'chosen':'c245_incoming[c245_phases-1]', 'forwarded':'c245_incoming[c245_phases-1]', 'during':'c245_incoming[c245_phases-1]',
              'beside':str(meta['sibling'])}
    cases='\n'.join('case '+str(symbols[n])+': assert(v=='+expected[n]+'); ++c245_locals['+str(i)+']; break;' for i,n in enumerate(symbols))
    return '''#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
static uint8_t *c245_a,*c245_b;
static size_t c245_phases,c245_results,c245_reads;
static size_t c245_locals['''+str(len(symbols))+'''];
static const size_t c245_headers[]={'''+array(meta['headers'])+'''};
static const size_t c245_header_child[]={'''+array(meta['header_child'])+'''};
static const size_t c245_olds[]={'''+array(meta['olds'])+'''};
static const size_t c245_incoming[]={'''+array(meta['incoming'])+'''};
static const size_t c245_expected_calls='''+str(len(meta['incoming']))+''';
static size_t c245_calls,c245_returns;
static void c245_helper_input(size_t phase){
 assert(c245_calls==c245_returns && c245_calls+1==c245_phases && c245_calls!=c245_expected_calls);
 assert(phase==c245_headers[c245_calls]);
 assert(*(('''+str(meta['index'])+''')?c245_b:c245_a)==c245_header_child[c245_calls]);++c245_calls;
}
static uint8_t c245_helper_output(uint8_t value){
 assert(c245_returns+1==c245_calls && value==c245_incoming[c245_returns]);
 assert(*(('''+str(meta['index'])+''')?c245_b:c245_a)==c245_header_child[c245_returns]);
 printf("C245 actual-helper phase=%zu result=%u\\n",c245_headers[c245_returns],(unsigned)value);++c245_returns;return value;
}

static void c245_finish(void){
 assert(c245_calls==c245_expected_calls && c245_returns==c245_calls);
 assert(c245_phases=='''+str(len(meta['headers']))+''' && c245_results==1 && c245_reads==2);
 for(size_t i=0;i<sizeof c245_locals/sizeof c245_locals[0];++i)assert(c245_locals[i]=='''+str(len(meta['olds']))+''');
 puts("C245 actual-original/phase/old/current/sibling/result PASS");
}
static void c245_bind(uint8_t *a,uint8_t *b){
 assert(!c245_a && !c245_b && a!=b && *a=='''+str(fields[0])+''' && *b=='''+str(fields[1])+''');
 c245_a=a;c245_b=b;assert(atexit(c245_finish)==0);
}
static void c245_phase(size_t n,uint8_t *a,uint8_t *b){
 assert(a==c245_a && b==c245_b && c245_phases<sizeof c245_headers/sizeof c245_headers[0]);
 assert(n==c245_headers[c245_phases]);
 assert(*(('''+str(meta['index'])+''')?b:a)==c245_header_child[c245_phases]);
 assert(*(('''+str(meta['index'])+''')?a:b)=='''+str(meta['sibling'])+''');++c245_phases;
}
static void c245_local(size_t symbol,uint8_t v){
 assert(c245_phases && c245_phases<=sizeof c245_olds/sizeof c245_olds[0]);
 switch(symbol){'''+cases+''' default: assert(0);}
}
static void c245_result(size_t n){
 assert(n==((size_t)UINTMAX_C('''+str(meta['result'])+''')));
 assert(*c245_a=='''+str(final[0])+''' && *c245_b=='''+str(final[1])+''');++c245_results;
}
static void c245_read(uint8_t *a,uint8_t v){
 assert((a==c245_a || a==c245_b) && v==*a);
 assert(v==(a==c245_a?'''+str(final[0])+''':'''+str(final[1])+'''));++c245_reads;
}
#define NL_LOOP_BIND(a,b) c245_bind(a,b)
#define NL_CURRENT_PHASE(n,a,b) c245_phase(n,a,b)
#define NL_CURRENT_LOCAL(s,v) c245_local(s,v)
#define NL_CURRENT_RESULT(n) c245_result(n)
#define NL_LOOP_READ(a,v) c245_read(a,v)
'''

summary={}
for name,meta in oracle['cases'].items():
    src=p/version['source_overrides'].get(name,'sources/'+name+'.nl');category=meta['category']
    check=run([compiler,'--check',src],name+'-check',(meta['check'],))
    if category=='negative' and check.returncode==0:raise AssertionError(('STOP negative CHECK accepted',name))
    if name in version['expected_diagnostics']:assert version['expected_diagnostics'][name].encode() in check.stderr
    proof=None
    if category=='positive':proof=run([probe,src],name+'-owned-trace')
    emitted=run([compiler,src],name+'-emit',(meta['emit'],))
    target=raw/(name+'-named.c');sentinel=b'C245 named-output sentinel\n';target.write_bytes(sentinel);before=target.stat()
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
        assert check.returncode==0 and 'C245 public-owned=YES private-current=0' in proof.stdout.decode()
        locals={n:int(i) for n,i in re.findall(r'C245 LOCAL name=(\w+) symbol=(\d+)',proof.stdout.decode())}
        assert {'old','old_copy','during','beside'}<=locals.keys()
        header=raw/(name+'-observer.h');header.write_text(observer(meta,locals))
        file=raw/(name+'.c');file.write_bytes(emitted.stdout)
        pattern=r'(static uint8_t nl_fn_\d+\((?:const )?size_t (nl_local_\d+)\)\n\{\n)(.*?)(^\})'
        def passive(m):
            body,count=re.subn(r'\breturn ([^;\n]+);',r'return c245_helper_output(\1);',m[3])
            assert count==1
            return m[1]+'    c245_helper_input('+m[2]+');\n'+body+m[4]
        text,count=re.subn(pattern,passive,emitted.stdout.decode(),flags=re.S|re.M)
        assert count==1
        observed_file=raw/(name+'-helper-observed.c');observed_file.write_text(text)

        for tag,cc in [('gcc',gcc)]+([] if gcc==clang else [('clang',clang)]):
            for opt in ['O0','O2']:
                for active in (False,True):
                    assert native_runs<48
                    label=name+'-'+tag+'-'+opt+('-observe' if active else '-plain');exe=raw/label
                    flags=['-std=c17','-Wall','-Wextra','-Wpedantic','-Werror','-UNDEBUG','-'+opt]
                    if active:flags+=['-include',str(header)]
                    if sanitizer!='none':flags+=['-fsanitize='+sanitizer,'-fno-sanitize-recover=all','-fno-omit-frame-pointer']
                    run([cc,*flags,observed_file if active else file,'-o',exe],label+'-build')
                    result=run([exe],label+'-native');native_runs+=1;summary[name]['native_runs']+=1
                    assert not result.stderr
                    assert (b'C245 actual-original' in result.stdout)==active
                    assert result.stdout.count(b'C245 actual-helper')==(len(meta['incoming']) if active else 0)
                    exe.unlink()
    (raw/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
assert len(summary)==20
print('C245 finite Gate0 observations complete; native runs',native_runs)
