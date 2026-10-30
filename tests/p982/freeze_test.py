"""Prospective C103 successor; original checker/manifests remain in parent history."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / "tests/support"))
from versioned_freeze import verify

CURRENT = "tests/p990/current-freeze-v9.json"
# New prospective closure pins the externally fixed original v8 separately;
# versioned_freeze.py and its existing historical algorithm/literals stay intact.
sys.path.insert(0, str(root / "tests/p1036"))
from history_v9 import verify as verify_v9

count = verify_v9(root, CURRENT)
print("P982 original Git/source/oracle lineage and current versioned hashes PASS", count)
