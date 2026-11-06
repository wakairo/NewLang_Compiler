"""C254 canonical-serialization recovery; failed v48 and CI FIRST stay exact."""
from pathlib import Path
import sys

V48 = "tests/p990/current-freeze-v48.json"
SHA256 = "d7936540ae3b1f259126432aa4e200d5a75d70f47aa23a82d00749e1dc07ee89"
OID = "a2fc196cb6d5a3a6fb07434a592509dbfdd9f248"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1188"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v48 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V48).read_bytes()
    require(digest(raw) == SHA256, "C254 fixed failed-v48 external SHA256")
    require(git_oid("blob", raw) == OID, "C254 fixed failed-v48 external Git blob")
    old = json_data(raw)
    require(old["generation"] == 48 and len(old["files"]) == 3735,
            "C254 fixed failed-v48 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 49 and old["files"].keys() <= value["files"].keys(),
            "C254 inherited failed-v48 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "C254 preserved predecessor evidence: " + name)
    return count
