#!/usr/bin/env python3
# R288 D: nested valid packets, loan conflict, nominal invariance.
import pathlib,subprocess,hashlib,json
R=pathlib.Path("r288-frozen-evidence")
good=(R/"p34_fixed_binding_independent_owner_identity.nl").read_text()
nest=(R/"p46_nested_independent_complete_transfer.nl").read_text()
nest=nest.replace("    key: u8,\n","").replace(", key: u8(9)","").replace("{ payload, key }","{ payload }").replace("            key;\n","")
assert "    payload: Capsule," in nest and "key" not in nest
cases={}
cases["p52_nested_one_noncopy_aggregate"]=nest
cases["n53_nested_duplicate_inner"]=nest.replace("Mailer { payload: capsule }","Mailer { payload: capsule, payload: capsule }")
cases["n54_nested_drop_inner"]=nest.replace("let Mailer { payload } = received;","let Mailer { } = received;")
cases["n55_nested_caller_reconsume"]=nest.replace(
 "let received = pass_through(mailer);","let received = pass_through(mailer);\n            mailer;")
two=nest.replace("struct Mailer {\n    payload: Capsule,\n}", """struct Mailer {
    payload: Capsule,
}
struct Envelope {
    piece: Mailer,
}""")
two=two.replace("fn pass_through(c: Mailer) -> Mailer", "fn pass_through(c: Envelope) -> Envelope").replace(
 "let received = pass_through(mailer);","let packet = Envelope { piece: mailer };\n            let received = pass_through(packet);").replace(
 "let Mailer { payload } = received;", "let Envelope { piece } = received;\n            let Mailer { payload } = piece;")
cases["p56_two_level_nested_complete_identity"]=two
cases["n57_two_level_nested_illegal_duplicate"]=two.replace(
 "let packet = Envelope { piece: mailer };",
 "let packet = Envelope { piece: mailer, piece: mailer };")
cases["p58_renamed_field_alpha"]=good.replace("owned_ptr", "pointer_field").replace("owned_allocation", "allocation_field").replace("owned_domain", "domain_field")
cases["p59_declaration_order_permutation"]=good.replace(
 "    owned_ptr: ptr<H>,\n    owned_allocation: Allocation,\n    owned_domain: LifetimeDomain,",
 "    owned_domain: LifetimeDomain,\n    owned_ptr: ptr<H>,\n    owned_allocation: Allocation,").replace(
 "owned_ptr: initial_ptr, owned_allocation: allocation, owned_domain: initial_domain",
 "owned_allocation: allocation, owned_domain: initial_domain, owned_ptr: initial_ptr")
cases["n60_transfer_domain_under_active_loan"]=good.replace(
 "let capsule = Capsule { owned_ptr: initial_ptr, owned_allocation: allocation, owned_domain: initial_domain };",
 """let capsule = loan_read(initial_domain) { |stable_active|
                Capsule { owned_ptr: initial_ptr, owned_allocation: allocation, owned_domain: initial_domain }
            };""")
# Two independently live H roots, attempt to reborrow p1 using d0.
two_src=(R/"p49_two_origins_matched_release.nl").read_text()
cases["n61_wrong_domain_reborrow"]=two_src.replace(
 "let dead1 = loan_exclusive_read(d1)",
 """let invalid_read = loan_read(d0) { |false_stable|
                        let wrong = ref_from_ptr(read, p1, false_stable);
                        unit
                    };
                    let dead1 = loan_exclusive_read(d1)""")
cases["n62_mismatched_domain_root_destroy"]=two_src.replace(
 "destroy(p1, end1)","destroy(p0, end1)")
# Test safe harmless packet extraction/reassembly using same owner; no release mismatch.
cases["p63_repack_noncopy_whole_values"]=good.replace(
 "let cleared = loan_exclusive_read(owned_domain)",
 """let reassembled = Capsule { owned_ptr: owned_ptr, owned_allocation: owned_allocation, owned_domain: owned_domain };
            let Capsule { owned_ptr, owned_allocation, owned_domain } = reassembled;
            let cleared = loan_exclusive_read(owned_domain)""") # May reject because same lexical names, record precisely.
out=[]
for name,src in cases.items():
 p=R/(name+".nl");p.write_text(src)
 z=subprocess.run(["build-r288/newlangc",str(p)],capture_output=True)
 if z.stdout:(R/(name+".c")).write_bytes(z.stdout)
 v=dict(case=name,exit=z.returncode,sha256=hashlib.sha256(src.encode()).hexdigest(),
        emitted_c=b"#include" in z.stdout,c_bytes=len(z.stdout),
        diagnostic=z.stderr.decode(errors="replace")[-2500:])
 out.append(v);print("R288_RESULT_D "+json.dumps(v,ensure_ascii=False),flush=True)
(R/"results_d.json").write_text(json.dumps(out,indent=2)+"\n")
print("R288_COUNT_D",len(out))
