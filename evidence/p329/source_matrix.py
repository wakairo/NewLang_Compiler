#!/usr/bin/env python3
"""P329 source-only observations; assertions distinguish safety/profile/backend."""
from pathlib import Path
import hashlib, json, subprocess, sys, tempfile
compiler, observer, boundary, calibration_observer, output = sys.argv[1:6]
repo = Path(__file__).resolve().parents[2]
s = (repo/'evidence/p329/returned_owner.nl').read_text()
def one(text, old, new):
    assert text.count(old) == 1, (old, text.count(old))
    return text.replace(old, new)
join = 'let returned=assemble(receiver,detached,ptr_B);'
terminal = 'finish_two(returned);'
# Wrong components stay live through harmless PURE transport. Move C cleanup
# after the requiring B/dst call so C's own terminal cannot mask B's failure.
c_start=s.index('                                            let empty_C=',s.index(join))
c_end=s.index('                                            let empty_src=',c_start)
c_cleanup=s[c_start:c_end]
before_c=s.replace(c_cleanup,'').replace(terminal,terminal+'\n'+c_cleanup)
wrong_a=one(before_c,'let detached=LiveRoot{p:final_ptr_B,a:final_a_B,d:final_d_B};','let detached=LiveRoot{p:final_ptr_B,a:allocation_C,d:final_d_B};')
wrong_a=one(wrong_a,'deallocate(allocation_C,full_C)','deallocate(final_a_B,full_C)')
wrong_d=one(before_c,'let detached=LiveRoot{p:final_ptr_B,a:final_a_B,d:final_d_B};','let detached=LiveRoot{p:final_ptr_B,a:final_a_B,d:life_C};')
wrong_d=one(wrong_d,'loan_exclusive_read(life_C){|ending_C|','loan_exclusive_read(final_d_B){|ending_C|')
cases=[
 ('composed-returned-only',s,0,'','component-positive'),
 ('alpha-types-and-functions',s.replace('Node','Element').replace('LiveRoot','Ticket').replace('DetachedHolder','Staging').replace('TreeBOwner','Pair').replace('assemble','pack').replace('finish_root','release_one').replace('finish_two','release_pair').replace('set_link','clear'),0,'','component-positive'),
 ('H0-no-join-independent-caller-custody',(repo/'tests/fixtures/experimental_cross_root_c_prev.nl').read_text(),0,'','legal-component-negative'),
 ('first-falsifier-inline-donor-packet',(repo/'evidence/p329/first_falsifier.nl').read_text(),3,'CAPTURED-CLOSURE-PRECISION','precision'),
 ('consumed-Holder-reused',one(s,join,'let duplicate=transferred; '+join),1,'P3-USE-AFTER-CONSUME','semantic'),
 ('B-owner-reused-after-call',one(s,join,join+' let duplicate=detached;'),1,'P3-USE-AFTER-CONSUME','semantic'),
 ('dst-owner-reused-after-call',one(s,join,join+' let duplicate=receiver;'),1,'P3-USE-AFTER-CONSUME','semantic'),
 ('returned-owner-reused-after-terminal',one(s,terminal,terminal+' let duplicate=returned;'),1,'P3-USE-AFTER-CONSUME','semantic'),
 ('active-B-loan-before-packet-move',one(s,'let detached=LiveRoot{p:final_ptr_B,a:final_a_B,d:final_d_B};','loan_read(final_d_B){|hold|let detached=LiveRoot{p:final_ptr_B,a:final_a_B,d:final_d_B};unit};'),1,'P3-REF-CONFLICT','semantic'),
 ('wrong-B-Allocation-at-requiring-terminal',wrong_a,1,'P193-CALL-BACKING','semantic'),
 ('wrong-B-Domain-at-requiring-terminal',wrong_d,1,'P193-CALL-DOMAIN','semantic'),
 ('callee-duplicate-return-member',one(s,'root:receiver,child:detached','root:detached,child:detached'),1,'P3-USE-AFTER-CONSUME','semantic'),
 ('missing-returned-owner-cleanup',one(s,terminal,''),1,'P5-SCOPE-OBLIGATION','semantic'),
 ('sixth-independent-H',one(s,join,'match try_allocate_one<Node>(){None=>{unit},Some(extra)=>{let OneBacking{allocation,storage}=extra;deallocate(allocation,storage);unit}};'+join),2,'ALLOCATED-CARDINALITY-PROFILE','unsupported'),
 ('early-B-EndRoot-with-active-ref',one(s,'set_link(write_B_next@next,Option<ptr<Node>>::None);','let early=loan_exclusive_read(moved_d_B){|ending|destroy(moved_ptr_B,ending)};set_link(write_B_next@next,Option<ptr<Node>>::None);'),1,'P3-REF-CONFLICT','semantic'),
]
rows=[]
with tempfile.TemporaryDirectory(prefix='p329-source-') as tmp:
    for name,text,status,code,category in cases:
        path=Path(tmp)/(name+'.nl'); path.write_text(text)
        runs=[subprocess.run([compiler,str(path)],capture_output=True,text=True) for _ in range(2)]
        a,b=runs
        assert (a.returncode,a.stdout,a.stderr)==(b.returncode,b.stdout,b.stderr)
        bound=json.loads(subprocess.run([boundary,str(path)],capture_output=True,text=True,check=True).stdout)
        assert bound['parse_status']==0 and bound['registration_status']==status and bound['code']==code,(name,bound)
        assert a.returncode==(3 if status else 4) and not a.stdout,(name,a.returncode,a.stderr)
        owned=[]; retry=False
        if status:
            assert bound['snapshot_unchanged'] and not bound['owned_entry_artifact']
            r=subprocess.run([observer,str(repo/'evidence/p329/returned_owner.nl'),'after-refusal',str(path)],capture_output=True,text=True,check=True)
            retry=json.loads(r.stdout.splitlines()[-1])['registration_refusal_rollback_retry']; assert retry
        else:
            tool=calibration_observer if category=='legal-component-negative' else observer
            r=subprocess.run([tool,str(path)],capture_output=True,text=True,check=True)
            owned=[json.loads(line) for line in r.stdout.splitlines()]
            assert owned[0]['owned_entry_artifact']
            if category=='component-positive':
                assert owned[-1]['exact_result_is_only_terminal_argument'] and owned[-1]['release_order']==[2,4,1,3,5]
        def position(byte):
            prefix=text.encode()[:byte].decode();return [prefix.count('\n')+1,len(prefix.rsplit('\n',1)[-1])+1]
        rows.append(dict(name=name,category=category,sha256=hashlib.sha256(text.encode()).hexdigest(),cli_exit=a.returncode,diagnostic=a.stderr.replace(str(path),'<source>'),boundary=bound,span_start=position(bound['start_byte']),span_end=position(bound['end_byte']),owned=owned,registration_refusal_rollback_retry=retry,c_generated=False))
        print(name,status,code,a.returncode,flush=True)
Path(output).write_text(json.dumps(rows,indent=2)+'\n')
