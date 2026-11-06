"""C257 F-1198 prospective repair; original v1-v51 evidence stays exact."""
from pathlib import Path
import sys

V51 = "tests/p990/current-freeze-v51.json"
SHA256 = "b11101965990ca91799952a6c383304b7195dd50335c10f3d1316ca5c1c1954e"
OID = "e372fab3f86e5df02b05ac7523ff0ce013f8bdfb"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1193"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v51 import verify as previous_verify
    from versioned_freeze import (
        digest,
        git_oid,
        json_data,
        local,
        manifest_data,
        require,
    )

    count = previous_verify(root, current)
    raw = local(root, V51).read_bytes()
    require(digest(raw) == SHA256, "C257 fixed original-v51 SHA256")
    require(git_oid("blob", raw) == OID, "C257 fixed original-v51 Git blob")
    old = json_data(raw)
    require(
        old["generation"] == 51 and len(old["files"]) == 3811,
        "C257 fixed original-v51 coverage",
    )
    value, _ = manifest_data(root, current)
    require(
        value["generation"] >= 52 and old["files"].keys() <= value["files"].keys(),
        "C257 inherited original-v51 coverage",
    )
    for name, expected in old["files"].items():
        if name.startswith("tests/") and (
            "/evidence/" in name
            or "/lineage/" in name
            or name.endswith((".json", ".nl"))
        ):
            require(
                digest(local(root, name).read_bytes()) == expected,
                "C257 preserved predecessor evidence: " + name,
            )
    return count
