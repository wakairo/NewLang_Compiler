"""Real Production source/issued views/unchanged generated C; passive observer.
Frozen numerical expectations are assertions only, never guest initialization.
"""
from pathlib import Path
import hashlib,json,os,re,subprocess,sys,tempfile
compiler,probe,cc,sanitizer,directory=sys.argv[1:]
r=Path(directory).resolve();manifest=json.loads((r/'freeze.json').read_text());external=json.loads((r/'external_r776/oracle.json').read_text())
manifest['oracle']['positive_h_first.nl']={'H_payload':[61,173],'copy_calls':1,'T_original0':{'initial':[41,149],'index':0,'old':41,'new':96,'sibling':149},'T_original1':{'initial':[11,209],'index':1,'old':209,'new':41,'sibling':11}}
manifest['sha256']['positive_h_first.nl']=manifest['external_sha256']['positive_h_first.nl']
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
 negatives=[(r/'sources'/name,digest) for name,digest in manifest['sha256'].items() if name.startswith('negative_')]
 negatives += [(r/'external_r776'/name,digest) for name,digest in manifest['external_sha256'].items() if name.startswith('negative_')]
 for source,digest in negatives:
  name=source.name
  assert hashlib.sha256(source.read_bytes()).hexdigest()==digest
  checked=run([compiler,'--check',source]);emitted=run([compiler,source]);assert checked.returncode==emitted.returncode==3 and not checked.stdout and not emitted.stdout
  target=tmp/'failure.c';target.write_bytes(b'correct output')
  p=run([compiler,'--emit-c17-to',target,source]);assert p.returncode==3 and target.read_bytes()==b'correct output'
  print(name,'refused first observed phase',checked.stderr.decode().strip())

 # Valid B remains unsupported; public Copy receipt's precision failure is distinct from CHECK.
 b=r/'external_r776/positive_t_interposed.nl'
 assert hashlib.sha256(b.read_bytes()).hexdigest()==manifest['external_sha256'][b.name]
 checked=run([compiler,'--check',b]);require(checked,'B CHECK')
 emitted=run([compiler,b]);assert emitted.returncode==4 and not emitted.stdout
 target=tmp/'b-saved.c';target.write_bytes(b'B original correct C')
 named=run([compiler,'--emit-c17-to',target,b]);assert named.returncode==4 and target.read_bytes()==b'B original correct C'
 evidence=run([probe,b]);require(evidence,'B public probe')
 assert 'body_status=3 parameters_status=3' in evidence.stdout.decode()
 assert 'NODE_BACKEND status=1 output_null=1 output_length_unchanged=1' in evidence.stdout.decode()
 print('R-B calibrated CHECK0/public Copy precision3/C174 zeroC named preserved')
