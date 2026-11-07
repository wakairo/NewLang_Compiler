"""T1230 prospective v60: only two test timeouts, fixed old v59 Git custody."""
from pathlib import Path
import sys

V59 = "tests/p990/current-freeze-v59.json"
V59_SHA = "228d16101791f78c6e7ad2011df7bdb2fe4db4611bb2bd9b570f6e9f32426f68"
V59_OID = "78a04a10d509b137d8e906590e6cf4d72072ec4b"

def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1217"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v59 import verify as previous
    from versioned_freeze import digest, git_oid, manifest_data, require
    count = previous(root, current)
    old = (root / V59).read_bytes()
    require(digest(old) == V59_SHA and git_oid("blob", old) == V59_OID,
            "T1230 externally fixed original adopted v59 Git/blob identity")
    value, _ = manifest_data(root, current)
    require(value["generation"] == 60 and
            value["previous"] == {"path": V59, "sha256": V59_SHA},
            "T1230 v60 descendant / predecessor")
    require(len(value["files"]) == count and
            len(value["files"]) >= 4072,
            "T1230 prospective full-current coverage")
    return count
