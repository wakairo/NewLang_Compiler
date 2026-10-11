#!/usr/bin/env python3
"""Independent semantic refusal & transactional rollback from fresh R input."""
from pathlib import Path
import json, subprocess, sys, hashlib
cc,boundary,path,folder=sys.argv[1:]
s=Path(path).read_text()
def change(old,new):
    if s.count(old)!=1: raise AssertionError((old,s.count(old)))
    return s.replace(old,new)
checks={
 "wrong_original_D":(change("loan_read(moved_d_B) { |reloan_original_B|","loan_read(life_C) { |reloan_original_B|"),"ALLOCATED-DOMAIN-MISMATCH"),
 "used_original_D":(change("loan_read(moved_d_B) { |reloan_original_B|","loan_read(life_B) { |reloan_original_B|"),"P3-USE-AFTER-CONSUME"),
 "duplicate_Holder":(change("let DetachedHolder{held:carried_B}=holder_B;","let DetachedHolder{held:carried_B}=holder_B; let duplicate=holder_B;"),"P3-USE-AFTER-CONSUME"),
 "active_loan_D_move":(change("set_link(mutable_B@prev,Option<ptr<Node>>::None);","let prematurely_moved=moved_d_B; set_link(mutable_B@prev,Option<ptr<Node>>::None);"),"P3-REF-CONFLICT"),
 "ptr_instead_ref":(change("set_link(mutable_B@prev,Option<ptr<Node>>::None);","set_link(moved_ptr_B,Option<ptr<Node>>::None);"),"P3-TYPE-MISMATCH"),
 "borrow_mode_not_write":(change("ref_from_ptr(write,moved_ptr_B,reloan_original_B)","ref_from_ptr(read,moved_ptr_B,reloan_original_B)"),"P3-TYPE-MISMATCH"),
 "wrong_terminal_D":(change("loan_exclusive_read(final_d_B) { |finish_B|","loan_exclusive_read(life_A) { |finish_B|"),"P3-DOMAIN-MISMATCH"),
 "second_helper_precision":(change("set_link(mutable_B@prev,Option<ptr<Node>>::None);","set_link(mutable_B@prev,Option<ptr<Node>>::None); set_link(mutable_B@prev,Option<ptr<Node>>::None);"),"P297-CALL-PRECISION"),
}
for name,(source,expect) in checks.items():
 p=Path(folder)/(name+".nl");p.write_text(source)
 cli=subprocess.run([cc,str(p)],capture_output=True,text=True)
 obs=subprocess.run([boundary,str(p)],capture_output=True,text=True)
 if obs.returncode: raise AssertionError((name,obs.stderr))
 datum=json.loads(obs.stdout)
 print(json.dumps(dict(negative=name,sha256=hashlib.sha256(source.encode()).hexdigest(),exit=cli.returncode,
    status=datum.get("registration_status"),code=datum.get("code"),rollback=datum.get("snapshot_unchanged"),
    start=datum.get("start_byte"),end=datum.get("end_byte")),flush=True)
 if cli.returncode!=3 or datum.get("code")!=expect or not datum.get("snapshot_unchanged"):
  raise AssertionError((name,expect,cli.stderr,datum))
 (Path(folder)/(name+".stderr")).write_text(cli.stderr)
print("R311_NEGATIVE_CONTROLS_OK",len(checks))
