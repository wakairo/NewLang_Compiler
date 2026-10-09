#!/usr/bin/env python3
"""Fresh R258 source corpus; only input is canonical §3.2b.3 extracted in this run."""
from pathlib import Path
from hashlib import sha256
import json
import re

out=Path("r_native_258/out")
src=(out/"canonical.nl").read_text()
assert src.count("match try_allocate_one<Node>()")==5
assert src.count("let old_")==6

def unique_replace(s, old, new):
    if s.count(old)!=1:
        raise ValueError(f"expected one occurrence of {old!r}, got {s.count(old)}")
    return s.replace(old,new,1)

def reorder(s, order):
    a=s.index("let old_src_child =")
    # leave whitespace prefix on original line undisturbed
    b=s.index("// At this exact point",a)
    body=s[a:b]
    hits=list(re.finditer(r"let old_(?:src_child|A_prev|A_next|B_prev|B_next|C_prev) = loan_read",body))
    if len(hits)!=6: raise ValueError("six scoped blocks not found")
    blocks=[body[hits[i].start(): hits[i+1].start() if i+1<len(hits) else len(body)]
            for i in range(6)]
    return s[:a]+''.join(blocks[i] for i in order)+s[b:]

def brace_end(s,start):
    op=s.index("{",start)
    depth=0
    for pos in range(op,len(s)):
        c=s[pos]
        if c=="{": depth+=1
        if c=="}":
            depth-=1
            if depth==0:return pos+1
    raise ValueError("unbalanced braces")

def swap_last_arms(s):
    start=s.rfind("match try_allocate_one<Node>()")
    x=s.index("None =>",start)
    ey=brace_end(s,x)
    y=s.index("Some(bundle_dst) =>",ey)
    ez=brace_end(s,y)
    first=s[x:ey]; second=s[y:ez]
    if not s[ey:y].strip()==",": raise ValueError("missing comma")
    return s[:x]+second+",\n"+first+s[ez:]

case={}
case["00_canonical"]=src
case["01_alpha_locals"]=src.replace("bundle_A","bundle_Alpha").replace("allocation_A","allocation_Alpha").replace("life_A","life_Alpha").replace("ptr_A","ptr_Alpha").replace("vacant_A","vacant_Alpha")
case["02_alpha_nominal"]=src.replace("Node","Cell")
case["03_permute_A_siblings"]=reorder(src,[0,2,1,3,4,5])
case["04_permute_B_siblings"]=reorder(src,[0,1,2,4,3,5])
case["05_reverse_disjoint_changes"]=reorder(src,[5,4,3,2,1,0])
case["06_swap_last_sum_arms"]=swap_last_arms(src)
case["07_policy_A_prev_B"]=unique_replace(src,"replace(w_A_prev@prev,\n                                                    Option<ptr<Node>>::Some(ptr_C))",
                                                     "replace(w_A_prev@prev,\n                                                    Option<ptr<Node>>::Some(ptr_B))")
case["08_policy_B_next_A"]=unique_replace(src,"replace(w_B_next@next,\n                                                    Option<ptr<Node>>::Some(ptr_C))",
                                                     "replace(w_B_next@next,\n                                                    Option<ptr<Node>>::Some(ptr_A))")
case["09_policy_src_child_C"]=unique_replace(src,"replace(w_src_child@child,\n                                                    Option<ptr<Node>>::Some(ptr_A))",
                                                     "replace(w_src_child@child,\n                                                    Option<ptr<Node>>::Some(ptr_C))")
case["10_policy_wrong_dst_initial"]=src[:src.index("payload: u8(5)")].rsplit("child: Option<ptr<Node>>::None",1)[0]+"child: Option<ptr<Node>>::Some(ptr_src)"+src[:src.index("payload: u8(5)")].rsplit("child: Option<ptr<Node>>::None",1)[1]+src[src.index("payload: u8(5)"):]
case["11_target_A_next_as_prev"]=unique_replace(src,"replace(w_A_next@next,","replace(w_A_next@prev,")
case["12_wrong_domain_A"]=unique_replace(src,"let old_A_next = loan_read(life_A)","let old_A_next = loan_read(life_B)")
case["13_wrong_root_A"]=unique_replace(src,"ref_from_ptr(write, ptr_A, stable_A_prev)","ref_from_ptr(write, ptr_B, stable_A_prev)")
case["14_double_release_A"]=src[:src.rfind("deallocate(allocation_A, full_A);")]+src[src.rfind("deallocate(allocation_A, full_A);"):].replace("deallocate(allocation_A, full_A);","deallocate(allocation_A, full_A);\n                                            deallocate(allocation_A, full_A);",1)
case["15_wrong_release_identity"]=src[:src.rfind("deallocate(allocation_B, full_B);")]+src[src.rfind("deallocate(allocation_B, full_B);"):].replace("deallocate(allocation_B, full_B);","deallocate(allocation_A, full_B);",1)
case["16_payload_projection_instead"]=unique_replace(src,"replace(w_A_prev@prev,","replace(w_A_prev@payload,")
case["17_sixth_site_on_failure"]=unique_replace(src,"None => {\n            unit\n        }",
    "None => {\n            match try_allocate_one<Node>() { None => { unit }, Some(extra) => { unit }, }\n        }")
case["18_missing_B_deallocation"]=src[:src.rfind("deallocate(allocation_B, full_B);")]+src[src.rfind("deallocate(allocation_B, full_B);"):].replace("deallocate(allocation_B, full_B);","unit;",1)
case["19_copy_ptr_wrong_EndRoot"]=src[:src.rfind("destroy(ptr_C, ending_C)")]+src[src.rfind("destroy(ptr_C, ending_C)"):].replace("destroy(ptr_C, ending_C)","destroy(ptr_B, ending_C)",1)
assert len(case)>=12
assert len({sha256(v.encode()).hexdigest() for v in case.values()})==len(case)
manifest=[]
for name,body in case.items():
    path=out/(name+".nl")
    path.write_text(body)
    manifest.append({"id":name,"sha256":sha256(path.read_bytes()).hexdigest(),"bytes":len(path.read_bytes())})
(out/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
print(json.dumps(manifest,indent=2))
