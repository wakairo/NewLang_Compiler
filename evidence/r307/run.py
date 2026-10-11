#!/usr/bin/env python3
"""R #307 independent source/public-owned quality evidence. No compiler modifications."""
import hashlib
import json
import pathlib
import subprocess
import sys
import tempfile

cc, observer, boundary, original_observer, result_path = sys.argv[1:6]
root = pathlib.Path(__file__).resolve().parents[2]
folder = root / "evidence/r307"
a = (folder / "r307_some_to_some_B.nl").read_text()
b = (folder / "r307_double_holder_none.nl").read_text()
cal = (folder / "calibration_P306_not_R.nl").read_text()
fixed = (root / "tests/fixtures/experimental_scoped_field_call.nl").read_text()
assert hashlib.sha256(cal.encode()).hexdigest() == "beffe69de3a858e6cc0a8a4d7acb86a569310a34cf5710716887a6a3b7c82e46"
assert hashlib.sha256(fixed.encode()).hexdigest() == "e83854ba2b37f8c522fa119c282ce48d5b4914e8868ebe7f51f96ad2e96f5096"
assert a.count("try_allocate_one<Node>()") == b.count("try_allocate_one<Node>()") == 5
assert "let LiveRoot{p:moved_ptr_B,a:moved_a_B,d:moved_d_B}" in a
assert "let LiveRoot{p:moved_ptr_B,a:moved_a_B,d:moved_d_B}" in b

def once(s, old, new):
    if s.count(old) != 1:
        raise ValueError(f"{old}: expected exactly one, got {s.count(old)}")
    return s.replace(old, new)

call_a = "set_link(write_B@next,Option<ptr<Node>>::Some(moved_ptr_B));"
call_b = "set_link(write_B@next,Option<ptr<Node>>::None);"
cases = [
    ("R-someB-new-occurrence",a,0,"","some_b"),
    ("R-two-holder-clear",b,0,"","none"),
    ("wrong_original_domain",once(a,"loan_read(moved_d_B) { |stable_B_field|","loan_read(life_C) { |stable_B_field|"),1,"ALLOCATED-DOMAIN-MISMATCH",None),
    ("stale_original_domain",once(b,"loan_read(moved_d_B) { |stable_B_field|","loan_read(life_B) { |stable_B_field|"),1,"P3-USE-AFTER-CONSUME",None),
    ("duplicate_consumed_holder",once(a,"let custody_second=custody_first;","let custody_second=custody_first; let duplicate=custody_first;"),1,"P3-USE-AFTER-CONSUME",None),
    ("active_domain_transfer",once(a,call_a,"let shadow_domain=moved_d_B; "+call_a),1,"P3-REF-CONFLICT",None),
    ("active_whole_owner_repack",once(b,call_b,"let illegal=LiveRoot{p:moved_ptr_B,a:moved_a_B,d:moved_d_B}; "+call_b),1,"P3-REF-CONFLICT",None),
    ("copy_pointer_as_domain",once(b,"ref_from_ptr(write,moved_ptr_B,stable_B_field)","ref_from_ptr(write,moved_ptr_B,moved_ptr_B)"),1,"ALLOCATED-REF-TYPE",None),
    ("write_ref_escape",once(a,call_a+"\n                                                unit",call_a+"\n                                                write_B@next"),1,"P297-LOAN-ESCAPE",None),
    ("wrong_B_terminal_domain",once(b,"loan_exclusive_read(final_d_B) { |ending_B|","loan_exclusive_read(life_A) { |ending_B|"),1,"P3-DOMAIN-MISMATCH",None),
    ("incorrect_B_terminal_allocation",once(a,"deallocate(final_a_B, full_B);","deallocate(allocation_A, full_B);"),1,"P4-ALLOCATION-MISMATCH",None),
    ("prev_precision_only",once(b,call_b,call_b.replace("@next","@prev")),3,"P297-CALL-PRECISION",None),
    ("second_helper_precision",once(a,call_a,call_a+"\n                                                 "+call_a),3,"P297-CALL-PRECISION",None),
    ("borrowed_old_occurrence_precision",once(b,call_b,"match write_B@next { None=>{unit},Some(old)=>{"+call_b+" let retained=old; unit}};"),3,"P9-MATCH-PRECISION",None),
    ("P306-calibration-only",cal,0,"","cal_P306"),
    ("PR302-calibration-only",fixed,0,"","cal_fixed"),
]
out=[]
with tempfile.TemporaryDirectory(prefix="r307-independent-") as tmp:
    for name,src,expected_status,expected_code,owned_mode in cases:
        path=pathlib.Path(tmp)/(name+".nl")
        path.write_text(src)
        h=hashlib.sha256(src.encode()).hexdigest()
        cli_runs=[subprocess.run([cc,str(path)],text=True,capture_output=True) for _ in range(2)]
        r=cli_runs[0]
        identical=all((x.returncode,x.stdout,x.stderr)==(r.returncode,r.stdout,r.stderr) for x in cli_runs)
        bnd=subprocess.run([boundary,str(path)],text=True,capture_output=True)
        boundary_state=None
        try: boundary_state=json.loads(bnd.stdout)
        except (ValueError,TypeError): pass
        public=None
        public_tool=observer if owned_mode in ("none","some_b","cal_P306") else original_observer
        mode=["none" if owned_mode=="cal_P306" else owned_mode] if public_tool==observer else []
        if expected_status==0:
            p=subprocess.run([public_tool,str(path),*mode],text=True,capture_output=True)
            public={"exit":p.returncode,"stdout":p.stdout,"stderr":p.stderr,
                    "actual_owned":p.returncode==0}
        actual_status=boundary_state.get("registration_status") if boundary_state else None
        actual_code=boundary_state.get("code") if boundary_state else None
        acceptable=(
            identical and bnd.returncode==0 and actual_status==expected_status and
            actual_code==expected_code and
            r.returncode==(4 if expected_status==0 else 3) and
            (expected_status!=0 or ("V1-BACKEND-UNSUPPORTED" in r.stderr and
                                     public and public["actual_owned"])) and
            (expected_status==0 or
             (boundary_state["snapshot_unchanged"] and not boundary_state["owned_entry_artifact"]))
        )
        def loc(n):
            if n is None: return None
            return [src[:n].count("\n")+1,len(src[:n].rsplit("\n",1)[-1])+1]
        rec={
            "name":name,"origin":"R-owned" if name.startswith(("R-","wrong","stale","duplicate","active","copy","write","incorrect","prev","second","borrowed")) else "calibration-only",
            "sha256":h,"source_bytes":len(src.encode()),"expected_registration":expected_status,
            "expected_diagnostic_code":expected_code,
            "cli_exit":r.returncode,
            "diagnostic":r.stderr.replace(str(path),"<source>"),
            "cli_deterministic_twice":identical,
            "boundary_exit":bnd.returncode,"boundary":boundary_state,
            "span_start":loc(boundary_state.get("start_byte")) if boundary_state else None,
            "span_end":loc(boundary_state.get("end_byte")) if boundary_state else None,
            "owned":public,"c_generated":False,"pass":acceptable
        }
        out.append(rec)
        print(name, "PASS" if acceptable else "MISMATCH",
              "cli",r.returncode,"reg",actual_status,"code",actual_code,
              "owned",public["exit"] if public else "-",
              flush=True)
result_path=pathlib.Path(result_path)
result_path.parent.mkdir(parents=True,exist_ok=True)
result_path.write_text(json.dumps(out,indent=2,ensure_ascii=False)+"\n")
# A red CI result still retains the exact empirical evidence for an INCONCLUSIVE finding.
sys.exit(0 if all(r["pass"] for r in out) else 1)
