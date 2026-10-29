"""C137 prospective closure of externally fixed original v8; old verifiers unchanged."""
from pathlib import Path
import sys

V8 = "tests/p990/current-freeze-v8.json"
SHA256 = "40e0d8dae49d66f05d87d80aa95b7b5387dd7088f9df1bf021d779ea575ee934"
OID = "3584b65ae16be735b642ff328e28286909e18284"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1030"))
    from history_v8 import verify as previous_verify
    # Preserve the trusted v1-v8 rejection precedence and algorithm.
    count = previous_verify(root, current)
    from versioned_freeze import digest, git_oid, json_data, local, manifest_data, require
    raw = local(root, V8).read_bytes()
    require(digest(raw) == SHA256, "P1045 fixed original-v8 SHA256")
    require(git_oid("blob", raw) == OID, "P1045 fixed original-v8 Git blob")
    previous = json_data(raw)
    require(previous["generation"] == 8 and len(previous["files"]) == 1808,
            "P1045 fixed original-v8 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 9 and previous["files"].keys() <= value["files"].keys(),
            "P1045 lost original-v8 coverage")
    for name, expected in previous["files"].items():
        if name.startswith("tests/") and (
            "/evidence/" in name or "/lineage/" in name or name.endswith((".json", ".nl"))
        ):
            require(digest(local(root, name).read_bytes()) == expected,
                    "P1045 preserved original-v8 data: " + name)
    return count
