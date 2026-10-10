#!/usr/bin/env python3
# Round C, independent repairs and deeper aggregate/boundary attacks.
import pathlib, subprocess, hashlib, json
R=pathlib.Path("r288-frozen-evidence")
raw=(R/"p19_one_link_named_owner_components.nl").read_text()
# Prior baseline unintentionally collided with destructured field names: use fresh source bindings.
good=raw.replace("let owned_ptr = loan_read(domain)", "let initial_ptr = loan_read(initial_domain)")
good=good.replace("let domain = lifetime_domain();","let initial_domain = lifetime_domain();")
good=good.replace("owned_ptr: owned_ptr, owned_allocation: allocation, owned_domain: domain", "owned_ptr: initial_ptr, owned_allocation: allocation, owned_domain: initial_domain")
assert "let initial_ptr" in good and "owned_ptr: initial_ptr" in good
cases={}
cases["p34_fixed_binding_independent_owner_identity"]=good
cases["n35_reuse_caller_after_transfer"]=good.replace(
 "let received = pass_through(capsule);", "let received = pass_through(capsule);\n            let stolen = pass_through(capsule);")
cases["n36_callee_duplicate_transfer"]=good.replace(
 "fn pass_through(c: Capsule) -> Capsule {\n    c\n}",
 "fn pass_through(c: Capsule) -> Capsule {\n    let first = c;\n    c\n}")
cases["n37_owner_double_release"]=good.replace(
 "deallocate(owned_allocation, full);",
 "deallocate(owned_allocation, full);\n            deallocate(owned_allocation, full);")
cases["n38_wrong_ending_authority"]=good.replace(
 "destroy(owned_ptr, ending)","destroy(owned_allocation, ending)")
cases["n39_premature_original_domain_transfer"]=good.replace(
 "let capsule = Capsule", "let theft = initial_domain;\n            let capsule = Capsule")
cases["n40_drop_returned_owner"]=good.replace(
 "let Capsule { owned_ptr, owned_allocation, owned_domain } = received;\n            let cleared",
 "let cleared")
cases["n41_copy_ptr_as_deallocator"]=good.replace(
 "deallocate(owned_allocation, full);","deallocate(owned_ptr, full);")
cases["n42_incomplete_whole_unpack"]=good.replace(
 "let Capsule { owned_ptr, owned_allocation, owned_domain } = received;",
 "let Capsule { owned_ptr, owned_allocation } = received;")
cases["n43_duplicate_whole_pack"]=good.replace(
 "owned_allocation: allocation, owned_domain: initial_domain",
 "owned_allocation: allocation, owned_allocation: allocation, owned_domain: initial_domain")
cases["n44_consume_local_and_repack"]=good.replace(
 "let received = pass_through(capsule);",
 "let saved = capsule;\n            let received = pass_through(capsule);")
cases["p45_no_helper_aggregate_transfer"]=good.replace(
 "fn pass_through(c: Capsule) -> Capsule {\n    c\n}\n","").replace(
 "let received = pass_through(capsule);","let received = capsule;")
nested=good.replace("fn pass_through", """struct Mailer {
    payload: Capsule,
    key: u8,
}
fn pass_through""")
nested=nested.replace("fn pass_through(c: Capsule) -> Capsule","fn pass_through(c: Mailer) -> Mailer")
nested=nested.replace("let received = pass_through(capsule);\n            let Capsule",
 "let mailer = Mailer { payload: capsule, key: u8(9) };\n            let received = pass_through(mailer);\n            let Mailer { payload, key } = received;\n            let Capsule")
nested=nested.replace("= received;\n            let cleared","= payload;\n            key;\n            let cleared")
cases["p46_nested_independent_complete_transfer"]=nested
cases["n47_nested_duplicate_inner"]=nested.replace(
 "Mailer { payload: capsule, key: u8(9) }",
 "Mailer { payload: capsule, payload: capsule, key: u8(9) }")
cases["n48_nested_dropped_member"]=nested.replace(
 "let Mailer { payload, key } = received;",
 "let Mailer { key } = received;")
# Produce independent two-root controls, separated from aggregate nominals.
header="""struct H {
    next: Option<ptr<H>>,
    payload: u8,
}
"""
def alloc(i):
 return f"""let OneBacking {{ allocation, raw }} = bundle{i};
            let a{i} = allocation;
            let s{i} = into_slot<H>(raw);
            let d{i} = lifetime_domain();
            let p{i} = loan_read(d{i}) {{ |stable{i}|
                initialize(s{i}, H {{
                    next: Option<ptr<H>>::None,
                    payload: u8({i+7})
                }}, stable{i})
            }};
"""
def cleanup(i):
 return f"""let dead{i} = loan_exclusive_read(d{i}) {{ |end{i}|
                destroy(p{i}, end{i})
            }};
            let raw{i} = erase_slot<H>(dead{i});
            finalize_domain(d{i});
"""
two=header+"""fn main() -> unit {
    match try_allocate_one<H>() {
        None => { unit },
        Some(bundle0) => {
            """+alloc(0)+"""
            match try_allocate_one<H>() {
                None => {
                    """+cleanup(0)+"""
                    deallocate(a0, raw0);
                    unit
                },
                Some(bundle1) => {
                    """+alloc(1)+"""
                    """+cleanup(1)+cleanup(0)+"""
                    deallocate(a0, raw0);
                    deallocate(a1, raw1);
                    unit
                },
            }
        },
    }
}
"""
cases["p49_two_origins_matched_release"]=two
cases["n50_two_origins_wrong_release"]=two.replace(
 "deallocate(a0, raw0);\n                    deallocate(a1, raw1);",
 "deallocate(a0, raw1);\n                    deallocate(a1, raw0);")
cases["n51_two_origins_double_use"]=two.replace(
 "deallocate(a1, raw1);",
 "deallocate(a1, raw1);\n                    deallocate(a1, raw1);")
out=[]
for name,src in cases.items():
    p=R/(name+".nl");p.write_text(src)
    z=subprocess.run(["build-r288/newlangc",str(p)],capture_output=True)
    if z.stdout:(R/(name+".c")).write_bytes(z.stdout)
    v=dict(case=name,exit=z.returncode,sha256=hashlib.sha256(src.encode()).hexdigest(),
           c_bytes=len(z.stdout),emitted_c=b"#include" in z.stdout,
           diagnostic=z.stderr.decode(errors="replace")[-2800:],
           stdout_preview=z.stdout.decode(errors="replace")[:400])
    out.append(v);print("R288_RESULT_C "+json.dumps(v,ensure_ascii=False),flush=True)
(R/"results_c.json").write_text(json.dumps(out,indent=2)+"\n")
print("R288_COUNT_C",len(out))
