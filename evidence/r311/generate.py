#!/usr/bin/env python3
"""Track R #311: generate independently structured five-original-H programs.
No fixture text is read or transformed. Each shape is built here from allocation
and explicit matched-cleanup rules. Profiles differ in Change order and result.
"""
import pathlib, sys, hashlib
mode=sys.argv[1]
if mode not in ("fresh-none","fresh-some"): raise SystemExit("bad mode")
out=pathlib.Path(sys.argv[2])
lines=[]
indent=0
def put(s):
    lines.append("    "*indent+s)
def op(s):
    global indent
    put(s);indent+=1
def cl(s):
    global indent
    indent-=1;put(s)
names=["src","A","B","C","dst"]
header="""// Track R #311 independently generated source; original five H allocation sites.
struct Node {
    next:Option<ptr<Node>>,
    prev:Option<ptr<Node>>,
    child:Option<ptr<Node>>,
    payload:u8,
}
struct LiveRoot { p:ptr<Node>, a:Allocation, d:LifetimeDomain, }
struct DetachedHolder { held:LiveRoot, }
fn set_link(field_ref:ref<write,Option<ptr<Node>>>, new_value:Option<ptr<Node>>) -> unit {
    let formerly=replace(field_ref,new_value);
    unit
}
"""
def cleanup(prefix, final=False):
    for n in reversed(prefix):
        d="final_d_B" if final and n=="B" else "life_"+n
        p="final_ptr_B" if final and n=="B" else "ptr_"+n
        a="final_a_B" if final and n=="B" else "allocation_"+n
        e="emptied_"+n
        z="restored_"+n
        op("let "+e+" = loan_exclusive_read("+d+") { |finish_"+n+"|")
        put("destroy("+p+",finish_"+n+")")
        cl("};")
        put("let "+z+"=erase_slot<Node>("+e+");")
        put("finalize_domain("+d+");")
        put("deallocate("+a+","+z+");")
def initial_link(n,field,target):
    tag=n+"_"+field
    op("let prior_"+tag+"=loan_read(life_"+n+") { |proof_"+tag+"|")
    put("let write_"+tag+"=ref_from_ptr(write,ptr_"+n+",proof_"+tag+");")
    put("replace(write_"+tag+"@"+field+",Option<ptr<Node>>::Some(ptr_"+target+"))")
    cl("};")
def full_case():
    # Deliberately different topological change order from P308 fixture;
    # six exact initial links, no second helper.
    links=[("C","prev","B"),("A","next","B"),("B","next","C"),
           ("src","child","A"),("A","prev","C"),("B","prev","A")]
    if mode=="fresh-some":
        links=[("src","child","A"),("C","prev","B"),("A","prev","C"),
               ("B","prev","A"),("B","next","C"),("A","next","B")]
    for args in links: initial_link(*args)
    put("let initial_owner_B=LiveRoot{p:ptr_B,a:allocation_B,d:life_B};")
    if mode=="fresh-none":
        put("let holder_B=DetachedHolder{held:initial_owner_B};")
        put("let DetachedHolder{held:carried_B}=holder_B;")
    else:
        put("let holder_B=DetachedHolder{held:initial_owner_B};")
        put("let handoff_B=holder_B;")
        put("let DetachedHolder{held:carried_B}=handoff_B;")
    put("let LiveRoot{p:moved_ptr_B,a:moved_a_B,d:moved_d_B}=carried_B;")
    op("loan_read(moved_d_B) { |reloan_original_B|")
    put("let mutable_B=ref_from_ptr(write,moved_ptr_B,reloan_original_B);")
    replacement="None" if mode=="fresh-none" else "Some(ptr_C)"
    put("set_link(mutable_B@prev,Option<ptr<Node>>::"+replacement+");")
    put("unit")
    cl("};")
    put("let terminal_box=DetachedHolder{held:LiveRoot{p:moved_ptr_B,a:moved_a_B,d:moved_d_B}};")
    put("let DetachedHolder{held:whole_B}=terminal_box;")
    put("let LiveRoot{p:final_ptr_B,a:final_a_B,d:final_d_B}=whole_B;")
    cleanup(names,True)
    put("unit")
def allocate(i):
    n=names[i]
    op("match try_allocate_one<Node>() {")
    op("None => {")
    cleanup(names[:i])
    put("unit")
    cl("},")
    op("Some(bundle_"+n+") => {")
    put("let OneBacking{allocation,raw}=bundle_"+n+";")
    put("let allocation_"+n+"=allocation;")
    put("let vacant_"+n+"=into_slot<Node>(raw);")
    put("let life_"+n+"=lifetime_domain();")
    op("let ptr_"+n+"=loan_read(life_"+n+") { |first_stable_"+n+"|")
    op("initialize(vacant_"+n+",Node {")
    put("next:Option<ptr<Node>>::None,")
    put("prev:Option<ptr<Node>>::None,")
    put("child:Option<ptr<Node>>::None,")
    put("payload:u8("+str(20+i)+")")
    cl("},first_stable_"+n+")")
    cl("};")
    if i<4: allocate(i+1)
    else: full_case()
    cl("},")
    cl("}")
op("fn main() -> unit {")
allocate(0)
cl("}")
out.parent.mkdir(parents=True,exist_ok=True)
s=header+"\n"+"\n".join(lines)+"\n"
out.write_text(s)
print(mode,"sha256",hashlib.sha256(s.encode()).hexdigest(),"lines",s.count("\n"))
