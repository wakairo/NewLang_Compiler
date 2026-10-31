"""Same frozen complete programs; raw C and passive live-carrier observation."""
from pathlib import Path
import hashlib,json,os,resource,signal,subprocess,sys,tempfile
cli,cc,sanitizer,directory=sys.argv[1:];root=Path(directory);freeze=json.loads((root/'freeze.json').read_text())
def run(args,**kw):return subprocess.run(list(map(str,args)),capture_output=True,**kw)
def ok(p,what):assert p.returncode==0,(what,p.returncode,p.stdout.decode(errors='replace'),p.stderr.decode(errors='replace'))
def quota():
 signal.signal(signal.SIGXFSZ,signal.SIG_IGN);resource.setrlimit(resource.RLIMIT_FSIZE,(128,128))
with tempfile.TemporaryDirectory(prefix='p945-') as name:
 temp=Path(name)
 for file,digest in freeze['sha256'].items():
  source=root/'sources'/file;assert hashlib.sha256(source.read_bytes()).hexdigest()==digest
  positive=file in ['dispatch_journal_r1.nl','thermal_recovery_r1.nl'] or file in freeze['controls']
  check=run([cli,'--check',source]);emitted=run([cli,source]);target=temp/'named.c';sentinel=b'P945 original output sentinel\n';target.write_bytes(sentinel);before=target.stat();named=run([cli,'--emit-c17-to',target,source])
  if not positive:
   assert check.returncode in ([0,3] if file=='unsupported_arm_effect.nl' else [3]),(file,check.stderr)
   assert emitted.returncode in [3,4] and not emitted.stdout and b'error(internal)' not in emitted.stderr
   assert named.returncode in [3,4] and not named.stdout and target.read_bytes()==sentinel
   assert (before.st_dev,before.st_ino)==(target.stat().st_dev,target.stat().st_ino)
   print('NEGATIVE',file,'CHECK',check.returncode,emitted.stderr.decode().strip());continue
  ok(check,'CHECK');ok(emitted,'C17');ok(named,'named');assert emitted.stdout and not emitted.stderr and target.read_bytes()==emitted.stdout
  c=temp/(file+'.c');c.write_bytes(emitted.stdout);case_a=file.startswith('dispatch');extra=int('optional' in file)
  defs={'CASE_A':int(case_a),'EXTRA':extra,'T00':39 if case_a else 43,'T01':122 if case_a else 184,'T10':15 if case_a else 91,'T11':207 if case_a else 126,'I0':0 if case_a else 1,'I1':1 if case_a else 0,'NEXT':166 if case_a else 52,'HV0':65 if case_a else 74,'HV1':193 if case_a else 218}
  flags=['-std=c17','-Wall','-Wextra','-Wpedantic','-Werror']
  if sanitizer!='none':flags+=['-fsanitize='+sanitizer,'-fno-omit-frame-pointer','-fno-sanitize-recover=all']
  for opt in ['-O0','-O2']:
   plain=temp/'raw';ok(run([cc,*flags,opt,c,'-o',plain]),'raw strict C17');guest=run([plain]);ok(guest,'raw native');assert not guest.stdout and not guest.stderr
   observed=temp/'passive';args=[cc,*flags,opt,c,root.parent/'integration/two_heap_platform.c','-Wl,--wrap=malloc','-Wl,--wrap=free','-o',observed,'-DNEWLANG_HEAP_OBSERVER="'+str((root/'observer.h').resolve())+'"',*['-D'+k+'='+str(v) for k,v in defs.items()]]
   ok(run(args),'passive strict C17')
   for fail in ['1','2','0']:
    guest=run([observed],env=dict(os.environ,NEWLANG_TEST_FAIL_CALL=fail,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1'));ok(guest,'passive native')
    allocated={'1':0,'2':1,'0':2}[fail];assert f'allocations={allocated} frees={allocated}' in guest.stdout.decode();assert not guest.stderr
    print(json.dumps({'source':file,'C_sha256':hashlib.sha256(emitted.stdout).hexdigest(),'cc':cc,'opt':opt,'sanitizer':sanitizer,'fail_call':fail,'observation':guest.stdout.decode().strip()}))
  # A real regular-file output size failure, not a no-hit allocation mock.
  target.write_bytes(sentinel);before=target.stat();fault=run([cli,'--emit-c17-to',target,source],preexec_fn=quota)
  assert fault.returncode==1 and not fault.stdout and b'OUTPUT' in fault.stderr,(file,fault.returncode,fault.stderr)
  assert target.read_bytes()==sentinel and (before.st_dev,before.st_ino)==(target.stat().st_dev,target.stat().st_ino)
  assert not list(temp.glob('*.tmp*'))
  retry=run([cli,'--emit-c17-to',target,source]);ok(retry,'same source retry');assert target.read_bytes()==emitted.stdout
  print('REAL FILE WRITE/FLUSH FAULT + SAME SOURCE RETRY PASS',file)
