"""C228 successor: externally pin fixed PR1149 d66fdf7 complete v36 bytes."""
from pathlib import Path
import sys

V36 = "tests/p990/current-freeze-v36.json"
SHA256 = "540261f3c7b3d42e59a828d8d7c282d9bcdc633212b9dffd8ca3be753bea99a2"
OID = "5901f2fc397b18bc1ac6d2acdbb7e9b98a36fcbc"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1146"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v36 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V36).read_bytes()
    require(digest(raw) == SHA256, "P1152 fixed predecessor-v36 external SHA256")
    require(git_oid("blob", raw) == OID, "P1152 fixed predecessor-v36 external Git blob")
    old = json_data(raw)
    require(old["generation"] == 36 and len(old["files"]) == 3374,
            "P1152 fixed predecessor-v36 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 37 and
            old["files"].keys() <= value["files"].keys(),
            "P1152 inherited v36 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "P1152 preserved fixed predecessor evidence: " + name)
    return count
