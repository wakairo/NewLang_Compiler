"""#222: actual source, two retained worlds, joined original terminal release."""
import hashlib
import pathlib
import re
import subprocess
import sys
import tempfile

compiler, fixture, evidence = sys.argv[1:]
fixture = pathlib.Path(fixture)
source = fixture.read_text()
consume = "let LiveTail{owned_ptr,owned_allocation,owned_domain}=packet;receive_and_release_tail(owned_ptr,owned_allocation,owned_domain);"
fork = (fixture.parent / "live_tail_packet_fork.nl").read_text()
old = consume + "unit"
assert fork.count(old) == 2
probe = fork.replace(old, "unit")
i = probe.rindex("                    let empty_h =")
assert source == probe[:i] + consume + "\n" + probe[i:], "not the exact existing #221 continuation probe"
policy = "let policy=Option<ptr<Node>>::None;"
match = "match policy {None=>{unit},Some(q)=>{unit}};"
positives = {
    "policy-none": source,
    "policy-some": source.replace(policy, "let policy=Option<ptr<Node>>::Some(ptr_h);"),
    "arms-reversed": source.replace(match, "match policy {Some(q)=>{unit},None=>{unit}};"),
    "alpha-renamed": re.sub(r"\bq\b", "branch_ptr", source.replace("packet", "owner"))
        .replace("receive_and_release_tail", "terminal"),
    "copy-policy-carrier": source.replace(policy, "let initial=Option<ptr<Node>>::Some(ptr_h);let policy=initial;"),
    "independent-shape": source.replace(match, "match policy {None=>{{let local=u8(7);local;unit}},Some(q)=>{{let local=u8(255);local;unit}}};"),
}
negatives = {}
for arm in ("None", "Some(q)"):
    key = arm.split("(")[0]
    for name, body, code in [
        ("release-vs-retain", old, "P219-POSTSTATE-PRECISION"),
        ("move-vs-retain", "let moved=packet;unit", "P5-SCOPE-OBLIGATION"),
        ("receive-vs-retain", consume.split(";receive_and_release_tail")[0] + ";unit", "P5-SCOPE-OBLIGATION"),
        ("wrong-owner", old.replace("owned_ptr,owned_allocation,owned_domain);", "ptr_h,owned_allocation,owned_domain);"), "P193-CALL-DOMAIN"),
        ("head-state-change", "let lost=allocation_h;unit", "P5-SCOPE-OBLIGATION"),
        ("head-fact-change", "loan_read(life_h){|s|let r=ref_from_ptr(write,ptr_h,s);replace(r@next,Option<ptr<Node>>::None);unit};unit", "P219-POSTSTATE-PRECISION"),
    ]:
        negatives[key + "-" + name] = (source.replace(arm + "=>{unit}", arm + "=>{" + body + "}"), code)
for name, changed, code in [
    ("wrong-root", consume.replace("(owned_ptr,", "(ptr_h,"), "P193-CALL-DOMAIN"),
    ("wrong-backing", consume.replace("receive_and_release_tail(owned_ptr,owned_allocation,owned_domain)", "receive_and_release_tail(owned_ptr,allocation_h,owned_domain)"), "P193-CALL-BACKING"),
    ("wrong-domain", consume.replace("receive_and_release_tail(owned_ptr,owned_allocation,owned_domain)", "receive_and_release_tail(owned_ptr,owned_allocation,life_h)"), "P193-CALL-DOMAIN"),
    ("missing-late-release", consume.split(";receive_and_release_tail")[0]+";", "P5-SCOPE-OBLIGATION"),
    ("double-late-receive", consume + "{" + consume + "unit};", "P3-USE-AFTER-CONSUME"),
    ("double-late-release", consume + "receive_and_release_tail(owned_ptr,owned_allocation,owned_domain);", "P3-USE-AFTER-CONSUME"),
    ("stale-reloan", consume + "loan_read(life_h){|s|let r=ref_from_ptr(read,owned_ptr,s);unit};", "P3-STALE-POINTER"),
    ("live-dependent-ref", consume.replace("receive_and_release_tail(owned_ptr,owned_allocation,owned_domain);", "loan_read(owned_domain){|s|let r=ref_from_ptr(read,owned_ptr,s);receive_and_release_tail(owned_ptr,owned_allocation,owned_domain);unit};"), "P3-REF-CONFLICT"),
    ("scope-escape", consume.replace("receive_and_release_tail", "let escaped=loan_read(owned_domain){|s|ref_from_ptr(read,owned_ptr,s)};receive_and_release_tail"), "P3-INTERNAL"),
]:
    assert consume in source
    negatives[name] = (source.replace(consume, changed), code)
negatives["active-fork-scope"] = (source.replace(match, "loan_read(life_h){|s|" + match + "unit};"), "P219-FORK-ENTRY-PRECISION")
negatives["missing-head-release"] = (source.replace("deallocate(allocation_h, full_h);", "unit;"), "P5-SCOPE-OBLIGATION")

with tempfile.TemporaryDirectory() as directory:
    root = pathlib.Path(directory)
    for name, text in positives.items():
        path = root / (name + ".nl")
        path.write_text(text)
        for _ in range(2):
            run = subprocess.run([compiler, str(path)], capture_output=True)
            assert run.returncode == 4 and not run.stdout and b"V1-BACKEND-UNSUPPORTED" in run.stderr, (name, run.returncode, run.stderr)
        subprocess.run([evidence, "evidence", str(path)], check=True)
        print(name, hashlib.sha256(text.encode()).hexdigest(), "source registration + actual call + joined owned release PASS; backend unsupported")
    for name, (text, code) in negatives.items():
        path = root / (name + ".nl")
        path.write_text(text)
        runs = [subprocess.run([compiler, str(path)], capture_output=True) for _ in range(2)]
        for run in runs:
            assert run.returncode == 3 and not run.stdout and code.encode() in run.stderr, (name, run.returncode, run.stderr)
        assert runs[0].stderr == runs[1].stderr
        subprocess.run([evidence, "reject", str(path)], check=True)
        if name == "Some-release-vs-retain":
            subprocess.run([evidence, "reject-oom", str(path)], check=True)
        print(name, code, "NewLang rejection; no C; public source-created carrier unchanged")
    assert all(p.suffix == ".nl" for p in root.iterdir())
print(f"{len(positives)} actual-source owned positives; {len(negatives)} destructive negatives; no custody/native claim")
