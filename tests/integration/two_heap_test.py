"""Full Draft17.26 source controls; no native two-heap execution claim."""
import pathlib
import subprocess
import sys
import tempfile

compiler, fixture, evidence = sys.argv[1:]
source = pathlib.Path(fixture).read_text()
draft = (pathlib.Path(fixture).parents[2] / "docs/reference/NewLang_v0_spec_Draft17_26.md").read_text()
verbatim = draft.split("#### Complete three-path admitted-source candidate", 1)[1].split("```newlang\n", 1)[1].split("```", 1)[0]
assert source == verbatim, "primary fixture must be verbatim Draft17.26 source"
head_cleanup = """                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<Node>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
"""
assert source.count(head_cleanup) == 2
none_start = source.index("                None =>")
some_start = source.index("                Some(tail_bundle)", none_start)
inner_end = source.index("            }\n        },", some_start)
reversed_arms = source[:none_start] + source[some_start:inner_end] + source[none_start:some_start] + source[inner_end:]
continuation = source.replace("            }\n        },", "            };\n            unit\n        },")
option_start = source.index("                    match observed {")
option_end = source.index("                    let old_some =", option_start)
option_match = source[option_start:option_end]
stale_q = source[:option_start] + source[option_end:]
stale_q = stale_q.replace("                    let full_t =", option_match + "                    let full_t =")
body = source.split("fn main() -> unit {", 1)[1].rsplit("}", 1)[0]
repeated = source.split("fn main() -> unit {", 1)[0] + "fn main() -> unit {" + body + ";" + body + "}"
looped = source.split("fn main() -> unit {", 1)[0] + "fn main() -> unit {loop () {" + body + ";break unit;}}"
# profile-valid positives have explicit input classification, never an
# outcome-based fallback after unexpected checker/native failure.
cases = [
    ("canonical", source, 0, ""),
    ("renamed", source.replace("Node", "Cell").replace("next", "link").replace("payload", "datum"), 0, ""),
    ("values", source.replace("u8(1)", "u8(11)").replace("u8(2)", "u8(37)"), 0, ""),
    ("binding-alias", source.replace("let allocation_h = allocation;", "let owned=allocation; let allocation_h=owned;").replace("head_w@next", "writer@next").replace("replace(writer@next", "let writer=head_w; replace(writer@next"), 0, ""),
    ("selected-operands", source.replace("let head_w = ref_from_ptr(write, ptr_h, stable_h);", "let p=ptr_h; let s=stable_h; let head_w=ref_from_ptr(write,p,s);"), 0, ""),
    ("reverse-inner", reversed_arms, 0, ""),
    ("continuation", continuation, 0, ""),
    ("none-leak-head", source.replace(head_cleanup, "", 1), 3, "ALLOCATED-CAPTURED-JOIN-PRECISION"),
    ("some-leak-head", source.rsplit(head_cleanup, 1)[0] + source.rsplit(head_cleanup, 1)[1], 3, "ALLOCATED-CAPTURED-JOIN-PRECISION"),
    ("omit-head-release", source.replace("deallocate(allocation_h, full_h);", "", 1), 3, "P5-SCOPE-OBLIGATION"),
    ("omit-tail-release", source.replace("deallocate(allocation_t, full_t);", ""), 3, "P5-SCOPE-OBLIGATION"),
    ("double-head-release", source.replace("deallocate(allocation_h, full_h);", "deallocate(allocation_h,full_h);deallocate(allocation_h,full_h);", 1), 3, "P3-USE-AFTER-CONSUME"),
    ("double-tail-release", source.replace("deallocate(allocation_t, full_t);", "deallocate(allocation_t,full_t);deallocate(allocation_t,full_t);"), 3, "P3-USE-AFTER-CONSUME"),
    ("cross-release", source.replace("deallocate(allocation_t, full_t);", "deallocate(allocation_h,full_t);"), 3, "P4-ALLOCATION-MISMATCH"),
    ("wrong-tail-stability", source.replace("                            loan_read(life_t)", "                            loan_read(life_h)"), 3, "ALLOCATED-DOMAIN-MISMATCH"),
    ("wrong-head-stability", source.replace("let old_none = loan_read(life_h)", "let old_none = loan_read(life_t)"), 3, "ALLOCATED-DOMAIN-MISMATCH"),
    ("wrong-tail-ending", source.replace("destroy(ptr_t, ending_t)", "destroy(ptr_h,ending_t)"), 3, "P3-DOMAIN-MISMATCH"),
    ("read-no-write", source.replace("let head_w = ref_from_ptr(write,", "let head_w = ref_from_ptr(read,"), 3, "P3-TYPE-MISMATCH"),
    ("ptr-field", source.replace("replace(head_w@next", "replace(ptr_h@next"), 3, "HEAP-LINK-REF-PROFILE"),
    ("payload-field", source.replace("replace(head_w@next", "replace(head_w@payload"), 3, "HEAP-LINK-FIELD-PROFILE"),
    ("active-root-at-end", source.replace("replace(head_w@next,", "let ended=loan_exclusive_read(life_h){|e|destroy(ptr_h,e)}; replace(head_w@next,"), 3, "P3-REF-CONFLICT"),
    ("active-field-at-end", source.replace("read(head_r@next)", "let field=head_r@next;loan_exclusive_read(life_h){|e|destroy(ptr_h,e)};unit"), 3, "P3-REF-CONFLICT"),
    ("tail-ref-escape", source.replace("let tail_r = ref_from_ptr(read, q, stable_t);\n                                unit", "let tail_r=ref_from_ptr(read,q,stable_t);tail_r"), 3, "P3-INTERNAL"),
    ("field-ref-escape", source.replace("read(head_r@next)", "head_r@next"), 3, "P3-INTERNAL"),
    ("stale-tail", source.replace("let full_t = erase_slot<Node>(empty_t);", "loan_read(life_t){|s|let r=ref_from_ptr(read,ptr_t,s);unit};let full_t=erase_slot<Node>(empty_t);"), 3, "P3-STALE-POINTER"),
    ("stale-q", stale_q, 3, "P3-STALE-POINTER"),
    ("joined-owner", continuation.replace("            unit\n        },", "            allocation_h;unit\n        },"), 3, "P3-USE-AFTER-CONSUME"),
    ("joined-stale", continuation.replace("            unit\n        },", "            let other=lifetime_domain();loan_read(other){|s|let r=ref_from_ptr(read,ptr_h,s);unit};finalize_domain(other);unit\n        },"), 3, "P3-STALE-POINTER"),
    ("omit-finalize", source.replace("finalize_domain(life_t);", ""), 3, "P5-SCOPE-OBLIGATION"),
    ("none-cannot-use-tail", source.replace("None => { unit }", "None=>{ptr_t;unit}", 1), 3, "P3-UNKNOWN-BINDING"),
    ("missing-option-none", source.replace("                        None => { unit },\n", ""), 3, "P6-EXHAUSTIVENESS"),
    ("missing-option-some", source.replace(option_match, "                    match observed {None=>{unit}};\n"), 3, "P6-EXHAUSTIVENESS"),
    ("consumed-owner-reuse", source.replace("let allocation_t = allocation;", "let allocation_t=allocation;allocation;"), 3, "P3-USE-AFTER-CONSUME"),
    ("bundle-leftover", source[:some_start] + "                Some(tail_bundle)=>{unit},\n" + source[inner_end:], 3, "P5-SCOPE-OBLIGATION"),
    ("bundle-wildcard", source.replace("Some(tail_bundle)", "Some(_)"), 3, "ALLOCATED-PATTERN"),
    ("looped-allocation", looped, 3, "ALLOCATED-CARDINALITY-PROFILE"),
    ("repeated-outer-sites", repeated, 3, "ALLOCATED-CARDINALITY-PROFILE"),
    ("non-H-target", source.replace("match try_allocate_one<Node>()", "match try_allocate_one<u8>()", 1), 3, "ALLOCATED-TARGET-PROFILE"),
    ("third-root", source.replace("                    let empty_t =", "                    match try_allocate_one<Node>(){None=>{unit},Some(third)=>{unit}};let empty_t ="), 3, "ALLOCATED-CARDINALITY-PROFILE"),
    ("general-read", source.replace("read(head_r@next)", "read(head_r)"), 3, "HEAP-LINK-READ-PROFILE"),
    ("borrowed-option-change", source.replace("replace(head_w2@next, Option<ptr<Node>>::None)", "let field=head_w2@next;match field{None=>{unit},Some(r)=>{replace(head_w2@next,Option<ptr<Node>>::None);unit}};replace(head_w2@next,Option<ptr<Node>>::None)"), 3, "P9-MATCH-PRECISION"),
]
with tempfile.TemporaryDirectory(prefix="two-heap-semantic-") as directory:
    root = pathlib.Path(directory)
    for name, text, status, diagnostic in cases:
        path = root / (name + ".nl")
        path.write_text(text)
        r = subprocess.run([compiler, str(path)], capture_output=True, text=True)
        assert r.returncode == status and (not r.stderr if status == 0 else diagnostic in r.stderr), (name, r.returncode, r.stderr)
        retry = subprocess.run([compiler, str(path)], capture_output=True, text=True)
        assert (r.returncode, r.stdout, r.stderr) == (retry.returncode, retry.stdout, retry.stderr), name
        assert (bool(r.stdout) if status == 0 else not r.stdout)
        assert not list(root.glob("*.c")) and not list(root.glob("*.o"))
        assert not list(root.glob("*.exe"))
        if status == 0:
            inspected = subprocess.run([evidence, str(path)], capture_output=True, text=True)
            assert inspected.returncode == 0, (name, inspected.stderr)
print(f"{len(cases)} explicit full-source two-heap controls passed")
