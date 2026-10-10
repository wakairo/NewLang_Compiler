"""Issue 274 opt-in empirical source/checker slice, NOT Draft adoption/native PASS.

All facts come from public source compilation. The observer reads owned evidence;
no source annotations, host seeds, Matched bits or private probe entry are used.
The compiled CLI and observer are exercised independently. Run with an optional
fourth argument to retain exact source inputs and JSON observations for review.
"""
import os
import hashlib
import json
import pathlib
import subprocess
import sys
import tempfile

compiler, evidence, closure = (str(pathlib.Path(x).resolve()) for x in sys.argv[1:4])
repo = pathlib.Path(__file__).resolve().parents[2]
source = (repo / "tests/fixtures/experimental_original_grant.nl").read_text()
assert source.count("try_allocate_one<Node>()") == 5
assert source.count("replace(") == 6


def change(text, old, new, count=1):
    assert text.count(old) >= count, old
    result = text.replace(old, new, count)
    assert result != text
    return result


def change_last(text, old, new):
    before, found, after = text.rpartition(old)
    assert found, old
    return before + new + after


wrong = change(source, "a: allocation_B, d: life_B", "a: allocation_C, d: life_B")
mixed = change(wrong, "deallocate(a, full_B);", "deallocate(allocation_B, full_B);")
mixed = change(mixed, "deallocate(allocation_C, full_C);\n                                            let empty_A", "deallocate(a, full_C);\n                                            let empty_A")
# Same mixed aggregate and identical valid B.prev write, but pair both raw
# terminals with the independently correct original Allocations. Must ACCEPT.

begin = source.index("                                            let empty_C =", source.index("let repacked ="))
end = source.index("                                            deallocate(allocation_C, full_C);", begin)
recover_c = source[begin:end]
wrong_storage = source[:begin] + source[end:]
wrong_storage = change_last(wrong_storage, "                                            let empty_B =", recover_c + "                                            let empty_B =")
wrong_storage = change(wrong_storage, "deallocate(a, full_B);", "deallocate(a, full_C);")

terminal_start = source.index("                                            let empty_dst =", source.index("let repacked ="))
terminal_end = source.index("                                            unit", terminal_start)
terminal = source[terminal_start:terminal_end]
starts = [terminal.index("                                            let empty_" + role + " =") for role in ("dst", "B", "C", "A", "src")]
chunks = {role:terminal[starts[i]:(starts[i+1] if i<4 else len(terminal))] for i,role in enumerate(("dst", "B", "C", "A", "src"))}
after_a = source[:terminal_start] + "".join(chunks[role] for role in ("A", "C", "src", "B", "dst")) + source[terminal_end:]

cases = [
    ("after-A-primitive-terminal", after_a, 4, "V1-BACKEND-UNSUPPORTED", "semantic-accepted/backend-unsupported", "matched"),
    ("correct-original", source, 4, "V1-BACKEND-UNSUPPORTED", "semantic-accepted/backend-unsupported", "matched"),
    ("mixed-harmless-repaired", mixed, 4, "V1-BACKEND-UNSUPPORTED", "semantic-accepted/backend-unsupported", "mixed"),
    ("wrong-original-allocation", wrong, 3, "P4-ALLOCATION-MISMATCH", "semantic-reject", None),
    ("wrong-original-domain", change(source, "a: allocation_B, d: life_B", "a: allocation_B, d: life_C"), 3, "ALLOCATED-DOMAIN-MISMATCH", "semantic-reject", None),
    ("duplicate-allocation", change(source, "let repacked =", "let ghost = LiveRoot { p: ptr_B, a: allocation_B, d: life_C }; let repacked ="), 3, "P3-USE-AFTER-CONSUME", "semantic-reject", None),
    ("duplicate-domain", change(source, "let repacked =", "let ghost = LiveRoot { p: ptr_B, a: allocation_C, d: life_B }; let repacked ="), 3, "P3-USE-AFTER-CONSUME", "semantic-reject", None),
    ("duplicate-whole-current", change(source, "let repacked =", "let ghost = packed; let repacked ="), 3, "P3-USE-AFTER-CONSUME", "semantic-reject", None),
    ("live-domain-loan-across-repack", change(source, "LiveRoot { p: p, a: a, d: d }", "loan_read(d) { |still_live| LiveRoot { p: p, a: a, d: d } }"), 3, "P3-REF-CONFLICT", "semantic-reject", None),
    ("live-derived-H-loan-across-repack", change(source, "LiveRoot { p: p, a: a, d: d }", "loan_read(d) { |still_live| let h = ref_from_ptr(write, p, still_live); LiveRoot { p: p, a: a, d: d } }"), 3, "P3-REF-CONFLICT", "semantic-reject", None),
    ("stale-ptr-after-end", change_last(source, "let full_B = erase_slot<Node>(empty_B);", "loan_read(d) { |stable| let stale = ref_from_ptr(read, p, stable); unit }; let full_B = erase_slot<Node>(empty_B);"), 3, "P3-STALE-POINTER", "semantic-reject", None),
    ("wrong-full-original-storage", wrong_storage, 3, "P4-ALLOCATION-MISMATCH", "semantic-reject", None),
    ("forgotten-owner", change(source, "let LiveRoot { p, a, d } = repacked;", "repacked;"), 3, "P5-DISCARDABLE-REQUIRED", "semantic-reject", None),
    ("partial-destructure", change(source, "let LiveRoot { p, a, d } = packed;", "let LiveRoot { p, a } = packed;"), 3, "P5-AGGREGATE-FIELD-COUNT", "semantic-reject", None),
    ("unknown-ordinary-field", change(source, "let LiveRoot { p, a, d } = packed;", "let LiveRoot { p, a, other } = packed;"), 3, "P5-AGGREGATE-FIELD", "semantic-reject", None),
    ("no-duplicate-original-raw", change(source, "let packed =", "let ghost_raw = raw; let packed ="), 3, "P3-USE-AFTER-CONSUME", "semantic-reject", None),
    ("priority-two-nested-record", change(source, "fn main()", "struct TreeTwo { root:LiveRoot, child:LiveRoot, }\nfn main()"), 3, "AVS-DECL-PROFILE", "parser-unsupported", None),
    ("priority-two-known-terminal", change(source, "fn main()", "fn finish_root(x:LiveRoot)->unit { let LiveRoot{p,a,d}=x; let empty=loan_exclusive_read(d){|ending|destroy(p,ending)}; let full=erase_slot<Node>(empty); finalize_domain(d); deallocate(a,full); unit }\nfn main()"), 3, "FIVE-ROOT-SOURCE-PROFILE", "semantic-profile-unsupported", None),
    ("missing-H", "struct Root{p:ptr<Node>,a:Allocation,d:LifetimeDomain} fn main()->unit{unit}", 3, "P274-RECORD-PROFILE", "semantic-profile-unsupported", None),
    ("duplicate-field-declaration", change(source, "a: Allocation, d: LifetimeDomain", "p: Allocation, d: LifetimeDomain"), 3, "P274-RECORD-DECLARATION", "semantic-registration-reject", None),
]
# Nominal/variable/field labels are not grants; physical shape and identities are.
alpha = source.replace("Node", "Cell").replace("LiveRoot", "Parcel").replace("ptr_", "address_").replace("life_", "governor_")
cases.append(("alpha-renamed", alpha, 4, "V1-BACKEND-UNSUPPORTED", "semantic-accepted/backend-unsupported", "matched"))
reorder = change(source, "LiveRoot { p: ptr_B, a: allocation_B, d: life_B }", "LiveRoot { d: life_B, a: allocation_B, p: ptr_B }")
cases.append(("constructor-field-order", reorder, 4, "V1-BACKEND-UNSUPPORTED", "semantic-accepted/backend-unsupported", "matched"))


# Only the new nested declaration is admitted by the separate #276 opt-in.
# The #275 source-only terminal control still lacks a nested declaration.
if os.environ.get("NEWLANG_P276") == "1":
    cases = [(n, t, 4, "V1-BACKEND-UNSUPPORTED",
              "semantic-accepted/backend-unsupported", "matched")
             if n == "priority-two-nested-record" else (n,t,s,c,k,p)
             for n,t,s,c,k,p in cases]

def run(root):
    observations = []
    for name, text, status, code, category, proof in cases:
        path = root / (name + ".nl")
        path.write_text(text)
        before = set(root.iterdir())
        a = subprocess.run([compiler, path.name], cwd=root, capture_output=True)
        b = subprocess.run([compiler, path.name], cwd=root, capture_output=True)
        assert (a.returncode, a.stdout, a.stderr) == (b.returncode, b.stdout, b.stderr), name
        assert a.returncode == status and code.encode() in a.stderr and not a.stdout, (name, a.returncode, a.stderr.decode())
        assert set(root.iterdir()) == before, (name, "unexpected artifact publication")
        if category.startswith("semantic-reject"):
            assert b"error(semantic)" in a.stderr, (name, a.stderr)
        proof_result = None
        if proof:
            checked = subprocess.run([evidence, str(path), proof], capture_output=True)
            assert checked.returncode == 0 and not checked.stderr, (name, checked.returncode, checked.stderr.decode())
            proof_result = json.loads(checked.stdout)
            assert proof_result["owned_validator"] and proof_result["field_write"]
            validated = subprocess.run([closure, "full-evidence", str(path), "5"], capture_output=True)
            assert validated.returncode == 0, (name, validated.stderr.decode())
            assert b"release worlds=0..5" in validated.stdout
            assert sorted(proof_result["full_original_release_order"]) == [1, 2, 3, 4, 5]
            if name == "after-A-primitive-terminal":
                assert proof_result["full_original_release_order"] == [2, 4, 1, 3, 5]
        if category in ("semantic-reject", "semantic-registration-reject", "semantic-profile-unsupported"):
            rejected = subprocess.run([evidence, str(path), "reject:" + code], capture_output=True)
            assert rejected.returncode == 0 and not rejected.stderr, (name, rejected.stderr.decode())
            proof_result = json.loads(rejected.stdout)
            assert proof_result["snapshot_unchanged"] and not proof_result["owned_artifact"]
            if code == "P4-ALLOCATION-MISMATCH":
                assert text[proof_result["start_byte"]:proof_result["end_byte"]] == "a"
        observations.append({"input": name, "source_sha256": hashlib.sha256(text.encode()).hexdigest(), "classification": category, "cli_exit": a.returncode, "stdout_bytes": len(a.stdout), "diagnostic": a.stderr.decode(), "repeat_identical": True, "output_files_created": [], "source_owned_evidence": proof_result})
    return {"kind": "UNADOPTED P274 empirical production-checker evidence", "native_execution": False, "observations": observations}


if len(sys.argv) > 4:
    root = pathlib.Path(sys.argv[4]).resolve()
    root.mkdir(parents=True, exist_ok=True)
    result = run(root)
    (root / "observations.json").write_text(json.dumps(result, indent=2) + "\n")
else:
    with tempfile.TemporaryDirectory(prefix="p274-original-grant-") as directory:
        result = run(pathlib.Path(directory))
print(json.dumps(result, indent=2))
