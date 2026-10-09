"""Source-derived original packet proof, not custody or native fork support."""
import hashlib
import pathlib
import subprocess
import sys
import tempfile

compiler, fixture, evidence = sys.argv[1:]
source = pathlib.Path(fixture).read_text()
consume = "let LiveTail{owned_ptr,owned_allocation,owned_domain}=packet;receive_and_release_tail(owned_ptr,owned_allocation,owned_domain);unit"
assert source.count(consume) == 2
policy = "let policy=Option<ptr<Node>>::None;"
positives = {
    "policy-none": source,
    "policy-some": source.replace(policy, "let policy=Option<ptr<Node>>::Some(ptr_h);"),
    "alpha-renamed": source.replace("packet", "original_owner").replace("receive_and_release_tail", "terminal").replace("policy", "decision"),
    "argument-source": source.replace("receive_and_release_tail(owned_ptr,owned_allocation,owned_domain)", "let token=owned_ptr;receive_and_release_tail(token,owned_allocation,owned_domain)"),
    "arms-reversed": source.replace("None=>{" + consume + "},Some(q)=>{" + consume + "}", "Some(q)=>{" + consume + "},None=>{" + consume + "}"),
}
negatives = {}
for arm in (0, 1):
    for kind, args in [
        ("wrong-pointer", "ptr_h,owned_allocation,owned_domain"),
        ("wrong-allocation", "owned_ptr,allocation_h,owned_domain"),
        ("wrong-domain", "owned_ptr,owned_allocation,life_h"),
        ("head-owner", "ptr_h,allocation_h,life_h"),
    ]:
        bad = consume.replace("owned_ptr,owned_allocation,owned_domain);unit", args + ");unit")
        pieces = source.split(consume)
        pieces[arm] += bad
        pieces[1-arm] += consume
        negatives[f"arm{arm}-{kind}"] = "".join(pieces)
    for kind, bad in [
        ("double-receive", consume.replace(";unit", ";{let LiveTail{owned_ptr,owned_allocation,owned_domain}=packet;unit};unit")),
        ("double-end", consume.replace(";unit", ";receive_and_release_tail(owned_ptr,owned_allocation,owned_domain);unit")),
        ("missing-terminal", consume.split(";receive_and_release_tail", 1)[0] + ";unit"),
        ("retains-packet", "unit"),
        ("stale-reloan", consume.replace(";unit", ";loan_read(life_h){|s|let r=ref_from_ptr(read,owned_ptr,s);unit};unit")),
        ("live-dependent-ref", consume.replace("receive_and_release_tail(owned_ptr,owned_allocation,owned_domain);unit", "loan_read(owned_domain){|s|let live=ref_from_ptr(read,owned_ptr,s);receive_and_release_tail(owned_ptr,owned_allocation,owned_domain);unit};unit")),
        ("leaked-scope-ref", consume.replace("receive_and_release_tail", "let leaked=loan_read(owned_domain){|s|ref_from_ptr(read,owned_ptr,s)};receive_and_release_tail")),
    ]:
        pieces = source.split(consume)
        pieces[arm] += bad
        pieces[1-arm] += consume
        negatives[f"arm{arm}-{kind}"] = "".join(pieces)
# A hypothetical Some payload is deliberately Unknown. It cannot discharge
# the tail relation even when the incoming policy is concretely None.
negatives["unknown-policy-payload"] = source.replace("Some(q)=>{" + consume, "Some(q)=>{" + consume.replace("receive_and_release_tail(owned_ptr", "receive_and_release_tail(q"))
negatives["post-join-reconsume"] = source.replace("}};\n", "}};let LiveTail{owned_ptr,owned_allocation,owned_domain}=packet;\n", 1)
expected = {
    "wrong-pointer": "P193-CALL-DOMAIN",
    "wrong-allocation": "P193-CALL-BACKING",
    "wrong-domain": "P193-CALL-DOMAIN",
    "head-owner": "P5-SCOPE-OBLIGATION",
    "double-receive": "P3-USE-AFTER-CONSUME",
    "double-end": "P3-USE-AFTER-CONSUME",
    "missing-terminal": "P5-SCOPE-OBLIGATION",
    "retains-packet": "P219-POSTSTATE-PRECISION",
    "stale-reloan": "P3-STALE-POINTER",
    "live-dependent-ref": "P3-REF-CONFLICT",
    # Current allocated-loan escape path conservatively fails at its semantic
    # module invariant, not a precise escape diagnostic. Report this limit.
    "leaked-scope-ref": "P3-INTERNAL",
    "unknown-policy-payload": "P3-UNKNOWN-PROVENANCE",
    "post-join-reconsume": "P3-USE-AFTER-CONSUME",
}
with tempfile.TemporaryDirectory() as directory:
    root = pathlib.Path(directory)
    for name, text in positives.items():
        path = root / (name + ".nl")
        path.write_text(text)
        for _ in range(2):
            run = subprocess.run([compiler, str(path)], capture_output=True)
            assert run.returncode == 4 and not run.stdout and b"V1-BACKEND-UNSUPPORTED" in run.stderr, (name, run.returncode, run.stderr)
        subprocess.run([evidence, "evidence", str(path)], check=True)
        print(name, hashlib.sha256(text.encode()).hexdigest(), "semantic+owned PASS; backend unsupported; no C")
    for name, text in negatives.items():
        path = root / (name + ".nl")
        path.write_text(text)
        suffix = name.split("-", 1)[1] if name.startswith("arm") else name
        runs = [subprocess.run([compiler, str(path)], capture_output=True) for _ in range(2)]
        for run in runs:
            assert run.returncode == 3 and not run.stdout and expected[suffix].encode() in run.stderr, (name, run.returncode, run.stderr)
        assert runs[0].stderr == runs[1].stderr
        subprocess.run([evidence, "reject", str(path)], check=True)
        if name == "arm1-retains-packet":
            subprocess.run([evidence, "reject-oom", str(path)], check=True)
        print(name, expected[suffix], "source checking rejects before C; public state unchanged")
    assert all(path.suffix == ".nl" for path in root.iterdir()), "unexpected output artifact"
print(f"{len(positives)} existing-source qualified positives; {len(negatives)} arm/continuation negatives; no native or custody claim")
