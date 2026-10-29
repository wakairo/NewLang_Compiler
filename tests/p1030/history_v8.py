"""C127 external fixed-v7 anchor, separate from the unchanged trusted verifier.

The literal is from fixed PR1025 c7ff99c / tree0f929e7 and F85/C128's
independent predecessor review, not a later mutable manifest. This is trusted
test code requiring review if changed, not protection against rewriting code.
"""
from pathlib import Path
import sys

V7 = "tests/p990/current-freeze-v7.json"
SHA256 = "69b4d69982e8d7112e60d657d344b26b36c46a0be0b10477b4250ae1f351d18e"
OID = "df98be23382971b4d9703b724c7564fda1fd663e"


def predecessor(root):
    sys.path.insert(0, str(Path(root) / "tests/support"))
    from versioned_freeze import digest, git_oid, json_data, local, require
    data = local(Path(root), V7).read_bytes()
    require(digest(data) == SHA256, "P1030 fixed original-v7 SHA256")
    require(git_oid("blob", data) == OID, "P1030 fixed original-v7 Git blob")
    value = json_data(data)
    require(value["generation"] == 7 and len(value["files"]) == 1666,
            "P1030 fixed original-v7 coverage")
    return frozenset(value["files"])


def verify(root, current):
    original = predecessor(root)
    from versioned_freeze import manifest_data, require, verify as legacy_verify
    # Preserve the existing verifier's historical failure diagnostics first.
    count = legacy_verify(root, current)
    value, _ = manifest_data(Path(root), current)
    require(value["generation"] >= 8 and original <= value["files"].keys(),
            "P1030 lost original-v7 coverage")
    # A prospective current digest may evolve implementation code, never
    # retroactively rewrite the predecessor's source/oracle/first-result data.
    from versioned_freeze import digest, json_data, local
    previous = json_data(local(Path(root), V7).read_bytes())
    for name, expected in previous["files"].items():
        if name.startswith("tests/") and (
            "/evidence/" in name or "/lineage/" in name or
            name.endswith((".json", ".nl"))
        ):
            require(digest(local(Path(root), name).read_bytes()) == expected,
                    "P1030 preserved original-v7 data: " + name)
    return count
