"""C137 forward v7-v8-v9 co-edit; original five rejection oracles unchanged."""
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
            candidate["files"][V7] = sha((work / V7).read_bytes())
            # Keep this adversarial rewrite coherent through every intervening
            # mutable generation. Otherwise an earlier digest mismatch masks
            # the original fixed-v7 authority check under a newer CURRENT.
            chain = []
            name = candidate["previous"]["path"]
            while name != V7:
                value = json.loads((work / name).read_bytes())
                chain.append((name, value))
                name = value["previous"]["path"]
            updated = V7
            for name, value in reversed(chain):
                value["previous"]["sha256"] = sha((work / updated).read_bytes())
                for previous_name, _ in chain:
                    if previous_name in value["files"]:
                        value["files"][previous_name] = sha((work / previous_name).read_bytes())
                value["files"][V7] = sha((work / V7).read_bytes())
                write(work, name, value)
                updated = name
            candidate["previous"]["sha256"] = sha((work / updated).read_bytes())
            for name, _ in chain:
                candidate["files"][name] = sha((work / name).read_bytes())
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

print("P1045 FORWARD original P1030 five rejection oracles retained with coherent history propagation", flush=True)
