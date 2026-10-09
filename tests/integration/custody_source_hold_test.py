"""Issue #217 partial implementation: definition evidence and precise HOLD.

These tests deliberately do NOT count guarded custody calls as owner-rule
verification, semantic acceptance, native success, or backend unsupported.
"""
import pathlib
import subprocess
import sys
import tempfile

compiler, fixture, evidence = sys.argv[1:]
fixture = pathlib.Path(fixture)
source = fixture.read_text()
spec = (fixture.parents[2] / "docs/reference/NewLang_v0_spec_Draft17_29.md").read_text()
section = spec[spec.index("### 18.1c.4"):]
canonical = section.split("~~~newlang\n", 1)[1].split("\n~~~", 1)[0]
# Only comment-only lines are removed: the current lexer rejects // comments.
# All actual statements, arms, names, declarations, and ordering are unchanged.
tokens = "\n".join(line for line in canonical.splitlines()
                   if not line.lstrip().startswith("//")) + "\n"
assert tokens == source, "primary witness changed beyond comment-only lines"
prefix = source[:source.index("fn main()")]
recipient_start = prefix.index("fn recipient_adopt(")
recipient_end = prefix.index("fn receive_and_release_tail(")
recipient = prefix[recipient_start:recipient_end]

def definition(text):
    return text + "fn main()->unit{unit}\n"

positives = {
    "independent": definition(prefix),
    "alpha-renamed": definition(prefix.replace("sink", "slot")
                                .replace("packet", "owner")
                                .replace("displaced", "previous")),
    "recipient-renamed": definition(prefix.replace("recipient_adopt", "retain_tail")),
    "header-renamed": definition(prefix.replace("Node", "Cell")),
    "main-first": "fn main()->unit{unit}\n" + prefix,
}
negatives = {}
for name, old, new, code in [
    ("store", "= replace(", "= store(", "CUSTODY-DEFINITION-REPLACE"),
    ("wrong-sink", "        sink,", "        packet,", "CUSTODY-DEFINITION-REPLACE"),
    ("wrong-packet", "::Some(packet)", "::Some(sink)", "CUSTODY-DEFINITION-PACKET"),
    ("no-packet", "::Some(packet)", "::None", "CUSTODY-DEFINITION-PACKET"),
    ("wrong-old-value", "match displaced", "match packet", "CUSTODY-DEFINITION-OLD-NONE"),
    ("missing-consumption", "match displaced {\n        None => { unit },\n    };", "unit;", "CUSTODY-DEFINITION-OLD-NONE"),
    ("old-value-discard", "let displaced = ", "", "CUSTODY-DEFINITION-PROFILE"),
    ("some-wildcard", "None => { unit },", "None => { unit }, Some(_) => { unit },", "CUSTODY-DEFINITION-OLD-NONE"),
    ("duplicate-none", "None => { unit },", "None => { unit }, None => { unit },", "CUSTODY-DEFINITION-OLD-NONE"),
    ("ref-escape", "    unit\n}", "    sink\n}", "CUSTODY-DEFINITION-EXIT"),
    ("packet-escape", "    unit\n}", "    packet\n}", "CUSTODY-DEFINITION-EXIT"),
    ("duplicate-packet-use", "    unit\n}", "    packet;\n    unit\n}", "CUSTODY-DEFINITION-EXIT"),
    ("extra-lifecycle-effect", "    unit\n}", "    finalize_domain(packet);\n    unit\n}", "CUSTODY-DEFINITION-EXIT"),
    ("read-only", "ref<write, Option<LiveTail>>", "ref<read, Option<LiveTail>>", "CUSTODY-DEFINITION-WRITE"),
    ("exclusive", "ref<write, Option<LiveTail>>", "exclusive ref<write, Option<LiveTail>>", "CUSTODY-DEFINITION-WRITE"),
    ("shadow-sink", "let displaced", "let sink", "CUSTODY-DEFINITION-NAME"),
]:
    assert old in recipient, name
    changed = prefix[:recipient_start] + recipient.replace(old, new, 1) + prefix[recipient_end:]
    negatives[name] = (definition(changed), code)

holds = {
    "full-primary": source,
    "full-refusal": source.replace("let admit_flag = Option<ptr<Node>>::None;",
                                   "let admit_flag = Option<ptr<Node>>::Some(ptr_h);"),
    "renamed-primary": source.replace("recipient_adopt", "retain_tail"),
}
# Minimal existing-source precision witness, without Option<LiveTail>,
# recipient, or special None-only match. Original producer evidence is lost at
# the new function-match fragment boundary before any richer post-state join.
old = (fixture.parent / "live_tail_return.nl").read_text()
start = old.index("                    let LiveTail {")
end = old.index("                    } = loan_read", start)
fork = old[:start] + "                    let packet" + old[end:].replace("                    } = loan_read", " = loan_read", 1)
start = fork.index("                    loan_read(owned_domain)")
end = fork.index("                    let empty_h =", start)
consume = "let LiveTail{owned_ptr,owned_allocation,owned_domain}=packet;receive_and_release_tail(owned_ptr,owned_allocation,owned_domain);unit"
fork = fork[:start] + "let policy=Option<ptr<Node>>::None;match policy {None=>{" + consume + "},Some(q)=>{" + consume + "}};\n" + fork[end:]
precision = {"fork-origin-none": (fork, "P208-DESTRUCTURE-ORIGIN"),
             "fork-origin-some": (fork.replace("let policy=Option<ptr<Node>>::None;", "let policy=Option<ptr<Node>>::Some(ptr_h);"), "P208-DESTRUCTURE-ORIGIN")}
existing_owner_rejections = {}
for name, args, code in [
    ("wrong-root", "head_w2@next, ptr_h, allocation_t, life_t", "P193-CALL-DOMAIN"),
    ("wrong-backing", "head_w2@next, ptr_t, allocation_h, life_t", "P193-CALL-BACKING"),
    ("wrong-domain", "head_w2@next, ptr_t, allocation_t, life_h", "P3-REF-CONFLICT"),
]:
    existing_owner_rejections[name] = (source.replace("head_w2@next, ptr_t, allocation_t, life_t", args), code)

with tempfile.TemporaryDirectory() as directory:
    root = pathlib.Path(directory)

    def invoke(name, text, code=None):
        path = root / (name + ".nl")
        path.write_text(text)
        runs = [subprocess.run([compiler, str(path)], capture_output=True) for _ in range(2)]
        for run in runs:
            if code is None:
                assert run.returncode == 0 and run.stdout and not run.stderr, (name, run.returncode, run.stderr)
            else:
                assert run.returncode == 3 and not run.stdout and code.encode() in run.stderr, (name, run.returncode, run.stderr)
        assert (runs[0].stdout, runs[0].stderr) == (runs[1].stdout, runs[1].stderr)
        assert all(p.suffix == ".nl" for p in root.iterdir()), "rejection emitted an artifact"
        return path

    for name, text in positives.items():
        path = invoke(name, text)
        subprocess.run([evidence, "evidence", str(path)], check=True)
    for name, (text, code) in negatives.items():
        invoke(name, text, code)
    for name, text in holds.items():
        invoke(name, text, "CUSTODY-ENTRY-PRECISION")
    for name, (text, code) in precision.items():
        invoke(name, text, code)
    for name, (text, code) in existing_owner_rejections.items():
        invoke(name, text, code)

print(f"{len(positives)} independently checked definition positives; "
      f"{len(negatives)} definition-shape negatives; {len(holds)} primary "
      f"custody HOLD controls; {len(precision)} original-packet fork precision "
      f"witnesses; {len(existing_owner_rejections)} preserved producer-entry "
      "owner-rule rejections. Actual custody acceptance/evidence NOT claimed.")
