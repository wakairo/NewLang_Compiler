"""C247 pins the first C247 candidate and the inherited C246a freeze RED."""
from pathlib import Path
import sys

V43 = "tests/p990/current-freeze-v43.json"
SHA256 = "e1eed02eb204cd1350554e71f16d334a154964cd2d83b4b5bae90036b346f8b1"
OID = "387fb88771f4b5ada69a0c5dc741e85b866346ee"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1188"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v43 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V43).read_bytes()
    require(digest(raw) == SHA256, "C247 fixed predecessor-v43 external SHA256")
    require(git_oid("blob", raw) == OID, "C247 fixed predecessor-v43 external Git blob")
    old = json_data(raw)
    require(old["generation"] == 43 and len(old["files"]) == 3648,
            "C247 fixed predecessor-v43 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 44 and old["files"].keys() <= value["files"].keys(),
            "C247 inherited v43 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "C247 preserved predecessor evidence: " + name)
    return count
