#!/usr/bin/env python3
"""Fail closed on exact source admission, actual owned/public proof and negative rollback."""
from pathlib import Path
import hashlib,json,re,subprocess,sys
root=Path(__file__).resolve().parent
build=Path(sys.argv[1]).resolve()
pins=json.loads((root/'source_hashes.json').read_text())
for name,sha in pins.items():
    assert hashlib.sha256((root/name).read_bytes()).hexdigest()==sha,name

def run(args):
    return subprocess.run([str(x) for x in args],capture_output=True,text=True)
def json_lines(result):
    assert result.returncode==0,(result.returncode,result.stderr)
    return [json.loads(x) for x in result.stdout.splitlines()]
def position(text,offset):
    prefix=text.encode()[:offset].decode()
    return [prefix.count('\n')+1,len(prefix.rsplit('\n',1)[-1])+1]
def metrics(text):
    return dict(physical_lines=len(text.splitlines()),nonblank_lines=sum(bool(x.strip()) for x in text.splitlines()),
                lexical_let=len(re.findall(r'\blet\b',text)),allocation_sites=text.count('try_allocate_one<Node>()'),
                initialize_sites=text.count('initialize('),loan_read_sites=text.count('loan_read('),
                exclusive_loan_sites=text.count('loan_exclusive_read('),replace_sites=text.count('replace('),
                safe_ref_sites=text.count('ref_from_ptr('),deallocate_sites=text.count('deallocate('))
rows=[]
for name,changes in [('calibration_returned_owner.nl',None),('w1_device_refresh.nl',2),('w2_scheduler_rotation.nl',4)]:
    path=root/name;text=path.read_text()
    assert text.count('try_allocate_one<Node>()')==5
    cli=[run([build/'newlangc',path]) for _ in range(2)]
    assert (cli[0].returncode,cli[0].stdout,cli[0].stderr)==(cli[1].returncode,cli[1].stdout,cli[1].stderr)
    assert cli[0].returncode==4 and not cli[0].stdout and '[V1-BACKEND-UNSUPPORTED]' in cli[0].stderr
    boundary=json_lines(run([build/'five_root_source_gate_evidence',path]))[0]
    assert boundary['parse_status']==0 and boundary['registration_status']==0
    if changes is None:
        owned=json_lines(run([build/'calibration_observer',path]))
        assert owned[0]['main_check_status']==0 and owned[0]['owned_entry_artifact']
        pure=next(x for x in owned if 'pure_call' in x)
        assert pure['two_root_validator_status']==0 and pure['release_order']==[2,4,1,3,5]
        assert len([x for x in owned if 'call_ordinal' in x])==2
        negative=None
    else:
        assert 'TreeBOwner' not in text and 'set_link' not in text and 'unchecked' not in text
        neg_path=root/(path.stem+'_wrong_domain.nl')
        ntext=neg_path.read_text()
        assert metrics(ntext)==metrics(text)
        nb=json_lines(run([build/'five_root_source_gate_evidence',neg_path]))[0]
        assert nb['parse_status']==0 and nb['registration_status']==1 and nb['code']=='ALLOCATED-DOMAIN-MISMATCH'
        assert nb['snapshot_unchanged'] and not nb['owned_entry_artifact'] and not nb['entry_check_attempted'] and not nb['backend_attempted']
        nr=run([build/'newlangc',neg_path]);assert nr.returncode==3 and not nr.stdout and 'error(semantic)[ALLOCATED-DOMAIN-MISMATCH]' in nr.stderr
        owned=json_lines(run([build/'public_probe',path,changes,neg_path]))
        assert owned[0]['refused_registration_status']==1 and owned[0]['snapshot_unchanged']
        assert owned[1]['main_check_status']==0 and owned[1]['owned_entry_artifact']
        assert len([x for x in owned if x.get('public_validator_status')==0])==5
        events=[x for x in owned if 'event' in x]
        assert len(events)==changes
        for event in events:
            assert text.encode()[event['start_byte']:event['end_byte']].startswith(b'replace(')
            event['span_start']=position(text,event['start_byte'])
            event['span_end']=position(text,event['end_byte'])
        if changes==2:
            assert [(x['region'],x['projection'][1],x['old_variant'],x['new_variant'],x['target_region']) for x in events]==[(1,2,1,2,2),(1,2,2,1,0)]
            assert events[0]['new_occurrence']==events[1]['old_occurrence']!=0
        else:
            assert [(x['region'],x['projection'][1],x['old_variant'],x['new_variant'],x['target_region']) for x in events]==[(1,0,1,2,3),(2,1,1,2,5),(1,0,2,2,4),(2,1,2,1,0)]
            assert events[0]['new_occurrence']==events[2]['old_occurrence']!=events[2]['new_occurrence']
        assert owned[-1]['same_context_actual_source_owned_clean_retry']
        summary=owned[-2]
        assert summary['source_owned_publicly_validated'] and summary['static_release_worlds_0_to_5']==[1]*6 and summary['success_release_order']==[5,4,3,2,1]
        negative=dict(source=neg_path.name,sha256=pins[neg_path.name],boundary=nb,cli_exit=nr.returncode,
                      diagnostic=nr.stderr.replace(str(neg_path),'<source>'),
                      span_start=position(ntext,nb['start_byte']),span_end=position(ntext,nb['end_byte']),
                      same_context_clean_owned_retry=True)
    rows.append(dict(source=name,sha256=pins[name],compiler='de9c4b175784f484675b670a109f62edeaf35de1',
                     compiler_sha256=hashlib.sha256((build/'newlangc').read_bytes()).hexdigest(),cli_exit=4,
                     diagnostic=cli[0].stderr.replace(str(path),'<source>'),parser_registration_boundary=boundary,
                     source_owned_public_observations=owned,negative=negative,burden=metrics(text),c_bytes=0,native=False))
    print(f'P338_ASSERT {name} parse=0 register=0 main=0 actual_owned=TRUE public_validators=PASS CLI=4 C=0',flush=True)
(build/'p338-observations.json').write_text(json.dumps(rows,indent=2)+'\n')
print('P338_W1_W2_BOTH_SOURCE_OWNED_CLEAN_LIMITED_PREFIXES_WITH_WRONG_DOMAIN_ROLLBACK_RETRY',flush=True)
