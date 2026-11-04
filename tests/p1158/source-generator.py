from pathlib import Path
import json

p=Path('/tmp/p1158-input-v1');p.mkdir(exist_ok=True);(p/'sources').mkdir(exist_ok=True)
cases=[
 dict(name='01-add-return-left',nominal='LatchPair',fields=['latch','stamp'],values=[83,151],index=0,start=2,limit=14,step=5,direction='add',trace=[2,7,12,17],result=17,operator='<',compare=18,truth=True,return_arm=0,incoming=204),
 dict(name='02-zero-normal-right',nominal='LevelPair',fields=['peer','level'],values=[37,109],index=1,start=16,limit=16,step=5,direction='add',trace=[16],result=16,operator='<',compare=16,truth=False,return_arm=0,incoming=182),
 dict(name='03-sub-return-right',nominal='GatePair',fields=['tag','gate'],values=[174,61],index=1,start=27,limit=9,step=7,direction='sub',trace=[27,20,13,6],result=6,operator='==',compare=8,truth=False,return_arm=1,incoming=211),
 dict(name='04-one-normal-left',nominal='TonePair',fields=['tone','echo'],values=[94,206],index=0,start=9,limit=12,step=4,direction='add',trace=[9,13],result=13,operator='==',compare=13,truth=True,return_arm=1,incoming=129)]

def source(c,known=False,two=False,extra=False,negative=None):
 a,b=c['fields'];target=c['fields'][c['index']];other=c['fields'][1-c['index']]
 decl=f"struct {c['nominal']} {{ {a}:u8, {b}:u8 }}\nfn main()->unit {{\n let original={c['nominal']}{{{a}:u8({c['values'][0]}),{b}:u8({c['values'][1]})}};\n"
 if known: decl+=' let result=usize(3);\n'
 else:
  step='18446744073709551615' if negative=='overflow' else str(c['step'])
  cont=f"continue(cursor {'+' if c['direction']=='add' else '-'} usize({step}));"
  yes,no=(cont,'break cursor;') if c['direction']=='add' else ('break cursor;',cont)
  decl+=f" let result=loop(cursor=usize({c['start']})){{\n  if(cursor < usize({c['limit']})){{{yes}}}else{{{no}}}\n }};\n"
 decl+=f" let copied=result;\n let decision=copied {c['operator']} usize({c['compare']});\n"
 mode='loan_read' if negative=='read-mode' else 'loan_write'
 incoming='copied' if negative=='incoming-type' else f"u8({c['incoming']})"
 normal=f"{mode}(original@{target}){{|writer|{'writer' if negative=='ending-reference' else 'replace(writer,'+incoming+')'}}}"
 terminal=f"loan_write(original@{target}){{|unused|replace(unused,u8(25))}}" if two else 'return unit;'
 arms=[normal,normal];arms[c['return_arm']]=terminal
 decl+=f" let old=if(decision){{\n  {arms[0]}\n }}else{{\n  {arms[1]}\n }};\n let updated=original@{target};\n let sibling=original@{other};\n let old_copy=old;\n"
 return decl+(' return unit;\n}\n' if extra else ' unit\n}\n')

positive={}
for c in cases:
 c.update(check=0,emit=0,named=0,selected_arm=0 if c['truth'] else 1,early_return=c['return_arm']==(0 if c['truth'] else 1),condition_evaluations=1)
 c['stores']=0 if c['early_return'] else 1;c['post_reads']=0 if c['early_return'] else 2;c['old']=c['values'][c['index']];c['sibling']=c['values'][1-c['index']]
 positive[c['name']]=c;(p/'sources'/(c['name']+'.nl')).write_text(source(c))
c=dict(cases[0]);c['compare']=9
(p/'sources/80-known-return-control.nl').write_text(source(c,known=True))
(p/'sources/81-two-normal-control.nl').write_text(source(cases[1],two=True))
(p/'sources/82-extra-return-control.nl').write_text(source(cases[1],extra=True))
negative={}
for n,k in [('90-overflow','overflow'),('91-read-mode','read-mode'),('92-ending-reference','ending-reference'),('93-incoming-type','incoming-type')]:
 (p/'sources'/(n+'.nl')).write_text(source(cases[1],negative=k));negative[n]={'check':3,'emit':3,'named':3}
oracle={'authority':'C232 / fixed PR1155 af16bba / maincdf23f6 / Draft17.34','positive':positive,'controls':{'80-known-return-control':{'check':0,'emit':0,'named':0},'81-two-normal-control':{'check':0,'emit':0,'named':0},'82-extra-return-control':{'check':0,'emit':4,'named':4,'classification':'outside existing single-RETURN profile; preserve actual earlier refusal if any'}},'negative':negative,'prediction':'CHECK and actual source-issued scalar-loop/one-normal-RETURN current proof may succeed; known-only private IF-return consumer should first refuse emission. Earlier refusal/unexpected acceptance remains FIRST.','native':{'compilers':['GCC13','Clang23'],'opts':['O0','O2'],'variants':['raw unchanged C','read-only passive observation'],'expected_exit':0},'scope':'One main/T/loop/unit-RETURN, normal tail unit, original T unchanged during loop; no new authority/bridge/semantics.'}
(p/'oracle.json').write_text(json.dumps(oracle,indent=2)+'\n')
print('Prepared independent source/oracle; no source executions yet')
