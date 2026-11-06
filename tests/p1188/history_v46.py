"""C250a preserves the original C247 candidate and its complete freeze history."""
from pathlib import Path
import sys

V45 = "tests/p990/current-freeze-v45.json"
SHA256 = "ce35aff20971d6975646569e25ba6f837a6b874f4b5d61889de5335fe4b540bf"
OID = "5cb74b3fc7dbc158e8b0610f41824b69c4d65519"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1188"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v45 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V45).read_bytes()
    require(digest(raw) == SHA256, "C250a fixed predecessor-v45 external SHA256")
    require(git_oid("blob", raw) == OID, "C250a fixed predecessor-v45 external Git blob")
    old = json_data(raw)
    require(old["generation"] == 45 and len(old["files"]) == 3660,
            "C250a fixed predecessor-v45 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 46 and old["files"].keys() <= value["files"].keys(),
            "C250a inherited v45 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "C250a preserved predecessor evidence: " + name)
    return count
