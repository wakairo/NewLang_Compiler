"""Prospective C103 successor; original checker/manifests remain in parent history."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / "tests/support"))
from versioned_freeze import verify

CURRENT = "tests/p990/current-freeze-v8.json"
# New prospective closure pins the externally fixed original v7 separately;
# versioned_freeze.py and its existing historical algorithm/literals stay intact.
sys.path.insert(0, str(root / "tests/p1030"))
from history_v8 import verify as verify_v8

count = verify_v8(root, CURRENT)
print("P982 original Git/source/oracle lineage and current versioned hashes PASS", count)
