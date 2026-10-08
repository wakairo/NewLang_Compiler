"""Actual-source nested cleanup admission and asymmetric join rejection.

All cases have explicit input-based expectations. A supported semantic path
must still stop BEFORE C/native output; this is not a two-Node native gate.
"""
import pathlib
import subprocess
import sys
import tempfile

compiler, fixture = sys.argv[1:]
source = pathlib.Path(fixture).read_text()
# Turn the inner match into a statement, followed by a real caller continuation.
continuation = source.replace("            }\n        },", "            };\n            unit\n        },")
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
cases = [
    ("canonical", source, 4, "V1-BACKEND-UNSUPPORTED"),
    ("reversed-inner-arms", reversed_arms, 4, "V1-BACKEND-UNSUPPORTED"),
    ("continuation", continuation, 4, "V1-BACKEND-UNSUPPORTED"),
    ("renamed", source.replace("Node", "Cell").replace("next", "link").replace("payload", "datum"), 4, "V1-BACKEND-UNSUPPORTED"),
    ("values-alias", source.replace("u8(1)", "u8(11)").replace("u8(2)", "u8(37)").replace("let allocation_h = allocation;", "let owned = allocation; let allocation_h = owned;"), 4, "V1-BACKEND-UNSUPPORTED"),
    ("none-leaks-head", source.replace(head_cleanup, "", 1), 3, "ALLOCATED-CAPTURED-JOIN-PRECISION"),
    ("some-leaks-head", source.rsplit(head_cleanup, 1)[0] + source.rsplit(head_cleanup, 1)[1], 3, "ALLOCATED-CAPTURED-JOIN-PRECISION"),
    ("none-release-omitted", source.replace("deallocate(allocation_h, full_h);", "", 1), 3, "P5-SCOPE-OBLIGATION"),
    ("some-release-omitted", source.replace("deallocate(allocation_t, full_t);", ""), 3, "P5-SCOPE-OBLIGATION"),
    ("none-double-release", source.replace("deallocate(allocation_h, full_h);", "deallocate(allocation_h,full_h); deallocate(allocation_h,full_h);", 1), 3, "P3-USE-AFTER-CONSUME"),
    ("some-double-release", source.replace("deallocate(allocation_t, full_t);", "deallocate(allocation_t,full_t); deallocate(allocation_t,full_t);"), 3, "P3-USE-AFTER-CONSUME"),
    ("cross-region", source.replace("deallocate(allocation_t, full_t);", "deallocate(allocation_h,full_t);"), 3, "P4-ALLOCATION-MISMATCH"),
    ("wrong-domain", source.replace("destroy(ptr_t, ending_t)", "destroy(ptr_h, ending_t)"), 3, "P3-DOMAIN-MISMATCH"),
    ("none-domain-live", source.replace("finalize_domain(life_h);", "", 1), 3, "ALLOCATED-CAPTURED-JOIN-PRECISION"),
    ("some-domain-live", source.replace("finalize_domain(life_t);", ""), 3, "P5-SCOPE-OBLIGATION"),
    ("joined-double-destroy", continuation.replace("            unit\n        },", "            loan_exclusive_read(life_h){|ending|destroy(ptr_h,ending)}; unit\n        },"), 3, "P3-USE-AFTER-CONSUME"),
    ("joined-owner-use", continuation.replace("            unit\n        },", "            allocation_h; unit\n        },"), 3, "P3-USE-AFTER-CONSUME"),
    ("joined-stale-ptr", continuation.replace("            unit\n        },", "            let other=lifetime_domain(); loan_read(other){|stable|let r=ref_from_ptr(read,ptr_h,stable);unit}; finalize_domain(other); unit\n        },"), 3, "P3-STALE-POINTER"),
    ("tail-claim-escape", continuation.replace("            unit\n        },", "            allocation_t; unit\n        },"), 3, "P3-UNKNOWN-BINDING"),
    ("some-owner-use-after", source.replace("deallocate(allocation_t, full_t);", "deallocate(allocation_t,full_t); allocation_t;"), 3, "P3-USE-AFTER-CONSUME"),
    ("escaped-tail-ref", source.replace("                    let empty_t =", "                    let escaped=loan_read(life_t){|s|ref_from_ptr(read,ptr_t,s)}; let empty_t ="), 3, "P3-INTERNAL"),
    ("extra-captured-domain", source.replace("            match try_allocate_one<Node>()", "            let extra=lifetime_domain(); match try_allocate_one<Node>()", 1), 3, "ALLOCATED-CAPTURE-PRECISION"),
    ("none-nested-trial", source.replace("None => { unit }", "None => {match try_allocate_one<Node>(){None=>{unit},Some(unexpected)=>{unit}}}", 1), 3, "ALLOCATED-CARDINALITY-PROFILE"),
    ("third-site", source.replace("                    let empty_t =", "                    match try_allocate_one<Node>(){None=>{unit},Some(third)=>{unit}}; let empty_t ="), 3, "ALLOCATED-CARDINALITY-PROFILE"),
    ("missing-inner-none", source.replace("                None => {\n" + head_cleanup + "                    unit\n                },\n", "", 1), 3, "ALLOCATED-EXHAUSTIVENESS"),
    ("inner-wildcard", source.replace("Some(tail_bundle)", "Some(_)"), 3, "ALLOCATED-PATTERN"),
    ("unit-required", source.replace("deallocate(allocation_h, full_h);\n                    unit", "deallocate(allocation_h,full_h); u8(7)", 1), 3, "ALLOCATED-CAPTURED-JOIN-PRECISION"),
    ("captured-copy-observation", source.replace("            match try_allocate_one<Node>()", "            let captured = u8(7); match try_allocate_one<Node>()", 1).replace("deallocate(allocation_h, full_h);\n                    unit", "deallocate(allocation_h,full_h); captured; unit", 1), 4, "V1-BACKEND-UNSUPPORTED"),
]
with tempfile.TemporaryDirectory(prefix="allocated-join-") as directory:
    root = pathlib.Path(directory)
    for name, text, status, diagnostic in cases:
        path = root / (name + ".nl")
        path.write_text(text)
        result = subprocess.run([compiler, str(path)], capture_output=True, text=True)
        assert result.returncode == status, (name, result.returncode, result.stderr)
        assert diagnostic in result.stderr, (name, result.stderr)
        retry = subprocess.run([compiler, str(path)], capture_output=True, text=True)
        assert (retry.returncode, retry.stdout, retry.stderr) == (result.returncode, result.stdout, result.stderr), name
        assert not result.stdout, (name, "partial C")
        assert not list(root.glob("*.c")) and not list(root.glob("*.o"))
        assert not list(root.glob("*.exe"))
print(f"{len(cases)} actual-source nested allocation controls passed")
