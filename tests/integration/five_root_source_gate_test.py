"""P281 first source gate; blocked mutations are never semantic safety passes."""
import os
import hashlib
import json
import pathlib
import subprocess
import sys
import tempfile

compiler, boundary, terminal = (str(pathlib.Path(x).resolve()) for x in sys.argv[1:4])
repo = pathlib.Path(__file__).resolve().parents[2]
full = (repo / "tests/fixtures/experimental_five_root_detach_attach.nl").read_text()
previous = (repo / "tests/fixtures/experimental_transitive_returned_whole.nl").read_text()
prefix = full[full.index("struct Node"):full.index("struct TreeFour")]


def change(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


cases = [("complete-five-root-detach-attach", full, "parser-unsupported", "AVS-DECL-PROFILE", None)]
mutations = {
    "wrong-original-Allocation-C": change(change(full, "a:allocation_B, d:life_B", "a:allocation_C, d:life_B"), "a:allocation_C, d:life_C", "a:allocation_B, d:life_C"),
    "wrong-original-Domain-C": change(change(full, "a:allocation_B, d:life_B", "a:allocation_B, d:life_C"), "a:allocation_C, d:life_C", "a:allocation_C, d:life_B"),
    "duplicate-B-current-packet": change(full, "child:LiveRoot { p:pb, a:ab, d:db }", "child:LiveRoot { p:pb, a:ad, d:db }"),
    "donor-after-detach-reuse": change(full, "let result = detach_middle(donor);", "let result = detach_middle(donor);\nlet again = detach_middle(donor);"),
    "active-B-loan-over-repack": change(full, "let detached = LiveRoot { p:pb, a:ab, d:db };", "let detached = loan_read(db) { |held| LiveRoot { p:pb, a:ab, d:db } };"),
    "partial-TreeFour-constructor": change(full, "src:LiveRoot { p:ptr_src, a:allocation_src, d:life_src },", ""),
    "partial-TreeThree-return": change(full, "let donor = TreeThree { src:src,", "let donor = TreeThree {"),
    "omitted-B-normal-exit": change(full, "let result = DetachResult { donor:donor, detached:detached };", "let result = DetachResult { donor:donor };"),
    "refusal-loses-whole-owner": change(full, "let TreeFour { src, first, middle, last } = whole;", "if true {return unit;} else {unit};\nlet TreeFour { src, first, middle, last } = whole;"),
    "Copy-B-link-without-B-responsibility": change(full, "let adopted = attach_whole(receiver, detached, ptr_B);", "let adopted = TreeTwo { root:receiver, child:receiver };"),
}
for name, text in mutations.items():
    cases.append((name, text, "parser-unsupported", "AVS-DECL-PROFILE", None))
alpha = full
for old, new in [("Node", "Cell"), ("LiveRoot", "Cargo"), ("TreeFour", "FourPackets"), ("TreeThree", "ThreePackets"), ("TreeTwo", "TwoPackets"), ("DetachResult", "SplitPackets"), ("detach_middle", "split_record"), ("attach_whole", "join_record")]:
    alpha = alpha.replace(old, new)
cases.append(("role-neutral-renaming", alpha, "parser-unsupported", "AVS-DECL-PROFILE", None))

for name, declaration in [
    ("minimal-TreeFour", "struct Four { a:LiveRoot,b:LiveRoot,c:LiveRoot,d:LiveRoot, }"),
    ("minimal-TreeThree", "struct Three { a:LiveRoot,b:LiveRoot,c:LiveRoot, }"),
]:
    cases.append((name, prefix + declaration + "\nfn main()->unit {unit}\n", "parser-unsupported", "AVS-DECL-PROFILE", None))
for name, declarations in [
    ("minimal-two-owner-declarations", "struct First {a:LiveRoot,b:LiveRoot,}\nstruct Second {a:LiveRoot,b:LiveRoot,}"),
    ("minimal-mixed-nested-result", "struct Pair {a:LiveRoot,b:LiveRoot,}\nstruct Split {donor:Pair,detached:LiveRoot,}"),
]:
    cases.append((name, prefix + declarations + "\nfn main()->unit {unit}\n", "semantic-profile-unsupported", "P276-NESTED-PROFILE", None))
helper = """fn edit_packet(ticket:LiveRoot)->LiveRoot {
    let LiveRoot {p,a,d}=ticket;
    let old=loan_read(d){|s| let w=ref_from_ptr(write,p,s); replace(w@prev,Option<ptr<Node>>::None)};
    let result=LiveRoot{p:p,a:a,d:d}; return result;
}
"""
cases.append(("independent-whole-edit-helper", change(previous, "fn main()", helper + "fn main()"), "semantic-profile-unsupported", "ALLOCATED-DOMAIN-LOAN", None))
cases.append(("P278-exact-returned-whole-control", previous, "semantic-accepted/backend-unsupported", "V1-BACKEND-UNSUPPORTED", "after-death"))

# Reuse two independently fixed P278 contrasts as predecessor controls only.
# Their successful/failed terminal does NOT stand in for actual detach/adopt.
ns = {"__file__": str(repo / "tests/integration/transitive_terminal_test.py")}
saved = sys.argv
try:
    sys.argv = [saved[0], compiler, terminal, terminal]
    script = (repo / "tests/integration/transitive_terminal_test.py").read_text()
    exec(script.split("\n\ndef run(root):")[0], ns)
finally:
    sys.argv = saved
for name in ("wrong-original-C-Allocation-at-finish-two", "wrong-original-C-Domain-at-finish-two"):
    matching = [item for item in ns["cases"] if item[0] == name]
    assert len(matching) == 1, name
    _, text, status, code, category, proof = matching[0]
    assert status == 3 and category == "semantic-reject"
    cases.append(("P278-" + name, text, category, code, "reject:" + code))


# The immutable inputs remain controls. Only the NEW default-OFF profile
# deliberately changes their first gate; do not count them as safety passes.
if os.environ.get("NEWLANG_P285") == "1":
    cases = [(name,text,
              "semantic-precision-unsupported" if category == "parser-unsupported" and not name.startswith("minimal-") else "semantic-profile-unsupported",
              "P8-SIGNATURE-PRECISION" if category == "parser-unsupported" and not name.startswith("minimal-") else "FIVE-ROOT-SOURCE-PROFILE",proof)
             if category == "parser-unsupported" or name.startswith("minimal-") else (name,text,category,code,proof)
             for name,text,category,code,proof in cases]

if os.environ.get("NEWLANG_P285") == "1":
    cases = [(name,text,"parser-syntax-error","P13-IF-OPEN",proof)
             if name == "refusal-loses-whole-owner" else (name,text,category,code,proof)
             for name,text,category,code,proof in cases]

if os.environ.get("NEWLANG_P289") == "1":
    cases = [(name,text,"semantic-profile-unsupported","ALLOCATED-DOMAIN-LOAN",proof)
             if code == "P8-SIGNATURE-PRECISION" else (name,text,category,code,proof)
             for name,text,category,code,proof in cases]

def run(directory):
    observations = []
    for name, text, category, code, proof in cases:
        path = directory / (name + ".nl")
        path.write_text(text)
        before = set(directory.iterdir())
        first = subprocess.run([compiler, path.name], cwd=directory, capture_output=True)
        second = subprocess.run([compiler, path.name], cwd=directory, capture_output=True)
        assert (first.returncode, first.stdout, first.stderr) == (second.returncode, second.stdout, second.stderr), name
        assert first.returncode == (4 if proof == "after-death" else 3) and not first.stdout, (name, first.stderr)
        assert code.encode() in first.stderr, (name, first.stderr)
        assert set(directory.iterdir()) == before, name
        expected_label = b"error(semantic)" if category == "semantic-reject" else b"error(cli)" if proof == "after-death" else b"error(syntax)" if category == "parser-syntax-error" else b"error(precision)" if category == "semantic-precision-unsupported" else b"error(unsupported)"
        assert expected_label in first.stderr, (name, first.stderr)
        checked = subprocess.run([boundary, str(path)], capture_output=True)
        assert checked.returncode == 0 and not checked.stderr, (name, checked.stderr)
        view = json.loads(checked.stdout)
        if category.startswith("parser-"):
            assert view["parse_status"] == (3 if category == "parser-syntax-error" else 2) and not view["syntax_tree"] and not view["registration_attempted"]
        else:
            assert view["parse_status"] == 0 and view["syntax_tree"] and view["registration_attempted"]
            assert view["registration_status"] == (0 if proof == "after-death" else 1 if category == "semantic-reject" else 3 if category == "semantic-precision-unsupported" else 2)
        if proof != "after-death":
            assert view["code"] == code and view["snapshot_unchanged"]
            view["source_span"] = text[view["start_byte"]:view["end_byte"]]
        inherited = None
        if proof:
            out = subprocess.run([terminal, str(path), proof], capture_output=True)
            assert out.returncode == 0 and not out.stderr, (name, out.stderr)
            inherited = json.loads(out.stdout)
            if proof == "after-death":
                assert inherited["terminal_is_exact_returned_whole"] and inherited["release_order"] == [2, 4, 1, 3, 5]
        observations.append({"input": name, "source_sha256": hashlib.sha256(text.encode()).hexdigest(), "classification": category, "cli_exit": first.returncode, "diagnostic": first.stderr.decode(), "repeat_identical": True, "C_artifact": False, "public_boundary": view, "inherited_terminal_control": inherited, "actual_P281_detach_adopt_validated": False})
    return {"kind": "P281 first SOURCE gate HOLD; no core changes", "native_execution": False, "observations": observations}


if len(sys.argv) > 4:
    directory = pathlib.Path(sys.argv[4]).resolve()
    directory.mkdir(parents=True, exist_ok=True)
    result = run(directory)
    (directory / "observations.json").write_text(json.dumps(result, indent=2) + "\n")
else:
    with tempfile.TemporaryDirectory(prefix="p281-source-gate-") as temporary:
        result = run(pathlib.Path(temporary))
print(json.dumps(result, indent=2))
