"""C243 successor: externally pin fixed PR1171's complete v39 history."""
from pathlib import Path
import sys

V39 = "tests/p990/current-freeze-v39.json"
SHA256 = "8eb1c8846478e5f431aab67d544c5f3a098fd4367fe5b08f477798e9ffb0df3d"
OID = "4fe37bcf5b4e2fb24e42bb1333adaa39f48cfc28"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1169"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v39 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V39).read_bytes()
    require(digest(raw) == SHA256, "P1176 fixed predecessor-v39 external SHA256")
    require(git_oid("blob", raw) == OID, "P1176 fixed predecessor-v39 external Git blob")
    old = json_data(raw)
    require(old["generation"] == 39 and len(old["files"]) == 3493,
            "P1176 fixed predecessor-v39 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 40 and old["files"].keys() <= value["files"].keys(),
            "P1176 inherited v39 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "P1176 preserved predecessor evidence: " + name)
    return count
