from pathlib import Path
import hashlib,json,subprocess,sys,tempfile,re
p=Path(__file__).resolve().parent
compiler,probe,gcc,clang,san,out=sys.argv[1:]
for n,h in json.loads((p/'freeze-v2.json').read_bytes()).items():assert hashlib.sha256((p/n).read_bytes()).hexdigest()==h,n
Path(out).mkdir(exist_ok=True,parents=True);raw=Path(tempfile.mkdtemp(prefix='c223-',dir=out));rows=[]
def run(cmd,label):
 r=subprocess.run(list(map(str,cmd)),capture_output=True,timeout=120);(raw/(label+'.out')).write_bytes(r.stdout);(raw/(label+'.err')).write_bytes(r.stderr);rows.append(dict(label=label,command=list(map(str,cmd)),status=r.returncode));(raw/'observations.json').write_text(json.dumps(rows,indent=2)+'\n');print(label,r.returncode,flush=True);return r
for name,e in json.loads((p/'oracle.json').read_bytes())['cases'].items():
 s=p/'sources'/(name+'.nl');r=run([probe,s],name+'-owned');assert not r.returncode and not r.stderr,r.stderr
 r=run([compiler,s],name+'-emit');assert not r.returncode and not r.stderr;c=r.stdout.decode()
 # Add only read-only callbacks around the real predicate input/return and
 # ordinary source Copy. No field writes, control or expected values injected.
 pattern=r'(static uint8_t nl_fn_\d+\(size_t (nl_local_\d+)\)\n\{\n)(.*?)(\n\})'
 def observe(m):
  body,n=re.subn(r'    return ([^;]+);',r'    const uint8_t c223_return = \1;\n    c223_tag(c223_return);\n    return c223_return;',m[3]);assert n==1
  return m[1]+'    c223_actual('+m[2]+');\n'+body+m[4]
 observed,n=re.subn(pattern,observe,c,flags=re.S);assert n==1
 observed,n=re.subn(r'(const uint8_t (nl_local_\d+) = nl_local_\d+;)',r'\1\n    c223_copy(\2);',observed);assert n==1
 for variant,text in [('raw',c),('observed',observed)]:
  f=raw/(name+'-'+variant+'.c');f.write_text(text)
  for cc in dict.fromkeys([gcc,clang]):
   for opt in ['-O0','-O2']:
    label=name+'-'+variant+'-'+Path(cc).name+opt;exe=raw/label;flags=['-std=c17','-Wall','-Wextra','-Wpedantic','-Werror',opt]
    if san!='none':flags+=['-fsanitize='+san,'-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    if variant=='observed':flags+=['-DNEWLANG_SCALAR_LOOP_OBSERVER="'+str(p/'observe.h')+'"','-DC223_TRACE={'+','.join('(size_t)UINTMAX_C('+str(x)+')' for x in e['trace'])+'}','-DC223_RESULT='+str(e['result']),'-DC223_TAG='+str(e['tag'])]
    r=run([cc,*flags,f,'-o',exe],label+'-build');assert not r.returncode,r.stderr
    r=run([exe],label+'-native');assert not r.returncode and not r.stderr,r.stderr
    if variant=='observed':assert b'runtime loop result/source Copy/actual/formal/decision/tag PASS' in r.stdout
    exe.unlink()
print('C223 source consumer finite cohort PASS')
