"""C226 successor: externally pin adopted main7e492ec's complete v35 bytes."""
from pathlib import Path
import sys

V35 = "tests/p990/current-freeze-v35.json"
SHA256 = "518851e3c35046ef1212684f6c8003b4413340b90574d772122c728dbf37f3e1"
OID = "2be536f5ab93dbc5513dd135f3a0492ab7204d6f"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1139"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v35 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V35).read_bytes()
    require(digest(raw) == SHA256, "P1146 adopted-v35 external SHA256")
    require(git_oid("blob", raw) == OID, "P1146 adopted-v35 external Git blob")
    old = json_data(raw)
    require(old["generation"] == 35 and len(old["files"]) == 3338,
            "P1146 adopted-v35 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 36 and
            old["files"].keys() <= value["files"].keys(),
            "P1146 inherited v35 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "P1146 preserved adopted evidence: " + name)
    return count
