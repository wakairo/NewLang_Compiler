"""Prospective C220 private cyclic relation; externally fixed v31 is immutable."""
from pathlib import Path
import sys

V31 = "tests/p990/current-freeze-v31.json"
SHA256 = "4a8efbebc381a308deae2ae671fdd3aad31e28cdc412eef217fb81fc8438633e"
OID = "ef321ea6f4571f6b698b7e66c219ae32b15a0a00"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1117"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v31 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require

    count = previous_verify(root, current)
    raw = local(root, V31).read_bytes()
    require(digest(raw) == SHA256, "P1123 external fixed v31 SHA256")
    require(git_oid("blob", raw) == OID, "P1123 external fixed v31 Git blob")
    old = json_data(raw)
    require(old["generation"] == 31 and len(old["files"]) == 3222,
            "P1123 fixed v31 inventory")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 32 and
            old["files"].keys() <= value["files"].keys(),
            "P1123 inherited v31 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "P1123 immutable pre-C220 evidence: " + name)
    return count
