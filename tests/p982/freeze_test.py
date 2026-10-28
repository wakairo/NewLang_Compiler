"""Prospective C103 successor; original checker/manifests remain in parent history."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / "tests/support"))
from versioned_freeze import verify

CURRENT = "tests/p990/current-freeze-v5.json"
count = verify(root, CURRENT)
print("P982 original Git/source/oracle lineage and current versioned hashes PASS", count)
