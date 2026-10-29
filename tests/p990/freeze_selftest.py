"""Counterexamples and a genuine forward-generation control, in scratch copies."""
from pathlib import Path
from contextlib import contextmanager
import base64
import hashlib
import json
import runpy
import shutil
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / "tests/support"))
from versioned_freeze import FreezeError, verify

current_name = runpy.run_path(str(root / "tests/p982/freeze_test.py"))["CURRENT"]
current_bytes = (root / current_name).read_bytes()
current = json.loads(current_bytes)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def rejected(work, manifest, text):
    try:
        verify(work, manifest)
    except FreezeError as error:
        assert text in str(error), (text, str(error))
        print("P990 freeze counterexample REJECTED", text)
        return str(error)
    else:
        raise AssertionError("freeze accepted deliberate mutation: " + text)


with tempfile.TemporaryDirectory(prefix="p990-freeze-") as temp:
    work = Path(temp)
    for name in set(current["files"]) | {current_name}:
        target = work / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(root / name, target)
    verify(work, current_name)
    print("P990 freeze current-generation control PASS")

    protected = "tests/p982/sources/after_true.nl"
    original = (work / protected).read_bytes()
    (work / protected).write_bytes(original + b"\n")
    rejected(work, current_name, "original Git blob mismatch")
    (work / protected).write_bytes(original)

    wrong = json.loads(current_bytes)
    wrong["files"]["src/checked_c_copy_internal.h"] = "0" * 64
    (work / current_name).write_text(json.dumps(wrong))
    rejected(work, current_name, "current freeze digest: src/checked_c_copy_internal.h")
    (work / current_name).write_bytes(current_bytes)

    archive = "tests/p990/lineage/original/CMakeLists.txt.data"
    original_archive = (work / archive).read_bytes()
    (work / archive).write_bytes(original_archive + b"\n")
    rejected(work, current_name, "original Git blob mismatch")
    (work / archive).write_bytes(original_archive)

    changed = "src/checked_c_node.c"
    (work / changed).write_bytes((work / changed).read_bytes() + b"\n/* next generation control */\n")
    rejected(work, current_name, "current freeze digest: " + changed)
    next_name = "tests/p990/protocol-selftest-next.json"
    wrapper = "tests/p982/freeze_test.py"
    (work / wrapper).write_text((work / wrapper).read_text().replace(current_name, next_name))
    future = json.loads(current_bytes)
    future["generation"] += 1
    future["previous"] = {"path": current_name, "sha256": sha(current_bytes)}
    future["files"][current_name] = sha(current_bytes)
    for name in [changed, wrapper]:
        future["files"][name] = sha((work / name).read_bytes())
    (work / next_name).write_text(json.dumps(future))
    verify(work, next_name)
    print("P990 freeze forward-generation production-change control PASS")

    bad = json.loads(json.dumps(future))
    bad["files"][changed] = "0" * 64
    (work / next_name).write_text(json.dumps(bad))
    rejected(work, next_name, "current freeze digest: " + changed)

    bad = json.loads(json.dumps(future))
    bad["previous"]["sha256"] = "0" * 64
    (work / next_name).write_text(json.dumps(bad))
    rejected(work, next_name, "previous freeze digest")

    # A future digest cannot launder an immutable source change.
    (work / protected).write_bytes(original + b"\n")
    bad = json.loads(json.dumps(future))
    bad["files"][protected] = sha((work / protected).read_bytes())
    (work / next_name).write_text(json.dumps(bad))
    rejected(work, next_name, "original Git blob mismatch")
print("P990 versioned freeze: two controls and seven deliberate failures PASS")

# C107 forward-only additions. The original seven negative expectations and
# controls above stay intact; no historic manifest or first RED is rewritten.
oracle = json.loads((root / "tests/p990/history-repair/oracle-v1.json").read_text())
originals = {}
for row in oracle["reviewed_generations"]:
    data = (root / row["path"]).read_bytes()
    assert sha(data) == row["sha256"]
    assert hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest() == row["oid"]
    originals[row["path"]] = data
print("P999 CONTROL reviewed-original-v1-v2-v3 PASS")


@contextmanager
def scratch():
    with tempfile.TemporaryDirectory(prefix="p999-freeze-") as temp:
        work = Path(temp)
        for name in set(current["files"]) | {current_name}:
            target = work / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(root / name, target)
        yield work


def write(work, name, value):
    (work / name).write_text(json.dumps(value, indent=2) + "\n")
    return sha((work / name).read_bytes())


def forward(work, name, previous_name=current_name, changed=(), removed=()):
    data = (work / previous_name).read_bytes()
    value = json.loads(data)
    value["generation"] += 1
    value["previous"] = {"path": previous_name, "sha256": sha(data)}
    value["stage"] = "C107 scratch forward-generation control"
    value["files"][previous_name] = sha(data)
    for path in changed:
        value["files"][path] = sha((work / path).read_bytes())
    for path in removed:
        del value["files"][path]
    write(work, name, value)
    return name


next_name = "tests/p990/protocol-c107-v5.json"
header = "src/checked_c_copy_internal.h"
wrapper = "tests/p982/freeze_test.py"
with scratch() as work:
    for name, comment in [(header, b"\n/* C107 legitimate future code */\n"),
                          ("CMakeLists.txt", b"\n# C107 legitimate future CMake\n")]:
        (work / name).write_bytes((work / name).read_bytes() + comment)
    (work / wrapper).write_text((work / wrapper).read_text().replace(current_name, next_name))
    selected = forward(work, next_name, changed=(header, "CMakeLists.txt", wrapper))
    verify(work, selected)
    assert all((work / name).read_bytes() == data for name, data in originals.items())
    print("P999 CONTROL v5-new-production-and-CMake-original-history-retained PASS")

with scratch() as work:
    forward(work, next_name)
    sixth = forward(work, "tests/p990/protocol-c107-v6.json", next_name)
    verify(work, sixth)
    print("P999 CONTROL intermediate-generation-coverage-retained PASS")


def coherent(work, first=1, first_path_swap=False, claimed_anchor=False):
    changed = {}
    # Keep the old oracle immutable, but include newly historical intermediates.
    chain = []
    name = current["previous"]["path"]
    while name:
        data = (work / name).read_bytes()
        doc = json.loads(data)
        chain.append((doc["generation"], name, data))
        name = doc["previous"]["path"] if doc["previous"] else None
    for generation, name, data in reversed(chain):
        if generation < first:
            continue
        value = json.loads(data)
        if generation == first:
            if first_path_swap:
                alias = "tests/p990/protocol-c107-first-alias.json"
                (work / alias).write_bytes(originals[oracle["reviewed_generations"][0]["path"]])
                value["previous"]["path"] = alias
                value["files"][alias] = sha((work / alias).read_bytes())
            else:
                value["files"][header] = "0" * 64
        if value["previous"] and value["previous"]["path"] in changed:
            value["previous"]["sha256"] = changed[value["previous"]["path"]]
        for prior, expected in changed.items():
            value["files"][prior] = expected
        changed[name] = write(work, name, value)
    value = json.loads(current_bytes)
    value["previous"]["sha256"] = changed[value["previous"]["path"]]
    value["files"].update(changed)
    if claimed_anchor:
        value["baseline_anchors"] = changed
    write(work, current_name, value)
    # These are coherent linked data, not the old single-zero predecessor test.
    for name in [*changed, current_name]:
        doc = json.loads((work / name).read_bytes())
        previous = doc["previous"]
        if previous:
            assert sha((work / previous["path"]).read_bytes()) == previous["sha256"]
            assert doc["files"][previous["path"]] == previous["sha256"]
    assert sha((work / header).read_bytes()) == current["files"][header]
    return current_name


new_results = []


def counterexample(label, mutate):
    with scratch() as work:
        result = mutate(work)
        selected = result if isinstance(result, str) else current_name
        message = rejected(work, selected, oracle["negatives"][label])
        row = {"case": label, "status": "REJECT", "error": message}
        if label == "f81-coherent-v1-v2-v3":
            sentinel = work / "c107-named-output.bin"
            sentinel.write_bytes(b"C107 EXISTING OUTPUT MUST SURVIVE\n")
            before = sentinel.stat()
            run = subprocess.run([sys.executable, str(work / wrapper)], cwd=work,
                                 capture_output=True, text=True, timeout=15)
            assert run.returncode == 1 and not run.stdout, (run.returncode, run.stdout)
            assert "historical freeze SHA256" in run.stderr
            after = sentinel.stat()
            assert sentinel.read_bytes() == b"C107 EXISTING OUTPUT MUST SURVIVE\n"
            assert (before.st_dev, before.st_ino) == (after.st_dev, after.st_ino)
            row.update({"CLI_exit": run.returncode, "CLI_stdout": "empty",
                        "named_bytes_dev_ino": "unchanged"})
        new_results.append(row)
        print("P999 CASE", json.dumps(row, sort_keys=True))


counterexample("f81-coherent-v1-v2-v3", lambda work: coherent(work))
counterexample("past-v2-coherent-successors", lambda work: coherent(work, first=2))
counterexample("past-v3-coherent-successor", lambda work: coherent(work, first=3))
counterexample("mutable-anchor-claim", lambda work: coherent(work, claimed_anchor=True))
counterexample("coherent-first-path-swap",
               lambda work: coherent(work, first=2, first_path_swap=True))


def first_alias(work):
    name = "tests/p990/protocol-c107-first-alias.json"
    (work / name).write_bytes(originals[oracle["reviewed_generations"][0]["path"]])
    return name


counterexample("first-generation-path-alias", first_alias)


def self_claim(work):
    value = json.loads(current_bytes)
    value["baseline_anchors"] = {row["path"]: row["sha256"]
                                for row in oracle["reviewed_generations"]}
    write(work, current_name, value)


counterexample("mutable-anchor-field", self_claim)
for label, path in oracle["coverage_drops"].items():
    counterexample(label, lambda work, path=path: forward(work, next_name, removed=(path,)))


def intermediate_drop(work):
    forward(work, next_name, removed=("tests/p990/observer.h",))
    sixth = forward(work, "tests/p990/protocol-c107-v6.json", next_name)
    value = json.loads((work / sixth).read_bytes())
    value["files"]["tests/p990/observer.h"] = current["files"]["tests/p990/observer.h"]
    write(work, sixth, value)
    return sixth


counterexample("intermediate-drop-restored-only-in-current", intermediate_drop)


def changed_input(work, path):
    (work / path).write_bytes((work / path).read_bytes() + b"\n")
    return forward(work, next_name, changed=(path,))


counterexample("changed-v2-source-with-current-digest",
               lambda work: changed_input(work, "tests/p990/sources/negative_additional_return_v2.nl"))
counterexample("changed-v2-oracle-with-current-digest",
               lambda work: changed_input(work, "tests/p990/oracle-v2.json"))
counterexample("changed-original-first-RED-with-current-digest",
               lambda work: changed_input(work, "tests/p990/evidence/candidate1-lineage-FIRST.stderr"))
counterexample("changed-T997-runner-with-current-digest",
               lambda work: changed_input(work, "cmake/independent-r-manifest.py"))
counterexample("missing-old-support",
               lambda work: (work / "tests/p990/lineage/original/CMakeLists.txt.data").unlink())


def bad_parent(work, commit=False):
    name = "tests/p990/lineage/parent-snapshot-v1.json"
    value = json.loads((work / name).read_bytes())
    if commit:
        data = base64.b64decode(value["commit_base64"]) + b"\n"
        value["commit_base64"] = base64.b64encode(data).decode()
    else:
        value["parent"] = "0" * 40
    write(work, name, value)


counterexample("unmatched-P982-parent-OID", bad_parent)
counterexample("unmatched-retained-parent-commit", lambda work: bad_parent(work, commit=True))


def symlink_old(work):
    name = oracle["reviewed_generations"][0]["path"]
    alias = work / "c107-original-v1.json"
    alias.write_bytes((work / name).read_bytes())
    (work / name).unlink()
    (work / name).symlink_to(alias)


counterexample("symlink-original-generation", symlink_old)


def symlink_parent(work):
    (work / "tests/p990").rename(work / "c107-relocated")
    (work / "tests/p990").symlink_to("../c107-relocated", target_is_directory=True)


counterexample("symlink-intermediate-directory", symlink_parent)


def nonregular(work):
    path = work / "tests/p990/observer.h"
    path.unlink()
    path.mkdir()


counterexample("nonregular-current-input", nonregular)
counterexample("invalid-current-JSON", lambda work: (work / current_name).write_bytes(b"{"))
counterexample("duplicate-current-JSON-key",
               lambda work: (work / current_name).write_bytes(b'{"schema":1,' + current_bytes.lstrip()[1:]))


def malicious_path(work, path):
    value = json.loads(current_bytes)
    value["files"][path] = "0" * 64
    write(work, current_name, value)


for label, path in oracle["malicious_paths"].items():
    counterexample(label, lambda work, path=path: malicious_path(work, path))


def bad_sha(work):
    value = json.loads(current_bytes)
    value["files"][header] = "0" * 63
    write(work, current_name, value)


counterexample("invalid-current-digest-format", bad_sha)
assert {row["case"] for row in new_results} == oracle["negatives"].keys()
print("P999 C107 provenance history: 3 new controls and", len(new_results),
      "new deliberate rejections PASS; original 2 controls/7 rejections retained")

# C111 adds only the finite original P982 evidence obligation found by F82.
p1011 = json.loads((root / "tests/p990/p1011/oracle-v1.json").read_bytes())
assert len(p1011["cohort"]) == 22
for row in p1011["reviewed_generations"]:
    data = (root / row["path"]).read_bytes()
    assert sha(data) == row["sha256"]
    assert hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest() == row["oid"]
for name, expected in p1011["cohort"].items():
    assert sha((root / name).read_bytes()) == expected
assert current["generation"] >= 5
print("P1011 CONTROL honest-v5-original-history-and-evidence PASS")

p1011_next = f"tests/p990/p1011-future-v{current['generation'] + 1}.json"
with scratch() as work:
    changes = (header, "CMakeLists.txt", "src/checked_c_node.c")
    for name in changes:
        suffix = b"\n# P1011 future control\n" if name == "CMakeLists.txt" else b"\n/* P1011 future control */\n"
        (work / name).write_bytes((work / name).read_bytes() + suffix)
    (work / wrapper).write_text((work / wrapper).read_text().replace(current_name, p1011_next))
    forward(work, p1011_next, changed=(*changes, wrapper))
    verify(work, p1011_next)
    assert all(sha((work / name).read_bytes()) == expected
               for name, expected in p1011["cohort"].items())
    print("P1011 CONTROL legitimate-v6-current-header-CMake-proof-consumer-update PASS")

p1011_results = []
first_red = p1011["first_red"]["path"]


def p1011_rejected(label, mutate, expected, cli=False):
    with scratch() as work:
        result = mutate(work)
        selected = result if isinstance(result, str) else current_name
        error = rejected(work, selected, expected)
        row = {"case": label, "status": "REJECT", "error": error}
        if cli:
            sentinel = work / "p1011-existing-output.bin"
            original = b"P1011 KEEP EXISTING OUTPUT\n"
            sentinel.write_bytes(original)
            before = sentinel.stat()
            run = subprocess.run([sys.executable, str(work / wrapper)], cwd=work,
                                 capture_output=True, text=True, timeout=15)
            assert run.returncode == 1 and not run.stdout
            assert expected in run.stderr
            after = sentinel.stat()
            assert sentinel.read_bytes() == original
            assert (before.st_dev, before.st_ino) == (after.st_dev, after.st_ino)
            row.update(CLI_exit=run.returncode, CLI_stdout="empty",
                       named_bytes_dev_ino="unchanged")
        p1011_results.append(row)
        print("P1011 CASE", json.dumps(row, sort_keys=True))


def coedit_evidence(work, name, future=False):
    data = (work / name).read_bytes()
    if name == first_red:
        records = json.loads(data)
        records[0]["kind"] = "P1011 SIMULATED COHERENT HISTORY REWRITE"
        changed = (json.dumps(records, indent=2) + "\n").encode()
    else:
        changed = data + b"\n"
    assert sha(data) != sha(changed)
    (work / name).write_bytes(changed)
    if future:
        return forward(work, p1011_next, changed=(name,))
    value = json.loads(current_bytes)
    value["files"][name] = sha(changed)
    write(work, current_name, value)
    return current_name


p1011_rejected("coedited-current-v5", lambda work: coedit_evidence(work, first_red),
               p1011["candidate_negatives"]["coedited_current_v5"], cli=True)
for name in p1011["cohort"]:
    expected = ("original P982 first RED changed" if name == first_red else
                "original P982 evidence changed: " + name)
    p1011_rejected("coedited-future-v6:" + name,
                   lambda work, name=name: coedit_evidence(work, name, future=True), expected)

p1011_rejected("historical-v4-coherent-update", lambda work: coherent(work, first=4),
               p1011["candidate_negatives"]["historical_v4_coherent_update"])


def drop_first_red(work):
    value = json.loads(current_bytes)
    del value["files"][first_red]
    write(work, current_name, value)


p1011_rejected("missing-first-red-key", drop_first_red,
               p1011["candidate_negatives"]["missing_first_red_key"])


def first_red_symlink(work):
    alias = work / "p1011-first-red-alias.json"
    alias.write_bytes((work / first_red).read_bytes())
    (work / first_red).unlink()
    (work / first_red).symlink_to(alias)


p1011_rejected("first-red-symlink", first_red_symlink,
               p1011["candidate_negatives"]["first_red_symlink"])


def first_red_nonregular(work):
    (work / first_red).unlink()
    (work / first_red).mkdir()


p1011_rejected("first-red-nonregular", first_red_nonregular,
               p1011["candidate_negatives"]["first_red_nonregular"])
assert len(p1011_results) == 27
print("P1011 C111: 2 controls and 27 deliberate rejections PASS; old expectations retained")

# The independently reviewed original v5 becomes history only in C112's v6.
# Do not let a successor coherently replace that original historical manifest.
with scratch() as work:
    selected = coherent(work, first=5)
    rejected(work, selected, "historical freeze SHA256: tests/p990/current-freeze-v5.json")
print("P1013 C112 original-v5 external-anchor counterexample REJECTED")
