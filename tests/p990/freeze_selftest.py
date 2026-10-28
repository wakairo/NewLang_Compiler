"""Counterexamples and a genuine forward-generation control, in scratch copies."""
from pathlib import Path
import hashlib
import json
import runpy
import shutil
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
