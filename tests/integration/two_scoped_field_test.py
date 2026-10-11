#!/usr/bin/env python3
"""P312 reproducible source/public-API evidence; each invocation uses a fixed compiler binary."""
from pathlib import Path
import hashlib,json,subprocess,sys,tempfile
compiler,observer,boundary,calibration=sys.argv[1:5]
root=Path(__file__).resolve().parents[2]
s=(root/'tests/fixtures/experimental_two_scoped_field_calls.nl').read_text()
call='set_link(write_B@prev,Option<ptr<Node>>::None);'
loan='loan_read(moved_d_B) { |stable_B_field|'
def one(text,old,new):
    assert text.count(old)==1,(old,text.count(old))
    return text.replace(old,new)
next_call='set_link(write_B_next@next,Option<ptr<Node>>::None);'
next_loan='loan_read(moved_d_B) { |stable_B_next|'
cases=[
 ('two-original-B-prev-next',s,0,''),
 ('alpha-helper-and-types',s.replace('Node','Element').replace('LiveRoot','Ticket').replace('DetachedHolder','Staging').replace('set_link','clear_edge'),0,''),
 ('wrong-second-Domain',one(s,next_loan,next_loan.replace('moved_d_B','life_C')),1,'ALLOCATED-DOMAIN-MISMATCH'),
 ('old-second-Domain',one(s,next_loan,next_loan.replace('moved_d_B','life_B')),1,'P3-USE-AFTER-CONSUME'),
 ('duplicate-consumed-Holder',one(s,'let transferred=staged;','let transferred=staged;let duplicate=staged;'),1,'P3-USE-AFTER-CONSUME'),
 ('duplicate-Domain',one(s,'let rebuilt=DetachedHolder','let first=moved_d_B;let second=moved_d_B;let rebuilt=DetachedHolder'),1,'P3-USE-AFTER-CONSUME'),
 ('duplicate-Allocation',one(s,'let rebuilt=DetachedHolder','let first=moved_a_B;let second=moved_a_B;let rebuilt=DetachedHolder'),1,'P3-USE-AFTER-CONSUME'),
 ('active-second-Domain-move',one(s,next_call,'let stolen=moved_d_B;'+next_call),1,'P3-REF-CONFLICT'),
 ('active-second-owner-repack',one(s,next_call,'let illegal=LiveRoot{p:moved_ptr_B,a:moved_a_B,d:moved_d_B};'+next_call),1,'P3-REF-CONFLICT'),
 ('second-Copy-ptr-stability',one(s,'ref_from_ptr(write,moved_ptr_B,stable_B_next)','ref_from_ptr(write,moved_ptr_B,moved_ptr_B)'),1,'ALLOCATED-REF-TYPE'),
 ('second-Copy-ptr-formal',one(s,next_call,next_call.replace('write_B_next@next','moved_ptr_B')),1,'P3-TYPE-MISMATCH'),
 ('second-read-to-write',one(s,'ref_from_ptr(write,moved_ptr_B,stable_B_next)','ref_from_ptr(read,moved_ptr_B,stable_B_next)'),1,'P3-TYPE-MISMATCH'),
 ('second-scope-escape',one(s,'                                                unit\n                                            };\n                                            let rebuilt','                                                write_B_next@next\n                                            };\n                                            let rebuilt'),1,'P297-LOAN-ESCAPE'),
 ('wrong-terminal-Domain',one(s,'loan_exclusive_read(final_d_B) { |ending_B|','loan_exclusive_read(life_A) { |ending_B|'),1,'P3-DOMAIN-MISMATCH'),
 ('orphan-Allocation',one(s,'deallocate(final_a_B, full_B);',''),1,'P5-SCOPE-OBLIGATION'),
 ('wrong-terminal-Allocation',one(s,'                                            let empty_dst','                                            let bad_empty=loan_exclusive_read(final_d_B){|ending|destroy(final_ptr_B,ending)};\n                                            let bad_raw=erase_slot<Node>(bad_empty);\n                                            finalize_domain(final_d_B);\n                                            deallocate(allocation_C,bad_raw);\n                                            let empty_dst'),1,'P4-ALLOCATION-MISMATCH'),
 ('swapped-fields',s.replace('write_B@prev','write_B@next').replace('write_B_next@next','write_B_next@prev'),3,'P297-CALL-PRECISION'),
 ('repeated-prev',one(s,next_call,next_call.replace('@next','@prev')),3,'P297-CALL-PRECISION'),
 ('child-second',one(s,next_call,next_call.replace('@next','@child')),3,'P297-CALL-PRECISION'),
 ('third-call',one(s,next_call,next_call+next_call),3,'P297-CALL-PRECISION'),
 ('second-new-Some',one(s,next_call,next_call.replace('::None','::Some(moved_ptr_B)')),3,'P297-CALL-PRECISION'),
 ('first-new-Some',one(s,call,call.replace('::None','::Some(moved_ptr_B)')),3,'P297-CALL-PRECISION'),
 ('second-field-alias-Unknown',one(s,next_call,'let alias=write_B_next@next;'+next_call),3,'CAPTURED-CLOSURE-PRECISION'),
 ('first-stale-occurrence',one(s,call,'match write_B@prev {None=>{unit},Some(previous)=>{'+call+'let stale=previous;unit}};'),3,'P9-MATCH-PRECISION'),
 ('second-stale-occurrence',one(s,next_call,'match write_B_next@next {None=>{unit},Some(previous)=>{'+next_call+'let stale=previous;unit}};'),3,'P9-MATCH-PRECISION'),
 ('second-Copy-ptr-field',one(s,next_call,next_call.replace('write_B_next@next','moved_ptr_B@next')),2,'FIELD-PROFILE'),
]
# Same-scope and nested scope are classification probes, not exclusivity assumptions.
first_start=s.index('                                            '+loan)
second_start=s.index('                                            '+next_loan)
second_end=s.index('                                            let rebuilt',second_start)
cases += [
 ('same-scope-overlap',s[:first_start]+s[first_start:second_start].replace(call,call+'set_link(write_B@next,Option<ptr<Node>>::None);')+s[second_end:],3,'P297-CALL-PRECISION'),
 ('nested-overlap',s[:first_start]+s[first_start:second_start].replace(call,call+s[second_start:second_end])+s[second_end:],3,'P297-CALL-PRECISION'),
 ('ended-Domain-before-second',one(s,next_loan,'finalize_domain(moved_d_B);'+next_loan),1,'P3-GOVERNED-ROOTS-LIVE'),
 ('second-after-original-domain-end',one(s,next_loan,'let early=loan_exclusive_read(moved_d_B){|ending|destroy(moved_ptr_B,ending)};let too_late_raw_B=erase_slot<Node>(early);finalize_domain(moved_d_B);'+next_loan),1,'P3-USE-AFTER-CONSUME'),
 ('immutable-full',(root/'tests/fixtures/experimental_five_root_detach_attach.nl').read_text(),2,'ALLOCATED-DOMAIN-LOAN'),
 ('one-prev-calibration',(root/'tests/fixtures/experimental_moved_b_prev_call.nl').read_text(),0,''),
 ('one-next-calibration',(root/'evidence/p308/P306_fixed_holder_calibration.nl').read_text(),0,''),
]
assert hashlib.sha256(cases[-2][1].encode()).hexdigest()=='9c6a6b4e6a8c453d7839b86a52c5195de221ae6456f030477e1af20c9a183d46'
assert hashlib.sha256(cases[-3][1].encode()).hexdigest()=='8763eb98f68d502b8a26b49d8a023ba38dcce45090253bf2c6695fc5391d3512'
rows=[]
with tempfile.TemporaryDirectory(prefix='p312-source-') as temp:
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
            clean=subprocess.run([observer,str(root/'tests/fixtures/experimental_two_scoped_field_calls.nl'),'after-refusal',str(path)],capture_output=True,text=True,check=True)
            retry=json.loads(clean.stdout.splitlines()[-1])['registration_refusal_rollback_retry']
            assert retry
        else: assert r.returncode==4 and 'V1-BACKEND-UNSUPPORTED' in r.stderr
        observed=[];oom=[]
        if status==0:
            tool=calibration if name.endswith('calibration') else observer
            o=subprocess.run([tool,str(path)],capture_output=True,text=True,check=True)
            observed=[json.loads(line) for line in o.stdout.splitlines()]
            effects=[x for x in observed if 'ProjectionId_index' in x]
            assert [x['ProjectionId_index'] for x in effects]==([1] if name=='one-prev-calibration' else [0] if name=='one-next-calibration' else [1,0])
            if not name.endswith('calibration'):
                assert effects[0]['root_fact_after']==effects[1]['root_fact_before']
                assert [x['old_occurrence'] for x in effects]==[4,5]
                assert all(x['sibling_and_child_current_preserved'] and x['old_payload_current_cleared'] for x in effects)
            if name=='two-original-B-prev-next':
                o=subprocess.run([observer,str(path),'oom'],capture_output=True,text=True,check=True)
                oom=[json.loads(line) for line in o.stdout.splitlines() if 'oom_stage' in line]
                assert len(oom)==4
        span=state['start_byte'];end=state['end_byte']
        def position(byte):
            prefix=text.encode()[:byte].decode();return [prefix.count('\n')+1,len(prefix.rsplit('\n',1)[-1])+1]
        rows.append(dict(name=name,sha256=hashlib.sha256(text.encode()).hexdigest(),cli_exit=r.returncode,diagnostic=r.stderr.replace(str(path),'<source>'),span_start=position(span),span_end=position(end),boundary=state,registration_refusal_rollback_retry=retry,owned=observed,oom=oom,c_generated=False))
        print(name,state['registration_status'],state['code'],r.returncode)
output=Path(sys.argv[5]) if len(sys.argv)>5 else Path('p312-observations.json')
output.write_text(json.dumps(rows,indent=2,ensure_ascii=False)+'\n')
