"""Issue 294: immutable R reproductions and the ended-shell cleanup seam."""
import hashlib
import json
import pathlib
import re
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
FIXTURES = ROOT / "tests/fixtures/p294"
compiler, boundary, owned = (str(pathlib.Path(x).resolve()) for x in sys.argv[1:4])
alloc = (FIXTURES / "mixed_alloc_repaired_5.nl").read_text()
domain = (FIXTURES / "mixed_domain_repaired_5.nl").read_text()
assert hashlib.sha256(alloc.encode()).hexdigest() == (
    "48f19bbd6ddd3b8ae1057a4f35a0fff95963c4af2930a9f44d9d5087ed5f12db"
)
assert hashlib.sha256(domain.encode()).hexdigest() == (
    "870516e459cfe85eb59d9ecf05af2f8481cdd7e2fe254e4e92d8935c34f6c4e5"
)


def change(source, old, new):
    assert source.count(old) == 1, old
    return source.replace(old, new)


def identity(source):
    result, count = re.subn(
        r"fn exchange\(v:Vessel\)->Vessel \{.*?\n\}\n",
        "fn exchange(v:Vessel)->Vessel { return v; }\n",
        source,
        count=1,
        flags=re.S,
    )
    assert count == 1
    return result


def direct_release(source):
    start = source.index("let LiveRoot {p:pr0")
    end = source.index("retire(x2);", start)
    return source[:start] + "retire(x0);\nretire(x1);\n" + source[end:]


two = (FIXTURES / "rebuild_first_2.nl").read_text()
one = (FIXTURES / "rebuild_first_1.nl").read_text()
cases = [
    ("R291-allocation-repaired", alloc, "precision", "CAPTURED-CLOSURE-PRECISION", True),
    ("R291-domain-repaired", domain, "precision", "CAPTURED-CLOSURE-PRECISION", True),
    ("R291-wrong-allocation-release", direct_release(alloc), "semantic", "P193-CALL-BACKING", False),
    ("R291-wrong-domain-release", direct_release(domain), "semantic", "P193-CALL-DOMAIN", False),
    ("five-matched-identity", direct_release(identity(alloc)), "cli", "V1-BACKEND-UNSUPPORTED", True),
    ("one-rebuild-control", one, "cli", "V1-BACKEND-UNSUPPORTED", True),
    ("two-rebuild-minimum", two, "cli", "V1-BACKEND-UNSUPPORTED", True),
]
# A mixed packet through the existing identity profile, then caller repair.
# This is distinct from R's callee rebuilding a DIFFERENT nominal packet ID.
mixed_identity = identity(alloc)
mixed_identity = change(mixed_identity, "p: p_0, a: a_0, d: d_0", "p: p_0, a: a_1, d: d_0")
mixed_identity = change(mixed_identity, "p: p_1, a: a_1, d: d_1", "p: p_1, a: a_0, d: d_1")
cases.append(("five-mixed-identity-caller-repair", mixed_identity, "cli", "V1-BACKEND-UNSUPPORTED", True))

for component in ("allocation", "domain"):
    rebuilt = change(two, "return Vessel {", "let LiveRoot {p:p_second,a:a_second,d:d_second}=second;\nreturn Vessel {")
    rebuilt = change(rebuilt, "second:second", "second:LiveRoot{p:p_second,a:a_second,d:d_second}")
    left = "a:a_second,d:d_first" if component == "allocation" else "a:a_first,d:d_second"
    right = "a:a_first,d:d_second" if component == "allocation" else "a:a_second,d:d_first"
    rebuilt = change(rebuilt, "first:LiveRoot{p:p_first,a:a_first,d:d_first}", "first:LiveRoot{p:p_first," + left + "}")
    rebuilt = change(rebuilt, "second:LiveRoot{p:p_second,a:a_second,d:d_second}", "second:LiveRoot{p:p_second," + right + "}")
    repair = """let LiveRoot {p:p_left,a:a_left,d:d_left}=keeper_0;
let LiveRoot {p:p_right,a:a_right,d:d_right}=keeper_1;
let repaired_left=LiveRoot{p:p_left,a:%s,d:%s};
let repaired_right=LiveRoot{p:p_right,a:%s,d:%s};
retire(repaired_left);
retire(repaired_right);""" % (("a_right", "d_left", "a_left", "d_right") if component == "allocation" else ("a_left", "d_right", "a_right", "d_left"))
    repaired = change(rebuilt, "retire(keeper_0);\nretire(keeper_1);", repair)
    code = "P193-CALL-BACKING" if component == "allocation" else "P193-CALL-DOMAIN"
    cases.extend([
        ("two-mixed-" + component + "-repaired", repaired, "cli", "V1-BACKEND-UNSUPPORTED", True),
        ("two-wrong-" + component + "-release", rebuilt, "semantic", code, False),
    ])

alpha = two
for old, new in [("Node", "Cell"), ("LiveRoot", "Parcel"), ("Vessel", "Crate"), ("transit", "repack"), ("retire", "release"), ("first", "left"), ("second", "right"), ("p", "pointer"), ("a", "backing"), ("d", "lifetime")]:
    alpha = re.sub(r"\b" + old + r"\b", new, alpha)
cases.append(("two-alpha-members-functions", alpha, "cli", "V1-BACKEND-UNSUPPORTED", True))
duplicate = change(two, "retire(keeper_1);", "retire(keeper_1);\nretire(keeper_1);")
cases.append(("two-receiver-duplicate", duplicate, "semantic", "P3-USE-AFTER-CONSUME", False))
duplicate_donor = change(two, "let shipped = transit(vessel);", "let shipped = transit(vessel);\nlet duplicate=vessel;")
cases.append(("two-donor-after-transfer", duplicate_donor, "semantic", "P3-USE-AFTER-CONSUME", False))
borrowed = change(alloc, "let corrected_0=LiveRoot{p:pr0,a:al1,d:dm0};", "let corrected_0=loan_read(dm0){|held|LiveRoot{p:pr0,a:al1,d:dm0}};")
cases.append(("five-repair-move-during-loan", borrowed, "semantic", "P3-REF-CONFLICT", False))

with tempfile.TemporaryDirectory(prefix="p294-repair-") as directory:
    directory = pathlib.Path(directory)
    for name, source, category, code, probe in cases:
        path = directory / (name + ".nl")
        path.write_text(source)
        before = set(directory.iterdir())
        results = [subprocess.run([compiler, str(path)], capture_output=True, timeout=90) for _ in range(2)]
        result = results[0]
        assert (result.returncode, result.stdout, result.stderr) == (results[1].returncode, results[1].stdout, results[1].stderr), name
        assert result.returncode == (4 if category == "cli" else 3) and not result.stdout, (name, result.stderr)
        assert ("error(" + category + ")[" + code + "]").encode() in result.stderr, (name, result.stderr)
        assert b"P3-INTERNAL" not in result.stderr and set(directory.iterdir()) == before, name
        r = subprocess.run([boundary, str(path)], capture_output=True, timeout=90)
        assert r.returncode == 0 and not r.stderr, (name, r.stderr)
        view = json.loads(r.stdout)
        assert view["parse_status"] == 0 and view["registration_status"] == (0 if category == "cli" else 1 if category == "semantic" else 3), (name, view)
        if category != "cli":
            assert view["snapshot_unchanged"] and view["code"] == code and not view["owned_entry_artifact"], name
        evidence = None
        if probe:
            r = subprocess.run([owned, str(path), "accepted" if category == "cli" else code], capture_output=True, timeout=90)
            assert r.returncode == 0 and not r.stderr, (name, r.stdout, r.stderr)
            evidence = r.stdout.decode()
        print(json.dumps({"case": name, "source_sha256": hashlib.sha256(source.encode()).hexdigest(), "cli_exit": result.returncode, "category": category, "diagnostic": result.stderr.decode(), "public_boundary": view, "owned_evidence": evidence}), flush=True)
