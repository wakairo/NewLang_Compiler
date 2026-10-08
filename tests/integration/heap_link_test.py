"""Actual-source admission/diagnostics and strict pre-backend output boundary."""
import pathlib
import subprocess
import sys
import tempfile

compiler, fixture = sys.argv[1:]
source = pathlib.Path(fixture).read_text()
cases = [
    ("canonical", source, 0, ""),
    ("renamed", source.replace("Node", "Cell").replace("next", "link").replace("payload", "datum"), 0, ""),
    ("write-read-weakening", source.replace("let root_r = ref_from_ptr(read,", "let root_r = ref_from_ptr(write,"), 4, "V1-BACKEND-UNSUPPORTED"),
    ("scoped-alias", source.replace("replace(root_w@next,", "let link_ref=root_w@next; replace(link_ref,"), 4, "V1-BACKEND-UNSUPPORTED"),
    ("ordinary-write-alias", source.replace("replace(root_w@next,", "let alias=root_w; replace(alias@next,"), 0, ""),
    ("read-no-write", source.replace("let root_w = ref_from_ptr(write,", "let root_w = ref_from_ptr(read,"), 3, "P3-TYPE-MISMATCH"),
    ("ptr-base", source.replace("replace(root_w@next,", "replace(heap_head@next,"), 3, "HEAP-LINK-REF-PROFILE"),
    ("payload", source.replace("replace(root_w@next,", "replace(root_w@payload,"), 3, "HEAP-LINK-FIELD-PROFILE"),
    ("unknown-field", source.replace("read(root_r@next)", "read(root_r@missing)"), 3, "HEAP-LINK-FIELD-PROFILE"),
    ("wrong-root-category", source.replace("read(root_r@next)", "read(lexical_tail@next)"), 3, "HEAP-LINK-REF-PROFILE"),
    ("wrong-ptr-type", source.replace("ref_from_ptr(write, heap_head, stable)", "ref_from_ptr(write, lexical_tail, stable)"), 3, "ALLOCATED-REF-TYPE"),
    ("wrong-stable-type", source.replace("ref_from_ptr(write, heap_head, stable)", "ref_from_ptr(write, heap_head, tail_ptr)"), 3, "ALLOCATED-REF-TYPE"),
    ("lexical-ptr-write", source.replace("ref_from_ptr(write, heap_head, stable)", "ref_from_ptr(write, tail_ptr, stable)"), 3, "ALLOCATED-DOMAIN-MISMATCH"),
    ("wrong-domain", source.replace("let old_none = loan_read(life)", "let other=lifetime_domain(); let old_none=loan_read(other)"), 3, "ALLOCATED-DOMAIN-MISMATCH"),
    ("field-ref-escape", source.replace("read(root_r@next)", "root_r@next"), 3, "P3-INTERNAL"),
    ("root-ref-escape", source.replace("read(root_r@next)", "root_r"), 3, "P3-INTERNAL"),
    ("active-ref-ending", source.replace("read(root_r@next)", "let live=root_r@next; loan_exclusive_read(life){|ending|destroy(heap_head,ending)}; unit"), 3, "P3-REF-CONFLICT"),
    ("borrowed-match-precision", source.replace("replace(root_w2@next, Option<ptr<Node>>::None)", "let field=root_w2@next; match field {None=>{unit},Some(payload_ref)=>{replace(root_w2@next,Option<ptr<Node>>::None);unit}}; replace(root_w2@next,Option<ptr<Node>>::None)"), 3, "P9-MATCH-PRECISION"),
    ("stale-after-end", source.replace("let full_raw =", "loan_read(life){|stable|let root=ref_from_ptr(write,heap_head,stable); read(root@next)}; let full_raw ="), 3, "P3-STALE-POINTER"),
    ("stale-after-free", source.replace("deallocate(allocation, full_raw);", "deallocate(allocation, full_raw); loan_read(life){|stable|let root=ref_from_ptr(write,heap_head,stable); read(root@next)};"), 3, "P3-USE-AFTER-CONSUME"),
    ("omitted-release", source.replace("deallocate(allocation, full_raw);", ""), 3, "P5-SCOPE-OBLIGATION"),
    ("double-release", source.replace("deallocate(allocation, full_raw);", "deallocate(allocation, full_raw); deallocate(allocation,full_raw);"), 3, "P3-USE-AFTER-CONSUME"),
    ("omitted-finalize", source.replace("finalize_domain(life);", ""), 3, "P5-SCOPE-OBLIGATION"),
    ("omitted-slot-reclaim", source.replace("let full_raw = erase_slot<Node>(empty);", ""), 3, "P3-UNKNOWN-BINDING"),
    ("none-checked", source.replace("None => { unit }", "None => { unknown(); unit }", 1), 3, "P3-UNKNOWN-CALLEE"),
    ("some-wildcard", source.replace("Some(bundle)", "Some(_)"), 3, "ALLOCATED-PATTERN"),
    ("missing-none", source.replace("    None => { unit },\n", "", 1), 3, "ALLOCATED-EXHAUSTIVENESS"),
    ("general-read", source.replace("read(root_r@next)", "read(root_r)"), 3, "HEAP-LINK-READ-PROFILE"),
    ("dotted", source.replace("root_r@next", "root_r.next"), 3, "SOURCE-DOT-RESERVED"),
    ("nested", source.replace("root_r@next", "root_r@next@next"), 3, "P5-EXPRESSION-UNSUPPORTED"),
    ("arbitrary-base", source.replace("read(root_r@next)", "read(ref_from_ptr(read,heap_head,stable)@next)"), 3, "P5-EXPRESSION-UNSUPPORTED"),
]
with tempfile.TemporaryDirectory(prefix="heap-link-source-") as directory:
    root = pathlib.Path(directory)
    for name, text, status, diagnostic in cases:
        path = root / (name + ".nl")
        path.write_text(text)
        result = subprocess.run([compiler, str(path)], capture_output=True, text=True)
        assert result.returncode == status, (name, result.returncode, result.stderr)
        if status == 0:
            # Explicit input-based native admission; no outcome-based fallback.
            # Only the separate native oracle claims actual execution.
            assert result.stdout.startswith("/* Bounded Checked-C") and not result.stderr
            continue
        assert diagnostic in result.stderr, (name, result.stderr)
        assert not result.stdout, (name, "partial C output")
        assert list(root.glob("*.c")) == [] and list(root.glob("*.o")) == []
        assert list(root.glob("*.exe")) == []
print(f"{len(cases)} actual-source heap link controls passed; strict static rejection retained")
