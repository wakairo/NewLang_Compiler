#!/usr/bin/env python3
"""Track R #314: independently generated source witnesses, no implementation fixtures."""
from pathlib import Path
import hashlib
import sys

ROOTS = ("src", "A", "B", "C", "dst")
TYPE = "Option<ptr<Node>>"
OUT = Path("r-evidence/314/generated")
OUT.mkdir(parents=True, exist_ok=True)

def lines(*items):
    return "\n".join(items)

def initial(x):
    return lines(
      "let OneBacking { allocation, raw } = bundle_" + x + ";",
      "let allocation_" + x + " = allocation;",
      "let empty_" + x + " = into_slot<Node>(raw);",
      "let life_" + x + " = lifetime_domain();",
      "let ptr_" + x + " = loan_read(life_" + x + ") { |stable_" + x + "|",
      "    initialize(empty_" + x + ", Node {",
      "        next: Option<ptr<Node>>::None,",
      "        prev: Option<ptr<Node>>::None,",
      "        child: Option<ptr<Node>>::None,",
      "        payload: u8(" + str(ROOTS.index(x)+1) + ")",
      "    }, stable_" + x + ")",
      "};")

def cleanup(xs, moved_b=False):
    buf=[]
    for x in reversed(xs):
        ptr, life, alloc = ("ptr_"+x, "life_"+x, "allocation_"+x)
        if x=="B" and moved_b:
            ptr, life, alloc = "held_ptr", "held_domain", "held_allocation"
        buf.extend([
          "let erased_" + x + " = loan_exclusive_read(" + life + ") { |end_" + x + "|",
          "    destroy(" + ptr + ", end_" + x + ")",
          "};",
          "let raw_" + x + " = erase_slot<Node>(erased_" + x + ");",
          "finalize_domain(" + life + ");",
          "deallocate(" + alloc + ", raw_" + x + ");"
        ])
    return "\n".join(buf)

def wiring(x,field,point,n):
    return lines(
      "let old_setup_"+str(n)+" = loan_read(life_"+x+") { |stable_setup_"+str(n)+"|",
      "    let node_w_"+str(n)+" = ref_from_ptr(write, ptr_"+x+", stable_setup_"+str(n)+");",
      "    replace(node_w_"+str(n)+"@"+field+", "+TYPE+"::Some(ptr_"+point+"))",
      "};")

LINKS = (("src","child","A"),("A","prev","C"),("A","next","B"),
         ("B","prev","A"),("B","next","C"),("C","prev","B"))

def terminal(permutation=False):
    order=(0,2,5,1,3,4) if permutation else range(6)
    buf=[wiring(*LINKS[i],i) for i in order]
    buf += [
      "let holder = DetachedHolder {",
      "    held_ptr: ptr_B,",
      "    held_allocation: allocation_B,",
      "    held_domain: life_B",
      "};",
      "let DetachedHolder { held_ptr, held_allocation, held_domain } = holder;",
      "let first_result = loan_read(held_domain) { |fresh_b_1|",
      "    let original_b_1 = ref_from_ptr(write, held_ptr, fresh_b_1);",
      "    clear_link(original_b_1@prev, "+TYPE+"::None)",
      "};",
      "let second_result = loan_read(held_domain) { |fresh_b_2|",
      "    let original_b_2 = ref_from_ptr(write, held_ptr, fresh_b_2);",
      "    clear_link(original_b_2@next, "+TYPE+"::None)",
      "};",
      "let restored = DetachedHolder {",
      "    held_ptr: held_ptr,",
      "    held_allocation: held_allocation,",
      "    held_domain: held_domain",
      "};",
      "{",
      "    let DetachedHolder { held_ptr, held_allocation, held_domain } = restored;",
      cleanup(ROOTS, moved_b=True),
      "    unit",
      "}"
    ]
    return "\n".join(buf)

def world(index, permutation=False):
    if index==len(ROOTS): return terminal(permutation)
    x=ROOTS[index]
    return lines("match try_allocate_one<Node>() {",
       "    None => {",
       cleanup(ROOTS[:index]),
       "        unit",
       "    },",
       "    Some(bundle_"+x+") => {",
       initial(x),
       world(index+1,permutation),
       "    },",
       "}")

PREFIX=lines(
"struct Node {",
"    next: Option<ptr<Node>>,",
"    prev: Option<ptr<Node>>,",
"    child: Option<ptr<Node>>,",
"    payload: u8,",
"}",
"struct DetachedHolder {",
"    held_ptr: ptr<Node>,",
"    held_allocation: Allocation,",
"    held_domain: LifetimeDomain,",
"}",
"fn clear_link(field: ref<write, Option<ptr<Node>>>, target: Option<ptr<Node>>) -> unit {",
"    replace(field, target);",
"    unit",
"}",
"fn main() -> unit {")
SUFFIX="}\n"
base=PREFIX+"\n"+world(0)+"\n"+SUFFIX
secondary=PREFIX+"\n"+world(0,True)+"\n"+SUFFIX
cases={"positive_a":base,"positive_permuted_wiring":secondary}

def mutate(case,label,old,new):
    if old not in base:
        raise RuntimeError("source mutation input missing: "+label)
    cases[label]=base.replace(old,new,1)

mutate(cases,"wrong_governing_domain",
       "loan_read(held_domain) { |fresh_b_1|",
       "loan_read(life_A) { |fresh_b_1|")
mutate(cases,"copy_ptr_as_write_ref",
       "clear_link(original_b_1@prev, "+TYPE+"::None)",
       "clear_link(held_ptr, "+TYPE+"::None)")
mutate(cases,"duplicate_consumed_holder",
       "let DetachedHolder { held_ptr, held_allocation, held_domain } = holder;",
       "let DetachedHolder { held_ptr, held_allocation, held_domain } = holder;\nlet stolen = holder;")
mutate(cases,"wrong_original_terminal_domain",
       "destroy(held_ptr, end_B)",
       "destroy(held_ptr, end_A)")
mutate(cases,"repeat_first_scope_ref",
       "let second_result = loan_read(held_domain) { |fresh_b_2|",
       "let escaped = original_b_1@prev;\nlet second_result = loan_read(held_domain) { |fresh_b_2|")
mutate(cases,"third_named_call",
       "let restored = DetachedHolder {",
       "let third_result = loan_read(held_domain) { |fresh_b_3|\n"
       "    let original_b_3 = ref_from_ptr(write, held_ptr, fresh_b_3);\n"
       "    clear_link(original_b_3@child, "+TYPE+"::None)\n"
       "};\nlet restored = DetachedHolder {")
mutate(cases,"swapped_field_order",
       "original_b_1@prev", "original_b_1@next")
mutate(cases,"stale_wrong_b_after_first",
       "original_b_2@next", "original_b_2@prev")
mutate(cases,"duplicate_original_alloc",
       "held_allocation: allocation_B",
       "held_allocation: allocation_A")
mutate(cases,"read_mode_as_write",
       "ref_from_ptr(write, held_ptr, fresh_b_2)",
       "ref_from_ptr(read, held_ptr, fresh_b_2)")
mutate(cases,"no_second_named_call",
       "    clear_link(original_b_2@next, "+TYPE+"::None)",
       "    unit")
for k,v in cases.items():
    p=OUT/(k+".nl")
    p.write_text(v,encoding="utf-8")
    print(k,hashlib.sha256(v.encode()).hexdigest(),len(v.encode()),file=sys.stdout)
