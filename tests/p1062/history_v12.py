"""Fixed PR1059 v11 anchor, using the unchanged existing freeze machinery.

New reviewable test code; not protection against arbitrary checker replacement.
No source issuer or Production state is created here.
"""
from pathlib import Path
import sys

V11 = "tests/p990/current-freeze-v11.json"
SHA256 = "e78dc38e5dabc83fd7b619c43479ed293871fc67d830aeb98ddac26cac5b6bd4"
OID = "e3b51f0cdd971418477aed4a5744b8a16789f073"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/support"))
    sys.path.insert(0, str(root / "tests/p1058"))
    from history_v11 import verify as previous_verify
    from versioned_freeze import digest, git_oid, json_data, local, manifest_data, require
    count = previous_verify(root, current)
    raw = local(root, V11).read_bytes()
    require(digest(raw) == SHA256, "P1062 fixed original-v11 SHA256")
    require(git_oid("blob", raw) == OID, "P1062 fixed original-v11 Git blob")
    previous = json_data(raw)
    require(previous["generation"] == 11 and len(previous["files"]) == 2307,
            "P1062 fixed original-v11 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 12 and previous["files"].keys() <= value["files"].keys(),
            "P1062 lost original-v11 coverage")
    for name, expected in previous["files"].items():
        if name.startswith("tests/") and (
            "/evidence/" in name or "/lineage/" in name or name.endswith((".json", ".nl"))
        ):
            require(digest(local(root, name).read_bytes()) == expected,
                    "P1062 preserved original-v11 data: " + name)
    return count
