"""Fixed PR1048 v9 anchor, using the unchanged existing freeze machinery.

New reviewable test code; not protection against arbitrary checker replacement.
No source issuer or Production state is created here.
"""
from pathlib import Path
import sys

V9 = "tests/p990/current-freeze-v9.json"
SHA256 = "53fa2ddb015b19c71b7271303647bda12d1c71324786a9fc09bc05f3c15b4e98"
OID = "a9e5ed14446f186851bcf11b02de30ef3b54e5ae"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/support"))
    sys.path.insert(0, str(root / "tests/p1045"))
    from history_v9 import verify as previous_verify
    from versioned_freeze import digest, git_oid, json_data, local, manifest_data, require
    count = previous_verify(root, current)
    raw = local(root, V9).read_bytes()
    require(digest(raw) == SHA256, "P1055 fixed original-v9 SHA256")
    require(git_oid("blob", raw) == OID, "P1055 fixed original-v9 Git blob")
    previous = json_data(raw)
    require(previous["generation"] == 9 and len(previous["files"]) == 2053,
            "P1055 fixed original-v9 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 10 and previous["files"].keys() <= value["files"].keys(),
            "P1055 lost original-v9 coverage")
    for name, expected in previous["files"].items():
        if name.startswith("tests/") and (
            "/evidence/" in name or "/lineage/" in name or name.endswith((".json", ".nl"))
        ):
            require(digest(local(root, name).read_bytes()) == expected,
                    "P1055 preserved original-v9 data: " + name)
    return count
