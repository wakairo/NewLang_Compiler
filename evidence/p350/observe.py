#!/usr/bin/env python3
"""Fail closed on real owned shadow proof, restorations and independent counterexample."""
from pathlib import Path
import hashlib,json,subprocess,sys,os
root=Path(__file__).resolve().parents[2];build=Path(sys.argv[1]).resolve()
pins=json.loads((root/'evidence/p338/source_hashes.json').read_text());rows=[]
for name,digest in pins.items():assert hashlib.sha256((root/'evidence/p338'/name).read_bytes()).hexdigest()==digest
required={'missing_tape_record','duplicate_tape_record','reordered_tape_records','missing_checked_replace','spliced_source_span',
          'duplicated_owned_event_node','wrong_original_domain_proof','unknown_write_ref_provenance','old_some_occurrence_revived',
          'nonfresh_post_occurrence_claim','unrelated_fourth_scalar_fact','disconnected_valid_primitive_pair','unmodelled_intermediate_effect'}
for motif,name,length in [('W1','w1_device_refresh',2),('W2','w2_scheduler_rotation',4)]:
    positive=root/'evidence/p338'/(name+'.nl');negative=root/'evidence/p338'/(name+'_wrong_domain.nl')
    args=[str(build/'p350-shadow/harness'),str(positive),str(negative)]
    a=subprocess.run(args,capture_output=True,text=True,check=True);b=subprocess.run(args,capture_output=True,text=True,check=True)
    assert a.stdout==b.stdout and not a.stderr and not b.stderr
    records=[json.loads(x) for x in a.stdout.splitlines()]
    assert records[0]['refused_registration_status']==1 and records[0]['snapshot_unchanged']
    proof=records[1];assert proof['status']==0 and proof['replayed']==proof['eligible']==length and proof['source_owned']
    assert proof['gaps_checked']==length-1 and proof['scalar_checks']>=length*5
    assert len([x for x in records if x.get('public_validator_status')==0 and 'captured_depth' in x])==5
    hostile=[x for x in records if 'shadow_status' in x];assert {x['case'] for x in hostile}==required and len(hostile)==len(required)
    assert all(x['shadow_status']==3 for x in hostile)
    tapes=[x for x in hostile if x['case'] in ['missing_tape_record','duplicate_tape_record','reordered_tape_records']]
    assert all(x['public_validator_status']==0 for x in tapes) # actual original artifact remains intact
    assert any(x['case']=='wrong_original_domain_proof' and x['public_validator_status']==3 for x in hostile)
    primitive=next(x for x in records if 'individual_primitive_status' in x);assert primitive['individual_primitive_status']==0
    assert next(x for x in hostile if x['case']=='disconnected_valid_primitive_pair')['reason']=='ORIGINAL_HEAP_ADJACENCY'
    failures=[x for x in records if x.get('case')=='shadow_OOM'];assert len(failures)==8 and all(x['status']==4 and x['same_artifact_clean_retry'] for x in failures)
    # Normal non-shadow libraries/binary remain unmodified and have no hook symbol.
    for file in ['libnewlang_semantic.a','newlangc']:
        assert b'nl_test_p350_' not in subprocess.check_output(['nm','-g',str(build/file)])
    rows.append(dict(motif=motif,source=positive.name,source_sha256=pins[positive.name],negative_source_sha256=pins[negative.name],
                     fixed_compiler='de9c4b175784f484675b670a109f62edeaf35de1',base='dc0fc7e7d2f9f92286973a4f5062089acf0d85f2',
                     shadow_only=True,records=records,sanitizer_environment={k:os.environ.get(k) for k in ['ASAN_OPTIONS','LSAN_OPTIONS','UBSAN_OPTIONS']},sanitizer='address' if '-fsanitize=address' in (build/'compile_commands.json').read_text() else 'undefined' if '-fsanitize=undefined' in (build/'compile_commands.json').read_text() else 'none',
                     normal_compiler_sha256=hashlib.sha256((build/'newlangc').read_bytes()).hexdigest(),
                     test_binary_sha256=hashlib.sha256((build/'p350-shadow/harness').read_bytes()).hexdigest()))
    print(f'P350_ASSERT {motif} real_source_owned={length}_events generic_shadow=0 gaps={length-1} scalar_preserved poison13_REJECT primitive0_adjacency3 independent_value OOM8_RETRY PASS',flush=True)
(build/'p350-observations.json').write_text(json.dumps(rows,indent=2)+'\n')
