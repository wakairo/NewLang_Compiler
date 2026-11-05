"""C238 successor: externally pin adopted main425's complete v38 bytes."""
from pathlib import Path
import sys

V38 = "tests/p990/current-freeze-v38.json"
SHA256 = "4cbe9477900967215dd4601d4536d1f4fcd8ca4b6a15589e112afad0341beea2"
OID = "46af7269da29c606756769ce6bcda19f2ae4e3bb"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1158"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v38 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V38).read_bytes()
    require(digest(raw) == SHA256, "P1169 fixed predecessor-v38 external SHA256")
    require(git_oid("blob", raw) == OID, "P1169 fixed predecessor-v38 external Git blob")
    old = json_data(raw)
    require(old["generation"] == 38 and len(old["files"]) == 3445,
            "P1169 fixed predecessor-v38 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 39 and old["files"].keys() <= value["files"].keys(),
            "P1169 inherited v38 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "P1169 preserved predecessor evidence: " + name)
    return count
