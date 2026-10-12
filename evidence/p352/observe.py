#!/usr/bin/env python3
"""Actual source admission, PUBLIC proof, owned corruption, rollback and OOM."""
from pathlib import Path
import hashlib,json,subprocess,sys,os,shlex
root=Path(__file__).resolve().parents[2];build=Path(sys.argv[1]).resolve();frozen=Path(sys.argv[2]).resolve()
pins=json.loads((root/'evidence/p352/source_hashes.json').read_text())
oldpins=json.loads((root/'evidence/p338/source_hashes.json').read_text())
for directory,manifest in [('p352',pins),('p338',oldpins)]:
    for name,digest in manifest.items():assert hashlib.sha256((root/'evidence'/directory/name).read_bytes()).hexdigest()==digest
prefixes=[]
for name,count in [('w1_device_refresh',2),('w2_scheduler_rotation',4)]:
    source=root/'evidence/p338'/(name+'.nl');negative=source.with_name(name+'_wrong_domain.nl')
    result=subprocess.run([str(build/'public_probe'),str(source),str(count),str(negative)],capture_output=True,text=True,check=True)
    records=[json.loads(x) for x in result.stdout.splitlines()];assert not result.stderr
    assert next(x for x in records if 'source_owned_publicly_validated' in x)['events']==count
    prefixes.append({'source':source.name,'records':records})
rows=[]
required={'missing_ninth_owned_record','duplicate_ninth_owned_record','reordered_ninth_owned_record','missing_checked_replace','spliced_source_span','duplicated_owned_event_node','wrong_original_domain_proof','unknown_write_ref_provenance','old_some_occurrence_revived','nonfresh_post_occurrence_claim','unrelated_fourth_scalar_fact','disconnected_valid_primitive_pair','unmodelled_intermediate_effect'}
for motif,name in [('W1','w1_device_refresh_9'),('W2','w2_scheduler_rotation_9')]:
    source=root/'evidence/p352'/(name+'.nl');negative=source.with_name(name+'_wrong_domain.nl')
    b=subprocess.run([str(frozen/'five_root_source_gate_evidence'),str(source)],capture_output=True,text=True,check=True)
    boundary=json.loads(b.stdout);assert boundary['parse_status']==0 and boundary['registration_status']==3 and boundary['code']=='CAPTURED-CLOSURE-PRECISION' and boundary['snapshot_unchanged'] and not boundary['owned_entry_artifact']
    first=source.read_text()[boundary['start_byte']:boundary['end_byte']];assert first.startswith('replace(write_7@')
    frozen_cli=subprocess.run([str(frozen/'newlangc'),str(source)],capture_output=True,text=True);assert frozen_cli.returncode==3 and 'CAPTURED-CLOSURE-PRECISION' in frozen_cli.stderr
    positive_boundary=json.loads(subprocess.check_output([str(build/'five_root_source_gate_evidence'),str(source)],text=True));assert positive_boundary['parse_status']==positive_boundary['registration_status']==0
    wrong_boundary=json.loads(subprocess.check_output([str(build/'five_root_source_gate_evidence'),str(negative)],text=True));assert wrong_boundary['parse_status']==0 and wrong_boundary['registration_status']==1 and wrong_boundary['code']=='ALLOCATED-DOMAIN-MISMATCH' and wrong_boundary['snapshot_unchanged']
    assert negative.read_text()[wrong_boundary['start_byte']:wrong_boundary['end_byte']].startswith('ref_from_ptr(write,')
    cli=subprocess.run([str(build/'newlangc'),str(source)],capture_output=True,text=True);assert cli.returncode==4 and 'V1-BACKEND-UNSUPPORTED' in cli.stderr
    wrong_cli=subprocess.run([str(build/'newlangc'),str(negative)],capture_output=True,text=True);assert wrong_cli.returncode==3 and 'ALLOCATED-DOMAIN-MISMATCH' in wrong_cli.stderr
    real=subprocess.run([str(build/'public_probe'),str(source),'9',str(negative)],capture_output=True,text=True,check=True)
    assert not real.stderr
    real_public=[json.loads(x) for x in real.stdout.splitlines()]
    assert len([x for x in real_public if x.get('public_validator_status')==0 and 'captured_depth' in x])==5
    assert next(x for x in real_public if 'source_owned_publicly_validated' in x)['events']==9
    args=[str(build/'p352-tests/harness'),str(source),str(negative)]
    a=subprocess.run(args,capture_output=True,text=True,check=True);b=subprocess.run(args,capture_output=True,text=True,check=True)
    assert a.stdout==b.stdout and not a.stderr and not b.stderr
    records=[json.loads(x) for x in a.stdout.splitlines()]
    assert records[0]['refused_registration_status']==1 and records[0]['snapshot_unchanged']
    assert len([x for x in records if x.get('public_validator_status')==0 and 'captured_depth' in x])==5
    events=[x for x in records if 'event' in x];assert len(events)==9 and [x['event'] for x in events]==list(range(9))
    assert all(x['unchanged_siblings']==19 and x['scope_ended'] and x['original_A_D_unique_and_preserved'] for x in events)
    proof=next(x for x in records if 'source_owned_publicly_validated' in x);assert proof['events']==9 and proof['static_release_worlds_0_to_5']==[1]*6
    poison=[x for x in records if x.get('case') in required and 'public_validator_status' in x]
    assert len(poison)==13 and {x['case'] for x in poison}==required and all(x['public_validator_status']==3 for x in poison)
    assert next(x for x in records if 'individual_primitive_status' in x)['individual_primitive_status']==0
    reg=[x for x in records if x.get('case')=='registration_OOM'];entry=[x for x in records if x.get('case')=='entry_OOM'];verify=[x for x in records if x.get('case')=='public_validator_OOM']
    assert len(reg)==len(entry)==11 and len(verify)==8 and all(x['status']==4 for x in reg+entry+verify)
    assert [x['ninth_phase'] for x in reg[:3]]==[1,2,3] and [x['ninth_phase'] for x in entry[:3]]==[1,2,3]
    assert all(x['snapshot_unchanged'] and x['same_context_retry'] for x in reg)
    assert all(x['snapshot_unchanged'] and x['failed_owned_null'] and x['same_context_owned_nine_retry'] for x in entry)
    rows.append(dict(motif=motif,source=source.name,sha256=pins[source.name],negative_sha256=pins[negative.name],frozen_boundary=boundary,frozen_cli_exit=frozen_cli.returncode,frozen_diagnostic=frozen_cli.stderr,first_operation=first,candidate_boundary=positive_boundary,wrong_domain_boundary=wrong_boundary,candidate_cli_exit=cli.returncode,diagnostic=cli.stderr,wrong_diagnostic=wrong_cli.stderr,normal_public_records=real_public,records=records))
    print(f'P352_ASSERT {motif}-9 frozen_event7_precision actual_owned9 PUBLIC5=0 poison13_REJECT registration_OOM11 entry_OOM11 validator_OOM8 SAME_CONTEXT_RETRY PASS',flush=True)
# Test hooks belong exclusively to copied test archive; normal API/ABI unchanged.
assert b'nl_test_p352_' not in subprocess.check_output(['nm','-g',str(build/'libnewlang_semantic.a')])
result={'base':'cc1707186e1338d71fdfba26bf90134d54f5de3e','revision_at_observation':subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),'uncommitted_changes_at_observation':bool(subprocess.check_output(['git','status','--porcelain'],text=True).strip()),'new_default_off_option':'NEWLANG_EXPERIMENTAL_FINITE_CHANGE_REPLAY','frozen_compiler_sha256':hashlib.sha256((frozen/'newlangc').read_bytes()).hexdigest(),'candidate_compiler_sha256':hashlib.sha256((build/'newlangc').read_bytes()).hexdigest(),'sanitizer':os.environ.get('P352_SANITIZER','none'),'build_type':os.environ.get('P352_BUILD_TYPE','Debug'),'prefixes':prefixes,'cases':rows}
(build/'p352-observations.json').write_text(json.dumps(result,indent=2)+'\n')
