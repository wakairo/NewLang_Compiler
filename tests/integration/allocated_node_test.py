"""Actual-source pre-backend gate: both paths checked, no native artifacts."""
import pathlib
import subprocess
import sys
import tempfile

compiler, fixture = sys.argv[1:]
source = pathlib.Path(fixture).read_text()
cases = [
    ("canonical", source, 0, ""),
    ("renamed", source.replace("Node", "Cell").replace("next", "link").replace("payload", "datum"), 0, ""),
    ("some-wildcard", source.replace("Some(bundle)", "Some(_)"), 3, "ALLOCATED-PATTERN"),
    ("missing-deallocate", source.replace("deallocate(allocation, full_raw);", ""), 3, "P5-SCOPE-OBLIGATION"),
    ("missing-finalize", source.replace("finalize_domain(life);", ""), 3, "P5-SCOPE-OBLIGATION"),
    ("double-deallocate", source.replace("deallocate(allocation, full_raw);", "deallocate(allocation, full_raw); deallocate(allocation, full_raw);"), 3, "P3-USE-AFTER-CONSUME"),
    ("drop-bundle", source[:source.index("    Some(bundle)")] + "Some(bundle) => {unit},\n}\n}\n", 3, "P5-SCOPE-OBLIGATION"),
    ("missing-slot-cleanup", source.replace("let full_raw = erase_slot<Node>(empty_again);", "").replace("deallocate(allocation, full_raw);", ""), 3, "P5-SCOPE-OBLIGATION"),
    ("ordinary-ending", source.replace("loan_exclusive_read(life)", "loan_read(life)"), 3, "P3-TYPE-MISMATCH"),
    ("wrong-domain", source.replace("let head = Node", "let other = lifetime_domain();\n        let head = Node").replace("ref_from_ptr(read, q, stable)", "ref_from_ptr(read, q, stable)").replace("Some(q) => {\n                loan_read(life)", "Some(q) => {\n                loan_read(other)"), 3, "ALLOCATED-DOMAIN-MISMATCH"),
    ("stale-after-destroy", source.replace("let full_raw = erase_slot<Node>(empty_again);", "loan_read(life){|stable| let access=ref_from_ptr(read,tail,stable); unit};\nlet full_raw = erase_slot<Node>(empty_again);"), 3, "P3-STALE-POINTER"),
    ("stale-after-deallocate", source.replace("finalize_domain(life);", "").replace("deallocate(allocation, full_raw);", "deallocate(allocation, full_raw); loan_read(life){|stable|let access=ref_from_ptr(read,tail,stable);unit}; finalize_domain(life);"), 3, "P3-STALE-POINTER"),
    ("conflicting-domain-loan", source.replace("let empty_again = loan_exclusive_read(life)", "let empty_again = loan_read(life){|stable| loan_exclusive_read(life)").replace("destroy(tail, ending)\n        };", "destroy(tail, ending)\n        }};"), 3, "P3-REF-CONFLICT"),
    ("wrong-ending-domain", source.replace("let empty_again = loan_exclusive_read(life)", "let other=lifetime_domain(); let empty_again = loan_exclusive_read(other)"), 3, "P3-DOMAIN-MISMATCH"),
    ("stale-new-incarnation", source.replace("let full_raw = erase_slot<Node>(empty_again);", "let renewed=loan_read(life){|stable|initialize(empty_again,Node{next:Option<ptr<Node>>::None,payload:u8(3)},stable)}; loan_read(life){|stable|let access=ref_from_ptr(read,tail,stable);unit}; let empty_again_again=loan_exclusive_read(life){|ending|destroy(renewed,ending)}; let full_raw=erase_slot<Node>(empty_again_again);"), 3, "P3-STALE-POINTER"),
    ("none-is-checked", source.replace("    None => {\n        unit", "    None => {\n        unknown_callee(); unit"), 3, "P3-UNKNOWN-CALLEE"),
    ("wrong-ptr-type", source.replace("ref_from_ptr(read, q, stable)", "ref_from_ptr(read, head, stable)"), 3, "ALLOCATED-REF-TYPE"),
    ("wrong-stability-type", source.replace("ref_from_ptr(read, q, stable)", "ref_from_ptr(read, q, head)"), 3, "ALLOCATED-REF-TYPE"),
    ("scoped-ref-escape", source.replace("let access = ref_from_ptr(read, q, stable);\n                    unit", "ref_from_ptr(read,q,stable)"), 3, "P3-INTERNAL"),
    ("extra-domain-backend", source.replace("let life = lifetime_domain();", "let extra=lifetime_domain(); finalize_domain(extra); let life=lifetime_domain();"), 4, "V1-BACKEND-UNSUPPORTED"),
    ("retained-link-backend", source.replace("replace(w, Option<ptr<Node>>::None)", "replace(w, Option<ptr<Node>>::Some(tail))"), 4, "V1-BACKEND-UNSUPPORTED"),
    ("missing-none", source.replace("    None => {\n        unit\n    },", ""), 3, "ALLOCATED-EXHAUSTIVENESS"),
]
with tempfile.TemporaryDirectory(prefix="allocated-source-") as directory:
    root = pathlib.Path(directory)
    for name, text, status, diagnostic in cases:
        path = root / f"{name}.nl"
        path.write_text(text)
        result = subprocess.run([compiler, str(path)], capture_output=True, text=True)
        assert result.returncode == status, (name, result.returncode, result.stderr)
        assert diagnostic in result.stderr, (name, result.stderr)
        if status == 0:
            assert "malloc(24)" in result.stdout and "free(" in result.stdout
        else:
            assert not result.stdout, (name, "partial C output")
        assert list(root.glob("*.c")) == [] and list(root.glob("*.o")) == []
        assert list(root.glob("*.exe")) == []
print(f"{len(cases)} allocated source admission/output controls passed; native execution tested separately")
