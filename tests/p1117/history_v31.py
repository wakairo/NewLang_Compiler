"""Append-only C217 Copy result lineage successor of preserved v30."""
from pathlib import Path
import sys

V30 = "tests/p990/current-freeze-v30.json"
SHA256 = "fa1b70125cb99afb23550bd33b8a135fe77285ff44f762f81efd0cfa4dfc2eea"
OID = "aac6524a934e9610fca6bc5a307458137f77ef1b"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1117"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v30 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require

    count = previous_verify(root, current)
    raw = local(root, V30).read_bytes()
    require(digest(raw) == SHA256, "P1117 fixed first-v30 SHA256")
    require(git_oid("blob", raw) == OID, "P1117 fixed first-v30 Git blob")
    old = json_data(raw)
    require(old["generation"] == 30 and len(old["files"]) == 3207,
            "P1117 first-v30 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 31 and
            old["files"].keys() <= value["files"].keys(),
            "P1117 inherited-v30 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "P1117 preserved first-v30 data: " + name)
    return count
