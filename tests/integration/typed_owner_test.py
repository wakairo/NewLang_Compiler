"""Draft17.27 applicability controls; accepted source may now lower to C."""
import pathlib
import subprocess
import sys
import tempfile

compiler, fixture, evidence = sys.argv[1:]
source = pathlib.Path(fixture).read_text()
call = "receive_and_release_tail(ptr_t, allocation_t, life_t);"
read = """    loan_read(tail_life) { |stable_t|
        let root_r = ref_from_ptr(read, tail_ptr, stable_t);
        unit
    };
"""
helper, main = source.split("fn main()", 1)
struct, receiver = helper.split("fn receive_and_release_tail", 1)
positives = {
    "primary": source,
    "renamed": source.replace("Node", "Cell").replace("next", "link").replace("payload", "data").replace("receive_and_release_tail", "release_cell"),
    "forward-definition": struct + "fn main()" + main + "\nfn receive_and_release_tail" + receiver,
    "optional-read-absent": source.replace(read, ""),
    "donor-aliases": source.replace(call, "let p=ptr_t;let a=allocation_t;let d=life_t;receive_and_release_tail(p,a,d);"),
    "parameter-aliases": helper.replace("    loan_read", "    let p=tail_ptr;let a=tail_allocation;let d=tail_life;\n    loan_read", 1).replace("loan_read(tail_life)", "loan_read(d)").replace("loan_exclusive_read(tail_life)", "loan_exclusive_read(d)").replace("read, tail_ptr,", "read, p,").replace("destroy(tail_ptr,", "destroy(p,").replace("finalize_domain(tail_life)", "finalize_domain(d)").replace("deallocate(tail_allocation,", "deallocate(a,") + "fn main()" + main,
}
none_start = source.index("                None =>")
some_start = source.index("                Some(tail_bundle)", none_start)
inner_end = source.index("            }\n        },", some_start)
positives["reverse-inner-arms"] = source[:none_start] + source[some_start:inner_end] + source[none_start:some_start] + source[inner_end:]
positives["renamed-parameter-binders"] = source.replace("tail_ptr", "p_arg").replace("tail_allocation", "a_arg").replace("tail_life", "d_arg")
negatives = {}
for name, args, code in [
    ("wrong-root", "ptr_h, allocation_t, life_t", "P193-CALL-DOMAIN"),
    ("wrong-region", "ptr_t, allocation_h, life_t", "P193-CALL-BACKING"),
    ("wrong-domain", "ptr_t, allocation_t, life_h", "P193-CALL-DOMAIN"),
]:
    bad = f"receive_and_release_tail({args});"
    negatives[name] = (source.replace(call, bad), code)
    negatives[name + "-after-good-branch"] = (source.replace(call, "match seen {Some(q)=>{" + call + "unit},None=>{" + bad + "unit}};"), code)
for name, replacement, code in [
    ("donor-allocation-reuse", call + "allocation_t;", "P3-USE-AFTER-CONSUME"),
    ("donor-domain-reuse", call + "life_t;", "P3-USE-AFTER-CONSUME"),
    ("repeated-call", call + call, "P3-USE-AFTER-CONSUME"),
    ("stale-after-call", call + "loan_read(life_h){|s|let r=ref_from_ptr(read,ptr_t,s);unit};", "P3-STALE-POINTER"),
    ("active-domain-loan", "loan_read(life_t){|s|" + call + "unit};", "P3-REF-CONFLICT"),
    ("active-root-loan", "loan_read(life_t){|s|let r=ref_from_ptr(read,ptr_t,s);" + call + "unit};", "P3-REF-CONFLICT"),
]:
    negatives[name] = (source.replace(call, replacement), code)
for name, old, new, code in [
    ("definition-leak", "deallocate(tail_allocation, full_t);", "", "P193-DEFINITION-OBLIGATION"),
    ("definition-missing-finalize", "finalize_domain(tail_life);", "", "P193-DEFINITION-RELATION"),
    ("definition-wrong-domain-role", "destroy(tail_ptr, ending_t)", "destroy(tail_ptr, tail_life)", "P193-DEFINITION-ORDER"),
    ("definition-ordinary-ending", "loan_exclusive_read(tail_life)", "loan_read(tail_life)", "P193-DEFINITION-ORDER"),
    ("definition-wrong-slot-type", "erase_slot<Node>(empty_t)", "erase_slot<u8>(empty_t)", "P193-DEFINITION-TYPE"),
    ("definition-double-release", "deallocate(tail_allocation, full_t);", "deallocate(tail_allocation, full_t);deallocate(tail_allocation, full_t);", "P193-DEFINITION-RELATION"),
    ("definition-early-return", "    loan_read(tail_life)", "    return unit;\n    loan_read(tail_life)", "P193-DEFINITION-OBLIGATION"),
    ("definition-unknown-name", "destroy(tail_ptr, ending_t)", "destroy(missing, ending_t)", "P193-DEFINITION-NAME"),
    ("definition-ref-escape", "        unit\n    };", "        root_r\n    };", "P193-DEFINITION-ESCAPE"),
]:
    assert old in helper
    negatives[name] = (helper.replace(old, new) + "fn main()" + main, code)
# These are input-defined cases. Unexpected failure never reclassifies overlap.
with tempfile.TemporaryDirectory() as directory:
    root = pathlib.Path(directory)
    uncalled = root / "uncalled.nl"
    uncalled.write_text(helper + "fn main() -> unit {unit}\n")
    subprocess.run([evidence, "definition", str(uncalled)], check=True)
    for name, text in positives.items():
        path = root / (name + ".nl")
        path.write_text(text)
        runs = [subprocess.run([compiler, str(path)], capture_output=True) for _ in range(2)]
        for run in runs:
            assert run.returncode == 0 and not run.stderr, (name, run.returncode, run.stderr)
            assert b"static void nl_owner_" in run.stdout, (name, "missing receiver C")
        assert runs[0].stdout == runs[1].stdout
        assert runs[0].stderr == runs[1].stderr
        subprocess.run([evidence, "evidence", str(path)], check=True)
    for name, (text, code) in negatives.items():
        path = root / (name + ".nl")
        path.write_text(text)
        runs = [subprocess.run([compiler, str(path)], capture_output=True) for _ in range(2)]
        for run in runs:
            assert run.returncode == 3 and code.encode() in run.stderr, (name, code, run.returncode, run.stderr)
            assert not run.stdout, (name, "unexpected C")
        assert runs[0].stderr == runs[1].stderr
        # The malformed definition must also fail with no favorable caller.
        if name.startswith("definition-"):
            uncalled.write_text(text.split("fn main()", 1)[0] + "fn main() -> unit {unit}\n")
            run = subprocess.run([compiler, str(uncalled)], capture_output=True)
            assert run.returncode == 3 and code.encode() in run.stderr and not run.stdout, (name, "uncalled", run.stderr)
    assert all(path.suffix == ".nl" for path in root.iterdir()), "no generated C or executable"
print(f"{len(positives)} actual-source positives, {len(negatives)} destructive controls; semantic safety guards retained")
