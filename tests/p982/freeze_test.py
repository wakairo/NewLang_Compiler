"""Prospective C103 successor; original checker/manifests remain in parent history."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / "tests/support"))
from versioned_freeze import verify

CURRENT = "tests/p990/current-freeze-v11.json"
# Prospective v9 pins the externally fixed original v9 after the unchanged
# v1-v8 historical algorithms/literals; predecessor source/data stay intact.
sys.path.insert(0, str(root / "tests/p1058"))
from history_v11 import verify as verify_v11

count = verify_v11(root, CURRENT)
print("P982 original Git/source/oracle lineage and current versioned hashes PASS", count)
