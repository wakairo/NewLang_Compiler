"""C245 successor: externally pin fixed PR1180's complete v40 history."""
from pathlib import Path
import sys

V40 = "tests/p990/current-freeze-v40.json"
SHA256 = "55ea6562006735d1633c6c802dd7eb986c311f240a4b3bbdcba97f1139bf22cd"
OID = "1b85b9de060b7d877de107daffcf6023767c3930"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1176"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v40 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V40).read_bytes()
    require(digest(raw) == SHA256, "P1184 fixed predecessor-v40 external SHA256")
    require(git_oid("blob", raw) == OID, "P1184 fixed predecessor-v40 external Git blob")
    old = json_data(raw)
    require(old["generation"] == 40 and len(old["files"]) == 3539,
            "P1184 fixed predecessor-v40 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 41 and old["files"].keys() <= value["files"].keys(),
            "P1184 inherited v40 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "P1184 preserved predecessor evidence: " + name)
    return count
