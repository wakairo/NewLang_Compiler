#!/usr/bin/env python3
"""P315 exact-profile source refusal and same-context clean-retry corpus."""
from pathlib import Path
import hashlib,json,subprocess,sys

compiler,boundary,observer=sys.argv[1:4]
root=Path(__file__).resolve().parents[2]
base=(root/"evidence/p315/generated/third-A-next-own-domain.nl").read_text()
out=root/"evidence/p315/generated/negative"
out.mkdir(parents=True,exist_ok=True)
def once(a,b):
    assert base.count(a)==1,(a,base.count(a))
    return base.replace(a,b,1)
cases={
 "A-uses-B-domain":once("loan_read(life_A) { |stable_A_after_B|","loan_read(moved_d_B) { |stable_A_after_B|"),
 "C-root-with-A-domain":once("ref_from_ptr(write, ptr_A, stable_A_after_B)","ref_from_ptr(write, ptr_C, stable_A_after_B)"),
 "read-ref-to-A-write":once("ref_from_ptr(write, ptr_A, stable_A_after_B)","ref_from_ptr(read, ptr_A, stable_A_after_B)"),
 "copy-ptr-as-A-write":once("replace(write_A_after_B@next,","replace(ptr_A@next,"),
 "move-A-domain-during-A-loan":once(
  "let write_A_after_B = ref_from_ptr(write, ptr_A, stable_A_after_B);",
  "let write_A_after_B = ref_from_ptr(write, ptr_A, stable_A_after_B);let stolen_A_D=life_A;"),
 "wrong-A-next-B-target":once(
  "replace(write_A_after_B@next,\n                                                    Option<ptr<Node>>::Some(ptr_C))",
  "replace(write_A_after_B@next,\n                                                    Option<ptr<Node>>::Some(ptr_B))"),
 "wrong-A-field-prev":once("replace(write_A_after_B@next,","replace(write_A_after_B@prev,"),
 "wrong-A-next-src-target":once(
  "replace(write_A_after_B@next,\n                                                    Option<ptr<Node>>::Some(ptr_C))",
  "replace(write_A_after_B@next,\n                                                    Option<ptr<Node>>::Some(ptr_src))"),
 "duplicate-consumed-holder":once("let transferred=staged;","let transferred=staged;let dup=staged;"),
 "extra-eight-direct-C-prev":once(
  "                                            let rebuilt=DetachedHolder{held:LiveRoot{",
  "                                            let old_C_prev_8=loan_read(life_C){|stable_C8|let w_C8=ref_from_ptr(write,ptr_C,stable_C8);replace(w_C8@prev,Option<ptr<Node>>::Some(ptr_A))};\n                                            let rebuilt=DetachedHolder{held:LiveRoot{"),
 "wrong-original-A-prev-initial":once(
  "replace(w_A_prev@prev,\n                                                    Option<ptr<Node>>::Some(ptr_C))",
  "replace(w_A_prev@prev,\n                                                    Option<ptr<Node>>::Some(ptr_B))"),
 "wrong-original-C-prev-initial":once(
  "replace(w_C_prev@prev,\n                                                    Option<ptr<Node>>::Some(ptr_B))",
  "replace(w_C_prev@prev,\n                                                    Option<ptr<Node>>::Some(ptr_C))"),
 "no-second-helper":once(
  "set_link(write_B_next@next,Option<ptr<Node>>::None);",
  "unit;"),
 "third-named-helper-on-A":once(
  "replace(write_A_after_B@next,\n                                                    Option<ptr<Node>>::Some(ptr_C))",
  "set_link(write_A_after_B@next,\n                                                    Option<ptr<Node>>::Some(ptr_C))")
}
rows=[]
for name,source in cases.items():
    path=out/(name+".nl")
    path.write_text(source)
    cli=subprocess.run([compiler,str(path)],capture_output=True,text=True)
    b=subprocess.run([boundary,str(path)],capture_output=True,text=True,check=True)
    d=json.loads(b.stdout)
    assert d["parse_status"]==0 and d["registration_status"]!=0 and d["snapshot_unchanged"],(name,d,cli.stderr)
    assert cli.returncode==3 and not cli.stdout,(name,cli.stderr,cli.stdout)
    retry=subprocess.run([observer,str(root/"evidence/p315/generated/third-A-next-own-domain.nl"),"after-refusal",str(path)],
                         capture_output=True,text=True)
    assert retry.returncode==0,(name,retry.stderr,retry.stdout)
    assert '"registration_refusal_rollback_retry":true' in retry.stdout
    row={"name":name,"sha256":hashlib.sha256(source.encode()).hexdigest(),
         "status":d["registration_status"],"code":d["code"],
         "start_byte":d["start_byte"],"end_byte":d["end_byte"],
         "rollback":True,"owned_clean_retry":True,"cli_exit":cli.returncode}
    rows.append(row)
    print("P315_NEGATIVE",json.dumps(row,sort_keys=True),flush=True)
assert len(rows)==14
(root/"evidence/p315/negative-observations.json").write_text(json.dumps(rows,indent=2)+"\n")
print("P315_FOURTEEN_REFUSALS_ROLLBACK_RETRY_PASS",len(rows),flush=True)
