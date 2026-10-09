"""#244 certificate probes. Explicit inputs/outcomes; NOT #237 admission."""
import hashlib
import json
import pathlib
import re
import subprocess
import sys
import tempfile

probe, compiler = map(lambda s: str(pathlib.Path(s).resolve()), sys.argv[1:3])
repo = pathlib.Path(__file__).resolve().parents[2]
fixtures = repo / "tests/fixtures"
source = (fixtures / "five_allocation_closure_probe.nl").read_text()


def block_end(text, start):
    depth = 0
    for i in range(start, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return i
    raise AssertionError("unclosed probe block")


def swap_arms(text, site):
    start = -1
    for _ in range(site):
        start = text.index("match try_allocate_one<Node>()", start + 1)
    first = text.index("None =>", start)
    opening = text.index("{", first)
    first_end = text.index("\n", block_end(text, opening)) + 1
    second = text.index("Some(bundle_", first_end)
    second_end = text.index("\n", block_end(text, text.index("{", second))) + 1
    first = text.rfind("\n", 0, first) + 1
    second = text.rfind("\n", 0, second) + 1
    return text[:first] + text[second:second_end] + text[first:first_end] + text[second_end:]


cases = [
    ("five", source, 0, "certificate-substrate accepted"),
    ("outer-some-first", swap_arms(source, 1), 0, "certificate-substrate accepted"),
    ("third-some-first", swap_arms(source, 3), 0, "certificate-substrate accepted"),
    ("alpha", source.replace("Node", "Cell").replace("ptr_", "pointer_").replace("allocation_", "owner_"), 0, "certificate-substrate accepted"),
    ("cross-allocation", source.replace("deallocate(allocation_A, full_A);", "deallocate(allocation_src, full_A);", 1), 3, "P4-ALLOCATION-MISMATCH"),
    ("cross-domain", source.replace("destroy(ptr_A, ending_A)", "destroy(ptr_src, ending_A)", 1), 3, "P3-DOMAIN-MISMATCH"),
    ("double-release", source.replace("deallocate(allocation_src, full_src);", "deallocate(allocation_src, full_src);deallocate(allocation_src, full_src);", 1), 3, "P3-USE-AFTER-CONSUME"),
    ("noncopy-reuse", source.replace("let allocation_src = allocation;", "let allocation_src = allocation;let moved=allocation_src;", 1), 3, "P3-USE-AFTER-CONSUME"),
    ("missing-release", source.replace("deallocate(allocation_A, full_A);", "", 1), 3, "P5-SCOPE-OBLIGATION"),
]
# Each failing site 2..5 owes every original from earlier Some outcomes.
# Remove exactly one complete captured-owner cleanup from that None arm.
for site in range(2, 6):
    start = -1
    for _ in range(site):
        start = source.index("match try_allocate_one<Node>()", start + 1)
    none = source.index("None =>", start)
    opening = source.index("{", none)
    end = block_end(source, opening)
    cleanup_start = source.index("let empty_", opening, end)
    cleanup_end = source.index("\n", source.index("deallocate(", cleanup_start, end)) + 1
    text = source[:cleanup_start] + source[cleanup_end:]
    cases.append((f"none-{site}-missing-original", text, 3, "ALLOCATED-CAPTURED-JOIN-PRECISION"))

ending = re.search(r"let empty_A = loan_exclusive_read\(life_A\) \{.*?\};", source, re.S).group(0)
assert ending in source
cases.append(("active-loan", source.replace(ending,
    "let empty_A = loan_read(life_A) { |held|loan_exclusive_read(life_A) { |ending_A| destroy(ptr_A,ending_A)}};", 1), 3, "P3-REF-CONFLICT"))
# Existing allocated-domain exit rejects scope escape as semantic error under
# its historical P3-INTERNAL diagnostic code; record rather than redesign it.
cases.append(("scope-escape", source.replace("let vacant_src = into_slot<Node>(raw);", "let vacant_src = into_slot<Node>(raw);", 1).replace(
    "            match try_allocate_one<Node>()", "            let escape=loan_read(life_src){|stable|ref_from_ptr(read,ptr_src,stable)};\n            match try_allocate_one<Node>()", 1), 3, "P3-INTERNAL"))
cases.append(("sixth", source.replace("                                            unit\n", "                                            match try_allocate_one<Node>() {None=>{unit},Some(extra)=>{unit}}\n", 1), 3, "ALLOCATED-CARDINALITY-PROFILE"))

observations = []
with tempfile.TemporaryDirectory(prefix="captured-closure-") as directory:
    root = pathlib.Path(directory)
    for name, text, status, diagnostic in cases:
        filename = name + ".nl"
        (root / filename).write_text(text)
        before = set(root.iterdir())
        args = [probe, "check", filename]
        a = subprocess.run(args, cwd=root, capture_output=True)
        b = subprocess.run(args, cwd=root, capture_output=True)
        assert (a.returncode, a.stdout, a.stderr) == (b.returncode, b.stdout, b.stderr), name
        assert a.returncode == status and diagnostic.encode() in a.stdout, (name, a.returncode, a.stdout, a.stderr)
        assert not a.stderr and set(root.iterdir()) == before, name
        observations.append({"input": name, "sha256": hashlib.sha256(text.encode()).hexdigest(),
                             "probe_exit": a.returncode, "stdout": a.stdout.decode(), "stderr": ""})
    # The reviewed three-link/five-site profile now admits its full source.
    # One-link reduced three/five-site probes retain their closed public gate;
    # outcomes are classified by explicit inputs, never generic failures.
    for name, status, code in [("five_root_three_field", 0, ""),
                              ("three_allocation_no_links", 3, "ALLOCATED-CARDINALITY-PROFILE"),
                              ("five_allocation_closure_probe", 3, "ALLOCATED-CARDINALITY-PROFILE"),
                              ("two_allocation_no_links", 4, "V1-BACKEND-UNSUPPORTED")]:
        text = (fixtures / (name + ".nl")).read_text()
        filename = name + ".nl"
        (root / filename).write_text(text)
        a = subprocess.run([compiler, filename], cwd=root, capture_output=True)
        assert a.returncode == status and code.encode() in a.stderr, (name, a.returncode, a.stderr)
        assert (bool(a.stdout), bool(a.stderr)) == ((True, False) if status == 0 else (False, True)), name
        observations.append({"input": name, "sha256": hashlib.sha256(text.encode()).hexdigest(),
                             "public_cli_exit": a.returncode, "stdout_sha256": hashlib.sha256(a.stdout).hexdigest(), "stderr": a.stderr.decode()})
    assert not list(root.glob("*.c")) and not list(root.glob("*.o")) and not list(root.glob("*.exe"))
print(json.dumps({"claim": "certificate substrate plus current CLI admission; native evidence is a separate test",
                  "source_controls": observations}, indent=2))
