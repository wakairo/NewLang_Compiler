"""C247 pins the repaired faad predecessor without rewriting v1-v42."""
from pathlib import Path
import sys

V42 = "tests/p990/current-freeze-v42.json"
SHA256 = "25c7ff691c1c2e64afe900aa07d1e6f0b3987cbf07c777ea68a2e1818e37c318"
OID = "88493021da1a639fa656e5ad9df7508106c3bc7b"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1184"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v42 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V42).read_bytes()
    require(digest(raw) == SHA256, "C247 fixed predecessor-v42 external SHA256")
    require(git_oid("blob", raw) == OID, "C247 fixed predecessor-v42 external Git blob")
    old = json_data(raw)
    require(old["generation"] == 42 and len(old["files"]) == 3598,
            "C247 fixed predecessor-v42 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 43 and old["files"].keys() <= value["files"].keys(),
            "C247 inherited v42 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "C247 preserved predecessor evidence: " + name)
    return count
