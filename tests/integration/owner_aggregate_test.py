import pathlib
import hashlib
import json
import subprocess
import sys
import tempfile
repo=pathlib.Path(__file__).resolve().parents[2]
s=(repo/'tests/fixtures/experimental_owner_aggregate_transport.nl').read_text()
def change(s,a,b):
 assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
four=change(s,'fn transport(whole:DetachResult)->DetachResult','fn transport(whole:TreeFour)->TreeFour')
four=change(four,'let TreeFour {src:original_src','let moved_four=transport(donor);\nlet TreeFour {src:original_src')
four=change(four,'last:original_C}=donor;','last:original_C}=moved_four;')
four=change(four,'let moved=transport(product);','let moved=product;')
mixed=change(change(s,'a:allocation_B, d:life_B','a:allocation_C, d:life_B'),'a:allocation_C, d:life_C','a:allocation_B, d:life_C')
repair='''let LiveRoot{p:pb,a:ac,d:db}=leaf;
let LiveRoot{p:pc,a:ab,d:dc}=own_C;
let repaired_B=LiveRoot{p:pb,a:ab,d:db};
let repaired_C=LiveRoot{p:pc,a:ac,d:dc};
'''
mixed=change(mixed,'finish_root(own_A);',repair+'finish_root(own_A);')
mixed=change(mixed,'finish_root(own_C);','finish_root(repaired_C);');mixed=change(mixed,'finish_root(leaf);','finish_root(repaired_B);')
wrong=change(change(s,'a:allocation_B, d:life_B','a:allocation_C, d:life_B'),'a:allocation_C, d:life_C','a:allocation_B, d:life_C')
wrong=change(wrong,'finish_root(leaf);','');wrong=change(wrong,'finish_root(own_A);','finish_root(leaf);\nfinish_root(own_A);')
# Distinct wrong-domain contrast uses an ordinary mixed return, then authority.
wrongd=change(change(s,'a:allocation_B, d:life_B','a:allocation_B, d:life_C'),'a:allocation_C, d:life_C','a:allocation_C, d:life_B')
wrongd=change(wrongd,'finish_root(leaf);','');wrongd=change(wrongd,'finish_root(own_A);','finish_root(leaf);\nfinish_root(own_A);')
alpha=s
for a,b in [('Node','Element'),('LiveRoot','Packet'),('TreeFour','Quartet'),('TreeThree','Trio'),('DetachResult','Product'),('TreeTwo','Pair'),('transport','identity_record'),('finish_root','dispose_record')]:alpha=alpha.replace(a,b)
# Declaration order must not create an ownership privilege.
forward=change(s,'struct TreeThree { src: LiveRoot, first: LiveRoot, last: LiveRoot, }\nstruct DetachResult { donor: TreeThree, detached: LiveRoot, }','struct DetachResult { donor: TreeThree, detached: LiveRoot, }\nstruct TreeThree { src: LiveRoot, first: LiveRoot, last: LiveRoot, }')
cases=[('mixed-product',s,'matched'),('four-identity',four,'matched'),('mixed-allocation-pure-return-repaired',mixed,'mixed'),('role-neutral-alpha',alpha,'matched'),('forward-owner-declaration',forward,'matched'),('wrong-C-allocation-at-B-release',wrong,None),('wrong-C-domain-at-B-release',wrongd,None),
('dup-consumed-A-B',change(s,'let receiver = LiveRoot','let ghost=allocation_B;\nlet receiver = LiveRoot'),None),
('dup-consumed-D-B',change(s,'let receiver = LiveRoot','let ghost=life_B;\nlet receiver = LiveRoot'),None),
('duplicate-member',change(s,'middle:LiveRoot { p:ptr_B, a:allocation_B, d:life_B }','middle:LiveRoot { p:ptr_B, a:allocation_A, d:life_B }'),None),
('omitted-four-member',change(s,'                                                middle:LiveRoot { p:ptr_B, a:allocation_B, d:life_B },\n',''),None),
('partial-whole-pattern',change(s,'middle:original_B,',''),None),
('partial-owner-field-move',change(s,'let TreeFour {src:original_src','let moved_leaf=donor@middle;\nlet TreeFour {src:original_src'),None),
('normal-exit-owner-loss',change(s,'                                            finish_root(own_A);\n',''),None),
('caller-after-transfer',change(s,'let moved=transport(product);','let moved=transport(product);let old=product;'),None),
('received-after-move',change(s,'let DetachResult{donor:remaining','let extra=moved;let DetachResult{donor:remaining'),None),
('callee-duplicates-formal',change(s,'return whole;','let duplicate=whole;return whole;'),None),
('callee-loses-formal',change(s,'return whole;','unit'),None),
('alias-duplicate-receiver',change(s,'middle:original_B','middle:original_A'),None),
('active-loan-over-whole-construction',change(s,'                                            let donor = TreeFour {','                                            let donor = loan_read(life_B){|held| TreeFour {').replace('                                            };\n                                            let receiver','                                            }};\n                                            let receiver',1),None),
('owner-by-value-cycle',change(s,'struct TreeFour { src: LiveRoot','struct TreeFour { src: TreeFour'),None),
('unknown-component',change(s,'struct TreeFour { src: LiveRoot','struct TreeFour { src: Missing'),None),
('five-member-parser-bound',change(s,'struct TreeFour { src: LiveRoot','struct TreeFour { fifth:LiveRoot, src: LiveRoot'),None),
('leaf-budget-over-four',change(s,'struct TreeFour { src: LiveRoot','struct TreeFour { src: TreeThree'),None),
]

expected = {
 'wrong-C-allocation-at-B-release': ('semantic-reject','P193-CALL-BACKING'),
 'wrong-C-domain-at-B-release': ('semantic-reject','P193-CALL-DOMAIN'),
 'dup-consumed-A-B': ('semantic-reject','P3-USE-AFTER-CONSUME'),
 'dup-consumed-D-B': ('semantic-reject','P3-USE-AFTER-CONSUME'),
 'duplicate-member': ('semantic-reject','P3-USE-AFTER-CONSUME'),
 'omitted-four-member': ('semantic-reject','P5-AGGREGATE-FIELD-COUNT'),
 'partial-whole-pattern': ('semantic-reject','P5-AGGREGATE-FIELD-COUNT'),
 'partial-owner-field-move': ('semantic-profile-unsupported','FIELD-PROFILE'),
 'normal-exit-owner-loss': ('semantic-reject','P5-SCOPE-OBLIGATION'),
 'caller-after-transfer': ('semantic-reject','P3-USE-AFTER-CONSUME'),
 'received-after-move': ('semantic-reject','P3-USE-AFTER-CONSUME'),
 'callee-duplicates-formal': ('semantic-reject','P3-USE-AFTER-CONSUME'),
 'callee-loses-formal': ('semantic-reject','P5-SCOPE-OBLIGATION'),
 'alias-duplicate-receiver': ('semantic-reject','P5-DUPLICATE-RECEIVER'),
 'active-loan-over-whole-construction': ('semantic-reject','P3-REF-CONFLICT'),
 'owner-by-value-cycle': ('semantic-reject','P285-OWNER-CYCLE'),
 'unknown-component': ('semantic-reject','P285-OWNER-COMPONENT'),
 'five-member-parser-bound': ('parser-unsupported','AVS-DECL-PROFILE'),
 'leaf-budget-over-four': ('semantic-profile-unsupported','P285-OWNER-PROFILE'),
}
compiler,evidence,boundary=(str(pathlib.Path(x).resolve()) for x in sys.argv[1:4])
def run(directory):
 observations=[]
 for name,text,proof in cases:
  assert text.count('try_allocate_one<')==5 and text.count('replace(')==6
  path=directory/(name+'.nl');path.write_text(text);files=set(directory.iterdir())
  a=subprocess.run([compiler,path.name],cwd=directory,capture_output=True)
  b=subprocess.run([compiler,path.name],cwd=directory,capture_output=True)
  assert (a.returncode,a.stdout,a.stderr)==(b.returncode,b.stdout,b.stderr),name
  category,code=('semantic-accepted/backend-unsupported','V1-BACKEND-UNSUPPORTED') if proof else expected[name]
  assert a.returncode==(4 if proof else 3) and code.encode() in a.stderr and not a.stdout,(name,a.stderr)
  label='cli' if proof else 'semantic' if category=='semantic-reject' else 'unsupported'
  assert ('error('+label+')').encode() in a.stderr,(name,a.stderr)
  assert set(directory.iterdir())==files,name
  r=subprocess.run([boundary,str(path)],capture_output=True)
  assert r.returncode==0 and not r.stderr,(name,r.stderr)
  view=json.loads(r.stdout)
  if category=='parser-unsupported':assert view['parse_status']==2 and not view['syntax_tree'] and not view['registration_attempted']
  else:
   assert view['parse_status']==0 and view['registration_attempted']
   assert view['registration_status']==(0 if proof else 1 if category=='semantic-reject' else 2),(name,view)
  owned=None
  if proof or category=='semantic-reject':
   r=subprocess.run([evidence,str(path),proof or 'reject:'+code],capture_output=True)
   assert r.returncode==0 and not r.stderr,(name,r.stderr)
   owned=json.loads(r.stdout)
  if not proof:
   assert view['snapshot_unchanged'] and view['code']==code
   view['source_span']=text[view['start_byte']:view['end_byte']]
  observations.append({'input':name,'source_sha256':hashlib.sha256(text.encode()).hexdigest(),'classification':category,'cli_exit':a.returncode,'diagnostic':a.stderr.decode(),'repeat_identical':True,'public_boundary':view,'owned_source_evidence':owned,'C_output':False,'detach_adopt_claim':False})
 return {'kind':'P285 finite acyclic owner aggregate substrate; UNSELECTED','native_execution':False,'observations':observations}
if len(sys.argv)>4:
 directory=pathlib.Path(sys.argv[4]).resolve();directory.mkdir(parents=True,exist_ok=True);result=run(directory)
 (directory/'observations.json').write_text(json.dumps(result,indent=2)+'\n')
else:
 with tempfile.TemporaryDirectory(prefix='p285-source-') as temporary:result=run(pathlib.Path(temporary))
print(json.dumps(result,indent=2))
