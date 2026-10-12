"""UNTRUSTED transcript consistency checks; never issues C or Python ownership.

Passing means CONSISTENT_UNTRUSTED only. Use real C rerun for P validation.
"""
import hashlib,json

def canonical(value):
    return json.dumps(value,sort_keys=True,separators=(',',':'),ensure_ascii=True).encode()

def seal(document):
    document.pop('integrity',None)
    document['integrity']={'canonical_payload_sha256':hashlib.sha256(canonical(document)).hexdigest(),
                           'authenticated':False,'authority_transfer':False}
    return document

def check(document,source_sha,negative_sha):
    integrity=document['integrity']
    payload={k:v for k,v in document.items() if k!='integrity'}
    assert integrity['canonical_payload_sha256']==hashlib.sha256(canonical(payload)).hexdigest()
    assert integrity['authenticated'] is False and integrity['authority_transfer'] is False
    assert document['schema']=='newlang.checked-observation.v0'
    assert document['mode']=='diagnostic_only_no_authority_transfer'
    assert document['issuer']['source_sha256']==source_sha
    assert document['source_refusal']['source_sha256']==negative_sha
    validation=document['validation']
    assert validation['issuer_tree_completeness']=='NOT_PUBLICLY_ATTESTED'
    assert validation['portable_authority'] is False and validation['source_completeness_certified'] is False
    assert validation['public_validator_statuses']==[0]*5
    assert validation['evidence_class']=='PUBLIC_VALIDATOR_CALLS_OBSERVED_IN_LIVE_PRODUCER'
    assert document['source_refusal']['status']==1 and document['source_refusal']['owned_artifact_present'] is False
    assert document['source_refusal']['diagnostic_code']=='ALLOCATED-DOMAIN-MISMATCH'
    assert document['admission']['owned_artifact_present'] and document['admission']['same_context_clean_retry']
    layers=document['issuer_layers'];assert len(layers)==5
    path='entry/call:1/body'
    for depth,layer in enumerate(layers):
        assert layer['depth']==depth and layer['fragment_path']==path
        assert layer['success_arm_path']==path+'/match:'+str(layer['match_node_id'])+'/arm:1'
        assert layer['failure_arm_path']==path+'/match:'+str(layer['match_node_id'])+'/arm:0'
        assert len(layer['captured_originals'])==depth
        assert len({x['allocation_value_id'] for x in layer['captured_originals']})==depth
        assert len({x['domain_value_id'] for x in layer['captured_originals']})==depth
        parent={n['node_id']:n for n in layer['parent_nodes']}
        assert len(parent)==len(layer['parent_nodes'])
        assert parent[layer['match_node_id']]['kind']==35 and parent[layer['match_node_id']]['initializer_id']==layer['trial_node_id']
        assert parent[layer['trial_node_id']]['kind']==46 and parent[layer['trial_node_id']]['allocation_trial']
        grants=[n for n in layer['success_nodes'] if n['allocation_success']]
        inits=[n for n in layer['success_nodes'] if n['kind']==7]
        domains=[n for n in layer['success_nodes'] if n['kind']==4]
        assert len(grants)==len(inits)==len(domains)==1
        assert grants[0]['backing_id']==inits[0]['backing_id']!=0
        assert inits[0]['domain_id']==domains[0]['domain_id']!=0
        if depth:
            assert layers[depth-1]['success_nodes']==layer['parent_nodes']
        path=layer['success_arm_path']
    events=document['events'];inventory=document['operation_inventory']
    assert inventory['fragment_path']==path and inventory['selected_fragment_bijection_observer_checked']
    count=inventory['checked_replace_count'];assert count==inventory['captured_change_count']==len(events) and count in (2,4)
    nodes={n['node_id']:n for n in layers[-1]['success_nodes']}
    replacements=[n['node_id'] for n in layers[-1]['success_nodes'] if n['kind']==10]
    assert [e['checked_operation_node_id'] for e in events]==replacements
    previous=None
    for ordinal,event in enumerate(events):
        assert event['ordinal']==ordinal and event['fragment_path']==path
        assert event['before_world']==path+'/event:'+str(ordinal)+'/before'
        assert event['after_world']==path+'/event:'+str(ordinal)+'/after'
        node=nodes[event['checked_operation_node_id']]
        assert event['source_span_bytes']==node['source_span_bytes']
        chain=event['loan_chain'];loan=nodes[chain['read_loan_node_id']];ref=nodes[chain['write_ref_node_id']];proj=nodes[chain['projection_node_id']]
        assert loan['kind']==13 and ref['kind']==47 and proj['kind']==48
        assert proj['node_id']==node['first_argument_id']
        assert loan['loan']['access']==0 and not loan['loan']['exclusive']
        assert loan['loan']['nonescape_proved'] and loan['loan']['prevent_lifetime_end']
        assert ref['reference']['writable'] and ref['reference']['readable']
        assert proj['reference']['writable'] and proj['reference']['scope_id']==chain['scope_id']==loan['loan']['scope_id']
        assert chain['close_node'] is None and chain['active_before'] and chain['closed_at_terminal']
        scopes=event['scope_observations']
        assert scopes['during_before'] and scopes['during_after_change'] and not scopes['terminal_active']
        if ordinal+1<len(events):assert scopes['next_checkpoint_active'] is False
        else:assert scopes['next_checkpoint_active'] is None
        assert event['pointer_operand']['type_kind']==2 and event['pointer_operand']['domain_id']==0 and event['pointer_operand']['allocation_region_id']==0
        assert event['read_stability_operand']['type_kind']==3 and event['read_stability_operand']['reference']['readable'] and not event['read_stability_operand']['reference']['writable']
        assert event['read_stability_operand']['reference']['scope_id']==chain['scope_id']
        assert event['projected_write_operand']['type_kind']==3 and event['projected_write_operand']['reference']['writable'] and event['projected_write_operand']['reference']['scope_id']==chain['scope_id']
        before=event['before_all_roots'];after=event['after_all_roots'];assert len(before)==len(after)==5
        if previous is not None:assert previous==before
        for b,a in zip(before,after):
            assert b['root_place_id']==a['root_place_id'] and b['incarnation_id']==a['incarnation_id']
            assert b['original_allocation']['type_kind']==5 and not b['original_allocation']['is_copy']
            assert not b['original_domain']['is_copy']
            assert b['original_allocation']==a['original_allocation'] and b['original_domain']==a['original_domain']
            assert b['domain_id']==b['original_domain']['domain_id']==a['domain_id']
            assert b['backing_id']==b['original_allocation']['allocation_region_id']==a['backing_id']
            assert len(b['fixed_fields'])==len(a['fixed_fields'])==4
            assert [x['name'] for x in b['fixed_fields']]==['next','prev','child','payload']
            for old,new in zip(b['fixed_fields'],a['fixed_fields']):
                assert old['index']==new['index'] and old['place_id']==new['place_id'] and old['incarnation_id']==new['incarnation_id']
                assert old['governing_domain_id']==new['governing_domain_id']==b['domain_id']
                if old['place_id']!=node['field']['child_id']:assert old==new
                else:
                    assert old['value']==event['old_option'] and new['value']==event['new_option']
                    assert b['root_place_id']==node['field']['parent_id']==ref['root_place_id']
                    assert b['domain_id']==loan['loan']['domain_id']==ref['domain_id']
                    assert chain['original_domain_value_id']==b['original_domain']['value_id']
                    assert old['current_fact_id']==event['facts']['child_before'] and new['current_fact_id']==event['facts']['child_after']
                    assert old['current_fact_id']!=new['current_fact_id']
            assert b['fixed_fields'][3]['value']['type_kind']==8 and b['fixed_fields'][3]['value']['scalar_known']
        assert len({r['original_allocation']['value_id'] for r in before})==5
        assert len({r['original_domain']['value_id'] for r in before})==5
        if event['old_occurrence']:
            assert event['old_occurrence']['live_before'] and not event['old_occurrence']['live_after']
            if event['new_occurrence']:assert event['new_occurrence']['id']!=event['old_occurrence']['id']
        if event['new_occurrence']:assert event['new_occurrence']['live']
        previous=after
    return 'CONSISTENT_UNTRUSTED_REQUIRES_C_RERUN'
