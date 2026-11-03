"""Prospective C103 successor; original checker/manifests remain in parent history."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / "tests/support"))
from versioned_freeze import verify

CURRENT = "tests/p990/current-freeze-v27.json"
# Prospective v9 pins the externally fixed original v9 after the unchanged
# v1-v8 historical algorithms/literals; predecessor source/data stay intact.
sys.path.insert(0, str(root / "tests/p1062"))
from history_v12 import verify as verify_v12
sys.path.insert(0, str(root / "tests/p1098"))
from history_v20 import verify as verify_v20

sys.path.insert(0, str(root / "tests/p1107"))
from history_v21 import verify as verify_v21
sys.path.insert(0, str(root / "tests/p1117"))
from history_v22 import verify as verify_v22
from history_v23 import verify as verify_v23
from history_v24 import verify as verify_v24
from history_v25 import verify as verify_v25
from history_v26 import verify as verify_v26
from history_v27 import verify as verify_v27
count = verify_v27(root, CURRENT)
print("P982 original Git/source/oracle lineage and current versioned hashes PASS", count)
