"""Scratch-only coherent v8/v9 rewrite and old P1030 data counterexamples."""
from pathlib import Path
import hashlib
import json
import runpy
import shutil
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / "tests/support"))
from versioned_freeze import FreezeError
from history_v9 import V8, verify

CURRENT = runpy.run_path(str(root / "tests/p982/freeze_test.py"))["CURRENT"]
current = json.loads((root / CURRENT).read_bytes())
sha = lambda b: hashlib.sha256(b).hexdigest()


def write(work, name, value):
    (work / name).write_text(json.dumps(value, indent=2) + "\n")


for fault in ["coedit-predecessor", "drop-old-key", "old-source", "old-oracle", "old-first-RED"]:
    with tempfile.TemporaryDirectory(prefix="p1036-history-") as directory:
        work = Path(directory)
        for name in set(current["files"]) | {CURRENT}:
            target = work / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(root / name, target)
        candidate = json.loads((work / CURRENT).read_bytes())
        if fault == "coedit-predecessor":
            previous = json.loads((work / V8).read_bytes())
            previous["stage"] += " simulated coherent rewrite"
            write(work, V8, previous)
            candidate["previous"]["sha256"] = sha((work / V8).read_bytes())
            candidate["files"][V8] = sha((work / V8).read_bytes())
            expected = "P1036 fixed original-v8 SHA256"
        elif fault == "drop-old-key":
            del candidate["files"]["tests/p1030/oracle-v1.json"]
            expected = "coverage"
        else:
            name = {"old-source": "tests/p1030/sources/pair_before_heap.nl",
                    "old-oracle": "tests/p1030/oracle-v1.json",
                    "old-first-RED": "tests/p1030/evidence/baseline-representative-FIRST/first-C17.stderr"}[fault]
            (work / name).write_bytes((work / name).read_bytes() + b"\n")
            candidate["files"][name] = sha((work / name).read_bytes())
            expected = "P1036 preserved original-v8 data: " + name
        write(work, CURRENT, candidate)
        try:
            verify(work, CURRENT)
        except FreezeError as error:
            assert expected in str(error), (fault, expected, str(error))
            print("P1036 HISTORY REJECT", fault, str(error), flush=True)
        else:
            raise AssertionError("accepted rewritten frozen history: " + fault)
legacy = subprocess.run([sys.executable, Path(__file__).with_name("history_legacy_forward.py")],
                        capture_output=True, timeout=90)
assert legacy.returncode == 0 and not legacy.stderr, (legacy.returncode, legacy.stdout, legacy.stderr)
assert legacy.stdout.count(b"P1030 HISTORY REJECT") == 5
print(legacy.stdout.decode(), end="", flush=True)
print("P1036 history original-v8 external anchor/data/coverage PASS", flush=True)
