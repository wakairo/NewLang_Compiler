"""Append-only sanitizer scheduling successor of preserved C220 v32."""
from pathlib import Path
import sys

V32 = "tests/p990/current-freeze-v32.json"
SHA256 = "38de177414c51286555dcd76e9cfd0a728e3c0f5204b6116c94523d83aa415b8"
OID = "868f9384495c2fda21070c5f9546eaffd1239311"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1123"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v32 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require

    count = previous_verify(root, current)
    raw = local(root, V32).read_bytes()
    require(digest(raw) == SHA256, "P1123 external first v32 SHA256")
    require(git_oid("blob", raw) == OID, "P1123 external first v32 Git blob")
    old = json_data(raw)
    require(old["generation"] == 32 and len(old["files"]) == 3264,
            "P1123 first v32 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 33 and
            old["files"].keys() <= value["files"].keys(),
            "P1123 inherited v32 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "P1123 preserved first v32 evidence: " + name)
    return count
