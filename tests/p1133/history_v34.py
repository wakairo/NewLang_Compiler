"""C222 prospective successor, externally anchored to adopted main's v33."""
from pathlib import Path
import sys

V33 = "tests/p990/current-freeze-v33.json"
SHA256 = "1b9b3ec33d962b43ecd8eb55acc15c53c1fdaf979b574d4a81694f4b66d5c8eb"
OID = "494aba9cc211f67f903c3ac068f6f34dee6ccc94"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1123"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v33 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V33).read_bytes()
    require(digest(raw) == SHA256, "P1133 adopted-main v33 SHA256")
    require(git_oid("blob", raw) == OID, "P1133 adopted-main v33 Git blob")
    old = json_data(raw)
    require(old["generation"] == 33 and len(old["files"]) == 3270,
            "P1133 adopted-main v33 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 34 and
            old["files"].keys() <= value["files"].keys(),
            "P1133 inherited v33 coverage")
    for name, expected in old["files"].items():
        if (name.startswith("tests/") and
                ("/evidence/" in name or "/lineage/" in name or
                 name.endswith((".json", ".nl")))):
            require(digest(local(root, name).read_bytes()) == expected,
                    "P1133 preserved adopted-main evidence: " + name)
    return count
