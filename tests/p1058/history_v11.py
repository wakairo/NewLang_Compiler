"""Fixed PR1056 v10 anchor, using the unchanged existing freeze machinery.

New reviewable test code; not protection against arbitrary checker replacement.
No source issuer or Production state is created here.
"""
from pathlib import Path
import sys

V10 = "tests/p990/current-freeze-v10.json"
SHA256 = "e96987e8c3dab5fad26b5418bae08fa3ba8343d37b1f7efbef1aa20c5fb889f8"
OID = "1046b33d592e2122dce60fa7fb0a5c89ac012dc4"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/support"))
    sys.path.insert(0, str(root / "tests/p1055"))
    from history_v10 import verify as previous_verify
    from versioned_freeze import digest, git_oid, json_data, local, manifest_data, require
    count = previous_verify(root, current)
    raw = local(root, V10).read_bytes()
    require(digest(raw) == SHA256, "P1058 fixed original-v10 SHA256")
    require(git_oid("blob", raw) == OID, "P1058 fixed original-v10 Git blob")
    previous = json_data(raw)
    require(previous["generation"] == 10 and len(previous["files"]) == 2177,
            "P1058 fixed original-v10 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 11 and previous["files"].keys() <= value["files"].keys(),
            "P1058 lost original-v10 coverage")
    for name, expected in previous["files"].items():
        if name.startswith("tests/") and (
            "/evidence/" in name or "/lineage/" in name or name.endswith((".json", ".nl"))
        ):
            require(digest(local(root, name).read_bytes()) == expected,
                    "P1058 preserved original-v10 data: " + name)
    return count
