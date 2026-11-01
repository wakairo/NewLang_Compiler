"""P1079 existing IF-field current receipt -> actual old Copy and common join."""
from pathlib import Path
import hashlib,json,os,re,resource,signal,subprocess,sys,tempfile
cli,cc,sanitizer,selection=sys.argv[1:]
cli=Path(cli).resolve();data=Path(__file__).resolve().parent;repo=data.parents[1]
sha=lambda b:hashlib.sha256(b).hexdigest()
for name in ('source-freeze-v1.json','source-freeze-v2.json','source-freeze-v3.json','source-freeze-v4.json','harness-freeze-v1.json','harness-freeze-v2.json','harness-freeze-v3.json','harness-freeze-v4.json'):
 for p,d in json.loads((data/name).read_bytes())['files'].items():assert sha((repo/p).read_bytes())==d,p
oracle=json.loads((data/'oracle-v4.json').read_bytes());profile=oracle['positive'][int(selection)]
resource.setrlimit(resource.RLIMIT_CORE,(0,0))
archive=None
if os.environ.get('P1079_EVIDENCE_DIR'):
 Path(os.environ['P1079_EVIDENCE_DIR']).mkdir(parents=True,exist_ok=True)
 archive=Path(tempfile.mkdtemp(prefix=profile['name']+'-',dir=os.environ['P1079_EVIDENCE_DIR']))
commands=[]
def run(label,args,**kw):
 args=list(map(str,args));r=subprocess.run(args,capture_output=True,timeout=180,**kw)
 commands.append(dict(label=label,command=args,exit=r.returncode,stdout_SHA256=sha(r.stdout),stderr_SHA256=sha(r.stderr)))
 if archive:
  (archive/(label+'.stdout')).write_bytes(r.stdout);(archive/(label+'.stderr')).write_bytes(r.stderr)
  (archive/'commands-FIRST.json').write_text(json.dumps(commands,indent=2)+'\n')
 assert r.returncode>=0,('unsafe signal STOP',label,r.returncode,r.stderr)
 return r
def ok(r):assert r.returncode==0 and not r.stderr,(r.returncode,r.stdout,r.stderr)
def preserved(p,s,b):
 a=p.stat();assert p.read_bytes()==s and (a.st_dev,a.st_ino)==(b.st_dev,b.st_ino)
 assert not list(p.parent.glob('*.tmp*'))
def quota():
 signal.signal(signal.SIGXFSZ,signal.SIG_IGN);resource.setrlimit(resource.RLIMIT_FSIZE,(128,128))
flags=['-std=c17','-Wall','-Wextra','-Werror','-pedantic-errors','-fno-pie','-no-pie']
if sanitizer!='none':flags+=['-fsanitize='+sanitizer,'-fno-omit-frame-pointer','-fno-sanitize-recover=all']
with tempfile.TemporaryDirectory(prefix='p1079-') as directory:
 temp=Path(directory)
 libraries=[cli.parent/('libnewlang_'+n+'.a') for n in ('semantic','parser','checked_c_node','lexer','checked','syntax','source','diagnostic')]
 for tool,path in [('observe',data/'observe-current.c'),('mutants',data/'if_mutants.c'),('consumer',data/'consumer-v2.c'),('oom',repo/'tests/p1062/oom.c')]:
  wraps=['-Wl,--wrap='+n for n in ('malloc','calloc','realloc')] if tool=='oom' else []
  ok(run('build-'+tool,[cc,*flags,'-g','-O0','-I'+str(repo/'include'),path,'-Wl,--start-group',*libraries,'-Wl,--end-group',*wraps,'-o',temp/tool]))
 source=repo/profile['source'];ok(run('CHECK',[cli,'--check',source]))
 current=run('current',[temp/'observe',source]);ok(current)
 rows=[json.loads(l) for l in current.stdout.splitlines() if l.startswith(b'{')]
 binding={r['name']:r for r in rows if r['kind']=='binding'}
 events=[r for r in rows if r['kind']=='event'];ifs=[r for r in rows if r['kind']=='IF']
 assert len(ifs)==1 and ifs[0]['current']==0 and not ifs[0]['common_result_known']
 assert [(e['old'],e['next'],e['current']) for e in events]==[(e['old'],e['next'],0) for e in profile['IF_changes_by_arm']]
 assert [binding[k]['value'] for k in ('old_primary','old_secondary')]==profile['helper_old']
 assert not binding['selected_old']['known'] and binding['last_old']['known'] and binding['last_old']['value']==profile['copy_results'][3]
 print(current.stdout.decode(),end='',flush=True)
 emitted=run('C17',[cli,source]);ok(emitted);assert emitted.stdout
 c=temp/'main.c';c.write_bytes(emitted.stdout)
 if archive:(archive/'main.c.data').write_bytes(emitted.stdout)
 if profile['control']:
  control=run('straight-control',[cli,repo/profile['control']]);ok(control)
  parent=json.loads((data/'control-C-parent-v1.json').read_bytes())[Path(profile['control']).stem]
  assert len(control.stdout)==parent['bytes'] and sha(control.stdout)==parent['SHA256']
 if selection=='0':
  negatives=[(repo/n['source'],3) for n in oracle['negative'].values()]
  negatives +=[(data/'sources-v2'/('if_'+n+'.nl'),4) for n in ('true','false')]
  negatives +=[(data/'sources'/'alternative_loop.nl',3)]
  for path,status in negatives:
   r=run(path.parent.name+'-'+path.stem+'-CHECK',[cli,'--check',path])
   assert r.returncode==(0 if status==4 else 3) and not r.stdout
   named=temp/'negative.c';sentinel=b'P1079 previous complete output\n';named.write_bytes(sentinel);before=named.stat()
   for mode in ('stdout','fresh','existing'):
    target=temp/'never.c' if mode=='fresh' else named
    r=run(path.parent.name+'-'+path.stem+'-'+mode,[cli]+([] if mode=='stdout' else ['--emit-c17-to',target])+[path])
    assert r.returncode==status and not r.stdout,(path,r.returncode,r.stderr)
    if mode=='fresh':assert not target.exists()
    preserved(named,sentinel,before)
   print('P1079 SOURCE REJECT',path,'C17',status,'no_clobber=YES',flush=True)
 frames=re.findall(rb'NL_MIXED_COPY_BIND\((\d+),\d+,&nl_b_\d+_\d+\.f0',emitted.stdout)
 assert len(frames)==2 and frames[0]==frames[1]
 values={prefix+str(i)+str(j):matrix[i][j] for prefix,matrix in [('T',profile['initial_fields']),('F',profile['final_fields'])] for i in range(2) for j in range(2)}
 for prefix,key in [('OLD','helper_old'),('NEXT','helper_next'),('SIB','helper_sibling'),('I','helper_index')]:values.update({prefix+str(i):v for i,v in enumerate(profile[key])})
 direct=[profile['IF_changes_by_arm'][profile['chosen_arm']],profile['post_IF_last_change']]
 for i,change in enumerate(direct):values.update({'D'+str(i)+'_'+k.upper():v for k,v in change.items()})
 values.update(dict(H0=17,H1=229,LOCAL_PACKETS=1,HEAD_READS=0,CALLER_READS=1,RECEIVER_READS=0,RESULT2=profile['copy_results'][2],RESULT3=profile['copy_results'][3]))
 names=('old_primary','old_secondary','selected_old','last_old')
 values.update({'RET'+str(i):'nl_b_'+frames[0].decode()+'_'+str(binding[name]['symbol']) for i,name in enumerate(names)})
 definitions=['-DP1079_'+k+'='+str(v) for k,v in values.items()]
 observer='-DNEWLANG_HEAP_OBSERVER="'+str(data/'observer-v2.h')+'"'
 named=temp/'named.c';ok(run('named-publish',[cli,'--emit-c17-to',named,source]));assert named.read_bytes()==emitted.stdout
 sentinel=b'P1079 previous complete output\n';named.write_bytes(sentinel);before=named.stat()
 r=run('named-error',[cli,'--emit-c17-to',named,source],preexec_fn=quota)
 assert r.returncode==1 and not r.stdout and b'OUTPUT' in r.stderr;preserved(named,sentinel,before)
 ok(run('named-retry',[cli,'--emit-c17-to',named,source]));assert named.read_bytes()==emitted.stdout
 for opt in ('-O0','-O2'):
  raw=temp/'raw';ok(run(opt+'-raw-build',[cc,*flags,opt,c,'-o',raw]));r=run(opt+'-raw',[raw]);ok(r);assert not r.stdout
  observed=temp/'observed'
  ok(run(opt+'-physical-build',[cc,*flags,opt,c,repo/'tests/integration/two_heap_platform.c','-Wl,--wrap=malloc','-Wl,--wrap=free',observer,*definitions,'-o',observed]))
  for fail,successes in [('1',0),('2',1),('0',2)]:
   r=run(opt+'-guest'+str(successes),[observed],env=dict(os.environ,NEWLANG_TEST_FAIL_CALL=fail,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'));ok(r)
   text=r.stdout.decode().strip()
   assert 'direct_writes=2' in text and 'all_H_closed_before_T=YES actual_caller_returns=YES' in text
   assert f'allocations={successes} frees={successes} trials={2 if successes else 1} live_tail={int(successes==2)} original_H=YES teardown=YES' in text
   print(json.dumps(dict(P1079='physical',source=profile['name'],opt=opt,sanitizer=sanitizer,C_SHA256=sha(emitted.stdout),observation=text)),flush=True)
 for label,tool in [('IF-mutants','mutants'),('current-owner-negative','consumer'),('reached-OOM','oom')]:
  r=run(label,[temp/tool,source]);ok(r)
  if tool=='mutants':
   assert r.stdout.count(b'P1079 IF REJECT')==19 and r.stdout.count(b'P1079 IF VIEW REJECT')==4 and r.stdout.count(b'P1079 IF COUNT REJECT')==2
   assert b'foreign_B_arms_A_destroyed_unrestored=YES B_current_exact_destroy=YES' in r.stdout
  elif tool=='consumer':
   assert r.stdout.count(b'P1062 ACTUAL REJECT')==14 and r.stdout.count(b'P1062 H REJECT')==4 and r.stdout.count(b'P1062 WORLD REJECT')==7
   assert b'A_destroyed_unrestored=YES B_current_exact_C_destroy=YES' in r.stdout and b'foreign_arm_A_destroyed_unrestored=YES B_exact_C_destroy=YES' in r.stdout
  else:assert b'P1062 OOM SUMMARY reached=' in r.stdout
  print(r.stdout.decode(),end='',flush=True)
print('P1079',profile['name'],'existing-current/actual-old-Copy/common-post/native/identity/OOM/no-clobber PASS',flush=True)
