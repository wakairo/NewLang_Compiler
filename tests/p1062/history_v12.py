"""Fixed PR1059 v11 anchor, using the unchanged existing freeze machinery.

New reviewable test code; not protection against arbitrary checker replacement.
No source issuer or Production state is created here.
"""
from pathlib import Path
import json
import subprocess
import sys

V11 = "tests/p990/current-freeze-v11.json"
SHA256 = "e78dc38e5dabc83fd7b619c43479ed293871fc67d830aeb98ddac26cac5b6bd4"
OID = "e3b51f0cdd971418477aed4a5744b8a16789f073"
CURRENT = "tests/p990/current-freeze-v12.json"


def verify(root, current):
    root = Path(root)
    sys.path.insert(0, str(root / "tests/support"))
    sys.path.insert(0, str(root / "tests/p1058"))
    from history_v11 import verify as previous_verify
    from versioned_freeze import digest, git_oid, json_data, local, manifest_data, require
    count = previous_verify(root, current)
    raw = local(root, V11).read_bytes()
    require(digest(raw) == SHA256, "P1062 fixed original-v11 SHA256")
    require(git_oid("blob", raw) == OID, "P1062 fixed original-v11 Git blob")
    previous = json_data(raw)
    require(previous["generation"] == 11 and len(previous["files"]) == 2307,
            "P1062 fixed original-v11 coverage")
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 12 and previous["files"].keys() <= value["files"].keys(),
            "P1062 lost original-v11 coverage")
    for name, expected in previous["files"].items():
        if name.startswith("tests/") and (
            "/evidence/" in name or "/lineage/" in name or name.endswith((".json", ".nl"))
        ):
            require(digest(local(root, name).read_bytes()) == expected,
                    "P1062 preserved original-v11 data: " + name)
    return count


def git_inputs(root):
    """Select the inherited inputs and P1062 files from Git, never rglob.

    The index permits a batched, staged repair before commit. Requiring each
    selected working file to equal its indexed blob prevents unstaged bytes
    from silently becoming the recorded inputs. A clean checkout uses HEAD's
    same blobs. Historical scratch-copy verification above stays offline.
    """
    root = Path(root)
    sys.path.insert(0, str(root / "tests/support"))
    from versioned_freeze import digest, git_oid, json_data, local, require
    records = subprocess.check_output(
        ["git", "-C", str(root), "ls-files", "--stage", "-z"]
    )
    tracked = {}
    for record in records.split(b"\0"):
        if not record:
            continue
        header, name = record.split(b"\t", 1)
        mode, oid, stage = header.split()
        require(stage == b"0", "unmerged prospective input")
        tracked[name.decode("utf-8")] = (mode, oid.decode("ascii"))
    raw = local(root, V11).read_bytes()
    require(digest(raw) == SHA256 and git_oid("blob", raw) == OID,
            "P1062 fixed original-v11 inventory anchor")
    names = set(json_data(raw)["files"]) | {V11}
    names.update(name for name in tracked if name.startswith("tests/p1062/"))
    require(names <= tracked.keys(), "missing Git-managed inherited input")
    for name in names:
        mode, oid = tracked[name]
        require(mode in (b"100644", b"100755"), "non-file Git input: " + name)
        require(git_oid("blob", local(root, name).read_bytes()) == oid,
                "unstaged prospective input: " + name)
    return names, tracked


def check_git_inventory(root, current=CURRENT):
    names, tracked = git_inputs(root)
    from versioned_freeze import git_oid, local, manifest_data, require
    value, _ = manifest_data(Path(root), current)
    extra = set(value["files"]) - tracked.keys()
    require(not extra, "untracked prospective freeze input: " + ", ".join(sorted(extra)))
    require(names <= value["files"].keys(), "prospective Git input coverage")
    for name in value["files"]:
        mode, oid = tracked[name]
        require(mode in (b"100644", b"100755") and
                git_oid("blob", local(Path(root), name).read_bytes()) == oid,
                "unstaged prospective input: " + name)
    return len(value["files"])


def render(root, current=CURRENT):
    root = Path(root)
    names, tracked = git_inputs(root)
    from versioned_freeze import digest, local, manifest_data, require
    value, _ = manifest_data(root, current)
    require(value["generation"] >= 12, "P1062 prospective generation")
    if value["generation"] == 12:
        require(value["previous"] == {"path": V11, "sha256": SHA256},
                "P1062 prospective-v12 predecessor")
        value["stage"] = (
            "C152 P1062 Git-index-only prospective-v12 repair; original failed "
            "0502751 manifest and six CI RED logs retained; v1-v11/source/oracle unchanged"
        )
    # Preserve legitimate tracked coverage when a later current generation
    # invokes the original P1062 regression; do not freeze future Production.
    names.update(set(value["files"]) & tracked.keys())
    value["files"] = {name: digest(local(root, name).read_bytes())
                      for name in sorted(names)}
    return (json.dumps(value, indent=2) + "\n").encode("utf-8")


if __name__ == "__main__":
    root = Path(__file__).resolve().parents[2]
    if sys.argv[1:] == ["--refresh"]:
        (root / CURRENT).write_bytes(render(root))
    elif sys.argv[1:] != ["--check-git"]:
        raise SystemExit("usage: python3 -B tests/p1062/history_v12.py --refresh|--check-git")
    count = check_git_inventory(root)
    verify(root, CURRENT)
    print("P1062 Git-managed prospective inputs and original history PASS", count)
