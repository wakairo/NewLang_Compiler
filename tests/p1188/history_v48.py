"""C254 applies its already intended finite OOM timeout after global defaults; v47 stays exact."""
from pathlib import Path
import sys

V47 = "tests/p990/current-freeze-v47.json"
SHA256 = "c91968e3a3e1593515f45105a329c149a0f0e6646022b35dbe456e82687e586f"
OID = "7658e86a2dc9a36d14072dd97a37408d7244bc95"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1188"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v47 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V47).read_bytes()
    require(digest(raw) == SHA256, "C254 fixed predecessor-v47 external SHA256")
    require(git_oid("blob", raw) == OID, "C254 fixed predecessor-v47 external Git blob")
    old = json_data(raw)
    require(old["generation"] == 47 and len(old["files"]) == 3727,
            "C254 fixed predecessor-v47 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 48 and old["files"].keys() <= value["files"].keys(),
            "C254 inherited v47 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "C254 preserved predecessor evidence: " + name)
    return count
