"""#237 actual public source gate; semantic evidence only, no native claim."""
import hashlib
import json
import pathlib
import subprocess
import sys
import tempfile

compiler, evidence = map(lambda p: str(pathlib.Path(p).resolve()), sys.argv[1:3])
repo = pathlib.Path(__file__).resolve().parents[2]
draft = (repo / "docs/reference/NewLang_v0_spec_Draft17_30.md").read_text()
canonical = draft.split("### 3.2b.3 Exact five-site full source-shaped positive witness\n", 1)[1].split("~~~newlang\n", 1)[1].split("~~~", 1)[0]
source = "".join(line for line in canonical.splitlines(keepends=True) if not line.lstrip().startswith("//"))
assert source == (repo / "tests/fixtures/five_root_three_field.nl").read_text()
assert hashlib.sha256(source.encode()).hexdigest() == "812042833072145f761ddc21cbf774d81b64da3a34f6d466ba7db01a6d704d7d"

def block_end(text, start):
    depth = 0
    for i in range(start, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return i
    raise AssertionError("unclosed source")

def swap(text, site):
    start = -1
    for _ in range(site):
        start = text.index("match try_allocate_one<Node>()", start + 1)
    first = text.index("None =>", start)
    first_end = text.index("\n", block_end(text, text.index("{", first))) + 1
    second = text.index("Some(bundle_", first_end)
    second_end = text.index("\n", block_end(text, text.index("{", second))) + 1
    first = text.rfind("\n", 0, first) + 1
    second = text.rfind("\n", 0, second) + 1
    return text[:first] + text[second:second_end] + text[first:first_end] + text[second_end:]

cases = [
    ("canonical", source, 4, "V1-BACKEND-UNSUPPORTED", "source-accepted"),
    ("alpha", source.replace("Node", "Cell").replace("ptr_", "pointer_").replace("life_", "domain_"), 4, "V1-BACKEND-UNSUPPORTED", "source-accepted"),
    ("two-arm-permutations", swap(swap(source, 3), 1), 4, "V1-BACKEND-UNSUPPORTED", "source-accepted"),
    ("canonical-comments", canonical, 4, "V1-BACKEND-UNSUPPORTED", "source-accepted"),
    ("same-address-reset", source.replace("replace(w_A_next@next,", "replace(w_A_next@prev,", 1).replace("Some(ptr_B))", "Some(ptr_C))", 1), 4, "V1-BACKEND-UNSUPPORTED", "source-accepted-reset-evidence"),
    ("library-tail-policy", source.replace("Some(ptr_C))", "Some(ptr_B))", 1), 4, "V1-BACKEND-UNSUPPORTED", "library-policy-mismatch"),
    ("ptr-base", source.replace("w_src_child@child", "ptr_src@child", 1), 3, "HEAP-LINK-REF-PROFILE", "profile-reject"),
    ("payload-field", source.replace("w_src_child@child", "w_src_child@payload", 1), 3, "HEAP-LINK-FIELD-PROFILE", "profile-reject"),
    ("unknown-field", source.replace("w_src_child@child", "w_src_child@unknown", 1), 3, "HEAP-LINK-FIELD-PROFILE", "profile-reject"),
    ("read-write-escalation", source.replace("ref_from_ptr(write, ptr_src,", "ref_from_ptr(read, ptr_src,", 1), 3, "P3-TYPE-MISMATCH", "semantic-reject"),
    ("wrong-domain", source.replace("ref_from_ptr(write, ptr_src,", "ref_from_ptr(write, ptr_A,", 1), 3, "ALLOCATED-DOMAIN-MISMATCH", "semantic-reject"),
    ("wrong-allocation", source.replace("deallocate(allocation_dst, full_dst)", "deallocate(allocation_C, full_dst)", 1), 3, "P4-ALLOCATION-MISMATCH", "semantic-reject"),
    ("wrong-end-domain", source.replace("destroy(ptr_dst, ending_dst)", "destroy(ptr_C, ending_dst)", 1), 3, "P3-DOMAIN-MISMATCH", "semantic-reject"),
    ("double-release", source.replace("deallocate(allocation_dst, full_dst);", "deallocate(allocation_dst, full_dst);deallocate(allocation_dst,full_dst);", 1), 3, "P3-USE-AFTER-CONSUME", "semantic-reject"),
    ("noncopy-reuse", source.replace("let allocation_src = allocation;", "let allocation_src=allocation;let duplicate=allocation;", 1), 3, "P3-USE-AFTER-CONSUME", "semantic-reject"),
    ("none-fabricated-owner", source.replace("None => {\n            unit", "None => {\n            deallocate(allocation_src,full_src);unit", 1), 3, "ALLOCATED-SOURCE-PROFILE", "profile-reject"),
    ("scope-escape", source.replace("Option<ptr<Node>>::Some(ptr_A))", "Option<ptr<Node>>::Some(ptr_A));w_src_child@child", 1), 3, "P3-INTERNAL", "semantic-reject-legacy-diagnostic"),
    ("active-loan-ending", source.replace("loan_exclusive_read(life_dst) { |ending_dst|", "loan_read(life_dst) { |held| loan_exclusive_read(life_dst) { |ending_dst|", 1).replace("destroy(ptr_dst, ending_dst)\n                                            };", "destroy(ptr_dst, ending_dst)}\n                                            };", 1), 3, "P3-REF-CONFLICT", "semantic-reject"),
    ("stale-pointer", source.replace("let full_dst =", "loan_read(life_dst){|stable|let r=ref_from_ptr(read,ptr_dst,stable);read(r@child)};let full_dst =", 1), 3, "P3-STALE-POINTER", "semantic-reject"),
    ("stale-some-borrow", source.replace("replace(w_A_next@next,", "let field=w_A_next@prev;match field {None=>{unit},Some(payload_ref)=>{replace(w_A_next@prev,Option<ptr<Node>>::None);unit}};replace(w_A_next@next,", 1), 3, "P9-MATCH-PRECISION", "precision-reject"),
    ("unknown-alias", source.replace("replace(w_A_next@next,", "let alias=w_A_next;replace(alias@next,", 1), 3, "CAPTURED-CLOSURE-PRECISION", "precision-reject"),
    ("sixth", source.replace("let old_src_child =", "match try_allocate_one<Node>(){None=>{unit},Some(extra)=>{unit}};let old_src_child =", 1), 3, "ALLOCATED-CARDINALITY-PROFILE", "profile-reject"),
    ("fourth-link", source.replace("    payload: u8,", "    extra:Option<ptr<Node>>,payload:u8,", 1), 3, "AVS-DECL-PROFILE", "profile-reject"),
    ("second-nominal", source + "struct Other{next:Option<ptr<Other>>,payload:u8}", 3, "REC-DECL-DUPLICATE", "semantic-registration-reject"),
]
for site in range(2, 6):
    start = -1
    for _ in range(site):
        start = source.index("match try_allocate_one<Node>()", start + 1)
    opening = source.index("{", source.index("None =>", start))
    end = block_end(source, opening)
    begin = source.index("let empty_", opening, end)
    finish = source.index("\n", source.index("deallocate(", begin, end)) + 1
    cases.append((f"none-{site}-missing-cleanup", source[:begin] + source[finish:], 3, "ALLOCATED-CAPTURED-JOIN-PRECISION", "precision-reject"))
    if site >= 3:
        segment = source[opening:end]
        owner = "allocation_A" if site == 3 else "allocation_B" if site == 4 else "allocation_C"
        wrong = segment.replace(f"deallocate({owner},", "deallocate(allocation_src,", 1)
        assert wrong != segment
        cases.append((f"none-{site}-wrong-cleanup", source[:opening] + wrong + source[end:], 3, "P4-ALLOCATION-MISMATCH", "semantic-reject"))

observations = []
with tempfile.TemporaryDirectory(prefix="five-root-source-") as directory:
    root = pathlib.Path(directory)
    for name, text, status, code, classification in cases:
        filename = name + ".nl"
        (root / filename).write_text(text)
        before = set(root.iterdir())
        a = subprocess.run([compiler, filename], cwd=root, capture_output=True)
        b = subprocess.run([compiler, filename], cwd=root, capture_output=True)
        assert (a.returncode, a.stdout, a.stderr) == (b.returncode, b.stdout, b.stderr), name
        assert a.returncode == status and code.encode() in a.stderr, (name, a.returncode, a.stderr)
        assert not a.stdout and set(root.iterdir()) == before, (name, "unexpected C/artifact")
        if classification == "source-accepted":
            checked = subprocess.run([evidence, "full-evidence", filename, "5"], cwd=root, capture_output=True)
            assert checked.returncode == 0 and b"seven actual field facts" in checked.stdout, (name, checked.stderr)
        if classification == "source-accepted-reset-evidence":
            checked = subprocess.run([evidence, "full-reset", filename], cwd=root, capture_output=True)
            assert checked.returncode == 0 and b"fresh Some occurrence" in checked.stdout, (name, checked.stderr)
        observations.append({"input": name, "sha256": hashlib.sha256(text.encode()).hexdigest(),
                             "classification": classification, "exit": a.returncode,
                             "stdout": a.stdout.decode(), "stderr": a.stderr.decode(), "output_files_created": []})
print(json.dumps({"claim": "full actual-source semantic admission; no C/native/cJSON PASS",
                  "observations": observations}, indent=2))
