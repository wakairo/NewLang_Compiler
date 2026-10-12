#!/usr/bin/env python3
"""Produces only diagnostic JSON after actual C validation, never O1 authority."""
from pathlib import Path
import copy,hashlib,json,re,subprocess,sys
from consumer import canonical,seal,check
root=Path(__file__).resolve().parents[2]
output=Path(sys.argv[2]);build=Path(sys.argv[1]).resolve();output.mkdir(parents=True,exist_ok=True)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
pins=json.loads((root/'evidence/p338/source_hashes.json').read_text())
for name,digest in pins.items():assert sha(root/'evidence/p338'/name)==digest
assert sha(root/'evidence/p338/public_probe.c')=='2fcf1a1622fb79273d8fbcc446e95330a05ec6115d5c259f6082b87a17fc2a55'
cache=(build/'CMakeCache.txt').read_text()
options=sorted(re.findall(r'^(NEWLANG_EXPERIMENTAL_\w+):BOOL=ON$',cache,re.M));assert len(options)==10
spec=subprocess.check_output(['git','show','a5343bf70004b30a8b026e52aa2fde93b5920be0:docs/reference/NewLang_v0_spec_Draft17_30.md'],cwd=root)
assert subprocess.check_output(['git','rev-parse','a5343bf70004b30a8b026e52aa2fde93b5920be0:docs/reference/NewLang_v0_spec_Draft17_30.md'],cwd=root,text=True).strip()=='db8fe08f811acc8286f2d37e9d3229b5d7a67067'
proof_classes={'issuer_layers':'PUBLIC_CHECKED_VIEWS_READ_IN_LIVE_ARTIFACT',
 'events':'PUBLIC_CHECKED_AND_SEMANTIC_VIEWS_READ_IN_LIVE_ARTIFACT',
 'validation':'PUBLIC_VALIDATOR_CALLS_OBSERVED_IN_LIVE_PRODUCER',
 'selected_fragment_bijection':'OBSERVER_CHECK_AGAINST_PUBLIC_CHECKED_NODE_INVENTORY',
 'continuity_and_siblings':'PINNED_P338_OBSERVER_CHECKS_ON_PUBLIC_SNAPSHOTS',
 'portable_issuer_attestation':'MISSING_FROM_PUBLIC_API'}
for motif,name,count in [('W1','w1_device_refresh',2),('W2','w2_scheduler_rotation',4)]:
    positive=root/'evidence/p338'/(name+'.nl');negative=root/'evidence/p338'/(name+'_wrong_domain.nl')
    args=[str(build/'export'),str(positive),str(count),str(negative)]
    a=subprocess.run(args,capture_output=True,check=True);b=subprocess.run(args,capture_output=True,check=True)
    assert a.stdout==b.stdout and not a.stderr and not b.stderr
    raw=[json.loads(line) for line in a.stdout.splitlines()]
    single=lambda tag:next(x for x in raw if x.get('record')==tag)
    statuses=[x['public_validator_status'] for x in raw if 'public_validator_status' in x];assert statuses==[0]*5
    inventory=single('inventory_check');assert inventory['checked_replace_count']==count
    refusal=single('source_refusal');refusal['source_sha256']=pins[negative.name]
    refusal['evidence_class']='REAL_C_REGISTRATION_REFUSAL_NO_CHECKED_ARTIFACT'
    def cli(path,expected,code):
        r=subprocess.run([str(build/'newlangc'),str(path)],capture_output=True,text=True)
        assert r.returncode==expected and not r.stdout and code in r.stderr
        return {'exit':r.returncode,'diagnostic':r.stderr.replace(str(path),'<source>'),'c_bytes':0}
    refusal['cli']=cli(negative,3,'error(semantic)[ALLOCATED-DOMAIN-MISMATCH]')
    source=positive.read_bytes()
    events=[x for x in raw if x.get('record')=='event_detail']
    for event in events:
        lo,hi=event['source_span_bytes'];assert source[lo:hi].startswith(b'replace(')
        event['source_text']=source[lo:hi].decode('ascii')
        for key in ['before_all_roots','after_all_roots']:
            assert len(event[key])==5 and all(len(r['fixed_fields'])==4 for r in event[key])
    doc=dict(schema='newlang.checked-observation.v0',mode='diagnostic_only_no_authority_transfer',
      issuer=dict(kind='C_PUBLIC_VALIDATED_OBSERVER',compiler_git_sha='de9c4b175784f484675b670a109f62edeaf35de1',
        source_sha256=pins[positive.name],source_file=positive.name,
        canonical_compiler_main_sha='a5343bf70004b30a8b026e52aa2fde93b5920be0',canonical='Draft17.30',
        canonical_spec_blob_sha='db8fe08f811acc8286f2d37e9d3229b5d7a67067',canonical_spec_sha256=hashlib.sha256(spec).hexdigest(),
        observer_base_git_sha='dc0fc7e7d2f9f92286973a4f5062089acf0d85f2',
        observer_source_hashes={x:sha(root/x) for x in ['evidence/p346/export.c','evidence/p346/collect.py','evidence/p346/consumer.py','evidence/p338/public_probe.c']},
        compiler_binary_sha256=sha(build/'newlangc'),observer_binary_sha256=sha(build/'export'),
        semantic_library_sha256=sha(build/'libnewlang_semantic.a'),existing_opt_ins_enabled=options,build_type='Debug',
        host_compiler=subprocess.check_output([re.search(r'^CMAKE_C_COMPILER:(?:FILEPATH|STRING)=(.+)$',cache,re.M)[1],'--version'],text=True).splitlines()[0]),
      namespace_policy={'scope':'THIS_TRANSCRIPT_ONLY','fragment_path':'public owned call/match-arm path',
        'world':'ancestor or event before/after label; IDs use enclosing world','id_types':'explicit *_id property category',
        'numeric_ids_are_not_cross_artifact_identity':True,'no_pointer_addresses':True},
      validation={'evidence_class':proof_classes['validation'],'public_validator_statuses':statuses,'issuer_tree_completeness':'NOT_PUBLICLY_ATTESTED',
        'portable_authority':False,'source_completeness_certified':False,'public_snapshot_unchanged':True},
      fact_evidence_classes=proof_classes,admission=single('admission'),source_refusal=refusal,
      issuer_layers=[x for x in raw if x.get('record')=='issuer_layer'],events=events,operation_inventory=inventory,
      existing_observer_checks=[x for x in raw if 'record' not in x],backend=cli(positive,4,'[V1-BACKEND-UNSUPPORTED]'),
      gaps=['No publicly portable issuer/tree attestation','No distinct checked CloseLoan node; only scope views/lexical plan',
            'No current O1 no-loss representation of four fields/read-loan chain','No native world','No Python oracle run or rights import'])
    seal(doc);assert check(doc,pins[positive.name],pins[negative.name])=='CONSISTENT_UNTRUSTED_REQUIRES_C_RERUN'
    poison_tests={}
    mutations={
      'missing_event':lambda d:d['events'].pop(),
      'duplicated_event':lambda d:d['events'].append(copy.deepcopy(d['events'][0])),
      'renumbered_event':lambda d:d['events'][0].update(ordinal=7),
      'wrong_source_digest':lambda d:d['issuer'].update(source_sha256='0'*64),
      'wrong_fragment_path':lambda d:d['events'][0].update(fragment_path='unqualified'),
      'missing_fourth_sibling':lambda d:d['events'][0]['before_all_roots'][0]['fixed_fields'].pop(),
      'false_validator_status':lambda d:d['validation']['public_validator_statuses'].__setitem__(0,3),
      'changed_typed_domain':lambda d:d['events'][0]['before_all_roots'][0]['original_domain'].update(domain_id=999),
    }
    for label,mutate in mutations.items():
        bad=copy.deepcopy(doc);mutate(bad);seal(bad) # attacker recomputes integrity: reject inconsistent relationships
        try:check(bad,pins[positive.name],pins[negative.name])
        except (AssertionError,KeyError,IndexError):poison_tests[label]='REJECTED_INTERNAL_INCONSISTENCY_AFTER_REHASH'
        else:raise AssertionError('consumer missed '+label)
    # Consistent globally rewritten domain claims can bypass internal checks.
    # They do NOT alter C and NEVER become a P-validated transcript or grant.
    forged=copy.deepcopy(doc)
    def rewrite_domains(x):
        if isinstance(x,dict):
            for k,v in list(x.items()):
                if k in ('domain_id','governing_domain_id') and isinstance(v,int) and v:x[k]=v+1000
                else:rewrite_domains(v)
        elif isinstance(x,list):
            for v in x:rewrite_domains(v)
    rewrite_domains(forged);seal(forged)
    assert check(forged,pins[positive.name],pins[negative.name])=='CONSISTENT_UNTRUSTED_REQUIRES_C_RERUN'
    tests={'poison_checks':poison_tests,'consistent_domain_rewrite_with_recomputed_digest':'CONSISTENT_UNTRUSTED_NOT_P_VALIDATED',
      'requires_actual_C_rerun':True,'synthetic_Original_constructed':False,'portable_authority_claimed':False}
    (output/(motif+'.json')).write_bytes(canonical(doc)+b'\n')
    (output/(motif+'-tests.json')).write_text(json.dumps(tests,indent=2)+'\n')
    print(f'P346_ASSERT {motif} actual_owned five_public_validators all_four_fields typed_issuer_layers {count}_event_bijection wrongD_rollback_retry deterministic_export eight_poison_rejections UNTRUSTED_REHASH_BYPASS_DEMONSTRATED',flush=True)
