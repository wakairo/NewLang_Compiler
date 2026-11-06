"""C254 adds a forward-only generation, keeping C250a/C253 evidence exact."""
from pathlib import Path
import sys

V46 = "tests/p990/current-freeze-v46.json"
SHA256 = "39c4534ea682dbbac0975b027cf550db1301242ca5bf4c2d2e836f59fe9ebb93"
OID = "e9ce6e4ee6f08f8666968b4bbbbb20c24c5587fd"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1188"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v46 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V46).read_bytes()
    require(digest(raw) == SHA256, "C254 fixed predecessor-v46 external SHA256")
    require(git_oid("blob", raw) == OID, "C254 fixed predecessor-v46 external Git blob")
    old = json_data(raw)
    require(old["generation"] == 46 and len(old["files"]) == 3683,
            "C254 fixed predecessor-v46 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 47 and old["files"].keys() <= value["files"].keys(),
            "C254 inherited v46 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "C254 preserved predecessor evidence: " + name)
    return count
