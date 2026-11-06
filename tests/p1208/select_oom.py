"""Freeze finite reached call-path ordinals before any injected trial.

Direct control-state/exit/anchor allocations plus every allocation in each
RETURN record-array/clone/anchor prefix, and the final signature ledger.
No unrelated full historical OOM sweep. ELF symbol ranges avoid dependence
on a host addr2line's support for the producer's DWARF line tables.
"""
from pathlib import Path
import hashlib,json,re,subprocess,sys

exe,census_file,destination=sys.argv[1:]
exe=Path(exe);raw=Path(destination);raw.mkdir(parents=True,exist_ok=False)
source=Path(census_file)
table=subprocess.check_output(['nm','-S','--defined-only',str(exe)],text=True)
(raw/'symbols.txt').write_text(table)
functions=[]
for line in table.splitlines():
    m=re.fullmatch(r'([0-9a-f]+) ([0-9a-f]+) ([tTwW]) (.+)',line)
    if m:functions.append((int(m[1],16),int(m[2],16),m[4]))
assert len([n for _,_,n in functions if n=='append'])==1,'control append must be unambiguous'
rows=[]
for stage,ordinal,kind,pc in re.findall(r'C263 SITE stage=(\w+) ordinal=(\d+) kind=(\w) pc=(0x[0-9a-f]+)',source.read_text()):
    address=int(pc,16)-1
    names=[name for start,size,name in functions if start<=address<start+size]
    assert len(names)==1,(pc,names)
    rows.append({'stage':stage,'ordinal':int(ordinal),'kind':kind,'pc':pc,'function':names[0]})
direct={'nl_control_state_create','nl_control_state_capture','append','nl_copy_state_anchor'}
plans={}
for stage in ('registration','actual','named'):
    events=[e for e in rows if e['stage']==stage]
    assert {e['ordinal'] for e in events}==set(range(1,len(events)+1))
    selected={e['ordinal'] for e in events if e['function'] in direct}
    starts=[e['ordinal'] for e in events if e['function']=='seal_current_cycle']
    signatures=[e['ordinal'] for e in events if e['function']=='current_signature_seal']
    assert len(starts)==len(signatures)==(0 if stage=='registration' else 1)
    windows=[]
    for begin in starts:
        stop=next(e['ordinal'] for e in events if e['ordinal']>begin and e['function']=='linear_seal_graph')
        assert stop>begin+20,'must contain real RETURN snapshot prefixes'
        selected.update(range(begin,stop))
        windows.append([begin,stop-1])
    selected.update(signatures)
    assert selected and len(selected)<=256,'finite per-stage investment bound'
    path=raw/f'{stage}.points'
    path.write_text(''.join(f'{p}\n' for p in sorted(selected)))
    plans[stage]={'full_reached_ordinals':len(events),'selected':len(selected),
        'return_seal_windows_inclusive':windows,'signature_ledger_points':signatures,
        'points_sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
report={'version':1,'before_injected_execution':True,'binary':str(exe),
    'binary_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),
    'source_census':str(source),'source_census_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
    'selector_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    'oracle':'registration/actual exact NL_CHECK_OUT_OF_MEMORY, no publication, unchanged semantic anchors/functions/types/facts, retry; named nonzero with bytes/inode/mtime/temporary preservation and retry',
    'stages':plans}
(raw/'pre-injection-plan-v1.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'binary':str(exe),'selected_per_stage':{k:v['selected'] for k,v in plans.items()},'plans':plans}))
