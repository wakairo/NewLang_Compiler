#!/usr/bin/env python3
"""P308 reproducible source/public-API evidence; each invocation uses a fixed compiler binary."""
from pathlib import Path
import hashlib,json,subprocess,sys,tempfile
compiler,observer,boundary,calibration=sys.argv[1:5]
root=Path(__file__).resolve().parents[2]
s=(root/'tests/fixtures/experimental_moved_b_prev_call.nl').read_text()
call='set_link(write_B@prev,Option<ptr<Node>>::None);'
loan='loan_read(moved_d_B) { |stable_B_field|'
def one(text,old,new):
    assert text.count(old)==1,(old,text.count(old))
    return text.replace(old,new)
cases=[
 ('holder-original-B-prev-clear',s,0,''),
 ('holder-Some-to-Some',one(s,call,'set_link(write_B@prev,Option<ptr<Node>>::Some(moved_ptr_B));'),0,''),
 ('wrapper-name-only',s.replace('DetachedHolder','StagedValue'),0,''),
 ('alpha-types-and-helper',s.replace('Node','Element').replace('LiveRoot','Ticket').replace('TreeDWithB','StagedValue').replace('set_link','update_edge'),0,''),
 ('wrong-Domain',one(s,loan,loan.replace('moved_d_B','life_C')),1,'ALLOCATED-DOMAIN-MISMATCH'),
 ('old-Domain-binding',one(s,loan,loan.replace('moved_d_B','life_B')),1,'P3-USE-AFTER-CONSUME'),
 ('old-whole-carrier',one(s,'let transferred=staged;','let transferred=staged; let duplicate=staged;'),1,'P3-USE-AFTER-CONSUME'),
 ('duplicate-Domain-after-loan',one(s,'let rebuilt=DetachedHolder','let first=moved_d_B;let second=moved_d_B;let rebuilt=DetachedHolder'),1,'P3-USE-AFTER-CONSUME'),
 ('duplicate-Allocation',one(s,'let rebuilt=DetachedHolder','let first=moved_a_B;let second=moved_a_B;let rebuilt=DetachedHolder'),1,'P3-USE-AFTER-CONSUME'),
 ('duplicate-Domain',one(s,call,'let first=moved_d_B;let second=moved_d_B;'+call),1,'P3-REF-CONFLICT'),
 ('active-loan-Domain-move',one(s,call,'let moved_again=moved_d_B;'+call),1,'P3-REF-CONFLICT'),
 ('active-loan-whole-pack',one(s,call,'let illegal=LiveRoot{p:moved_ptr_B,a:moved_a_B,d:moved_d_B};'+call),1,'P3-REF-CONFLICT'),
 ('Copy-ptr-stability',one(s,'ref_from_ptr(write,moved_ptr_B,stable_B_field)','ref_from_ptr(write,moved_ptr_B,moved_ptr_B)'),1,'ALLOCATED-REF-TYPE'),
 ('Copy-ptr-safe-formal',one(s,call,call.replace('write_B@prev','moved_ptr_B')),1,'P3-TYPE-MISMATCH'),
 ('read-to-write',one(s,'ref_from_ptr(write,moved_ptr_B,stable_B_field)','ref_from_ptr(read,moved_ptr_B,stable_B_field)'),1,'P3-TYPE-MISMATCH'),
 ('loan-escape',one(s,'                                                unit\n                                            };\n                                            let rebuilt','                                                write_B@prev\n                                            };\n                                            let rebuilt'),1,'P297-LOAN-ESCAPE'),
 ('wrong-Domain-terminal',one(s,'loan_exclusive_read(final_d_B) { |ending_B|','loan_exclusive_read(life_A) { |ending_B|'),1,'P3-DOMAIN-MISMATCH'),
 ('normal-exit-orphan',one(s,'deallocate(final_a_B, full_B);',''),1,'P5-SCOPE-OBLIGATION'),
 ('wrong-Allocation-terminal',one(s,'                                            let empty_dst', '''                                            let bad_empty=loan_exclusive_read(final_d_B){|ending|destroy(final_ptr_B,ending)};
                                            let bad_raw=erase_slot<Node>(bad_empty);
                                            finalize_domain(final_d_B);
                                            deallocate(allocation_C,bad_raw);
                                            let empty_dst'''),1,'P4-ALLOCATION-MISMATCH'),
 ('Copy-ptr-field',one(s,call,call.replace('write_B@prev','moved_ptr_B@prev')),2,'FIELD-PROFILE'),
 ('old-occurrence-ref',one(s,call,'''match write_B@prev {None=>{unit},Some(previous)=>{
set_link(write_B@prev,Option<ptr<Node>>::Some(moved_ptr_B));let stale=previous;unit}};'''),3,'P9-MATCH-PRECISION'),
 ('child-helper',one(s,call,call.replace('@prev','@child')),3,'P297-CALL-PRECISION'),
 ('field-alias-Unknown',one(s,call,'let alias=write_B@prev;'+call),3,'CAPTURED-CLOSURE-PRECISION'),
 ('second-helper',one(s,call,call+call),3,'P297-CALL-PRECISION'),
 ('immutable-full', (root/'tests/fixtures/experimental_five_root_detach_attach.nl').read_text(),2,'ALLOCATED-DOMAIN-LOAN'),
 ('P306-next-calibration',(root/'evidence/p308/P306_fixed_holder_calibration.nl').read_text(),0,''),
 ('fixed-calibration',(root/'tests/fixtures/experimental_scoped_field_call.nl').read_text(),0,''),
]
assert hashlib.sha256(cases[-1][1].encode()).hexdigest()=='e83854ba2b37f8c522fa119c282ce48d5b4914e8868ebe7f51f96ad2e96f5096'
assert hashlib.sha256(cases[-3][1].encode()).hexdigest()=='8763eb98f68d502b8a26b49d8a023ba38dcce45090253bf2c6695fc5391d3512'
rows=[]
with tempfile.TemporaryDirectory(prefix='p308-source-') as temp:
    temp=Path(temp)
    for name,text,status,code in cases:
        path=temp/(name+'.nl');path.write_text(text)
        runs=[subprocess.run([compiler,str(path)],capture_output=True,text=True) for _ in range(2)]
        assert (runs[0].returncode,runs[0].stdout,runs[0].stderr)==(runs[1].returncode,runs[1].stdout,runs[1].stderr)
        r=runs[0];b=subprocess.run([boundary,str(path)],capture_output=True,text=True,check=True)
        state=json.loads(b.stdout)
        assert (state['parse_status'],state['registration_status'],state['code'])==(0,status,code),(name,state)
        assert not r.stdout and not state['owned_entry_artifact']
        retry=False
        if status:
            assert r.returncode==3 and state['snapshot_unchanged']
            clean=subprocess.run([observer,str(root/'tests/fixtures/experimental_moved_b_prev_call.nl'),'after-refusal',str(path)],capture_output=True,text=True,check=True)
            retry=json.loads(clean.stdout.splitlines()[-1])['registration_refusal_rollback_retry']
            assert retry
        else: assert r.returncode==4 and 'V1-BACKEND-UNSUPPORTED' in r.stderr
        observed=[];oom=[]
        if status==0:
            tool=calibration if name=='fixed-calibration' else observer
            o=subprocess.run([tool,str(path)],capture_output=True,text=True,check=True)
            observed=[json.loads(line) for line in o.stdout.splitlines()]
            assert observed[-1]['ProjectionId_index']==(0 if name in ('P306-next-calibration','fixed-calibration') else 1)
            if name!='fixed-calibration':
                assert observed[0]['original_B_domain']==3 and observed[0]['original_B_allocation_value']==29 and observed[0]['original_B_domain_value']==34
                assert observed[-1]['sibling_and_child_current_preserved'] and observed[-1]['old_payload_current_cleared']
            if name=='holder-original-B-prev-clear':
                o=subprocess.run([observer,str(path),'oom'],capture_output=True,text=True,check=True)
                oom=[json.loads(line) for line in o.stdout.splitlines() if 'oom_stage' in line]
                assert len(oom)==3
        span=state['start_byte'];end=state['end_byte']
        def position(byte):
            prefix=text.encode()[:byte].decode();return [prefix.count('\n')+1,len(prefix.rsplit('\n',1)[-1])+1]
        rows.append(dict(name=name,sha256=hashlib.sha256(text.encode()).hexdigest(),cli_exit=r.returncode,diagnostic=r.stderr.replace(str(path),'<source>'),span_start=position(span),span_end=position(end),boundary=state,registration_refusal_rollback_retry=retry,owned=observed,oom=oom,c_generated=False))
        print(name,state['registration_status'],state['code'],r.returncode)
output=Path(sys.argv[5]) if len(sys.argv)>5 else Path('p308-observations.json')
output.write_text(json.dumps(rows,indent=2,ensure_ascii=False)+'\n')
