#!/usr/bin/env python3
"""Independent Track R Issue 303 source corpus builder and CLI recorder.

Derived from the selected Draft 17.30 language contract, not P fixtures.
Tests the exact 5-site original allocation tree and a separate scoped
fixed-field helper. The frozen compiler is not modified by this script.
"""
import hashlib
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent
OUT = ROOT / "results"
OUT.mkdir(exist_ok=True)
NAMES = ["oak", "pine", "birch", "cedar", "elm"]

TYPE = """struct Node {
    next: Option<ptr<Node>>,
    prev: Option<ptr<Node>>,
    child: Option<ptr<Node>>,
    payload: u8,
}
"""
NONE = "Option<ptr<Node>>::None"
def some(name):
    return f"Option<ptr<Node>>::Some(ptr_{name})"

def release(name, mode):
    pieces = [
        f"let empty_{name} = loan_exclusive_read(life_{name}) {{ |end_{name}|",
        f"    destroy(ptr_{name}, end_{name})",
        "};",
        f"let full_{name} = erase_slot<Node>(empty_{name});",
        f"finalize_domain(life_{name});",
        f"deallocate(allocation_{name}, full_{name});",
    ]
    if mode == "double-finalize" and name == "pine":
        pieces.append("finalize_domain(life_pine);")
    if mode == "stale-owner" and name == "pine":
        pieces.append("let reused_pine_allocation = allocation_pine;")
    if mode == "missing-free" and name == "pine":
        pieces = [p for p in pieces if not p.startswith("deallocate(allocation_pine")]
    return "\n".join(pieces)

def reloan(name, action, domain=None, mode="write"):
    domain = domain or name
    return f"""loan_read(life_{domain}) {{ |stable_{domain}_probe|
    let view_{name}_{domain} = ref_from_ptr({mode}, ptr_{name}, stable_{domain}_probe);
    {action.replace('VIEW', f'view_{name}_{domain}')}
}}"""

def success(mode):
    actions = []
    # The untouched sibling is established first and must remain current.
    actions.append("let old_pine_prev = " +
        reloan("pine", "replace(VIEW@prev, " + some("cedar") + ")") + ";")
    if mode not in ("none-some", "wrong-domain", "ptr-direct", "read-mode", "leaked-ref"):
        before = "cedar" if mode == "some-equal" else "birch"
        actions.append("let old_pine_next = " +
            reloan("pine", f"replace(VIEW@next, {some(before)})") + ";")
    if mode == "wrong-domain":
        actions.append("let wrong_domain = " +
            reloan("pine", f"retarget_edge(VIEW@next, ptr_elm)", domain="oak") + ";")
    elif mode == "ptr-direct":
        actions.append("let incorrect_ptr_formal = retarget_edge(ptr_pine, ptr_elm);")
    elif mode == "read-mode":
        actions.append("let incorrect_read_formal = " +
            reloan("pine", "retarget_edge(VIEW@next, ptr_elm)", mode="read") + ";")
    elif mode == "leaked-ref":
        actions.append("let escaped_edge = " + reloan("pine", "VIEW@next") + ";")
        actions.append("let should_not_use_escaped = retarget_edge(escaped_edge, ptr_elm);")
    elif mode == "some-none":
        actions.append("let clear_result = " +
            reloan("pine", "clear_edge(VIEW@next)") + ";")
    elif mode == "alias-nonexclusive":
        actions.append("""let alias_result = loan_read(life_pine) { |still_pine|
            let first = ref_from_ptr(write, ptr_pine, still_pine);
            let second = ref_from_ptr(write, ptr_pine, still_pine);
            let result = retarget_edge(first@next, ptr_elm);
            let observed = read(second@next);
            unit
        };""")
    else:
        actions.append("let helper_result = " +
            reloan("pine", "retarget_edge(VIEW@next, ptr_elm)") + ";")
    # Distinct fields checked as source-visible current reads; no pointer-based owner mint.
    if mode not in ("ptr-direct", "read-mode", "wrong-domain", "leaked-ref"):
        actions.append("let sibling_still_current = " +
            reloan("pine", "read(VIEW@prev)",mode="read") + ";")
        actions.append("let updated_next_current = " +
            reloan("pine", "read(VIEW@next)",mode="read") + ";")
    # A copied ptr exists only as provenance; never as A/D/root authority.
    actions.append("let saved_nonowning_ptr = ptr_pine;")
    for n in reversed(NAMES):
        actions.append(release(n, mode))
    actions.append("unit")
    return "\n".join(actions)

def make_tree(index, mode):
    name = NAMES[index]
    init = f"""let OneBacking {{ allocation, raw }} = newly_{name};
let allocation_{name} = allocation;
let vacancy_{name} = into_slot<Node>(raw);
let life_{name} = lifetime_domain();
let ptr_{name} = loan_read(life_{name}) {{ |stable_new_{name}|
    initialize(vacancy_{name}, Node {{
        next: {NONE},
        prev: {NONE},
        child: {NONE},
        payload: u8({index + 11})
    }}, stable_new_{name})
}};"""
    prior = "\n".join(release(n, "baseline") for n in reversed(NAMES[:index]))
    fail = (prior + "\nunit") if prior else "unit"
    succ = success(mode) if index == len(NAMES)-1 else make_tree(index+1,mode)
    return f"""match try_allocate_one<Node>() {{
  None => {{
    {fail}
  }},
  Some(newly_{name}) => {{
    {init}
    {succ}
  }},
}}"""

def generate(mode):
    helper = """fn retarget_edge(slot: ref<write,Option<ptr<Node>>>, successor: ptr<Node>) -> unit {
    let previous_link = replace(slot, Option<ptr<Node>>::Some(successor));
    unit
}
"""
    if mode == "some-none":
        helper = """fn clear_edge(slot: ref<write,Option<ptr<Node>>>) -> unit {
    let previous_link = replace(slot, Option<ptr<Node>>::None);
    unit
}
"""
    return TYPE + "\n" + helper + "\nfn main() -> unit {\n" + make_tree(0,mode) + "\n}\n"

MODES = [
    "some-different", "some-equal", "none-some", "some-none",
    "alias-nonexclusive", "wrong-domain", "ptr-direct", "read-mode",
    "leaked-ref", "double-finalize", "stale-owner", "missing-free",
]
def main():
    binary = pathlib.Path(sys.argv[1]).resolve()
    version = subprocess.run([str(binary),"--version"],capture_output=True,text=True)
    (OUT/"compiler_version.txt").write_text(version.stdout+version.stderr)
    records = []
    for mode in MODES:
        src = generate(mode)
        path = OUT / f"{mode}.nl"
        path.write_text(src,encoding="utf-8")
        sha = hashlib.sha256(src.encode("utf-8")).hexdigest()
        proc = subprocess.run([str(binary), str(path)], capture_output=True, text=True,timeout=60)
        (OUT/f"{mode}.stdout.c").write_text(proc.stdout)
        (OUT/f"{mode}.stderr.txt").write_text(proc.stderr)
        diagnostic = proc.stderr.strip()
        parsed = "UNKNOWN"
        if "parser" in diagnostic.lower() or "syntax" in diagnostic.lower():
            parsed = "PARSER-REJECT"
        elif "V1-BACKEND-UNSUPPORTED" in diagnostic:
            parsed = "SOURCE-ADMITTED-BACKEND-UNSUPPORTED"
        elif proc.returncode == 0:
            parsed = "SOURCE-ADMITTED-C-EMITTED" if proc.stdout.strip() else "SOURCE-ADMITTED-EMPTY"
        elif "SEMANTIC" in diagnostic.upper() or "checker" in diagnostic.lower():
            parsed = "POSSIBLE-SEMANTIC-REJECT-NEEDS-REVIEW"
        record = dict(case=mode,source_sha256=sha,exit_code=proc.returncode,
           parser_registration_gate=parsed,first_diagnostic=diagnostic.splitlines()[:5],
           stderr_file=f"{mode}.stderr.txt",c_bytes=len(proc.stdout),
           c_output_present=bool(proc.stdout.strip()))
        print(json.dumps(record,ensure_ascii=False),flush=True)
        records.append(record)
    (OUT/"matrix.json").write_text(json.dumps(records,indent=2,ensure_ascii=False))
if __name__=="__main__":
    main()
