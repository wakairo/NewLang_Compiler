#!/usr/bin/env python3
"""R402 fresh BLIND, source-authored only. No production fixture/test imports.

First-pass diagnostic survey: reports checker/backend/transaction boundaries for
independent two-original bounded Node match programs. All code in this file is
R-authored from canonical Draft17.30 and the public README/header contract.
"""
import hashlib
import pathlib
import subprocess
import sys
import tempfile

DECL = """struct Node { next: Option<ptr<Node>>, payload: u8, }

fn main() -> unit {
    let a = Node { next: Option<ptr<Node>>::None, payload: u8(1) };
    let b = Node { next: Option<ptr<Node>>::None, payload: u8(2) };
    let pa = loan_read(a) { |ra| ptr_from_ref(ra) };
    let pb = loan_read(b) { |rb| ptr_from_ref(rb) };
"""
END = "\n    unit\n}\n"
S = """        Some(selected) => {
            loan_write(a@next) { |wa| replace(wa, Option<ptr<Node>>::Some(pb)) };
            unit
        }"""
N = """        None => {
            loan_write(a@next) { |wa| replace(wa, Option<ptr<Node>>::None) };
            unit
        }"""
S_SAME = """        Some(selected) => {
            loan_write(a@next) { |wa| replace(wa, Option<ptr<Node>>::Some(pa)) };
            unit
        }"""
N_SAME = """        None => {
            loan_write(a@next) { |wa| replace(wa, Option<ptr<Node>>::Some(pa)) };
            unit
        }"""
N_OTHER = """        None => {
            loan_write(b@next) { |wb| replace(wb, Option<ptr<Node>>::Some(pa)) };
            unit
        }"""
AFTER = """
    let copy_after = a@next;
    loan_write(a@next) { |w2| replace(w2, Option<ptr<Node>>::None) };
    let final_copy = a@next;
"""
def case(selector, sa, na, after=AFTER):
    return DECL + "    let selector = " + selector + ";\n    match selector {\n" + sa + ",\n" + na + ",\n    };\n" + after + END

CASES = {
    # Both normal edges; different field current facts, no owner transfer.
    "01_diff_current_some": case("Option<ptr<Node>>::Some(pa)", S, N),
    "02_diff_current_none": case("Option<ptr<Node>>::None", S, N),
    # Numerically identical Copy locator before/after Change is NOT fresh ref authority.
    "03_same_locator": case("Option<ptr<Node>>::Some(pa)", S_SAME, N_SAME),
    # Two different original roots and distinct fixed field writes.
    "04_distinct_originals": case("Option<ptr<Node>>::Some(pb)", S, N_OTHER,
                                   "    let copy_a = a@next;\n    let copy_b = b@next;\n"
                                   + AFTER),
    # Selected Some is known, but invalid None arm MUST still be checked.
    "05_unselected_bad_none": case("Option<ptr<Node>>::Some(pa)", S,
                                   "        None => { illegal_unbound_r402; unit }"),
    "06_unselected_bad_some": case("Option<ptr<Node>>::None",
                                   "        Some(selected) => { illegal_unbound_r402; unit }", N),
    # Old copied Option does not turn into a ptr or ref after a field mutation.
    "07_option_as_ptr_postjoin": case("Option<ptr<Node>>::Some(pa)", S, N,
                               "\n    let old_option = a@next;\n"
                               "    loan_read_ptr(old_option) { |r| unit };\n"),
    # Scoped ref must not escape the loan.
    "08_scoped_ref_escape": DECL +
        "    let escaped = loan_read(a) { |ra| ra };\n" + END,
    # Pointer whose lexical origin has ended may not be safely reloaned.
    "09_stale_lexical_ptr": DECL +
        "    let stale = { let brief = Node { next: Option<ptr<Node>>::None, payload: u8(3) };"
        " loan_read(brief) { |r| ptr_from_ref(r) } };\n"
        "    loan_read_ptr(stale) { |rs| unit };\n" + END,
    # Ref from a different original cannot use the current root as a matching stability witness.
    "10_wrong_stability": DECL +
        "    loan_read(a) { |stable_a| let forged = ref_from_ptr(read, pb, stable_a); unit };\n"
        + END,
}
def summarize(text, limit=180):
    return " ".join(text.strip().split())[:limit]
def run(compiler, source, workdir, name):
    path = workdir / (name + ".nl")
    path.write_text(source, encoding="utf-8")
    p = subprocess.run([str(compiler), str(path)], capture_output=True,
                       text=True, timeout=35)
    st = "emitted-c" if p.returncode == 0 and p.stdout else (
        "accepted-no-c" if p.returncode == 0 else
        "backend-unsupported" if "BACKEND-UNSUPPORTED" in p.stderr else
        "rejected-or-unsupported")
    return p, st

def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: check_r402.py path/to/newlangc")
    compiler = pathlib.Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="r402-blind-") as temp:
        root = pathlib.Path(temp)
        problematic = []
        for name, source in CASES.items():
            p, status = run(compiler, source, root, name)
            sha = hashlib.sha256(source.encode()).hexdigest()[:16]
            print(f"R402 {name} srcSHA256={sha} status={status} rc={p.returncode} "
                  f"outbytes={len(p.stdout.encode())} "
                  f"outSHA256={hashlib.sha256(p.stdout.encode()).hexdigest()[:16]} "
                  f"stderr={summarize(p.stderr)}", flush=True)
            if p.returncode != 0 and p.stdout:
                problematic.append(name + ": failed compilation emitted partial C")
            if name in ("05_unselected_bad_none", "06_unselected_bad_some",
                        "07_option_as_ptr_postjoin", "08_scoped_ref_escape",
                        "09_stale_lexical_ptr", "10_wrong_stability") and p.returncode == 0:
                problematic.append(name + ": unsafe or unknown acceptance needs inspection")
        if problematic:
            print("R402 SUSPECT: " + " | ".join(problematic), flush=True)
    # During first survey make CTest expose all observations in GitHub Actions logs.
    print("R402 SURVEY COMPLETE — first-pass classification pending; intentional failure", flush=True)
    raise SystemExit(1)

if __name__ == "__main__":
    main()
