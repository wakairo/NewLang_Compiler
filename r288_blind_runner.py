#!/usr/bin/env python3
# Independent R288 adversarial SOURCE corpus; NOT imported P fixtures.
import json
import pathlib
import subprocess
import hashlib
import os
import sys

ROOT = pathlib.Path("r288-frozen-evidence")
ROOT.mkdir(exist_ok=True)
BASE = "struct H {\n    next: Option<ptr<H>>,\n    prev: Option<ptr<H>>,\n    child: Option<ptr<H>>,\n    payload: u8,\n}\nstruct Capsule {\n    handle: ptr<H>,\n    token: Allocation,\n    guard: LifetimeDomain,\n}\nfn pass_through(c: Capsule) -> Capsule {\n    c\n}\nfn main() -> unit {\n    match try_allocate_one<H>() {\n        None => { unit },\n        Some(backing) => {\n            let OneBacking { allocation, raw } = backing;\n            let vacant = into_slot<H>(raw);\n            let domain = lifetime_domain();\n            let handle = loan_read(domain) { |stable|\n                initialize(vacant, H {\n                    next: Option<ptr<H>>::None,\n                    prev: Option<ptr<H>>::None,\n                    child: Option<ptr<H>>::None,\n                    payload: u8(37),\n                }, stable)\n            };\n            let capsule = Capsule { handle: handle, token: allocation, guard: domain };\n            let received = pass_through(capsule);\n            let Capsule { handle, token, guard } = received;\n            let cleared = loan_exclusive_read(guard) { |ending|\n                destroy(handle, ending)\n            };\n            let full = erase_slot<H>(cleared);\n            finalize_domain(guard);\n            deallocate(token, full);\n            unit\n        },\n    }\n}\n"
cases = {}
def add(name, source):
    cases[name] = source
add("c00_basic_v1_control", """fn main() -> unit {
 let n = u8(2);
 n;
 unit
}
""")
add("p01_independent_single_owner_identity", BASE)
add("n02_reconsume_caller_capsule", BASE.replace(
    "let received = pass_through(capsule);",
    "let received = pass_through(capsule);\n            let duplicate = pass_through(capsule);"
))
add("n03_double_transfer_domain", BASE.replace(
    "let capsule = Capsule { handle: handle, token: allocation, guard: domain };",
    "let stolen = domain;\n            let capsule = Capsule { handle: handle, token: allocation, guard: domain };"
))
add("n04_double_transfer_allocation", BASE.replace(
    "let capsule = Capsule { handle: handle, token: allocation, guard: domain };",
    "let stolen = allocation;\n            let capsule = Capsule { handle: handle, token: allocation, guard: domain };"
))
add("n05_incomplete_pattern_omits_guard", BASE.replace(
    "let Capsule { handle, token, guard } = received;",
    "let Capsule { handle, token } = received;"
))
add("n06_incomplete_constructor_omits_guard", BASE.replace(
    "Capsule { handle: handle, token: allocation, guard: domain }",
    "Capsule { handle: handle, token: allocation }"
))
add("n07_duplicate_constructor_token", BASE.replace(
    "Capsule { handle: handle, token: allocation, guard: domain }",
    "Capsule { handle: handle, token: allocation, token: allocation, guard: domain }"
))
add("n08_drop_returned_capsule", BASE.replace(
    "let Capsule { handle, token, guard } = received;\n            let cleared",
    "let cleared"
))
add("n09_double_finalize_domain", BASE.replace(
    "finalize_domain(guard);", "finalize_domain(guard);\n            finalize_domain(guard);"
))
add("n10_double_deallocate", BASE.replace(
    "deallocate(token, full);", "deallocate(token, full);\n            deallocate(token, full);"
))
add("n11_active_loan_move", BASE.replace(
    "let received = pass_through(capsule);",
    """let received = loan_read(domain) { |still_live|
                pass_through(capsule)
            };"""
))
add("n12_fake_ptr_authority", BASE.replace(
    "let capsule = Capsule { handle: handle, token: allocation, guard: domain };",
    "let capsule = Capsule { handle: handle, token: handle, guard: handle };"
))
add("n13_callee_double_use", BASE.replace(
    "fn pass_through(c: Capsule) -> Capsule {\n    c\n}",
    "fn pass_through(c: Capsule) -> Capsule {\n    let first = c;\n    c\n}"
))
add("n14_callee_drop_parameter", BASE.replace(
    "fn pass_through(c: Capsule) -> Capsule {\n    c\n}",
    "fn pass_through(c: Capsule) -> unit {\n    unit\n}"
).replace("let received = pass_through(capsule);","pass_through(capsule);").replace("let Capsule { handle, token, guard } = received;",""))
add("n15_postmove_use", BASE.replace(
    "let received = pass_through(capsule);",
    "let received = pass_through(capsule);\n            capsule;"
))
# Nominal nesting exercises complete, ordinary wrapping and unpacking.
nest = BASE.replace("fn pass_through", """struct Outer {
    inner: Capsule,
    tag: u8,
}
fn pass_through""").replace("fn pass_through(c: Capsule) -> Capsule", "fn pass_through(c: Outer) -> Outer").replace(
    "let received = pass_through(capsule);\n            let Capsule",
    "let wrapper = Outer { inner: capsule, tag: u8(19) };\n            let received = pass_through(wrapper);\n            let Outer { inner, tag } = received;\n            let Capsule"
).replace("= received;\n            let cleared","= inner;\n            tag;\n            let cleared")
add("p16_independent_nested_owner_identity", nest)
add("n17_nested_constructor_duplicates", nest.replace(
    "let wrapper = Outer { inner: capsule, tag: u8(19) };",
    "let wrapper = Outer { inner: capsule, inner: capsule, tag: u8(19) };"
))
# Emit every source and run frozen compiler directly, report exact statuses.
binary = os.environ.get("NEWLANG_BIN", "build-r288/newlangc")
output = []
for name, source in cases.items():
    path = ROOT / (name+".nl")
    path.write_text(source, encoding="utf-8")
    proc = subprocess.run([binary, str(path)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    cpath = ROOT / (name+".c")
    if proc.stdout: cpath.write_bytes(proc.stdout)
    stderr = proc.stderr.decode(errors="replace")
    stdout = proc.stdout.decode(errors="replace")
    record = dict(case=name, sha256=hashlib.sha256(source.encode()).hexdigest(), exit=proc.returncode,
                  emitted_c=bool(proc.stdout and (b"#include" in proc.stdout or b"int main" in proc.stdout or b"void main" in proc.stdout)),
                  stdout_bytes=len(proc.stdout), diagnostic=stderr[-3000:], stdout_preview=stdout[:600])
    output.append(record)
    print("R288_RESULT "+json.dumps(record, ensure_ascii=False), flush=True)
(ROOT/"results.json").write_text(json.dumps(output,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
print("R288_CASE_COUNT",len(cases),flush=True)
