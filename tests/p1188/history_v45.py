"""C247 pins the first C247 candidate and the inherited C246a freeze RED."""
from pathlib import Path
import sys

V44 = "tests/p990/current-freeze-v44.json"
SHA256 = "645f506ff5406f0b333723362b86e87cdd1aa7576113d156d37110d98a4253fe"
OID = "87267b74d77962255233427e6f17addfcf3c0e22"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1188"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v44 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V44).read_bytes()
    require(digest(raw) == SHA256, "C247 fixed predecessor-v44 external SHA256")
    require(git_oid("blob", raw) == OID, "C247 fixed predecessor-v44 external Git blob")
    old = json_data(raw)
    require(old["generation"] == 44 and len(old["files"]) == 3655,
            "C247 fixed predecessor-v44 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 45 and old["files"].keys() <= value["files"].keys(),
            "C247 inherited v44 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "C247 preserved predecessor evidence: " + name)
    return count
