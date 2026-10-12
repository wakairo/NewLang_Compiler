#!/usr/bin/env python3
from pathlib import Path
import subprocess,json,hashlib,sys
root=Path(__file__).resolve().parents[2];base=Path(sys.argv[1]).resolve();rows=[]
pins=json.loads((root/'evidence/p362/source_hashes.json').read_text())
for name,digest in pins.items():assert hashlib.sha256((root/'evidence/p362'/name).read_bytes()).hexdigest()==digest
for config in ['old-only','both','new-only']:
    b=Path(str(base)+'-'+config)
    for name in ['first_adoption.nl','returned_owner_calibration.nl']:
        p=root/'evidence/p362'/name
        boundary=json.loads(subprocess.check_output([str(b/'boundary'),str(p)],text=True))
        c=subprocess.run([str(b/'newlangc'),str(p)],capture_output=True,text=True)
        expected=(2,None,'AVS-DECL-PROFILE',3) if config=='new-only' else (0,0,'',4) if config=='old-only' and name.startswith('returned') else (0,3,'CAPTURED-CLOSURE-PRECISION',3)
        assert (boundary['parse_status'],boundary['registration_status'],boundary['code'],c.returncode)==expected
        if expected[1]!=0:assert boundary['snapshot_unchanged'] and not boundary['owned_entry_artifact']
        rows.append({'config':config,'source':name,'boundary':boundary,'cli_exit':c.returncode,'diagnostic':c.stderr,'compiler_sha256':hashlib.sha256((b/'newlangc').read_bytes()).hexdigest()})
old=Path(str(base)+'-old-only');both=Path(str(base)+'-both')
cal=root/'evidence/p362/returned_owner_calibration.nl';adopt=root/'evidence/p362/first_adoption.nl'
retry=subprocess.run([str(old/'calibration_observer'),str(cal),'after-refusal',str(adopt)],capture_output=True,text=True,check=True)
assert not retry.stderr and 'registration_refusal_rollback_retry' in retry.stdout
owned=[json.loads(x) for x in retry.stdout.splitlines()];assert owned[0]['owned_entry_artifact']
edges=subprocess.run([str(old/'owned_edges'),str(cal)],capture_output=True,text=True,check=True);assert not edges.stderr
edge_records=[json.loads(x) for x in edges.stdout.splitlines()];assert all(x['heap_root_and_all_20_fields_equal'] for x in edge_records if 'edge' in x)
oom=subprocess.run([str(old/'calibration_observer'),str(cal),'oom'],capture_output=True,text=True,check=True);assert not oom.stderr
negatives=[]
for name in ['wrong_dst_domain.nl','wrong_B_domain.nl','read_ref_to_write_formal.nl']:
    p=root/'evidence/p362'/name
    d=json.loads(subprocess.check_output([str(both/'boundary'),str(p)],text=True));assert d['parse_status']==0 and d['registration_status']==1 and d['snapshot_unchanged'] and not d['owned_entry_artifact']
    negatives.append({'source':name,'boundary':d})
# Existing direct-only positives stay true, while the composed program fails.
direct=[]
for name in ['w1_device_refresh_9','w2_scheduler_rotation_9']:
    s=root/'evidence/p352'/(name+'.nl');n=s.with_name(name+'_wrong_domain.nl')
    r=subprocess.run([str(both/'public_probe'),str(s),'9',str(n)],capture_output=True,text=True,check=True);assert not r.stderr
    data=[json.loads(x) for x in r.stdout.splitlines()];assert next(x for x in data if 'source_owned_publicly_validated' in x)['events']==9
    direct.append({'source':s.name,'records':data})
finite_retry=[]
for name in ['w1_device_refresh_9','w2_scheduler_rotation_9']:
    r=subprocess.run([str(both/'precision_retry'),str(root/'evidence/p352'/(name+'.nl')),'9',str(adopt)],capture_output=True,text=True,check=True)
    assert not r.stderr
    records=[json.loads(x) for x in r.stdout.splitlines()]
    assert records[0]['refused_registration_status']==3 and records[0]['snapshot_unchanged']
    assert records[-1]['same_context_actual_source_owned_clean_retry']
    finite_retry.append({'source':name,'records':records})
result={'base':'82578ff6eb23970e3ae934c98a38383ca01468f7','evidence_head':subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),'compiler_edits':False,'composition_owned':False,'classification':'P G3 COMPOSITION: HOLD — FIRST SOURCE/OWNED/PUBLIC PROOF BARRIER','boundaries':rows,'legacy_owned':owned,'legacy_edges':edge_records,'legacy_oom':[json.loads(x) for x in oom.stdout.splitlines()],'source_negatives':negatives,'direct9_controls':direct,'finite_refusal_clean_retry':finite_retry,'composed_public_poison':'UNTESTED: no successful composed owned artifact','composed_OOM':'UNTESTED: no accepted composed route'}
(base.parent/(base.name+'-observations.json')).write_text(json.dumps(result,indent=2)+'\n')
print('P362_ASSERT HOLD unchanged353 composition registration3 rollback; legacy actual_owned8 5+2+1 validators; source_negative3 semantic1; direct9_controls2 PASS; COMPOSED_OWNED_NONE')
