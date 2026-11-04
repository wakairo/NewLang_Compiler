"""C232 successor: externally pin fixed PR1155 af16bba complete v37 bytes."""
from pathlib import Path
import sys

V37 = "tests/p990/current-freeze-v37.json"
SHA256 = "cf0f3d2caea3ba2fc06ec6a0f38bdc7c3792e627b11a251c9b62ef7fd2879255"
OID = "cf7449fc682ba1e15632ffcd45efae6ccc19097a"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1152"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v37 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V37).read_bytes()
    require(digest(raw) == SHA256, "P1158 fixed predecessor-v37 external SHA256")
    require(git_oid("blob", raw) == OID, "P1158 fixed predecessor-v37 external Git blob")
    old = json_data(raw)
    require(old["generation"] == 37 and len(old["files"]) == 3412,
            "P1158 fixed predecessor-v37 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 38 and
            old["files"].keys() <= value["files"].keys(),
            "P1158 inherited v37 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "P1158 preserved fixed predecessor evidence: " + name)
    return count
