"""C256 prospective metadata repair; failed v50 and full-GCC FIRST stay exact."""
from pathlib import Path
import sys

V50 = "tests/p990/current-freeze-v50.json"
SHA256 = "9a4f881a3b3f882977981c1d73d380dcee2ca0a3a8c5a0a9c872b5d1893df86b"
OID = "9145231a3ca9301d6a72a7cae8a1b14f721ee6e8"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1193"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v50 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V50).read_bytes()
    require(digest(raw) == SHA256, "C256 fixed original-v50 SHA256")
    require(git_oid("blob", raw) == OID, "C256 fixed original-v50 Git blob")
    old = json_data(raw)
    require(old["generation"] == 50 and len(old["files"]) == 3805,
            "C256 fixed original-v50 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 51 and old["files"].keys() <= value["files"].keys(),
            "C256 inherited original-v50 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "C256 preserved predecessor evidence: " + name)
    return count
