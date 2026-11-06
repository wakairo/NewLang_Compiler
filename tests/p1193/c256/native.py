"""Finite C256 native cohort; every real proof/structural gate precedes guest."""
from pathlib import Path
import hashlib,json,os,re,subprocess,sys
from mapping import roles,inspect
from native_observer import observer
here=Path(__file__).resolve().parent
repo=here.parents[2]
compiler,probe,gcc,clang,destination=sys.argv[1:]
raw=Path(destination);raw.mkdir(parents=True,exist_ok=False)
freeze=json.loads((here/'native-freeze-v1.json').read_text())
for name,digest in freeze['files'].items():assert hashlib.sha256((repo/name).read_bytes()).hexdigest()==digest,name
oracle=json.loads((here/'native-oracle-v1.json').read_text())
rows=[];native_count=0;accepted=[]
def run(args,label):
 args=list(map(str,args));r=subprocess.run(args,capture_output=True,timeout=120,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=1:abort_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'})
 (raw/(label+'.out')).write_bytes(r.stdout);(raw/(label+'.err')).write_bytes(r.stderr)
 rows.append(dict(label=label,status=r.returncode,args=args))
 (raw/'observations.json').write_text(json.dumps(rows,indent=2)+'\n')
 assert r.returncode==0,rows[-1]
 return r
for name,meta in oracle['cases'].items():
 source=here/'sources'/(name+'.nl')
 run([compiler,'--check',source],name+'-check')
 proof=run([probe,source],name+'-roles').stdout.decode()
 text=run([compiler,source],name+'-emit').stdout.decode()
 view=roles(proof);assert view['index']==meta['index']
 mapped=inspect(text,view,meta)
 h=re.search(r'C256 HELPER formal=(\d+) selection=(\d+) normal=(\d+) returns=(\d+)',proof);assert h
 formal,selection,normal,exits=map(int,h.groups())
 assert (normal,exits)==(meta['normal_arms'],meta['return_edges'])
 assert [int(v) for _,v in re.findall(r'C256 LITERAL arm=(\d+) value=(\d+)',proof)]==meta['arms']
 # Validate the actual dynamic helper argument; phase and final caller result
 # share a semantic symbol but are distinct C carriers in different worlds.
 byname={v['name']:s for s,v in view['locals'].items()}
 assert set(byname)==set(meta['locals'])
 phase=f"nl_loop_phase_{view['loop']}";helper=f"nl_fn_{view['helper']}"
 if 'actual' in byname:
  arg=f"nl_local_{byname['actual']}"
  assert f'const size_t {arg} = {phase};' in mapped['body']
 else:arg=phase
 incoming=f"nl_local_{byname['incoming']}"
 assert f'const uint8_t {incoming} = {helper}({arg});' in mapped['body']
 assert len(re.findall(r'\*nl_local_\d+ = '+incoming+r';',mapped['body']))==1
 pattern=r'(static uint8_t '+helper+r'\(size_t (nl_local_\d+)\)\n\{\n)(.*?)(^\})'
 match=re.search(pattern,text,flags=re.S|re.M);assert match
 assert match[2]==f'nl_local_{formal}'
 initialized={match[2]}
 for line in match[3].splitlines():
  d=re.match(r'\s*const (?:size_t|uint8_t) (nl_\w+) = ([^;]+);',line)
  expression=d[2] if d else line
  used=set(re.findall(r'\bnl_(?:local|less_left|less_right)_\d+\b',expression))
  assert used<=initialized,('helper uninitialized/wrong role',line,used,initialized)
  if d:initialized.add(d[1])
 assert match[3].count('return ')==1
 assert re.search(r'\breturn \(\(nl_less_left_\d+ '+re.escape(meta['guard'])+r' nl_less_right_\d+\) \? '+str(meta['arms'][0])+r' : '+str(meta['arms'][1])+r'\);',match[3])
 assert f'((size_t)UINTMAX_C({meta["guard_literal"]}))' in match[3]
 (raw/(name+'-structural.json')).write_text(json.dumps(dict(roles=view,mapping=mapped,helper_formal=formal,helper_exits=exits),indent=2)+'\n')
 header=raw/(name+'-observer.h');header.write_text(observer(meta,view,mapped))
 def passive(m):
  body,n=re.subn(r'\breturn ([^;\n]+);',r'return helper_return(\1);',m[3]);assert n==1
  return m[1]+'    helper_input('+m[2]+');\n'+body+m[4]
 observed,n=re.subn(pattern,passive,text,flags=re.S|re.M);assert n==1
 plain=raw/(name+'.c');plain.write_text(text)
 instrumented=raw/(name+'-passive.c');instrumented.write_text(observed)
 accepted.append((name,plain,instrumented,header))
print('C256 all5 CHECK/owned-current/exit/role/initialization gates PASS; native0',flush=True)
for name,plain,instrumented,header in accepted:
 for tag,cc in [('gcc',gcc),('clang',clang)]:
  for opt in ['O0','O2']:
   for active in [False,True]:
    assert native_count<40
    label=name+'-'+tag+'-'+opt+('-passive' if active else '-raw');exe=raw/label
    flags=['-std=c17','-Wall','-Wextra','-Wpedantic','-Werror','-UNDEBUG','-'+opt]
    if active:flags+=['-include',header]
    if tag=='clang':flags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
    run([cc,*flags,instrumented if active else plain,'-o',exe],label+'-build')
    result=run([exe],label+'-native');native_count+=1
    (raw/'native-count.json').write_text(json.dumps(dict(native=native_count,ceiling=40))+'\n')
    assert not result.stderr
    assert (b'C256 physical-original/old/current/helper/phase PASS' in result.stdout)==active
    exe.unlink()
 print(name,'strict raw/passive native PASS',flush=True)
assert native_count==40
print('C256 strict GCC/Clang O0/O2 raw/passive native PASS40; Clang20 with ASan/LSan/UBSan')
