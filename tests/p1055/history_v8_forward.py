"""Forward-only v8 counterexample propagation through v9/v10; all five old oracles unchanged."""
from pathlib import Path
import hashlib
import json
import runpy
import shutil
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / "tests/support"))
sys.path.insert(0, str(root / "tests/p1045"))
from versioned_freeze import FreezeError
from history_v9 import V8, verify

CURRENT = runpy.run_path(str(root / "tests/p982/freeze_test.py"))["CURRENT"]
current = json.loads((root / CURRENT).read_bytes())
sha = lambda b: hashlib.sha256(b).hexdigest()


def write(work, name, value):
    (work / name).write_text(json.dumps(value, indent=2) + "\n")


for fault in ["coedit-v8-v9", "drop-original-key", "old-source", "old-oracle", "old-first-RED"]:
    with tempfile.TemporaryDirectory(prefix="p1045-history-") as directory:
        work = Path(directory)
        for name in set(current["files"]) | {CURRENT}:
            target = work / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(root / name, target)
        candidate = json.loads((work / CURRENT).read_bytes())
        if fault == "coedit-v8-v9":
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
            expected = "P1045 fixed original-v8 SHA256"
        elif fault == "drop-original-key":
            del candidate["files"]["tests/p1030/oracle-v1.json"]
            expected = "coverage"
        else:
            name = {"old-source": "tests/p1030/sources/pair_before_heap.nl",
                    "old-oracle": "tests/p1030/oracle-v1.json",
                    "old-first-RED": "tests/p1030/evidence/candidate-v1-FIRST-full-RED/gcc13-full-FIRST.log"}[fault]
            (work / name).write_bytes((work / name).read_bytes() + b"\n")
            candidate["files"][name] = sha((work / name).read_bytes())
            expected = "P1045 preserved original-v8 data: " + name
        write(work, CURRENT, candidate)
        try:
            verify(work, CURRENT)
        except FreezeError as error:
            assert expected in str(error), (fault, expected, str(error))
            print("P1045 HISTORY REJECT", fault, str(error), flush=True)
        else:
            raise AssertionError("accepted rewritten original history: " + fault)
print("P1045 history original-v8 external anchor/data/coverage PASS", flush=True)
