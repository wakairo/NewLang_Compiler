"""C223 local successor anchored to fixed PR1137 v34, not an adoption claim."""
from pathlib import Path
import sys

V34 = "tests/p990/current-freeze-v34.json"
SHA256 = "8faad77b5d5383b1b9ba581b5261d6ec6ea21d8a80fe1f8c67715d2c559c0aa7"
OID = "4154a56b757a03992d2d3007cd0b2ba5882adc75"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1133"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v34 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V34).read_bytes()
    require(digest(raw) == SHA256, "P1139 fixed-PR1137 v34 SHA256")
    require(git_oid("blob", raw) == OID, "P1139 fixed-PR1137 v34 Git blob")
    old = json_data(raw)
    require(old["generation"] == 34 and len(old["files"]) == 3314,
            "P1139 fixed-PR1137 v34 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 35 and
            old["files"].keys() <= value["files"].keys(),
            "P1139 inherited v34 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "P1139 preserved fixed-PR1137 evidence: " + name)
    return count
