#!/usr/bin/env python3
"""R328 independently constructed source mutations of R's calibrated source.
Each case has a single localized edit, retains the R source basis, and records a
unified diff + SHA. No P negative fixture is used."""
import difflib
import hashlib
import json
from pathlib import Path

source_path = Path("r328/independent_calibrated.nl")
source = source_path.read_text()
out = Path("r328/variants")
out.mkdir(exist_ok=True)
def unique(a,b):
    assert source.count(a)==1, (a,source.count(a))
    return source.replace(a,b)
def final(a,b):
    assert a in source
    left,sep,right=source.rpartition(a)
    return left+b+right
cases = [
 ("B_prev_wrong_A_domain",unique("let cleared_previous = loan_read(life_B_m)", "let cleared_previous = loan_read(life_A)")),
 ("B_next_wrong_C_domain",unique("let cleared_next = loan_read(life_B_m)", "let cleared_next = loan_read(life_C)")),
 ("A_next_wrong_B_domain",unique("let swapped_next = loan_read(life_A)", "let swapped_next = loan_read(life_B_m)")),
 ("C_prev_wrong_A_domain",unique("let swapped_prev = loan_read(life_C)", "let swapped_prev = loan_read(life_A)")),
 ("B_prev_read_not_write",unique("ref_from_ptr(write, ptr_B_m, first_stable)","ref_from_ptr(read, ptr_B_m, first_stable)")),
 ("B_next_ptr_not_ref",unique("unlink_field(second_root@next, Option<ptr<Node>>::None)","unlink_field(ptr_B_m, Option<ptr<Node>>::None)")),
 ("B_original_owner_after_move",unique("let ptr_B_m = node;","let ptr_B_m = ptr_B;")),
 ("B_allocation_double_consume",unique("let cleared_previous = loan_read(life_B_m)", "let owned_once = allocation_B_m; let owned_twice = allocation_B_m; let cleared_previous = loan_read(life_B_m)")),
 ("C_terminal_wrong_D",final("let empty_C = loan_exclusive_read(life_C)","let empty_C = loan_exclusive_read(life_A)")),
 ("C_prev_wrong_new_target",unique("replace(direct_C@prev, Option<ptr<Node>>::Some(ptr_A))","replace(direct_C@prev, Option<ptr<Node>>::Some(ptr_B_m))")),
 ("A_next_lost_direct_rewire",unique("replace(direct_A@next, Option<ptr<Node>>::Some(ptr_C))","replace(direct_A@next, Option<ptr<Node>>::Some(ptr_B_m))")),
 ("C_prev_wrong_initial_edge",unique("replace(accessor_5@prev, Option<ptr<Node>>::Some(ptr_B))","replace(accessor_5@prev, Option<ptr<Node>>::Some(ptr_A))")),
 ("active_B_loan_during_move",unique("let carried_B = DetachedHolder { node: ptr_B, grant: allocation_B, dom: life_B };","let carried_B = loan_read(life_B) { |active_B| DetachedHolder { node: ptr_B, grant: allocation_B, dom: life_B } };")),
 ("old_C_prev_copy_after_replace",unique("replace(direct_C@prev, Option<ptr<Node>>::Some(ptr_A))","let old_copy = read(direct_C@prev); let result_c = replace(direct_C@prev, Option<ptr<Node>>::Some(ptr_A)); let later_copy = old_copy; result_c"))
]
assert len({label for label,_ in cases})==14
report=[]
for name,program in cases:
    assert program!=source
    target=out/(name+".nl")
    target.write_text(program)
    diff="".join(difflib.unified_diff(source.splitlines(True),program.splitlines(True),
                                      fromfile=str(source_path),tofile=str(target)))
    sha=hashlib.sha256(program.encode()).hexdigest()
    (out/(name+".diff")).write_text(diff)
    report.append({"name":name,"sha256":sha,"diff":str(out/(name+".diff"))})
    print("R328_VARIANT",name,sha,"diff-lines",len(diff.splitlines()))
(out/"manifest.json").write_text(json.dumps(report,indent=2)+"\n")
