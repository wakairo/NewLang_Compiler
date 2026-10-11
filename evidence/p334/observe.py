#!/usr/bin/env python3
"""P334 fails closed: calibration succeeds, both ninth-Change shapes refuse."""
from pathlib import Path
import hashlib,json,subprocess,sys
compiler,observer,boundary,output=sys.argv[1:5]
root=Path(__file__).resolve().parent
baseline=root/'calibration_returned_owner.nl'
base=baseline.read_text()
assert hashlib.sha256(base.encode()).hexdigest()=='bd7692f8118c8e4de2b7662b0f90fcf503a548d9948bfded27bd430ad127716d'
cases=[('calibration_returned_owner.nl',0,'',0,0),
       ('dst_child_return_old.nl',3,'CAPTURED-CLOSURE-PRECISION',14661,14728),
       ('dst_child_scope_unit.nl',3,'CAPTURED-CLOSURE-PRECISION',14659,14726)]
rows=[]
for filename,status,code,start,end in cases:
    path=root/filename; text=path.read_text()
    assert text.count('try_allocate_one<Node>()')==5
    if status:
        at=text.index('                                            ',text.index('replace(write_C_detach@prev'))
        lo=text.index('                                            ',text.index('};',at)+2)
        hi=text.index('                                            let rebuilt=',lo)
        # Each new file differs from the calibrated source ONLY by its one
        # ended original-dst loan with a single direct child replace.
        added=text[lo:hi]
        assert text[:lo]+text[hi:]==base
        assert added.count('loan_read(life_dst)')==1
        assert added.count('ref_from_ptr(write,ptr_dst,stable_dst_adopt)')==1
        assert added.count('replace(write_dst_adopt@child,Option<ptr<Node>>::Some(moved_ptr_B))')==1
        assert '@prev' not in added and '@next' not in added
    runs=[subprocess.run([compiler,str(path)],capture_output=True,text=True) for _ in range(2)]
    a,b=runs
    assert (a.returncode,a.stdout,a.stderr)==(b.returncode,b.stdout,b.stderr)
    assert a.returncode==(3 if status else 4) and not a.stdout
    bound=json.loads(subprocess.run([boundary,str(path)],capture_output=True,text=True,check=True).stdout)
    assert bound['parse_status']==0 and bound['registration_status']==status and bound['code']==code
    assert bound['start_byte']==start and bound['end_byte']==end
    args=[observer,str(baseline)]
    if status:
        assert bound['snapshot_unchanged'] and not bound['entry_check_attempted'] and not bound['owned_entry_artifact'] and not bound['backend_attempted']
        args+=['after-refusal',str(path)]
    observed=subprocess.run(args,capture_output=True,text=True,check=True)
    owned=[json.loads(line) for line in observed.stdout.splitlines()]
    assert owned[0]['owned_entry_artifact']
    pure=next(x for x in owned if 'pure_call' in x)
    assert pure['exact_result_is_only_terminal_argument'] and pure['two_root_validator_status']==0 and pure['release_order']==[2,4,1,3,5]
    assert len([x for x in owned if 'call_ordinal' in x])==2
    if status:assert owned[-1]['registration_refusal_rollback_retry']
    def pos(byte):
        prefix=text.encode()[:byte].decode();return [prefix.count('\n')+1,len(prefix.rsplit('\n',1)[-1])+1]
    row=dict(source=filename,sha256=hashlib.sha256(text.encode()).hexdigest(),compiler='de9c4b175784f484675b670a109f62edeaf35de1',cli_exit=a.returncode,diagnostic=a.stderr.replace(str(path),'<source>'),boundary=bound,span_start=pos(start),span_end=pos(end),new_adoption_owned_artifact=False,baseline_owned_or_retry=owned,actual_source_generated_clean_retry=bool(status),c_generated=False)
    rows.append(row)
    print(f'P334_ASSERT {filename} parse=0 registration={status} CLI={a.returncode} code={code or "accepted/backend-unsupported"} baseline_owned_validators=PASS rollback_retry={bool(status)}',flush=True)
Path(output).write_text(json.dumps(rows,indent=2)+'\n')
print('P334_HOLD_BOTH_NINTH_CHANGE_SHAPES_PRECISION_REFUSED_WITH_ROLLBACK_AND_OWNED_RETRY',flush=True)
