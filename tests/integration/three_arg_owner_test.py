"""P289 actual five-original source; pure 3-arg transport, no detach/adopt."""
import hashlib
import json
import pathlib
import subprocess
import sys
import tempfile
repo = pathlib.Path(__file__).resolve().parents[2]
s = (repo/'tests/fixtures/experimental_three_arg_owner_call.nl').read_text()
def change(text, old, new):
    assert text.count(old)==1,(old,text.count(old))
    return text.replace(old,new)
call='let moved=assemble(receiver,original_B,ptr_B);'
body='let result=TreeTwo{root:receiver,child:detached}; return result;'
alpha=s
for a,b in [('Node','Element'),('LiveRoot','Packet'),('TreeFour','Quartet'),('TreeThree','Trio'),('DetachResult','Product'),('TreeTwo','Pair'),('assemble','pack_pair'),('finish_root','dispose_record')]: alpha=alpha.replace(a,b)
mix=change(change(s,'a:allocation_B, d:life_B','a:allocation_C, d:life_B'),'a:allocation_C, d:life_C','a:allocation_B, d:life_C')
repair='''let LiveRoot{p:pb,a:ac,d:db}=leaf;
let LiveRoot{p:pc,a:ab,d:dc}=original_C;
let repaired_B=LiveRoot{p:pb,a:ab,d:db};
let repaired_C=LiveRoot{p:pc,a:ac,d:dc};
'''
repaired=change(mix,'finish_root(original_A);',repair+'finish_root(original_A);')
repaired=change(change(repaired,'finish_root(original_C);','finish_root(repaired_C);'),'finish_root(leaf);','finish_root(repaired_B);')
wrong=change(change(mix,'finish_root(leaf);',''),'finish_root(original_A);','finish_root(leaf);finish_root(original_A);')
wrongd=change(change(s,'a:allocation_B, d:life_B','a:allocation_B, d:life_C'),'a:allocation_C, d:life_C','a:allocation_C, d:life_B')
wrongd=change(change(wrongd,'finish_root(leaf);',''),'finish_root(original_A);','finish_root(leaf);finish_root(original_A);')
full=(repo/'tests/fixtures/experimental_five_root_detach_attach.nl').read_text()
two=full[full.index('fn finish_two('):full.index('fn main()')]
finish_two=change(s,'fn main()',two+'fn main()')
finish_two=change(finish_two,'                                            let TreeTwo{root:owner_dst,child:leaf}=moved;\n','')
finish_two=change(change(finish_two,'finish_root(leaf);','finish_two(moved);'),'finish_root(owner_dst);','')
# A/C/src really discharged before source finish_two, despite no physical handoff.
late=change(change(change(s,'finish_root(original_A);',''),'finish_root(original_C);',''),'finish_root(original_src);','')
late=change(late,call,'finish_root(original_A);finish_root(original_C);finish_root(original_src);'+call.replace('ptr_B','ptr_A'))
independent=s[:s.index('fn main()')]+s[s.index('fn main()'):].replace('original_B','packet_B').replace('original_A','keep_A').replace('original_C','keep_C').replace('original_src','keep_src').replace('receiver','dst_packet').replace('donor','tree_before').replace('moved','returned_pair')
cases=[
 ('independent-caller-bindings',independent,'matched'),
 ('fifth-owner-field-parser-profile',change(s,'struct TreeFour { src: LiveRoot','struct TreeFour { extra:LiveRoot, src: LiveRoot'),None),
 ('three-formal-source',s,'matched'),('pure-direct-return',change(s,body,'return TreeTwo{root:receiver,child:detached};'),'matched'),
 ('role-neutral-alpha',alpha,'matched'),('reversed-whole-fields',change(s,body,'let result=TreeTwo{child:detached,root:receiver}; return result;'),'matched'),
 ('mixed-allocation-return-repaired',repaired,'mixed'),('Copy-link-C-confers-no-authority',change(s,call,call.replace('ptr_B','ptr_C')),'matched'),
 ('stale-Copy-link-after-A-C-src-release',late,'matched'),('real-finish-two-after-A-C-src-release',finish_two,'matched'),
 ('wrong-C-allocation-B-release',wrong,None),('wrong-C-domain-B-release',wrongd,None),
 ('caller-reuses-dst',change(s,call,call+'let again=receiver;'),None),('caller-reuses-B',change(s,call,call+'let again=original_B;'),None),
 ('returned-owner-duplicate',change(s,call,call+'let again=moved;'),None),('same-packet-two-arguments',change(s,call,call.replace('original_B','receiver')),None),
 ('callee-formal-double-use',change(s,body,'let extra=detached;'+body),None),
 ('callee-omits-owner',change(s,body,'return TreeTwo{root:receiver};'),None),
 ('callee-normal-exit-loss',change(s,body,'unit'),None),('caller-normal-exit-loss',change(s,'finish_root(leaf);',''),None),
 ('duplicate-original-Allocation',change(s,'a:allocation_B, d:life_B','a:allocation_A, d:life_B'),None),
 ('duplicate-original-Domain',change(s,'a:allocation_B, d:life_B','a:allocation_B, d:life_A'),None),
 ('Copy-ptr-as-owner-Allocation',change(s,'a:allocation_B, d:life_B','a:ptr_B, d:life_B'),None),
 ('Copy-ptr-as-third-nonCopy-packet',change(s,call,call.replace('original_B','ptr_B')),None),
 ('wrong-third-parameter-type',change(s,call,call.replace('ptr_B','u8(1)')),None),
 ('live-domain-loan-crosses-call',change(s,call,'let LiveRoot{p:pb,a:ab,d:db}=original_B;let moved=loan_read(db){|held| let packet=LiveRoot{p:pb,a:ab,d:db};assemble(receiver,packet,ptr_B)};'),None),
 ('independent-definition-unsafe-third-ref',change(s,body,'let unsafe_ref=ref_from_ptr(write,link,link);'+body),None),
 ('independent-definition-releases-mixed-packet',change(s,body,'finish_root(detached);'+body),None),
 ('independent-definition-extra-inert-read',change(s,body,'let unused=link;'+body),None),
 ('implicit-normal-result-outside-body-slice',change(s,body,'TreeTwo{root:receiver,child:detached}'),None),
 ('fourth-argument-profile',change(s,'link:ptr<Node>','link:ptr<Node>, fourth:ptr<Node>'),None),
 ('original-full-first-next-blocker',full,None),
]
compiler,evidence,boundary=(str(pathlib.Path(x).resolve()) for x in sys.argv[1:4])
# Exact observed status/code; source rejection and implementation limits separate.
expected={
 'fifth-owner-field-parser-profile':(None,'AVS-DECL-PROFILE'),
 'wrong-C-allocation-B-release':(1,'P193-CALL-BACKING'), 'wrong-C-domain-B-release':(1,'P193-CALL-DOMAIN'),
 **{name:(1,'P3-USE-AFTER-CONSUME') for name in ['caller-reuses-dst','caller-reuses-B','returned-owner-duplicate','same-packet-two-arguments','callee-formal-double-use','duplicate-original-Allocation','duplicate-original-Domain']},
 'callee-omits-owner':(1,'P5-AGGREGATE-FIELD-COUNT'),
 **{name:(1,'P5-SCOPE-OBLIGATION') for name in ['callee-normal-exit-loss','caller-normal-exit-loss']},
 'Copy-ptr-as-owner-Allocation':(1,'P5-FIELD-TYPE'),
 **{name:(1,'P3-TYPE-MISMATCH') for name in ['Copy-ptr-as-third-nonCopy-packet','wrong-third-parameter-type']},
 'live-domain-loan-crosses-call':(1,'P3-REF-CONFLICT'),
 'independent-definition-unsafe-third-ref':(2,'ALLOCATED-REF-PROFILE'),
 'independent-definition-releases-mixed-packet':(3,'P276-TERMINAL-SUMMARY-PRECISION'),
 'independent-definition-extra-inert-read':(3,'P289-BODY-PRECISION'),
 'implicit-normal-result-outside-body-slice':(3,'P289-BODY-PRECISION'),
 'fourth-argument-profile':(3,'P8-SIGNATURE-PRECISION'),
 'original-full-first-next-blocker':(2,'ALLOCATED-DOMAIN-LOAN'),
}
def run(directory):
 observations=[]
 for name,text,proof in cases:
  assert text.count('try_allocate_one<')==5
  assert text.count('replace(')==(12 if name=='original-full-first-next-blocker' else 6)
  path=directory/(name+'.nl');path.write_text(text);files=set(directory.iterdir())
  a=subprocess.run([compiler,path.name],cwd=directory,capture_output=True)
  b=subprocess.run([compiler,path.name],cwd=directory,capture_output=True)
  assert (a.returncode,a.stdout,a.stderr)==(b.returncode,b.stdout,b.stderr),name
  assert not a.stdout and set(directory.iterdir())==files,name
  r=subprocess.run([boundary,str(path)],capture_output=True)
  assert r.returncode==0 and not r.stderr,(name,r.stderr)
  view=json.loads(r.stdout)
  if not expected and '--discover' in sys.argv:
   print(name,a.returncode,view.get('registration_status'),view.get('code'),a.stderr.decode().strip(),flush=True)
   continue
  status,code=(0,'V1-BACKEND-UNSUPPORTED') if proof else expected[name]
  if status is None:
   assert view['parse_status']==2 and not view['syntax_tree'] and not view['registration_attempted'],(name,view)
  else:
   assert view['parse_status']==0 and view['registration_status']==status,(name,view)
  assert a.returncode==(4 if proof else 3) and code.encode() in a.stderr,(name,a.stderr)
  category='parser-unsupported' if status is None else 'semantic-accepted/backend-unsupported' if proof else 'semantic-reject' if status==1 else 'semantic-profile-unsupported' if status==2 else 'semantic-precision-unsupported'
  label='unsupported' if status is None else 'cli' if proof else 'semantic' if status==1 else 'unsupported' if status==2 else 'precision'
  assert ('error('+label+')').encode() in a.stderr,(name,a.stderr)
  owned=None
  if proof or status==1:
   r=subprocess.run([evidence,str(path),proof or 'reject:'+code],capture_output=True)
   assert r.returncode==0 and not r.stderr,(name,r.stderr,r.stdout)
   owned=json.loads(r.stdout)
  if not proof:
   assert view['snapshot_unchanged'] and view['code']==code
   view['source_span']=text[view['start_byte']:view['end_byte']]
  observations.append({'input':name,'source_sha256':hashlib.sha256(text.encode()).hexdigest(),'classification':category,'cli_exit':a.returncode,'diagnostic':a.stderr.decode(),'repeat_identical':True,'public_boundary':view,'owned_source_evidence':owned,'C_output':False,'detach_adopt_claim':False})
 return {'kind':'P289 pure finite 3-arg source candidate; UNSELECTED','native_execution':False,'observations':observations}
if len(sys.argv)>4:
 directory=pathlib.Path(sys.argv[4]).resolve();directory.mkdir(parents=True,exist_ok=True);result=run(directory)
 (directory/'observations.json').write_text(json.dumps(result,indent=2)+'\n')
else:
 with tempfile.TemporaryDirectory(prefix='p289-source-') as temporary:result=run(pathlib.Path(temporary))
print(json.dumps(result,indent=2))
