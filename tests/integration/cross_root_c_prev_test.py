#!/usr/bin/env python3
"""P319 real-source classification, public four-operation owned and rollback."""
from pathlib import Path
import hashlib,json,subprocess,sys,tempfile
compiler,observer,boundary,three=sys.argv[1:5]
root=Path(__file__).resolve().parents[2]
s=(root/'tests/fixtures/experimental_cross_root_c_prev.nl').read_text()
loan='loan_read(life_C) { |stable_C_detach|'
ref='ref_from_ptr(write,ptr_C,stable_C_detach)'
write='replace(write_C_detach@prev,Option<ptr<Node>>::Some(ptr_A))'
def one(text,a,b):
    assert text.count(a)==1,(a,text.count(a))
    return text.replace(a,b)
def last(text,a,b):
    prefix,sep,suffix=text.rpartition(a);assert sep;return prefix+b+suffix
cases=[
 ('four-original-changes',s,0,''),
 ('alpha-helper-and-types',s.replace('Node','Element').replace('LiveRoot','Ticket').replace('DetachedHolder','Staging').replace('set_link','clear_edge'),0,''),
 ('wrong-B-Domain-for-C',one(s,loan,loan.replace('life_C','moved_d_B')),1,'ALLOCATED-DOMAIN-MISMATCH'),
 ('wrong-A-Domain-for-C',one(s,loan,loan.replace('life_C','life_A')),1,'ALLOCATED-DOMAIN-MISMATCH'),
 ('Copy-ptr-as-ref',one(s,write,write.replace('write_C_detach@prev','ptr_C')),1,'P3-WRITE-REF-REQUIRED'),
 ('Copy-ptr-stability',one(s,ref,ref.replace('stable_C_detach','ptr_C')),1,'ALLOCATED-REF-TYPE'),
 ('Copy-ptr-field',one(s,write,write.replace('write_C_detach@prev','ptr_C@prev')),2,'HEAP-LINK-REF-PROFILE'),
 ('read-C-ref-as-write',one(s,ref,ref.replace('write,','read,')),1,'P3-TYPE-MISMATCH'),
 ('active-C-loan-Domain-move',one(s,write,'let illegal=life_C;'+write),1,'P3-REF-CONFLICT'),
 ('active-C-loan-owner-consume',one(s,write,'let illegal=LiveRoot{p:ptr_C,a:allocation_C,d:life_C};'+write),1,'P3-REF-CONFLICT'),
 ('duplicate-B-Allocation-before-fourth',one(s,'let detached_C_prev = ','let first=moved_a_B;let duplicate=moved_a_B;let detached_C_prev = '),1,'P3-USE-AFTER-CONSUME'),
 ('duplicate-B-Domain-before-fourth',one(s,'let detached_C_prev = ','let first=moved_d_B;let duplicate=moved_d_B;let detached_C_prev = '),1,'P3-USE-AFTER-CONSUME'),
 ('duplicate-consumed-Holder',one(s,'let detached_C_prev = ','let duplicate=transferred;let detached_C_prev = '),1,'P3-USE-AFTER-CONSUME'),
 ('B-owner-consumed-during-C-operation',one(s,write,'let transported=LiveRoot{p:moved_ptr_B,a:moved_a_B,d:moved_d_B};'+write),1,'P5-SCOPE-OBLIGATION'),
 ('stale-C-prev-occurrence',one(s,write,'match write_C_detach@prev {None=>{unit},Some(previous)=>{'+write+';let stale=previous;unit}}'),3,'P9-MATCH-PRECISION'),
 ('wrong-C-terminal-Domain',last(s,'loan_exclusive_read(life_C) { |ending_C|','loan_exclusive_read(life_A) { |ending_C|'),1,'P3-DOMAIN-MISMATCH'),
 ('wrong-C-terminal-Allocation',last(s,'deallocate(allocation_C, full_C);','deallocate(allocation_A, full_C);'),1,'P4-ALLOCATION-MISMATCH'),
 ('wrong-C-terminal-pointer',last(s,'destroy(ptr_C, ending_C)','destroy(ptr_A, ending_C)'),1,'P3-DOMAIN-MISMATCH'),
 ('orphan-C-Allocation',last(s,'deallocate(allocation_C, full_C);',''),1,'P5-SCOPE-OBLIGATION'),
 ('C-ended-before-fourth-loan',one(s,'let detached_C_prev = ','let early=loan_exclusive_read(life_C){|ending|destroy(ptr_C,ending)};let early_raw=erase_slot<Node>(early);finalize_domain(life_C);let detached_C_prev = '),1,'P3-USE-AFTER-CONSUME'),
 ('C-domain-finalize-with-live-root',one(s,'let detached_C_prev = ','finalize_domain(life_C);let detached_C_prev = '),1,'P3-GOVERNED-ROOTS-LIVE'),
 ('C-field-scope-escape',one(s,write,write+';write_C_detach@prev'),1,'P297-LOAN-ESCAPE'),
 ('C-field-alias-Unknown',one(s,write,'let alias=write_C_detach@prev;'+write),3,'CAPTURED-CLOSURE-PRECISION'),
 # Finite required-witness controls; do not call these semantic-invalid.
 ('new-pointer-C',one(s,write,write.replace('Some(ptr_A)','Some(ptr_C)')),3,'CAPTURED-CLOSURE-PRECISION'),
 ('new-pointer-B-no-real-rewire',one(s,write,write.replace('Some(ptr_A)','Some(moved_ptr_B)')),3,'CAPTURED-CLOSURE-PRECISION'),
 ('new-None',one(s,write,write.replace('Some(ptr_A)','None')),3,'CAPTURED-CLOSURE-PRECISION'),
 ('C-next-instead-prev',one(s,write,write.replace('@prev','@next')),3,'CAPTURED-CLOSURE-PRECISION'),
 ('wrong-initial-C-prev-target',one(s,'replace(w_C_prev@prev,\n                                                    Option<ptr<Node>>::Some(ptr_B))','replace(w_C_prev@prev,\n                                                    Option<ptr<Node>>::Some(ptr_A))'),3,'CAPTURED-CLOSURE-PRECISION'),
 ('fourth-named-helper',one(s,write,'set_link(write_C_detach@prev,Option<ptr<Node>>::Some(ptr_A))'),3,'P297-CALL-PRECISION'),
 ('lost-third-A-effect',one(s,'replace(write_A_detach@next,Option<ptr<Node>>::Some(ptr_C))','replace(write_A_detach@next,Option<ptr<Node>>::Some(ptr_B))'),3,'CAPTURED-CLOSURE-PRECISION'),
]
third_block=s[s.index('                                            let detached_A_next'):s.index('                                            let detached_C_prev')]
fourth_block=s[s.index('                                            let detached_C_prev'):s.index('                                            let rebuilt')]
without=s.replace(fourth_block,'')
cases += [
 ('fourth-before-third',without.replace(third_block,fourth_block+third_block),3,'CAPTURED-CLOSURE-PRECISION'),
 ('fifth-direct-outside-profile',s.replace(fourth_block,fourth_block+fourth_block.replace('detached_C_prev','fifth_result')),3,'CAPTURED-CLOSURE-PRECISION'),
 ('third-skipped',s.replace(third_block,''),3,'CAPTURED-CLOSURE-PRECISION'),
 ('A-scope-overlaps-C',s.replace(fourth_block,'').replace('replace(write_A_detach@next,Option<ptr<Node>>::Some(ptr_C))','replace(write_A_detach@next,Option<ptr<Node>>::Some(ptr_C));'+fourth_block+'unit'),3,'CAPTURED-CLOSURE-PRECISION'),
 ('three-call-immutable-calibration',(root/'tests/fixtures/experimental_cross_root_a_next.nl').read_text(),0,''),
 ('full-immutable',(root/'tests/fixtures/experimental_five_root_detach_attach.nl').read_text(),2,'ALLOCATED-DOMAIN-LOAN'),
]
rows=[]
with tempfile.TemporaryDirectory(prefix='p319-source-') as tmp:
    for name,text,status,code in cases:
        path=Path(tmp)/(name+'.nl');path.write_text(text)
        probes=[subprocess.run([compiler,str(path)],capture_output=True,text=True) for _ in range(2)]
        assert (probes[0].returncode,probes[0].stdout,probes[0].stderr)==(probes[1].returncode,probes[1].stdout,probes[1].stderr)
        cli=probes[0]
        b=json.loads(subprocess.run([boundary,str(path)],capture_output=True,text=True,check=True).stdout)
        assert b['parse_status']==0 and b['registration_status']==status,(name,b)
        assert b['code']==code,(name,b)
        assert not cli.stdout and cli.returncode==(3 if status else 4)
        observed=[];oom=[];retry=False
        if status:
            assert b['snapshot_unchanged'] and not b['owned_entry_artifact']
            r=subprocess.run([observer,str(root/'tests/fixtures/experimental_cross_root_c_prev.nl'),'after-refusal',str(path)],capture_output=True,text=True,check=True)
            retry=json.loads(r.stdout.splitlines()[-1])['registration_refusal_rollback_retry'];assert retry
        else:
            tool=three if name.endswith('calibration') else observer
            r=subprocess.run([tool,str(path)],capture_output=True,text=True,check=True)
            observed=[json.loads(line) for line in r.stdout.splitlines()]
            if not name.endswith('calibration'):
                assert len([x for x in observed if 'call_ordinal' in x])==2
                directs=[x for x in observed if x.get('direct_cross_root_change')]
                assert [x['ordinal'] for x in directs]==[3,4]
                assert all(x['old_occurrence']!=x['new_occurrence'] for x in directs)
                assert directs[1]['third_after_equals_fourth_before']
            if name=='four-original-changes':
                r=subprocess.run([observer,str(path),'oom'],capture_output=True,text=True,check=True)
                oom=[json.loads(line) for line in r.stdout.splitlines() if 'oom_stage' in line]
                assert len(oom)==5
        def pos(byte):
            prefix=text.encode()[:byte].decode();return [prefix.count('\n')+1,len(prefix.rsplit('\n',1)[-1])+1]
        rows.append(dict(name=name,sha256=hashlib.sha256(text.encode()).hexdigest(),cli_exit=cli.returncode,diagnostic=cli.stderr.replace(str(path),'<source>'),boundary=b,span_start=pos(b['start_byte']),span_end=pos(b['end_byte']),owned=observed,oom=oom,registration_refusal_rollback_retry=retry,c_generated=False))
        print(name,b['registration_status'],b['code'],cli.returncode)
Path(sys.argv[5] if len(sys.argv)>5 else 'p319-observations.json').write_text(json.dumps(rows,indent=2,ensure_ascii=False)+'\n')
