"""Draft17.28 semantic regression and explicit bounded backend profiles."""
import pathlib
import subprocess
import sys
import tempfile

compiler, fixture, evidence = sys.argv[1:]
source = pathlib.Path(fixture).read_text()
head, main = source.split("fn main()", 1)
call = "head_w2@next, ptr_t, allocation_t, life_t"
producer, receiver = head.split("fn receive_and_release_tail", 1)
struct, producer_fn = producer.split("fn detach_and_return_tail", 1)
read_start = source.index("                    loan_read(owned_domain)")
read_end = source.index("                    receive_and_release_tail(", read_start)
seen_start = source.index("                    match seen {")
seen_end = source.index("                    let LiveTail {", seen_start)
receive = """                    let LiveTail {
                        owned_ptr,
                        owned_allocation,
                        owned_domain
                    } = """
positives = {
    "primary": source,
    "producer-result-local": source.replace("    return LiveTail {", "    let owner = LiveTail {").replace("    };\n}\n\nfn receive", "    };\n    return owner;\n}\n\nfn receive", 1),
    "renamed": source.replace("Node", "Cell").replace("next", "link").replace("payload", "data").replace("detach_and_return_tail", "detach_cell").replace("receive_and_release_tail", "release_cell"),
    "declaration-order": struct + "fn main()" + main + "\nfn receive_and_release_tail" + receiver + "\nfn detach_and_return_tail" + producer_fn,
    "optional-tail-read-absent": source[:read_start] + source[read_end:],
    "tail-argument-aliases": source.replace(receive, "                    let p=ptr_t;let a=allocation_t;let d=life_t;\n" + receive).replace(call, "head_w2@next, p, a, d"),
    "producer-parameter-aliases": source.replace("    let old_link", "    let pp=p;let aa=a;let dd=d;\n    let old_link", 1).replace("owned_ptr: p,", "owned_ptr: pp,").replace("owned_allocation: a,", "owned_allocation: aa,").replace("owned_domain: d", "owned_domain: dd"),
    "result-placement": source.replace(receive, "                    let owner = ").replace("                    loan_read(owned_domain)", "                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = owner;\n                    loan_read(owned_domain)", 1),
}
none = source.index("                None =>")
some = source.index("                Some(tail_bundle)", none)
end = source.index("            }\n        },", some)
positives["reversed-inner-arms"] = source[:none] + source[some:end] + source[none:some] + source[end:]
positives["reversed-constructor-fields"] = source.replace("owned_ptr: p,\n        owned_allocation: a,\n        owned_domain: d", "owned_domain: d,\n        owned_ptr: p,\n        owned_allocation: a")

negatives = {}
for name, args, code in [
    ("wrong-root", "head_w2@next, ptr_h, allocation_t, life_t", "P193-CALL-DOMAIN"),
    ("wrong-backing", "head_w2@next, ptr_t, allocation_h, life_t", "P193-CALL-BACKING"),
    ("wrong-domain", "head_w2@next, ptr_t, allocation_t, life_h", "P3-REF-CONFLICT"),
]:
    negatives[name] = (source.replace(call, args), code)
    # A previously examined good call in a separate branch cannot bless bad.
    op = "detach_and_return_tail(\n                            " + call + ")"
    bad = "detach_and_return_tail(" + args + ")"
    negatives[name + "-sibling-good"] = (source.replace(op, "match seen {Some(q)=>{" + op + "},None=>{" + bad + "}}"), code)
without_seen = source[:seen_start] + source[seen_end:]
negatives["head-none"] = (without_seen.replace("Option<ptr<Node>>::Some(ptr_t)", "Option<ptr<Node>>::None", 1), "P208-CALL-HEAD-VALUE")
negatives["head-other-payload"] = (without_seen.replace("Option<ptr<Node>>::Some(ptr_t)", "Option<ptr<Node>>::Some(ptr_h)", 1), "P208-CALL-HEAD-VALUE")
negatives["head-read-only"] = (source.replace("ref_from_ptr(write, ptr_h, stable_h);\n                        detach", "ref_from_ptr(read, ptr_h, stable_h);\n                        detach"), "P3-TYPE-MISMATCH")
for name, old, new, code in [
    ("donor-allocation-reuse", "                    loan_read(owned_domain)", "                    allocation_t;\n                    loan_read(owned_domain)", "P3-USE-AFTER-CONSUME"),
    ("donor-domain-reuse", "                    loan_read(owned_domain)", "                    life_t;\n                    loan_read(owned_domain)", "P3-USE-AFTER-CONSUME"),
    ("missing-return-field", "owned_domain: d\n", "\n", "P208-DEFINITION-RETURN"),
    ("duplicate-return-field", "owned_domain: d\n", "owned_domain: d, owned_domain: d\n", "P208-DEFINITION-RETURN"),
    ("head-ref-escape", "owned_ptr: p,", "owned_ptr: head_link,", "P208-DEFINITION-RETURN"),
    ("missing-detach", "    let old_link = replace(\n        head_link, Option<ptr<Node>>::None);", "", "P208-DEFINITION-RETURN"),
    ("early-return", "    let old_link", "    return unit;\n    let old_link", "P193-DEFINITION-ESCAPE"),
    ("wrong-detach-operand", "        head_link, Option<ptr<Node>>::None", "        p, Option<ptr<Node>>::None", "P208-DEFINITION-DETACH"),
    ("caller-partial-destructure", "                        owned_domain\n                    } =", "                    } =", "P5-AGGREGATE-FIELD-COUNT"),
    ("missing-terminal-release", "                    receive_and_release_tail(\n                        owned_ptr, owned_allocation, owned_domain);", "", "P5-SCOPE-OBLIGATION"),
    ("wrong-returned-receiver", "owned_ptr, owned_allocation, owned_domain);", "ptr_h, owned_allocation, owned_domain);", "P193-CALL-DOMAIN"),
    ("stale-after-receiver", "                    let empty_h =", "                    loan_read(life_h){|s|let stale=ref_from_ptr(read,owned_ptr,s);unit};\n                    let empty_h =", "P3-STALE-POINTER"),
]:
    assert old in source, name
    text = source.replace(old, new, 1)
    if name == "stale-after-receiver":
        text = source[:read_end] + source[read_end:].replace(old, new, 1)
    negatives[name] = (text, code)
op = "detach_and_return_tail(\n                            " + call + ")"
negatives["active-tail-domain-scope"] = (source.replace(op, "loan_read(life_t){|held|" + op + "}"), "P3-REF-CONFLICT")
negatives["discard-live-tail"] = (source.replace(receive, "                    "), "P5-DISCARDABLE-REQUIRED")
placement = positives["result-placement"]
negatives["duplicate-live-tail"] = (placement.replace("let LiveTail { owned_ptr, owned_allocation, owned_domain } = owner;", "let second=owner;let third=owner;"), "P3-USE-AFTER-CONSUME")
negatives["return-only-copy-ptr"] = (source.replace("return LiveTail {\n        owned_ptr: p,\n        owned_allocation: a,\n        owned_domain: d\n    };", "return p;"), "P193-DEFINITION-ESCAPE")
negatives["repeated-returned-consumer"] = (source.replace("owned_ptr, owned_allocation, owned_domain);", "owned_ptr, owned_allocation, owned_domain);receive_and_release_tail(owned_ptr,owned_allocation,owned_domain);"), "P3-USE-AFTER-CONSUME")
head_cleanup = source.index("                    let empty_h =", read_end)
negatives["forgotten-live-tail"] = (placement[:placement.index("                    let LiveTail { owned_ptr")] + source[head_cleanup:], "P5-SCOPE-OBLIGATION")
# Valid source-shaped branching beyond the bounded result join must remain
# precision unsupported, never type-only fresh authority/package synthesis.
precision = source.replace(op, "match seen {Some(q)=>{" + op + "},None=>{" + op + "}}")
# No C/executable path: accepted source is intentionally pre-backend unsupported.
with tempfile.TemporaryDirectory() as directory:
    root = pathlib.Path(directory)
    supported = {"primary", "renamed", "declaration-order", "tail-argument-aliases", "reversed-inner-arms", "reversed-constructor-fields"}
    for name, text in positives.items():
        path = root / (name + ".nl")
        path.write_text(text)
        runs = [subprocess.run([compiler, str(path)], capture_output=True) for _ in range(2)]
        for run in runs:
            if name in supported:
                assert run.returncode == 0 and run.stdout and not run.stderr, (name, run.returncode, run.stderr)
            else:
                assert run.returncode == 4 and not run.stdout and b"V1-BACKEND-UNSUPPORTED" in run.stderr, (name, run.returncode, run.stderr)
        assert (runs[0].stdout, runs[0].stderr) == (runs[1].stdout, runs[1].stderr)
        subprocess.run([evidence, "evidence", str(path)], check=True)
    for name, (text, code) in negatives.items():
        path = root / (name + ".nl")
        path.write_text(text)
        run = subprocess.run([compiler, str(path)], capture_output=True)
        assert run.returncode == 3 and not run.stdout and code.encode() in run.stderr, (name, code, run.returncode, run.stderr)
        if name in {"missing-return-field", "duplicate-return-field", "head-ref-escape", "missing-detach", "early-return", "wrong-detach-operand"}:
            path.write_text(text.split("fn main()", 1)[0] + "fn main()->unit {unit}")
            run = subprocess.run([compiler, str(path)], capture_output=True)
            assert run.returncode == 3 and not run.stdout and code.encode() in run.stderr, (name, "uncalled", run.stderr)
    path = root / "branch-result-precision.nl"
    path.write_text(precision)
    run = subprocess.run([compiler, str(path)], capture_output=True)
    assert run.returncode == 3 and not run.stdout and b"precision" in run.stderr, ("branch-result-precision", run.stderr)
    assert all(path.suffix == ".nl" for path in root.iterdir())
print(f"{len(positives)} source positives / {len(negatives)} destructive controls; explicit emission profiles; native covered separately")
