#!/usr/bin/env python3
# Round B: controlled source-profile reachability; independent R corpus.
import pathlib, subprocess, hashlib, json, os
ROOT=pathlib.Path("r288-frozen-evidence")
source=(ROOT/"p01_independent_single_owner_identity.nl").read_text()
one=source.replace("    prev: Option<ptr<H>>,\n    child: Option<ptr<H>>,\n","").replace("                    prev: Option<ptr<H>>::None,\n                    child: Option<ptr<H>>::None,\n","")
assert "prev:" not in one and "child:" not in one
def labeled(s):
    return s.replace("handle","owned_ptr").replace("token","owned_allocation").replace("guard","owned_domain")
cases={}
cases["p18_one_link_custom_capsule"]=one
cases["p19_one_link_named_owner_components"]=labeled(one)
cases["p20_one_link_named_live_tail"]=labeled(one).replace("Capsule","LiveTail")
cases["p21_one_link_named_owner_type"]=labeled(one).replace("Capsule","Owner")
cases["p22_one_link_custody_no_call"]=labeled(one).replace("fn pass_through(c: Capsule) -> Capsule {\n    c\n}\n","").replace("let received = pass_through(capsule);","let received = capsule;")
cases["p23_one_link_custody_no_call_owner"]=cases["p22_one_link_custody_no_call"].replace("Capsule","Owner")
cases["n24_one_link_stolen_domain"]=cases["p19_one_link_named_owner_components"].replace(
    "let capsule = Capsule", "let stolen = domain;\n            let capsule = Capsule")
cases["n25_one_link_stolen_allocation"]=cases["p19_one_link_named_owner_components"].replace(
    "let capsule = Capsule", "let stolen = allocation;\n            let capsule = Capsule")
cases["n26_one_link_double_callee"]=cases["p19_one_link_named_owner_components"].replace(
    "fn pass_through(c: Capsule) -> Capsule {\n    c\n}",
    "fn pass_through(c: Capsule) -> Capsule {\n    let old = c;\n    c\n}")
cases["n27_one_link_use_after_move"]=cases["p19_one_link_named_owner_components"].replace(
    "let received = pass_through(capsule);","let received = pass_through(capsule);\n            capsule;")
cases["n28_one_link_duplicate_constructor"]=cases["p19_one_link_named_owner_components"].replace(
    "owned_allocation: allocation, owned_domain: domain }","owned_allocation: allocation, owned_allocation: allocation, owned_domain: domain }")
cases["n29_one_link_partial_destructure"]=cases["p19_one_link_named_owner_components"].replace(
    "let Capsule { owned_ptr, owned_allocation, owned_domain } = received;",
    "let Capsule { owned_ptr, owned_allocation } = received;")
cases["n30_one_link_double_deallocation"]=cases["p19_one_link_named_owner_components"].replace(
    "deallocate(owned_allocation, full);","deallocate(owned_allocation, full);\n            deallocate(owned_allocation, full);")
cases["n31_one_link_fake_copy_ptr"]=cases["p19_one_link_named_owner_components"].replace(
    "owned_allocation: allocation, owned_domain: domain","owned_allocation: owned_ptr, owned_domain: owned_ptr")
cases["n32_one_link_drop_returned"]=cases["p19_one_link_named_owner_components"].replace(
    "let Capsule { owned_ptr, owned_allocation, owned_domain } = received;","")
cases["n33_one_link_wrong_primitive_release"]=cases["p19_one_link_named_owner_components"].replace(
    "deallocate(owned_allocation, full);","deallocate(owned_ptr, full);")
out=[]
for name,text in cases.items():
    f=ROOT/(name+".nl")
    f.write_text(text)
    p=subprocess.run(["build-r288/newlangc",str(f)],capture_output=True)
    if p.stdout:(ROOT/(name+".c")).write_bytes(p.stdout)
    rec=dict(case=name,exit=p.returncode,sha256=hashlib.sha256(text.encode()).hexdigest(),
             stdout_bytes=len(p.stdout),emitted_c=b"#include" in p.stdout,
             diagnostic=p.stderr.decode(errors="replace")[-3000:],
             stdout_preview=p.stdout.decode(errors="replace")[:450])
    print("R288_RESULT_B "+json.dumps(rec,ensure_ascii=False),flush=True)
    out.append(rec)
(ROOT/"results_b.json").write_text(json.dumps(out,indent=2)+"\n")
print("R288_COUNT_B",len(cases))
