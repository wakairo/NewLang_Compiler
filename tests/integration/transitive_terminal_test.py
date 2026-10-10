"""Issue 278 source inputs, independently fixed expected statuses and spans."""
import hashlib
import json
import pathlib
import re
import subprocess
import sys
import tempfile

compiler, evidence, closure = (str(pathlib.Path(x).resolve()) for x in sys.argv[1:4])
repo = pathlib.Path(__file__).resolve().parents[2]
source = (repo/'tests/fixtures/experimental_transitive_terminal.nl').read_text()
old = (repo/'tests/fixtures/experimental_nested_caller.nl').read_text()


def change(s, old, new):
    assert old in s, old
    return s.replace(old, new, 1)


def wrap(s):
    s=change(s,'fn main()', 'fn finish_two(whole:TreeTwo)->unit {let TreeTwo{root,child}=whole; finish_root(child); finish_root(root); unit}\nfn main()')
    s=change(s,'finish_root(tail);','let pair=TreeTwo{root:root,child:tail}; finish_two(pair);')
    return change(s,'                                            finish_root(root);\n','')


before_c = wrap(old)
wrong_a = change(before_c,'a: allocation_B, d: life_B','a: allocation_C, d: life_B')
wrong_d = change(before_c,'let pair=TreeTwo{root:root,child:tail}; finish_two(pair);','{let LiveRoot{p,a,d}=tail; let wrong=LiveRoot{p:p,a:a,d:life_C}; let pair=TreeTwo{root:root,child:wrong}; finish_two(pair);};')
mixed = change(wrong_a,'let pair=TreeTwo{root:root,child:tail}; finish_two(pair);','''let restored={let LiveRoot{p,a,d}=tail; let c_allocation=a;
let good=LiveRoot{p:p,a:allocation_B,d:d}; let pair=TreeTwo{root:root,child:good}; finish_two(pair); c_allocation};''')
mixed = change(mixed,'deallocate(allocation_C, full_C);\n                                            let empty_src','deallocate(restored, full_C);\n                                            let empty_src')
call='let pair=TreeTwo{root:root,child:tail}; finish_two(pair);'
definition='fn finish_two(whole:TreeTwo)->unit {let TreeTwo{root,child}=whole; finish_root(child); finish_root(root); unit}'

cases=[
    ('after-A-C-src-only-TreeTwo',source,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','after-death'),
    ('before-C-src-two-independent',before_c,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','matched'),
    ('mixed-ordinary-return-repaired',mixed,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','mixed'),
    ('wrong-original-C-Allocation-at-finish-two',wrong_a,3,'P193-CALL-BACKING','semantic-reject',None),
    ('wrong-original-C-Domain-at-finish-two',wrong_d,3,'P193-CALL-DOMAIN','semantic-reject',None),
    ('duplicate-original-Allocation',change(source,'let repacked =','let ghost=allocation_B; let repacked ='),3,'P3-USE-AFTER-CONSUME','semantic-reject',None),
    ('duplicate-original-Domain',change(source,'let repacked =','let ghost=life_B; let repacked ='),3,'P3-USE-AFTER-CONSUME','semantic-reject',None),
    ('donor-after-assembler-return',change(source,'let TreeTwo { root, child }','let ghost=detached; let TreeTwo { root, child }'),3,'P3-USE-AFTER-CONSUME','semantic-reject',None),
    ('result-after-consume',change(source,call,call+' let ghost=pair;'),3,'P3-USE-AFTER-CONSUME','semantic-reject',None),
    ('callee-duplicate-member',change(source,'finish_root(root); unit}','finish_root(child); unit}'),3,'P278-DEFINITION-CONSUMED','semantic-reject',None),
    ('callee-loses-second-member',change(source,'finish_root(root); unit}','unit}'),3,'P278-DEFINITION-OBLIGATION','semantic-reject',None),
    ('live-local-Domain-loan-at-call',change(source,call,'let LiveRoot{p,a,d}=tail; loan_read(d){|hold|let tail=LiveRoot{p:p,a:a,d:d}; let pair=TreeTwo{root:root,child:tail}; finish_two(pair); unit};'),3,'P3-REF-CONFLICT','semantic-reject',None),
    ('live-derived-H-at-call',change(source,call,'let LiveRoot{p,a,d}=tail; loan_read(d){|hold|let h=ref_from_ptr(write,p,hold); let tail=LiveRoot{p:p,a:a,d:d}; let pair=TreeTwo{root:root,child:tail}; finish_two(pair); unit};'),3,'P3-REF-CONFLICT','semantic-reject',None),
    ('whole-value-loan-at-call',change(source,call,'let pair=TreeTwo{root:root,child:tail}; loan_read(pair){|hold|finish_two(pair); unit};'),3,'LOCAL-LOAN-PROFILE','semantic-profile-unsupported',None),
    ('stale-original-B',change(source,call,'let LiveRoot{p,a,d}=tail; let stale=loan_exclusive_read(d){|end|destroy(p,end)}; let stale_ticket=LiveRoot{p:p,a:a,d:d}; '+call.replace('child:tail','child:stale_ticket')),3,'P193-CALL-ROOT','semantic-reject',None),
    ('old-A-pointer-at-terminal',change(source,call,'let LiveRoot{p,a,d}=tail; let stale_ticket=LiveRoot{p:ptr_A,a:a,d:d}; '+call.replace('child:tail','child:stale_ticket')),3,'P193-CALL-ROOT','semantic-reject',None),
    ('wrong-full-raw-body',change(source,'deallocate(a, full);','deallocate(a, empty);'),3,'P193-DEFINITION-CONSUMED','semantic-reject',None),
    ('old-A-head-terminal-body',change(source,'deallocate(a, full);','destroy(ptr_A,d); deallocate(a, full);'),3,'P193-DEFINITION-ORDER','semantic-reject',None),
    ('unused-bad-root-definition',change(source,'deallocate(a, full);','unit;'),3,'P193-DEFINITION-OBLIGATION','semantic-reject',None),
    ('nested-branch-loses-owner',change(source,definition,'fn finish_two(whole:TreeTwo)->unit {let TreeTwo{root,child}=whole; if (true) {finish_root(child); finish_root(root); unit} else {finish_root(child); unit}}'),3,'P278-MEMBER-PATH-PRECISION','semantic-precision-unsupported',None),
    ('explicit-return-wrapper',change(source,'finish_root(root); unit}','finish_root(root); return unit;}'),3,'P278-MEMBER-PATH-PRECISION','semantic-precision-unsupported',None),
    ('third-transitive-layer',change(source,'fn main()','fn finish_outer(whole:TreeTwo)->unit {finish_two(whole); unit}\nfn main()'),3,'P278-MEMBER-PATH-PRECISION','semantic-precision-unsupported',None),
    ('temporary-terminal-argument',change(source,call,'finish_two(TreeTwo{root:root,child:tail});'),3,'P278-CALL-ARGUMENT','semantic-precision-unsupported',None),
    ('partial-root-parser',change(source,'a: Allocation, d: LifetimeDomain','a: Allocation'),3,'AVS-DECL-PROFILE','parser-unsupported',None),
]
alpha=source.replace('Node','Cell').replace('LiveRoot','Cargo').replace('TreeTwo','OrdinaryPair').replace('finish_two','dispose_pair').replace('finish_root','release_ticket').replace('assemble','move_record')
cases.append(('all-type-function-names-renamed',alpha,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','after-death'))
alpha_bad=wrong_a.replace('Node','Cell').replace('LiveRoot','MatchedOwner').replace('TreeTwo','BlessedTree').replace('finish_two','proof_of_safety').replace('finish_root','grant')
cases.append(('blessed-names-mixed-Allocation',alpha_bad,3,'P193-CALL-BACKING','semantic-reject',None))
reorder=change(source,'let TreeTwo{root,child}=whole','let TreeTwo{child,root}=whole')
reorder=change(reorder,'let LiveRoot { p, a, d } = ticket','let LiveRoot { d, a, p } = ticket')
cases.append(('whole-pattern-field-order',reorder,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','after-death'))
swap=change(source,'finish_root(child); finish_root(root); unit}','finish_root(root); finish_root(child); unit}')
cases.append(('terminal-path-order-reversed',swap,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','matched'))

# Move the last B write before assemble. The delayed terminal now consumes
# EXACTLY the returned current whole value, without destructuring/repacking it.
a=source.index('                                            let receiver =')
z=source.index('                                            let old_C_prev =',a)
b=source.index('                                            let tail =',a)
receiver=source[a:source.index('                                            let detached =',a)]
tail=source[b:z].replace('} = child;', '} = detached;')
direct=source[:a]+receiver+'                                            let detached=repacked;\n'+tail+'                                            let adopted=assemble(receiver,tail);\n'+source[z:]
direct=change(direct,call,'finish_two(adopted);')
cases.append(('after-death-exact-returned-whole',direct,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','after-death'))
cases.append(('independent-dst-wrong-Allocation',change(before_c,'a: allocation_dst, d: life_dst','a: allocation_C, d: life_dst'),3,'P193-CALL-BACKING','semantic-reject',None))
cases.append(('independent-dst-wrong-Domain',change(before_c,call,'{let LiveRoot{p,a,d}=root; let bad_root=LiveRoot{p:p,a:a,d:life_C}; let pair=TreeTwo{root:bad_root,child:tail}; finish_two(pair);};'),3,'P193-CALL-DOMAIN','semantic-reject',None))
cases.append(('current-child-duplicate',change(source,'let pair=TreeTwo{root:root,child:tail}','let pair=TreeTwo{root:tail,child:tail}'),3,'P3-USE-AFTER-CONSUME','semantic-reject',None))
cases.append(('old-A-Domain-use-after-death',change(source,call,'loan_read(life_A){|old| unit}; '+call),3,'P3-USE-AFTER-CONSUME','semantic-reject',None))
# Physical function declaration order supplies no premise for summary inference.
root_definition=source[source.index('fn finish_root'):source.index('fn finish_two')]
decl_order=source.replace(root_definition,'').replace('fn main()',root_definition+'fn main()',1)
cases.append(('independent-definition-order',decl_order,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','after-death'))

cases.append(('unused-invalid-formal-shadow',change(source,definition,definition.replace('whole','root')),3,'P278-DEFINITION-NAME','semantic-reject',None))

# Keep the predecessor H0 actual source control: caller-owned B and Copy B
# pointer in dst.child are legal core custody, not the H1 one-result predicate.
legacy={'__file__':str(repo/'tests/integration/nested_caller_test.py')}
exec((repo/'tests/integration/nested_caller_test.py').read_text().split('# Expected values')[0],legacy)
h0=change(legacy['h0'],'fn main()',definition+'\nfn main()')
h0=change(h0,'                                            finish_root(child);','let pair=TreeTwo{root:root,child:child}; finish_two(pair);')
h0=change(h0,'                                            finish_root(root);\n','')
cases.append(('H0-independent-caller-B-core-safe',h0,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','h0'))


def run(root):
    observations=[]
    for name,text,status,code,category,proof in cases:
        path=root/(name+'.nl'); path.write_text(text)
        before=set(root.iterdir())
        a=subprocess.run([compiler,path.name],cwd=root,capture_output=True)
        b=subprocess.run([compiler,path.name],cwd=root,capture_output=True)
        assert (a.returncode,a.stdout,a.stderr)==(b.returncode,b.stdout,b.stderr),name
        assert a.returncode==status and code.encode() in a.stderr and not a.stdout,(name,a.returncode,a.stderr.decode())
        assert set(root.iterdir())==before,name
        if category=='semantic-reject': assert b'error(semantic)' in a.stderr,(name,a.stderr)
        if category=='semantic-precision-unsupported': assert b'error(precision)' in a.stderr
        if category.endswith('profile-unsupported') or category=='parser-unsupported': assert b'error(unsupported)' in a.stderr
        observed=subprocess.run([evidence,str(path),proof or 'reject:'+code],capture_output=True)
        # Parser rejection is measured by CLI; semantic public registration has no parsed unit.
        if category=='parser-unsupported': result=None
        else:
            assert observed.returncode==0 and not observed.stderr,(name,observed.stderr.decode())
            result=json.loads(observed.stdout)
            if proof:
                assert result['transitive_source_requirements'] and result['whole_current_at_terminal_entry']
                if proof!='h0':
                    closed=subprocess.run([closure,'full-evidence',str(path),'5'],capture_output=True)
                    assert closed.returncode==0,(name,closed.stderr.decode())
                assert sorted(result['release_order'])==[1,2,3,4,5]
                if name=='after-death-exact-returned-whole': assert result['terminal_is_exact_returned_whole']
                if proof=='h0': assert not result['h1_result_members_dst_B']
                if proof=='mixed': assert result['ordinary_return_allocation_regions']==[5,4] and result['mixed_ordinary_return']
                if proof=='after-death': assert result['A_C_src_ended_before_call']==3 and result['release_order']==[2,4,1,3,5]
            else:
                assert result['snapshot_unchanged'] and not result['owned_artifact']
                result['source_span']=text[result['start_byte']:result['end_byte']]
                if code in ('P193-CALL-BACKING','P193-CALL-DOMAIN','P193-CALL-ROOT'):
                    assert 'finish_two(pair)' in result['source_span'] or 'proof_of_safety(pair)' in result['source_span']
        observations.append({'input':name,'source_sha256':hashlib.sha256(text.encode()).hexdigest(),'classification':category,'cli_exit':a.returncode,'diagnostic':a.stderr.decode(),'repeat_identical':True,'source_owned_evidence':result})
    return {'kind':'UNADOPTED P278 transitive terminal evidence','native_execution':False,'observations':observations}

if len(sys.argv)>4:
    root=pathlib.Path(sys.argv[4]).resolve(); root.mkdir(parents=True,exist_ok=True)
    result=run(root); (root/'observations.json').write_text(json.dumps(result,indent=2)+'\n')
else:
    with tempfile.TemporaryDirectory(prefix='p278-terminal-') as directory: result=run(pathlib.Path(directory))
print(json.dumps(result,indent=2))
