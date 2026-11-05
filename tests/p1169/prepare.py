from pathlib import Path
import hashlib,json,zipfile

root=Path(__file__).resolve().parent
src=root/'sources';src.mkdir(exist_ok=True)
with zipfile.ZipFile('/workspace/p1164-c236-checkpoint/inputs-prefreeze-v1.zip') as z:
    oracle=json.loads(z.read('oracle.json'))
    for name in z.namelist():
        if name.startswith('sources/'):
            (root/name).write_bytes(z.read(name))
    (root/'predecessor-oracle.json').write_bytes(z.read('oracle.json'))
oracle['authority']='C238 main425; new candidate success expectations; C236 oracle retained separately unchanged'
oracle['positive_prediction']={'canonical':'legal selected family','candidate':'CHECK0 -> genuine owned -> private current consumer0 -> C17 -> native','baseline':'expected precision3; actual FIRST retained separately','fact_rule':'each actual Change refreshes child and ancestor root facts, even with equal scalar bits; sibling fact/incarnations unchanged'}
oracle['runner']='bounded source gate0 then targeted current/native/OOM/sanitizer and final GCC suite; no R inputs'
for key,val in oracle['cases'].items():
    if val['category']=='positive':val['candidate_check']=0
source=(src/'03-multiple.nl').read_text()
# Fresh P-owned extreme bound, one iteration; same shape, different field.
extra=source.replace('LatchNode','BoundaryNode').replace('latch','flag').replace('echo','side').replace('u8(112)','u8(17)').replace('u8(39)','u8(214)').replace('u8(203)','u8(88)').replace('cursor=usize(1)','cursor=usize(18446744073709551613)').replace('cursor < usize(10)','cursor < usize(18446744073709551614)').replace('cursor + usize(4)','cursor + usize(1)')
(src/'05-high-bound.nl').write_text(extra)
oracle['cases']['05-high-bound']={'category':'positive','canonical_legal':True,'headers':[18446744073709551613,18446744073709551614],'backedges':1,'loop_result':18446744073709551614,'old_values':[17],'iteration_current_values':[88],'final_child':88,'sibling':214,'target_index':0,'fresh_current_transitions':1,'candidate_check':0}
extra=(src/'02-one.nl').read_text().replace('BeaconNode','TransitNode').replace('u8(67)','u8(236)').replace('u8(156)','u8(31)').replace('u8(98)','u8(79)').replace('cursor=usize(2)','cursor=usize(0)').replace('cursor < usize(4)','cursor < usize(7)').replace('cursor + usize(5)','cursor + usize(3)').replace('let old_copy=old;','let alias=old;\n   let old_copy=alias;')
(src/'06-alias-right.nl').write_text(extra)
oracle['cases']['06-alias-right']={'category':'positive','canonical_legal':True,'headers':[0,3,6,9],'backedges':3,'loop_result':9,'old_values':[31,79,79],'iteration_current_values':[79,79,79],'final_child':79,'sibling':236,'target_index':1,'fresh_current_transitions':3,'candidate_check':0}
bad={
 '94-phase-shadow':source.replace('continue(cursor + usize(4));','let cursor=usize(0);\n   continue(cursor + usize(4));'),
 '95-wrong-arm':source.replace('cursor < usize(10)','cursor == usize(10)'),
 '96-subtract':source.replace('cursor + usize(4)','cursor - usize(4)'),
 '97-second-writer':source.replace('let old_copy=old;','let other=loan_write(original@latch){|again|replace(again,u8(7))};\n   let old_copy=old;'),
 '98-sibling-writer':source.replace('let old_copy=old;','let other=loan_write(original@echo){|again|replace(again,u8(7))};\n   let old_copy=old;'),
 '99-literal-carry':source.replace('cursor + usize(4)','usize(4)'),
 '100-zero-step':source.replace('cursor + usize(4)','cursor + usize(0)'),
 '101-wrong-carry-type':source.replace('cursor + usize(4)','u8(4)'),
}
for name,text in bad.items():
    (src/(name+'.nl')).write_text(text)
    oracle['cases'][name]={'category':'negative','canonical_legal':'some are legal but outside the promised finite ADD family','required_property':'reject unsupported or invalid '+name,'candidate_check':'nonzero','stdout':'empty on reject','named':'unchanged bytes/inode/mtime; no residue'}
# One additional existing control, no new source invention.
control=Path('/tmp/p1169-c238/tests/p1158/sources/80-known-return-control.nl')
(src/'81-existing-return-control.nl').write_bytes(control.read_bytes())
oracle['cases']['81-existing-return-control']={'category':'control','candidate_check':0,'c17':'byte-identical baseline; existing P1158 contract','native':'covered by old regression'}
(root/'oracle.json').write_text(json.dumps(oracle,indent=2)+'\n')
