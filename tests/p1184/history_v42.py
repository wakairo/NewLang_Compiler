"""C246a successor: externally pin C245's complete v41 history."""
from pathlib import Path
import sys

V41 = "tests/p990/current-freeze-v41.json"
SHA256 = "08a5228c80136fc226caa403bb39b11fd11c4b7ad0e1756e7bcd83f38a6df577"
OID = "b3ea1ae1afcafed5b18dace15e9f4c94882b6421"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/p1184"))
    sys.path.insert(0, str(root / "tests/support"))
    from history_v41 import verify as previous_verify
    from versioned_freeze import digest, git_oid, local, manifest_data, json_data, require
    count = previous_verify(root, current)
    raw = local(root, V41).read_bytes()
    require(digest(raw) == SHA256,
            "C246a fixed predecessor-v41 external SHA256")
    require(git_oid("blob", raw) == OID,
            "C246a fixed predecessor-v41 external Git blob")
    old = json_data(raw)
    require(old["generation"] == 41 and len(old["files"]) == 3587,
            "C246a fixed predecessor-v41 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 42 and
            old["files"].keys() <= value["files"].keys(),
            "C246a inherited v41 coverage")
    return count
