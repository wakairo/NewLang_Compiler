"""Forward-only v8 counterexample propagation through v9/v10; all five old oracles unchanged."""
from pathlib import Path
import hashlib
import json
import runpy
import shutil
import sys
import tempfile
import zipfile

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / "tests/support"))
sys.path.insert(0, str(root / "tests/p1058"))
from versioned_freeze import FreezeError
from history_v12 import V11 as V8, verify

CURRENT = runpy.run_path(str(root / "tests/p982/freeze_test.py"))["CURRENT"]
current = json.loads((root / CURRENT).read_bytes())
# The new source freeze remains byte-identical; v1's full immutable input
# snapshot is packed as data, with each member checked against its original map.
source = json.loads((root / "tests/p1062/source-freeze-v1.json").read_bytes())
for name, expected in source["files"].items():
    assert hashlib.sha256((root / name).read_bytes()).hexdigest() == expected, name
original = json.loads((root / "tests/p1062/harness-freeze-v1.json").read_bytes())
with zipfile.ZipFile(root / "tests/p1062/evidence/pre-run-v1-inputs.zip") as archive:
    assert set(archive.namelist()) == set(original["files"])
    for name, expected in original["files"].items():
        assert hashlib.sha256(archive.read(name)).hexdigest() == expected, name
print("P1062 source freeze and archived first harness inputs PASS", flush=True)
sha = lambda b: hashlib.sha256(b).hexdigest()


def write(work, name, value):
    (work / name).write_text(json.dumps(value, indent=2) + "\n")


for fault in ["coedit-v11-v12", "drop-original-key", "old-source", "old-oracle", "old-first-RED"]:
    with tempfile.TemporaryDirectory(prefix="p1045-history-") as directory:
        work = Path(directory)
        for name in set(current["files"]) | {CURRENT}:
            target = work / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(root / name, target)
        candidate = json.loads((work / CURRENT).read_bytes())
        if fault == "coedit-v11-v12":
            previous = json.loads((work / V8).read_bytes())
            previous["stage"] += " simulated coherent history rewrite"
            write(work, V8, previous)
            chain = []
            name = CURRENT
            while name != V8:
                value = json.loads((work / name).read_bytes())
                chain.append(name)
                name = value["previous"]["path"]
            changed = [V8]
            for name in reversed(chain):
                value = json.loads((work / name).read_bytes())
                value["previous"]["sha256"] = sha((work / value["previous"]["path"]).read_bytes())
                for old in changed:
                    if old in value["files"]:
                        value["files"][old] = sha((work / old).read_bytes())
                write(work, name, value)
                changed.append(name)
            candidate = json.loads((work / CURRENT).read_bytes())
            expected = "P1062 fixed original-v11 SHA256"
        elif fault == "drop-original-key":
            del candidate["files"]["tests/p1058/oracle-v1.json"]
            expected = "coverage"
        else:
            name = {"old-source": "tests/p1058/sources/closed_then_two.nl",
                    "old-oracle": "tests/p1058/oracle-v1.json",
                    "old-first-RED": "tests/p1058/evidence/parent-FIRST/closed_then_two-C17.stderr"}[fault]
            (work / name).write_bytes((work / name).read_bytes() + b"\n")
            candidate["files"][name] = sha((work / name).read_bytes())
            expected = "P1062 preserved original-v11 data: " + name
        write(work, CURRENT, candidate)
        try:
            verify(work, CURRENT)
        except FreezeError as error:
            assert expected in str(error), (fault, expected, str(error))
            print("P1062 HISTORY REJECT", fault, str(error), flush=True)
        else:
            raise AssertionError("accepted rewritten original history: " + fault)
print("P1062 history original-v11 external anchor/data/coverage PASS", flush=True)
