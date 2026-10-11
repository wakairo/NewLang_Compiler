#!/usr/bin/env python3
"""R314 independent black-box calibration: helper-only source-body alternatives."""
from pathlib import Path
import hashlib
base=Path("r-evidence/314/generated/positive_a.nl").read_text()
start=base.index("fn clear_link(")
end=base.index("fn main()",start)
forms={
 "direct_replace_unit": ["replace(field, target);","unit"],
 "let_and_copy_statement": ["let old = replace(field, target);","old;","unit"],
 "let_without_explicit_copy": ["let old = replace(field, target);","unit"],
 "copy_from_old": ["let old = replace(field, target);","let observed = old;","observed;","unit"],
 "copy_disposal_before_unit": ["let old = replace(field, target);","let observed = old;","unit"],
 "match_old": ["let old = replace(field, target);","match old { None => { unit }, Some(p) => { unit }, };","unit"],
 "return_unit": ["let old = replace(field, target);","old;","return unit;"],
 "replace_then_return": ["replace(field, target);","return unit;"],
 "result_match_direct": ["match replace(field, target) { None => { unit }, Some(p) => { unit }, };","unit"],
 "replaced_twice": ["let old = replace(field, target);","let old2 = replace(field, target);","old;","old2;","unit"],
}
for label,stmts in forms.items():
    definition="fn set_link(field: ref<write, Option<ptr<Node>>>, target: Option<ptr<Node>>) -> unit {\n"
    definition+="\n".join("    "+x for x in stmts)+"\n}\n"
    output=base[:start]+definition+base[end:]
    output=output.replace("clear_link(", "set_link(")
    p=Path("r-evidence/314/generated/helper_"+label+".nl")
    p.write_text(output)
    print(p.name, hashlib.sha256(output.encode()).hexdigest(),len(output))
