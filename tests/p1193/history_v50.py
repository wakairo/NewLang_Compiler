"""C256 prospective RETURN consumer; original v1-v49 and all FIRST stay exact."""
from pathlib import Path
import sys

V49 = "tests/p990/current-freeze-v49.json"
SHA256 = "721083514aba3644b9a3af0695ca339009d4c7cc394978634b6d08eed625e2e9"
OID = "c7677382391774a902a6dee0083d7a1f66137784"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1188"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v49 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V49).read_bytes()
    require(digest(raw) == SHA256, "C256 fixed original-v49 SHA256")
    require(git_oid("blob", raw) == OID, "C256 fixed original-v49 Git blob")
    old = json_data(raw)
    require(old["generation"] == 49 and len(old["files"]) == 3739,
            "C256 fixed original-v49 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 50 and old["files"].keys() <= value["files"].keys(),
            "C256 inherited original-v49 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "C256 preserved predecessor evidence: " + name)
    return count
