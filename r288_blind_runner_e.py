#!/usr/bin/env python3
# R288 E: independent two-origin owner-packet composition and exact release obligation.
import pathlib,subprocess,hashlib,json
R=pathlib.Path("r288-frozen-evidence")
two=(R/"p49_two_origins_matched_release.nl").read_text()
defs="""struct CapsuleA {
    first_ptr: ptr<H>,
    first_allocation: Allocation,
    first_domain: LifetimeDomain,
}
struct CapsuleB {
    second_ptr: ptr<H>,
    second_allocation: Allocation,
    second_domain: LifetimeDomain,
}
struct Envelope {
    first: CapsuleA,
    second: CapsuleB,
}
fn carry(v: Envelope) -> Envelope {
    v
}
"""
begin=two.find("let dead1 = loan_exclusive_read(d1)")
end=two.find("deallocate(a1, raw1);",begin)+len("deallocate(a1, raw1);")
assert begin!=-1 and end>begin
fragment="""let packet = Envelope {
                        first: CapsuleA {
                            first_ptr: p0, first_allocation: a0, first_domain: d0
                        },
                        second: CapsuleB {
                            second_ptr: p1, second_allocation: a1, second_domain: d1
                        },
                    };
                    let result = carry(packet);
                    let Envelope { first, second } = result;
                    let CapsuleA { first_ptr, first_allocation, first_domain } = first;
                    let CapsuleB { second_ptr, second_allocation, second_domain } = second;
                    let freed0 = loan_exclusive_read(first_domain) { |end0|
                        destroy(first_ptr, end0)
                    };
                    let raw0 = erase_slot<H>(freed0);
                    finalize_domain(first_domain);
                    let freed1 = loan_exclusive_read(second_domain) { |end1|
                        destroy(second_ptr, end1)
                    };
                    let raw1 = erase_slot<H>(freed1);
                    finalize_domain(second_domain);
                    deallocate(first_allocation, raw0);
                    deallocate(second_allocation, raw1);"""
packet=two[:begin]+fragment+two[end:]
packet=packet.replace("fn main() -> unit {",defs+"fn main() -> unit {")
cases={}
cases["p64_two_origins_complete_packet_identity"]=packet
cases["n65_two_origins_packet_wrong_release"]=packet.replace(
 "first_allocation: a0, first_domain: d0","first_allocation: a1, first_domain: d0").replace(
 "second_allocation: a1, second_domain: d1","second_allocation: a0, second_domain: d1")
# p65 is a harmless mixed owner construction/transfer; release is properly re-paired.
cases["p66_mixed_packet_then_repaired_release"]=cases["n65_two_origins_packet_wrong_release"].replace(
 "deallocate(first_allocation, raw0);\n                    deallocate(second_allocation, raw1);",
 "deallocate(second_allocation, raw0);\n                    deallocate(first_allocation, raw1);")
cases["n67_duplicate_member_alloc"]=packet.replace(
 "second_allocation: a1, second_domain: d1",
 "second_allocation: a0, second_domain: d1")
cases["n68_packet_reconsume_after_call"]=packet.replace(
 "let result = carry(packet);","let result = carry(packet);\n                    packet;")
cases["n69_packet_member_missing"]=packet.replace(
 "let Envelope { first, second } = result;",
 "let Envelope { first } = result;")
cases["n70_packet_type_mistyped_domain"]=packet.replace(
 "second_allocation: a1, second_domain: d1",
 "second_allocation: a1, second_domain: p1")
# NonCopy obligation on successful branch cannot be dropped.
good=(R/"p34_fixed_binding_independent_owner_identity.nl").read_text()
start=good.index("let Capsule { owned_ptr, owned_allocation, owned_domain } = received;")
stop=good.index("            unit",start)
cases["n71_owner_normal_exit_drop"]=good[:start]+good[stop:]
# On second-site failure cleanup must explicitly discharge first resource.
f=two.replace(
 """let dead0 = loan_exclusive_read(d0) { |end0|
                destroy(p0, end0)
            };
            let raw0 = erase_slot<H>(dead0);
            finalize_domain(d0);

                    deallocate(a0, raw0);""", "unit;")
# exact replace may not match indentation; use fenced None section.
a=f.index("None => {",f.index("match try_allocate_one<H>()",f.index("Some(bundle0)")))
b=f.index("},",a)
f=f[:a]+"None => { unit "+f[b:]
cases["n72_two_root_failure_branch_drops_one"]=f
nested=(R/"p56_two_level_nested_complete_identity.nl").read_text()
nested=nested.replace("fn pass_through", """struct Top {
    parcel: Envelope,
}
fn pass_through""")
nested=nested.replace("fn pass_through(c: Envelope) -> Envelope","fn pass_through(c: Top) -> Top")
nested=nested.replace("let received = pass_through(packet);","let cap = Top { parcel: packet };\n            let received = pass_through(cap);")
nested=nested.replace("let Envelope { piece } = received;","let Top { parcel } = received;\n            let Envelope { piece } = parcel;")
cases["p73_three_level_nested_complete_identity"]=nested
nested4=nested.replace("fn pass_through", """struct Top2 {
    bundle: Top,
}
fn pass_through""")
nested4=nested4.replace("fn pass_through(c: Top) -> Top","fn pass_through(c: Top2) -> Top2")
nested4=nested4.replace("let received = pass_through(cap);","let cap2 = Top2 { bundle: cap };\n            let received = pass_through(cap2);")
nested4=nested4.replace("let Top { parcel } = received;","let Top2 { bundle } = received;\n            let Top { parcel } = bundle;")
cases["p74_four_level_nested_complete_identity"]=nested4
out=[]
for name,src in cases.items():
 p=R/(name+".nl");p.write_text(src)
 z=subprocess.run(["build-r288/newlangc",str(p)],capture_output=True)
 if z.stdout:(R/(name+".c")).write_bytes(z.stdout)
 v=dict(case=name,exit=z.returncode,sha256=hashlib.sha256(src.encode()).hexdigest(),
        emitted_c=b"#include" in z.stdout,c_bytes=len(z.stdout),
        diagnostic=z.stderr.decode(errors="replace")[-2500:])
 out.append(v);print("R288_RESULT_E "+json.dumps(v,ensure_ascii=False),flush=True)
(R/"results_e.json").write_text(json.dumps(out,indent=2)+"\n")
print("R288_COUNT_E",len(out))
