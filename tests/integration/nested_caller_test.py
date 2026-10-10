"""Issue 276 source-derived opt-in falsifiers; backend UNSUPPORTED != semantic error.
The public observer never seeds a fact or supplies a grant to the checker.
"""
import hashlib
import json
import pathlib
import re
import subprocess
import sys
import tempfile

compiler, evidence, closure = (str(pathlib.Path(x).resolve()) for x in sys.argv[1:4])
repo = pathlib.Path(__file__).resolve().parents[2]
source = (repo / 'tests/fixtures/experimental_nested_caller.nl').read_text()
assert source.count('try_allocate_one<Node>()') == 5 and source.count('replace(') == 6


def change(s, old, new):
    assert old in s, old
    return s.replace(old,new,1)


wrong = change(source,'a: allocation_B, d: life_B','a: allocation_C, d: life_B')
mixed = change(wrong,'finish_root(tail);','''let restored = {
                                                let LiveRoot {p,a,d} = tail;
                                                let c_allocation = a;
                                                let good = LiveRoot {p:p,a:allocation_B,d:d};
                                                finish_root(good);
                                                c_allocation
                                            };''')
mixed = change(mixed,'deallocate(allocation_C, full_C);\n                                            let empty_src','deallocate(restored, full_C);\n                                            let empty_src')
wrong_domain = change(source,'finish_root(tail);','{let LiveRoot{p,a,d}=tail; let wrong=LiveRoot{p:p,a:a,d:life_C}; finish_root(wrong);};')
start=source.index('                                            let empty_A =',source.index('let tail ='))
end=source.index('                                            unit',start)
terminal=source[start:end]
a=terminal[:terminal.index('                                            finish_root(tail);')]
b='                                            finish_root(tail);\n'
c=terminal[terminal.index('                                            let empty_C ='):terminal.index('                                            let empty_src =')]
src=terminal[terminal.index('                                            let empty_src ='):terminal.index('                                            finish_root(root);')]
dst='                                            finish_root(root);\n'
after_a=source[:start]+a+c+src+b+dst+source[end:]
reversed_result=change(source,'root:first, child:second','root:second, child:first')
reversed_result=change(reversed_result,'} = child;','} = root;')
reversed_result=change(reversed_result,'finish_root(root);','finish_root(child);')
identity=change(source,'fn assemble(first:LiveRoot, second:LiveRoot)->TreeTwo {\n    let combined = TreeTwo { root:first, child:second };\n    return combined;\n}','fn transfer(whole:TreeTwo)->TreeTwo { return whole; }')
identity=change(identity,'assemble(receiver, detached)','transfer(TreeTwo{root:receiver,child:detached})')
# The bounded call proof requires a current named caller packet, not an
# expression temporary. This positive has one pre-call current whole value.
identity=change(identity,'let adopted = transfer(TreeTwo{root:receiver,child:detached});','let before = TreeTwo{root:receiver,child:detached}; let adopted = transfer(before);')
finish_two=change(source,'fn main()', 'fn finish_two(whole:TreeTwo)->unit {let TreeTwo{root,child}=whole; finish_root(child); finish_root(root); unit}\nfn main()')
finish_two=change(finish_two,'finish_root(tail);','let pair=TreeTwo{root:root,child:tail}; finish_two(pair);')
finish_two=change(finish_two,'                                            finish_root(root);\n','')
# H0: B stays separately and legitimately owned in this caller. The returned
# result has dst+C, and dst.child holds only a Copy ptr to B. This is core-safe
# with complete cleanup, while the particular returned value lacks original B.
receiver_start=source.index('                                            let receiver =')
cprev_start=source.index('                                            let old_C_prev =',receiver_start)
cprev_end=source.index('                                            let empty_A =',cprev_start)
tail_start=source.index('                                            let tail =',receiver_start)
tail_end=cprev_start
receiver=source[receiver_start:source.index('                                            let detached =',receiver_start)]
tail=source[tail_start:tail_end].replace('} = child;','} = detached;')
cprev=source[cprev_start:cprev_end]
h0=source[:receiver_start]+receiver+'                                            let detached = repacked;\n'+tail+cprev+'''                                            let c_packet=LiveRoot{p:ptr_C,a:allocation_C,d:life_C};
                                            let adopted=assemble(receiver,c_packet);
                                            let TreeTwo{root,child}=adopted;
'''+a+b+'                                            finish_root(child);\n'+src+dst+source[end:]
h0=change(h0,'child: Option<ptr<Node>>::None,\n                                                    payload: u8(5)','child: Option<ptr<Node>>::Some(ptr_B),\n                                                    payload: u8(5)')

# Expected values are fixed falsifiable assertions, not learned classifications.
cases=[
    ('correct-two-originals',source,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','matched'),
    ('after-A-C-src-terminal',after_a,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','matched'),
    ('mixed-return-repaired',mixed,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','mixed'),
    ('swapped-result-core-safe',reversed_result,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','reversed'),
    ('whole-input-identity',identity,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','matched'),
    ('H0-result-without-B',h0,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','h0'),
    ('wrong-original-C-allocation',wrong,3,'P193-CALL-BACKING','semantic-reject',None),
    ('wrong-original-C-domain-at-terminal',wrong_domain,3,'P193-CALL-DOMAIN','semantic-reject',None),
    ('wrong-domain-before-B-write',change(source,'a: allocation_B, d: life_B','a: allocation_B, d: life_C'),3,'ALLOCATED-DOMAIN-MISMATCH','semantic-reject',None),
    ('duplicate-allocation',change(source,'let repacked =','let ghost=LiveRoot{p:ptr_B,a:allocation_B,d:life_C}; let repacked ='),3,'P3-USE-AFTER-CONSUME','semantic-reject',None),
    ('duplicate-domain',change(source,'let repacked =','let ghost=LiveRoot{p:ptr_B,a:allocation_C,d:life_B}; let repacked ='),3,'P3-USE-AFTER-CONSUME','semantic-reject',None),
    ('duplicate-current-packet-arguments',change(source,'assemble(receiver, detached)','assemble(detached, detached)'),3,'P3-USE-AFTER-CONSUME','semantic-reject',None),
    ('old-caller-packet-after-return',change(source,'let TreeTwo { root, child }','let ghost=detached; let TreeTwo { root, child }'),3,'P3-USE-AFTER-CONSUME','semantic-reject',None),
    ('phantom-original-allocation-after-call',change(source,'let TreeTwo { root, child }','let ghost=LiveRoot{p:ptr_B,a:allocation_B,d:life_C}; let TreeTwo { root, child }'),3,'P3-USE-AFTER-CONSUME','semantic-reject',None),
    ('second-current-result-reused',change(source,'let TreeTwo { root, child }','let ghost=adopted; let TreeTwo { root, child }'),3,'P3-USE-AFTER-CONSUME','semantic-reject',None),
    ('callee-duplicates-formal',change(source,'root:first, child:second','root:first, child:first'),3,'P3-USE-AFTER-CONSUME','semantic-reject',None),
    ('callee-reuses-returned-local',change(source,'return combined;','let ghost=combined; return combined;'),3,'P3-USE-AFTER-CONSUME','semantic-reject',None),
    ('partial-nested-pattern',change(source,'TreeTwo { root, child } = adopted','TreeTwo { root } = adopted'),3,'P5-AGGREGATE-FIELD-COUNT','semantic-reject',None),
    ('forgotten-second-owner',change(source,'finish_root(root);','unit;'),3,'P5-SCOPE-OBLIGATION','semantic-reject',None),
    ('duplicate-terminal',change(source,'finish_root(tail);','finish_root(tail); finish_root(tail);'),3,'P3-USE-AFTER-CONSUME','semantic-reject',None),
    ('live-domain-loan-over-repack',change(source,'LiveRoot { p: p, a: a, d: d }','loan_read(d){|still_live|LiveRoot{p:p,a:a,d:d}}'),3,'P3-REF-CONFLICT','semantic-reject',None),
    ('live-H-loan-over-repack',change(source,'LiveRoot { p: p, a: a, d: d }','loan_read(d){|still_live|let h=ref_from_ptr(write,p,still_live); LiveRoot{p:p,a:a,d:d}}'),3,'P3-REF-CONFLICT','semantic-reject',None),
    ('whole-loan-during-call',change(source,'let adopted = assemble(receiver, detached);','let adopted=loan_read(detached){|hold|assemble(receiver,detached)};'),3,'LOCAL-LOAN-PROFILE','semantic-profile-unsupported',None),
    ('wrong-terminal-body-order',change(source,'finalize_domain(d);\n    deallocate(a, full);','deallocate(a, full);\n    finalize_domain(d);'),3,'P193-DEFINITION-RELATION','semantic-reject',None),
    ('unused-bad-terminal-definition',change(source,'    deallocate(a, full);','    unit;'),3,'P193-DEFINITION-OBLIGATION','semantic-reject',None),
    ('named-finish-two-summary',finish_two,3,'P276-TERMINAL-SUMMARY-PRECISION','semantic-precision-unsupported',None),
    ('third-nested-layer',change(source,'fn assemble','struct Outer{left:TreeTwo,right:TreeTwo}\nfn assemble'),3,'P276-NESTED-PROFILE','semantic-profile-unsupported',None),
    ('partial-root-declaration',change(source,'a: Allocation, d: LifetimeDomain','a: Allocation'),3,'AVS-DECL-PROFILE','parser-unsupported',None),
]
alpha=source.replace('Node','Cell').replace('LiveRoot','Cargo').replace('TreeTwo','MatchedOwner').replace('assemble','finish_two').replace('finish_root','close_ticket').replace('ptr_','address_').replace('life_','governor_')
cases.append(('alpha-all-names',alpha,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','matched'))
alpha_wrong=wrong.replace('Node','Cell').replace('LiveRoot','Cargo').replace('TreeTwo','MatchedOwner').replace('assemble','finish_two').replace('finish_root','close_ticket')
cases.append(('blessed-names-wrong-allocation',alpha_wrong,3,'P193-CALL-BACKING','semantic-reject',None))
reorder=change(source,'root:first, child:second','child:second, root:first')
reorder=change(reorder,'let LiveRoot { p, a, d } = ticket','let LiveRoot { d, a, p } = ticket')
cases.append(('field-label-order',reorder,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','matched'))


wrong_pointer=change(source,'finish_root(tail);','{let LiveRoot{p,a,d}=tail; let wrong=LiveRoot{p:ptr_C,a:a,d:d}; finish_root(wrong);};')
cases.append(('wrong-original-pointer-at-terminal',wrong_pointer,3,'P193-CALL-DOMAIN','semantic-reject',None))
# Returning directly without a callee local is also ordinary custody. The
# observer currently selects a local-return incarnation contrast; the public
# closure observer still checks every original release for this source.
direct_return=change(source,'let combined = TreeTwo { root:first, child:second };\n    return combined;','return TreeTwo { root:first, child:second };')
cases.append(('direct-constructor-return',direct_return,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','direct'))
opaque=change(source,'assemble(receiver, detached)','external_make(receiver,detached)')
cases.append(('unknown-external-callee',opaque,3,'P3-UNKNOWN-CALLEE','semantic-reject',None))

labelled=source
for old,new in [('p','address'),('a','backing'),('d','governor'),('root','primary')]:
    labelled=re.sub(r'\b'+old+r'\b',new,labelled)
labelled=labelled.replace('child: LiveRoot','secondary: LiveRoot').replace('child:second','secondary:second').replace('primary, child','primary, secondary').replace('} = child;','} = secondary;')
cases.append(('arbitrary-field-labels',labelled,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','matched'))
nested_only=change(source,'assemble(receiver, detached)','TreeTwo{root:receiver,child:detached}')
cases.append(('nested-without-whole-result-call',nested_only,4,'V1-BACKEND-UNSUPPORTED','semantic-accepted/backend-unsupported','nested-only'))

explicit_terminal_return=change(source,'deallocate(a, full);\n    unit','deallocate(a, full);\n    return unit;')
cases.append(('terminal-explicit-return-profile',explicit_terminal_return,3,'CAPTURED-CLOSURE-PRECISION','semantic-precision-unsupported',None))
# A second whole-result call in the same arm needs a richer owned call list.
second_call=change(source,'let TreeTwo { root, child } = adopted;','let TreeTwo{root,child}=adopted; let adopted_again=assemble(root,child); let TreeTwo{root,child}=adopted_again;')
cases.append(('second-whole-result-call-profile',second_call,3,'P276-CALL-PROFILE','semantic-profile-unsupported',None))

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
        result=None
        if proof=='nested-only':
            closed=subprocess.run([closure,'full-evidence',str(path),'5'],capture_output=True)
            assert closed.returncode==0,(name,closed.stderr.decode())
            result={'owned_validator':True,'nested_source_only':True,'whole_call_certificate':False,'release_worlds':'0..5'}
        expectation=(None if proof=='nested-only' else proof) or ('reject:'+code if category.startswith('semantic-') and not proof else None)
        if expectation:
            observed=subprocess.run([evidence,str(path),expectation],capture_output=True)
            assert observed.returncode==0 and not observed.stderr,(name,observed.stderr.decode())
            result=json.loads(observed.stdout)
            if proof:
                assert result['owned_validator'] and result['ordinary_whole_return'] and result['unchanged_member_ids']
                # H0 changes dst.child's initial Copy value; the old topology
                # observer intentionally requires None there. New public
                # original closure validator remains the authority for both.
                if proof!='h0':
                    closed=subprocess.run([closure,'full-evidence',str(path),'5'],capture_output=True)
                    assert closed.returncode==0,(name,closed.stderr.decode())
                assert sorted(result['full_original_release_order'])==[1,2,3,4,5]
                if name=='after-A-C-src-terminal': assert result['full_original_release_order']==[2,4,1,3,5]
                if name=='mixed-return-repaired': assert result['allocation_regions']==[5,4] and not result['h1_dst_B_result']
                if proof in ('h0','reversed'): assert not result['h1_dst_B_result']
            else:
                assert result['snapshot_unchanged'] and not result['owned_artifact']
                if code in ('P193-CALL-BACKING','P193-CALL-DOMAIN'):
                    span=text[result['start_byte']:result['end_byte']]
                    assert '(' in span and ')' in span,(name,span)
                    result['source_span']=span
        observations.append({'input':name,'source_sha256':hashlib.sha256(text.encode()).hexdigest(),'classification':category,'cli_exit':a.returncode,'stdout_bytes':len(a.stdout),'diagnostic':a.stderr.decode(),'repeat_identical':True,'output_files_created':[],'source_owned_evidence':result})
    return {'kind':'UNADOPTED P276 empirical production-checker evidence','native_execution':False,'observations':observations}

if len(sys.argv)>4:
    root=pathlib.Path(sys.argv[4]).resolve(); root.mkdir(parents=True,exist_ok=True)
    result=run(root)
    (root/'observations.json').write_text(json.dumps(result,indent=2)+'\n')
else:
    with tempfile.TemporaryDirectory(prefix='p276-nested-caller-') as directory:
        result=run(pathlib.Path(directory))
print(json.dumps(result,indent=2))
