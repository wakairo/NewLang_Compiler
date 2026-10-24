"""Real Production source/issued views/unchanged generated C; passive observer.
Frozen numerical expectations are assertions only, never guest initialization.
"""
from pathlib import Path
import hashlib,json,os,re,subprocess,sys,tempfile
compiler,probe,cc,sanitizer,directory=sys.argv[1:]
r=Path(directory).resolve();f=json.loads((r/'freeze.json').read_text())
manifest={'oracle':{},'sha256':f['sha256']}
for name in f['sha256']:
 o=f['expectation'][Path(name).stem];roots=[]
 for i,t in enumerate(o['T']):
  update=next((u for u in o['updates'] if u[0]==i),None)
  roots.append({'initial':t,'index':update[1] if update else 0,'old':update[2] if update else t[0],'new':update[3] if update else t[0],'sibling':t[1-(update[1] if update else 0)]})
 manifest['oracle'][name]={'copy_calls':o['calls'],'H_payload':o['H'],'T_original0':roots[0],'T_original1':roots[1]}
def run(args,**kw):return subprocess.run(list(map(str,args)),capture_output=True,**kw)
def require(p,what):assert p.returncode==0,(what,p.returncode,p.stdout.decode(errors='replace'),p.stderr.decode(errors='replace'))
with tempfile.TemporaryDirectory(prefix='p778-native-') as temporary:
 tmp=Path(temporary)
 for name,oracle in manifest['oracle'].items():
  source=r/('external_r776' if name=='positive_h_first.nl' else 'sources')/name;assert hashlib.sha256(source.read_bytes()).hexdigest()==manifest['sha256'][name]
  checked=run([compiler,'--check',source]);require(checked,'CHECK');assert not checked.stdout and not checked.stderr
  emitted=run([compiler,source]);require(emitted,'C17');assert emitted.stdout and not emitted.stderr
  evidence=run([probe,source]);require(evidence,'actual owned source probe');text=evidence.stdout.decode()
  copy=[tuple(map(int,x)) for x in re.findall(r'index=(\d+).*old=(\d+) new=(\d+) sibling=(\d+) body_status=(\d+) parameters_status=(\d+)',text)]
  expected=[]
  for i in range(oracle['copy_calls']):
   o=oracle['T_original'+str(i)];expected.append((o['index'],o['old'],o['new'],o['sibling'],0,0))
  assert copy==expected,(copy,expected)
  assert 'original_valid=1 local_view=1 local_valid=1 body_same_caller_world=1 entry_distinct_return=1' in text
  assert 'same_ptr_root=1 same_allocation=1 same_domain=1' in text
  assert 'post_proved=1 entry_original_exact=1' in text
  assert 'NODE_BACKEND status=0' in text
  initialized=[tuple(map(int,x)) for x in re.findall(r'INITIALIZE .*root=(\d+) incarnation=(\d+) backing=(\d+) domain=(\d+)',text)]
  assert len(initialized)==2 and all(a!=b for a,b in zip(*initialized))
  local=int(re.search(r'PRODUCER .*local=(\d+) availability=1',text)[1])
  assert 'all_closed=0' not in text
  print(name,text.strip())
  cfile=tmp/(source.stem+'.c');cfile.write_bytes(emitted.stdout)
  target=tmp/'saved.c';target.write_bytes(b'old valid C sentinel')
  named=run([compiler,'--emit-c17-to',target,source]);require(named,'named C');assert target.read_bytes()==emitted.stdout
  flags=['-std=c17','-pedantic-errors','-Wall','-Wextra','-Werror']
  if sanitizer!='none':flags+=['-fsanitize='+sanitizer,'-fno-omit-frame-pointer','-fno-sanitize-recover=all']
  defs={}
  for i,(place,inc,backing,domain) in enumerate(initialized):
   prefix='H' if i==0 else 'T';defs.update({prefix+'P':place,prefix+'I':inc,prefix+'D':domain,prefix+'V':oracle['H_payload'][i]})
  projection=re.search(r'PROJECTION nominal=(\d+) child=(\d+) incarnation=(\d+)',text)
  assert projection
  defs.update(zip(['HN','HC','HCI'],map(int,projection.groups())))
  scopes=re.findall(r'REF_SCOPE place=\d+ scope=(\d+)',text)
  assert len(scopes)==4
  defs.update({'S'+str(i):int(v) for i,v in enumerate(scopes)})
  caller=int(re.search(r'CALLER_LOCAL .*symbol=(\d+) availability=1',text)[1])
  defs.update(OWNER_LOCAL=local,CALLER_LOCAL=caller,COPY_CALLS=oracle['copy_calls'])
  for i in range(2):
   o=oracle['T_original'+str(i)];final=o['initial'][:];final[o['index']]=o['new']
   for j in range(2):defs['T'+str(i)+str(j)]=o['initial'][j];defs['F'+str(i)+str(j)]=final[j]
   defs['OLD'+str(i)]=o['old'];defs['NEW'+str(i)]=o['new'];defs['INDEX'+str(i)]=o['index']
  for opt in ['-O0','-O2']:
   plain=tmp/(source.stem+opt+'plain');require(run([cc,*flags,opt,cfile,'-o',plain]),'strict unmodified C17 build')
   require(run([plain]),'unobserved actual native')
   observed=tmp/(source.stem+opt+'observed')
   args=[cc,*flags,opt,cfile,r.parent/'integration/two_heap_platform.c','-Wl,--wrap=malloc','-Wl,--wrap=free','-o',observed,
    '-DNEWLANG_NODE_OBSERVER="'+str(r/'observer_node.h')+'"','-DNEWLANG_HEAP_OBSERVER="'+str(r/'observer.h')+'"',
    *['-D'+k+'='+str(v) for k,v in defs.items()]]
   require(run(args),'passive observer build')
   for fail in ['1','2','0']:
    env=dict(os.environ,NEWLANG_TEST_FAIL_CALL=fail,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    guest=run([observed],env=env);require(guest,'observed native failure world '+fail)
    expected_alloc={'1':0,'2':1,'0':2}[fail]
    assert f'allocations={expected_alloc} frees={expected_alloc}' in guest.stdout.decode()
    print(json.dumps(dict(source=name,opt=opt,cc=cc,sanitizer=sanitizer,fail=fail,c_sha256=hashlib.sha256(emitted.stdout).hexdigest(),observed=guest.stdout.decode().strip())))
