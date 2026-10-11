#!/usr/bin/env python3
"""R304: independent non-blind evidence. Never count P's tests as independent."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

compiler = Path(sys.argv[1]).resolve()
observer = Path(sys.argv[2]).resolve()
repo = Path(__file__).resolve().parents[2]
base = repo / "evidence/r304"
results = []
report = base / "observations.json"
def checkpoint():
    report.write_text(json.dumps(results, ensure_ascii=False, indent=2)+"\n")
def check_case(name, path, owned_expected=None):
    sha=hashlib.sha256(path.read_bytes()).hexdigest()
    work=base/"case_outputs"/name
    work.mkdir(parents=True,exist_ok=True)
    run=subprocess.run([str(compiler),str(path.resolve())],
                       cwd=work,capture_output=True,text=True)
    c_outputs=[str(p.relative_to(work)) for p in work.rglob("*.c")]
    item={"case":name,"source":str(path.relative_to(repo)),
          "source_sha256":sha,"cli_exit":run.returncode,
          "diagnostics":run.stderr,"stdout":run.stdout,
          "generated_c":c_outputs,"parse_registration_cli":"classification pending",
          "public_owned":None}
    results.append(item)
    checkpoint()
    print(json.dumps(item,ensure_ascii=False),flush=True)
    if owned_expected is not None:
        if run.returncode!=4 or "V1-BACKEND-UNSUPPORTED" not in run.stderr or c_outputs:
            raise RuntimeError("calibrated source admission/CLI gate failed: "+name)
        owned=subprocess.run([str(observer),str(path.resolve()),str(owned_expected)],
                             capture_output=True,text=True)
        item["public_owned"]={"exit":owned.returncode,"stdout":owned.stdout,
                              "stderr":owned.stderr}
        item["parse_registration_cli"]="source CLI accepted; observer checks parser+registration+owned entry"
        checkpoint()
        print(json.dumps({"case":name,"owned":item["public_owned"]},ensure_ascii=False),flush=True)
        if owned.returncode!=0 or '"source_owned":true' not in owned.stdout:
            raise RuntimeError("first owned witness failed: "+name)
    else:
        diag=run.stderr
        if run.returncode==3:
            if any(code in diag for code in
                   ("P8-SIGNATURE-PRECISION","P9-MATCH-PRECISION",
                    "P297-CALL-PRECISION","CAPTURED-CLOSURE-PRECISION")):
                item["parse_registration_cli"]="precision limit"
            elif any(code in diag for code in
                     ("UNSUPPORTED","FIELD-PROFILE","ALLOCATED-DOMAIN-LOAN")):
                item["parse_registration_cli"]="unsupported source/profile"
            elif any(code in diag for code in
                     ("SYNTAX","PARSE","P5-EXPRESSION-UNSUPPORTED")):
                item["parse_registration_cli"]="parser gate"
            else:
                item["parse_registration_cli"]="semantic-gated (diagnostic must be audited)"
        else:
            item["parse_registration_cli"]="unexpected CLI state"
        checkpoint()
    return item

fixture=repo/"tests/fixtures/experimental_scoped_field_call.nl"
expected_hash="e83854ba2b37f8c522fa119c282ce48d5b4914e8868ebe7f51f96ad2e96f5096"
if hashlib.sha256(fixture.read_bytes()).hexdigest()!=expected_hash:
    raise RuntimeError("fixed calibration fixture SHA256 mismatch")
# Explicitly stop at the FIRST calibration failure before independent cases.
check_case("calibration",fixture,4)
check_case("same_pointer",base/"same_pointer_fresh.nl",3)
check_case("none_clears",base/"none_clears_occurrence.nl",0)
for key in ("wrong_domain","read_ref_as_write","domain_consume_reuse",
            "end_root_while_loaned","reference_escape","unreleased_owner",
            "copy_ptr_authority"):
    check_case(key,base/(key+".nl"))
semantic=sum(r["parse_registration_cli"]=="semantic-gated (diagnostic must be audited)"
             for r in results[3:])
print(json.dumps({"cases":len(results),"new_positive_owned":2,
                  "negative_semantic_gates":semantic,"CI_url":
                  os.getenv("GITHUB_SERVER_URL","")+"/"+os.getenv("GITHUB_REPOSITORY","")+
                  "/actions/runs/"+os.getenv("GITHUB_RUN_ID","")}),flush=True)
if semantic<3:
    raise RuntimeError("less than three negative semantic gates")
