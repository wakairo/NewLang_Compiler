#!/usr/bin/env python3
"""P315 exact-source classification, public owned checks and rollback/retry."""
from pathlib import Path
import hashlib,json,subprocess,sys,tempfile
compiler,observer,boundary,two=sys.argv[1:5]
root=Path(__file__).resolve().parents[2]
s=(root/'tests/fixtures/experimental_cross_root_a_next.nl').read_text()
loan='loan_read(life_A) { |stable_A_detach|'
ref='ref_from_ptr(write,ptr_A,stable_A_detach)'
write='replace(write_A_detach@next,Option<ptr<Node>>::Some(ptr_C))'
def one(text,a,b):
    assert text.count(a)==1,(a,text.count(a))
    return text.replace(a,b)
def last(text,a,b):
    prefix,sep,suffix=text.rpartition(a);assert sep;return prefix+b+suffix
cases=[
 ('three-original-changes',s,0,''),
 ('alpha-helper-and-types',s.replace('Node','Element').replace('LiveRoot','Ticket').replace('DetachedHolder','Staging').replace('set_link','clear_edge'),0,''),
 ('wrong-B-Domain-for-A',one(s,loan,loan.replace('life_A','moved_d_B')),1,'ALLOCATED-DOMAIN-MISMATCH'),
 ('wrong-A-Domain-for-B',one(s,'loan_read(moved_d_B) { |stable_B_next|','loan_read(life_A) { |stable_B_next|'),1,'ALLOCATED-DOMAIN-MISMATCH'),
 ('Copy-ptr-as-ref',one(s,write,write.replace('write_A_detach@next','ptr_A')),1,'P3-WRITE-REF-REQUIRED'),
 ('Copy-ptr-stability',one(s,ref,ref.replace('stable_A_detach','ptr_A')),1,'ALLOCATED-REF-TYPE'),
 ('Copy-ptr-field',one(s,write,write.replace('write_A_detach@next','ptr_A@next')),2,'HEAP-LINK-REF-PROFILE'),
 ('read-A-ref-as-write',one(s,ref,ref.replace('write,','read,')),1,'P3-TYPE-MISMATCH'),
 ('active-A-loan-Domain-move',one(s,write,'let illegal=life_A;'+write),1,'P3-REF-CONFLICT'),
 ('active-A-loan-owner-consume',one(s,write,'let illegal=LiveRoot{p:ptr_A,a:allocation_A,d:life_A};'+write),1,'P3-REF-CONFLICT'),
 ('active-B-loan-owner-consume',one(s,'set_link(write_B_next@next,Option<ptr<Node>>::None);','let illegal=LiveRoot{p:moved_ptr_B,a:moved_a_B,d:moved_d_B};set_link(write_B_next@next,Option<ptr<Node>>::None);'),1,'P3-REF-CONFLICT'),
 ('duplicate-A-Allocation',one(s,'let detached_A_next = ','let original=allocation_A;let duplicate=allocation_A;let detached_A_next = '),1,'P3-USE-AFTER-CONSUME'),
 ('duplicate-A-Domain',one(s,'let detached_A_next = ','let original=life_A;let duplicate=life_A;let detached_A_next = '),1,'P3-USE-AFTER-CONSUME'),
 ('duplicate-consumed-Holder',one(s,'let detached_A_next = ','let duplicate=transferred;let detached_A_next = '),1,'P3-USE-AFTER-CONSUME'),
 ('stale-A-next-occurrence',one(s,write,'match write_A_detach@next {None=>{unit},Some(previous)=>{'+write+';let stale=previous;unit}}'),3,'P9-MATCH-PRECISION'),
 ('wrong-A-terminal-Domain',last(s,'loan_exclusive_read(life_A) { |ending_A|','loan_exclusive_read(life_src) { |ending_A|'),1,'P3-DOMAIN-MISMATCH'),
 ('wrong-terminal-Allocation',one(s,'deallocate(allocation_dst, full_dst);','deallocate(allocation_A, full_dst);'),1,'P4-ALLOCATION-MISMATCH'),
 ('wrong-B-terminal-pointer',one(s,'destroy(final_ptr_B, ending_B)','destroy(ptr_A, ending_B)'),1,'P3-DOMAIN-MISMATCH'),
 ('orphan-A-Allocation',last(s,'deallocate(allocation_A, full_A);',''),1,'P5-SCOPE-OBLIGATION'),
 ('A-ended-before-third-loan',one(s,'let detached_A_next = ','let early=loan_exclusive_read(life_A){|ending|destroy(ptr_A,ending)};let early_raw=erase_slot<Node>(early);finalize_domain(life_A);let detached_A_next = '),1,'P3-USE-AFTER-CONSUME'),
 ('A-domain-finalize-with-live-root',one(s,'let detached_A_next = ','finalize_domain(life_A);let detached_A_next = '),1,'P3-GOVERNED-ROOTS-LIVE'),
 ('A-field-scope-escape',one(s,write,write+';write_A_detach@next'),1,'P297-LOAN-ESCAPE'),
 ('A-field-alias-Unknown',one(s,write,'let alias=write_A_detach@next;'+write),3,'CAPTURED-CLOSURE-PRECISION'),
 # These are finite profile / required witness coverage controls, not semantic-invalid claims.
 ('new-pointer-A',one(s,write,write.replace('Some(ptr_C)','Some(ptr_A)')),3,'CAPTURED-CLOSURE-PRECISION'),
 ('new-pointer-B-no-real-rewire',one(s,write,write.replace('Some(ptr_C)','Some(moved_ptr_B)')),3,'CAPTURED-CLOSURE-PRECISION'),
 ('new-None',one(s,write,write.replace('Some(ptr_C)','None')),3,'CAPTURED-CLOSURE-PRECISION'),
 ('A-prev-instead-next',one(s,write,write.replace('@next','@prev')),3,'CAPTURED-CLOSURE-PRECISION'),
 ('wrong-initial-A-next-target',one(s,'Option<ptr<Node>>::Some(ptr_B))\n                                            };\n                                            let old_B_prev','Option<ptr<Node>>::Some(ptr_C))\n                                            };\n                                            let old_B_prev'),3,'CAPTURED-CLOSURE-PRECISION'),
 ('third-named-helper',one(s,write,'set_link(write_A_detach@next,Option<ptr<Node>>::Some(ptr_C))'),3,'P297-CALL-PRECISION'),
]
block=s[s.index('                                            let detached_A_next'):s.index('                                            let rebuilt')]
first=s.index('                                            loan_read(moved_d_B) { |stable_B_field|')
second=s.index('                                            loan_read(moved_d_B) { |stable_B_next|')
without=s.replace(block,'')
cases += [
 ('third-before-first',without[:first]+block+without[first:],3,'CAPTURED-CLOSURE-PRECISION'),
 ('third-between-B-calls',without[:second]+block+without[second:],3,'CAPTURED-CLOSURE-PRECISION'),
 ('second-B-scope-overlaps-A',one(s,'set_link(write_B_next@next,Option<ptr<Node>>::None);','set_link(write_B_next@next,Option<ptr<Node>>::None);'+block).replace(block+'                                            let rebuilt','                                            let rebuilt'),3,'CAPTURED-CLOSURE-PRECISION'),
 ('two-call-immutable-calibration',(root/'tests/fixtures/experimental_two_scoped_field_calls.nl').read_text(),0,''),
 ('full-immutable',(root/'tests/fixtures/experimental_five_root_detach_attach.nl').read_text(),2,'ALLOCATED-DOMAIN-LOAN'),
]
rows=[]
with tempfile.TemporaryDirectory(prefix='p315-source-') as tmp:
    for name,text,status,code in cases:
        path=Path(tmp)/(name+'.nl');path.write_text(text)
        probes=[subprocess.run([compiler,str(path)],capture_output=True,text=True) for _ in range(2)]
        assert (probes[0].returncode,probes[0].stdout,probes[0].stderr)==(probes[1].returncode,probes[1].stdout,probes[1].stderr)
        cli=probes[0]
        b=json.loads(subprocess.run([boundary,str(path)],capture_output=True,text=True,check=True).stdout)
        assert b['parse_status']==0 and b['registration_status']==status,(name,b)
        if code is not None: assert b['code']==code,(name,b)
        assert not cli.stdout and cli.returncode==(3 if status else 4)
        observed=[];oom=[];retry=False
        if status:
            assert b['snapshot_unchanged'] and not b['owned_entry_artifact']
            r=subprocess.run([observer,str(root/'tests/fixtures/experimental_cross_root_a_next.nl'),'after-refusal',str(path)],capture_output=True,text=True,check=True)
            retry=json.loads(r.stdout.splitlines()[-1])['registration_refusal_rollback_retry'];assert retry
        else:
            tool=two if name.endswith('calibration') else observer
            r=subprocess.run([tool,str(path)],capture_output=True,text=True,check=True)
            observed=[json.loads(line) for line in r.stdout.splitlines()]
            if not name.endswith('calibration'):
                assert len([x for x in observed if 'call_ordinal' in x])==2
                direct=observed[-1];assert direct['direct_cross_root_change'] and direct['old_occurrence']!=direct['new_occurrence']
            if name=='three-original-changes':
                r=subprocess.run([observer,str(path),'oom'],capture_output=True,text=True,check=True)
                oom=[json.loads(line) for line in r.stdout.splitlines() if 'oom_stage' in line]
                assert len(oom)==5
        def pos(byte):
            prefix=text.encode()[:byte].decode();return [prefix.count('\n')+1,len(prefix.rsplit('\n',1)[-1])+1]
        rows.append(dict(name=name,sha256=hashlib.sha256(text.encode()).hexdigest(),cli_exit=cli.returncode,diagnostic=cli.stderr.replace(str(path),'<source>'),boundary=b,span_start=pos(b['start_byte']),span_end=pos(b['end_byte']),owned=observed,oom=oom,registration_refusal_rollback_retry=retry,c_generated=False))
        print(name,b['registration_status'],b['code'],cli.returncode)
Path(sys.argv[5] if len(sys.argv)>5 else 'p315-observations.json').write_text(json.dumps(rows,indent=2,ensure_ascii=False)+'\n')
