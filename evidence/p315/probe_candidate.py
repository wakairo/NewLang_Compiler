#!/usr/bin/env python3
"""Track P #315: read-only falsification of exact frozen PR313 compiler."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
FIXTURE=ROOT/"tests/fixtures/experimental_two_scoped_field_calls.nl"
OUT=ROOT/"evidence/p315/generated"
OUT.mkdir(parents=True,exist_ok=True)
original=FIXTURE.read_text(encoding="utf-8")
expected="40b5dad768a0c265e85bd111e9aca7a9160c325e04bf96f249767896b616f73e"
assert hashlib.sha256(original.encode()).hexdigest()==expected
needle="                                            let rebuilt=DetachedHolder{held:LiveRoot{p:moved_ptr_B,a:moved_a_B,d:moved_d_B}};"
assert original.count(needle)==1
addition="""                                            let old_A_next_after_B = loan_read(life_A) { |stable_A_after_B|
                                                let write_A_after_B = ref_from_ptr(write, ptr_A, stable_A_after_B);
                                                replace(write_A_after_B@next,
                                                    Option<ptr<Node>>::Some(ptr_C))
                                            };
"""
new=original.replace(needle,addition+needle)
# A directly rewrites its own original next edge while original B's custody remains whole.
wrong_domain=new.replace("loan_read(life_A) { |stable_A_after_B|","loan_read(moved_d_B) { |stable_A_after_B|")
assert new!=wrong_domain
cases={
 "baseline-two-scoped":original,
 "third-A-next-own-domain":new,
 "third-A-next-wrong-B-domain":wrong_domain,
}
for name,text in cases.items():
    path=OUT/(name+".nl")
    path.write_text(text,encoding="utf-8")
    sha=hashlib.sha256(text.encode()).hexdigest()
    lines=text.splitlines()
    at=next((i for i,line in enumerate(lines,1) if "old_A_next_after_B" in line),None)
    print("P315_SOURCE",name,"sha256="+sha,"bytes="+str(len(text.encode())),"inserted_line="+str(at),flush=True)
    command=[sys.argv[1],str(path)]
    p=subprocess.run(command,capture_output=True,text=True)
    print("P315_CLI",name,"exit="+str(p.returncode),"c_bytes="+str(len(p.stdout.encode())),"stderr="+json.dumps(p.stderr),flush=True)
    b=subprocess.run([sys.argv[2],str(path)],capture_output=True,text=True)
    print("P315_PUBLIC",name,"status="+str(b.returncode),"json="+b.stdout.strip(),"stderr="+json.dumps(b.stderr),flush=True)
    if b.returncode==0:
        d=json.loads(b.stdout)
        assert d["parse_status"]==0 and d["snapshot_unchanged"]== (d["registration_status"]!=0)
        if name=="baseline-two-scoped":
            assert d["registration_status"]==0 and p.returncode==4, (d,p.stderr)
        elif name=="third-A-next-wrong-B-domain":
            assert d["registration_status"]!=0 and p.returncode==3, (d,p.stderr)
        elif d["registration_status"]==0:
            assert p.returncode==4, (d,p.stderr)
            print("P315_CANDIDATE_SOURCE_ADMITTED",name,flush=True)
        else:
            print("P315_CANDIDATE_BLOCKER",name,d["code"],flush=True)
    else:
        raise SystemExit("Public probe failed: "+name)
print("P315_CANDIDATE_REGISTRATION_PROBE_COMPLETE",flush=True)
