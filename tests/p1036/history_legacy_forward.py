"""Forward scratch fixture for unchanged v7 rejection across later generations.

The original P1030 fixture and five rejection oracles remain immutable. Only
co-edit propagation now walks every intermediate predecessor, so a v9 current
really tests the same historical v7 external anchor rather than a stale link.
"""
from pathlib import Path
import hashlib
import json
import runpy
import shutil
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / "tests/support"))
sys.path.insert(0, str(root / "tests/p1030"))
from versioned_freeze import FreezeError
from history_v8 import V7, verify

CURRENT = runpy.run_path(str(root / "tests/p982/freeze_test.py"))["CURRENT"]
current = json.loads((root / CURRENT).read_bytes())
sha = lambda b: hashlib.sha256(b).hexdigest()


def write(root, name, value):
    (root / name).write_text(json.dumps(value, indent=2) + "\n")


for fault in ["coedit-predecessor", "drop-old-key", "old-source", "old-oracle", "old-first-RED"]:
    with tempfile.TemporaryDirectory(prefix="p1030-history-") as directory:
        work = Path(directory)
        for name in set(current["files"]) | {CURRENT}:
            target = work / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(root / name, target)
        candidate = json.loads((work / CURRENT).read_bytes())
        if fault == "coedit-predecessor":
            previous = json.loads((work / V7).read_bytes())
            previous["stage"] += " simulated coherent rewrite"
            write(work, V7, previous)
            chain = []
            name = CURRENT
            while name != V7:
                value = json.loads((work / name).read_bytes())
                chain.append(name)
                name = value["previous"]["path"]
            changed = [V7]
            for name in reversed(chain):
                value = json.loads((work / name).read_bytes())
                value["previous"]["sha256"] = sha((work / value["previous"]["path"]).read_bytes())
                for old in changed:
                    if old in value["files"]:
                        value["files"][old] = sha((work / old).read_bytes())
                write(work, name, value)
                changed.append(name)
            candidate = json.loads((work / CURRENT).read_bytes())
            expected = "P1030 fixed original-v7 SHA256"
        elif fault == "drop-old-key":
            del candidate["files"]["tests/p1020/oracle-v1.json"]
            expected = "coverage"
        else:
            name = {"old-source": "tests/p1020/sources/two_normal_true.nl",
                    "old-oracle": "tests/p1020/oracle-v1.json",
                    "old-first-RED": "tests/p1020/evidence/candidate-v1/gcc-build-FIRST.log"}[fault]
            (work / name).write_bytes((work / name).read_bytes() + b"\n")
            candidate["files"][name] = sha((work / name).read_bytes())
            expected = "P1030 preserved original-v7 data: " + name
        write(work, CURRENT, candidate)
        try:
            verify(work, CURRENT)
        except FreezeError as error:
            assert expected in str(error), (fault, expected, str(error))
            print("P1030 HISTORY REJECT", fault, str(error), flush=True)
        else:
            raise AssertionError("accepted rewritten frozen history: " + fault)
print("P1030 history original-v7 external anchor/data/coverage PASS", flush=True)
