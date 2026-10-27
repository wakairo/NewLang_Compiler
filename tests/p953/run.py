"""Ordinary CLI bytes, unchanged strict C17, passive original observation."""
from pathlib import Path
import hashlib,json,os,resource,signal,subprocess,sys,tempfile
cli,cc,sanitizer,directory=sys.argv[1:];root=Path(directory)
def run(args,**kw):return subprocess.run(list(map(str,args)),capture_output=True,**kw)
def ok(r):assert r.returncode==0,(r.returncode,r.stdout,r.stderr)
def quota():
 signal.signal(signal.SIGXFSZ,signal.SIG_IGN);resource.setrlimit(resource.RLIMIT_FSIZE,(128,128))
with tempfile.TemporaryDirectory(prefix='p953-') as temp:
 temp=Path(temp)
 for file,entry in json.loads((root/'freeze.json').read_text())['cases'].items():
  source=root/'sources'/file;assert hashlib.sha256(source.read_bytes()).hexdigest()==entry['sha256']
  a=run([cli,'--check',source]);b=run([cli,source]);out=temp/'output.c';sentinel=b'P953 previous valid output\n';out.write_bytes(sentinel);st=out.stat();c=run([cli,'--emit-c17-to',out,source])
  if file in ['wrong_domain.nl','third_original.nl']:
   assert (a.returncode,b.returncode,c.returncode)==((3,3,3) if file=='wrong_domain.nl' else (0,4,4))
   assert not b.stdout and not c.stdout and out.read_bytes()==sentinel and (st.st_dev,st.st_ino)==(out.stat().st_dev,out.stat().st_ino)
   assert b'P193-CALL-DOMAIN' in a.stderr if file=='wrong_domain.nl' else b'V1-BACKEND-UNSUPPORTED' in b.stderr
   print('NEGATIVE',file,a.returncode,b.returncode,b.stderr.decode().strip());continue
  for r in [a,b,c]:ok(r);assert not r.stderr
  assert out.read_bytes()==b.stdout and b.stdout
  path=temp/(file+'.c');path.write_bytes(b.stdout)
  own=file=='window_true.nl'
  expected={'CASE_A':0,'EXTRA':0,'T00':155 if own else 61,'T01':73 if own else 189,'T10':17 if own else 224,'T11':209 if own else 11,'I0':0 if own else 1,'I1':1 if own else 0,'NEXT':128 if own else 137,'HV0':41 if own else 59,'HV1':197 if own else 233}
  flags=['-std=c17','-Wall','-Wextra','-Wpedantic','-Werror']
  if sanitizer!='none':flags+=['-fsanitize='+sanitizer,'-fno-omit-frame-pointer','-fno-sanitize-recover=all']
  for opt in ['-O0','-O2']:
   raw=temp/'raw';ok(run([cc,*flags,opt,path,'-o',raw]));r=run([raw]);ok(r);assert not r.stdout and not r.stderr
   observed=temp/'observed';ok(run([cc,*flags,opt,path,root.parent/'integration/two_heap_platform.c','-Wl,--wrap=malloc','-Wl,--wrap=free','-o',observed,'-DNEWLANG_HEAP_OBSERVER="'+str((root/'observer.h').resolve())+'"',*['-D'+k+'='+str(v) for k,v in expected.items()]]))
   for fail in ['1','2','0']:
    r=run([observed],env=dict(os.environ,NEWLANG_TEST_FAIL_CALL=fail,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'));ok(r);assert not r.stderr
    count={'1':0,'2':1,'0':2}[fail];assert f'allocations={count} frees={count}' in r.stdout.decode()
    print(json.dumps({'source':file,'C_sha256':hashlib.sha256(b.stdout).hexdigest(),'cc':cc,'opt':opt,'sanitizer':sanitizer,'allocation_successes':count,'observation':r.stdout.decode().strip()}))
  out.write_bytes(sentinel);st=out.stat();r=run([cli,'--emit-c17-to',out,source],preexec_fn=quota)
  assert r.returncode==1 and b'OUTPUT' in r.stderr and not r.stdout
  assert out.read_bytes()==sentinel and (st.st_dev,st.st_ino)==(out.stat().st_dev,out.stat().st_ino) and not list(temp.glob('*.tmp*'))
  ok(run([cli,'--emit-c17-to',out,source]));assert out.read_bytes()==b.stdout
  print('REACHED FILE WRITE/FLUSH FAILURE + RETRY',file)
