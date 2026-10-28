"""Offline P982 historical Git provenance plus prospective current freezes.

The original manifests never describe today's changed production. Their exact
bytes and original blob paths are authenticated against the pinned parent tree.
Unchanged protected inputs are checked both historically and currently. A new
manifest generation changes current expectations without editing earlier ones.
"""
import base64
import hashlib
import json
from pathlib import Path

PARENT = "c981ff742865731decb434c2382a6fb418c83de5"
TREE = "aa01141aa821f4433725aa3b533cccd268cb064f"
SNAPSHOT = "tests/p990/lineage/parent-snapshot-v1.json"
FIRST = "tests/p990/current-freeze-v1.json"
SOURCE = "tests/p990/source-freeze-v1.json"
SOURCE_SHA = "f51773f9709413ae5eb0486e88876bf4a54e51d2b691e71ab90ca70a729ac0da"
EVOLVING = {
    "CMakeLists.txt", "src/checked_c_copy_internal.h",
    "src/checked_c_copy_if_internal.h", "src/checked_c_node.c",
    "tests/p982/freeze_test.py",
}
MANIFESTS = [
    "tests/p982/source-freeze-v1.json",
    *[f"tests/p982/harness-freeze-v{i}.json" for i in [1, 2, 3]],
]


class FreezeError(AssertionError):
    pass


def require(condition, message):
    if not condition:
        raise FreezeError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def git_oid(kind, data):
    return hashlib.sha1(kind.encode() + b" " + str(len(data)).encode() + b"\0" + data).hexdigest()


def local(root, name):
    path = root / name
    require(not Path(name).is_absolute() and ".." not in Path(name).parts,
            "nonlocal freeze path: " + name)
    require(path.resolve().is_relative_to(root.resolve()), "escaped freeze path")
    return path


def tree_entry(snapshot, path):
    current = TREE
    parts = Path(path).parts
    for depth, part in enumerate(parts):
        require(current in snapshot["trees_base64"], "missing retained Git tree")
        raw = base64.b64decode(snapshot["trees_base64"][current], validate=True)
        require(git_oid("tree", raw) == current, "retained Git tree digest")
        entries = {}
        offset = 0
        while offset < len(raw):
            space = raw.index(b" ", offset)
            zero = raw.index(b"\0", space)
            require(zero + 21 <= len(raw), "truncated retained Git tree")
            mode = raw[offset:space]
            name = raw[space + 1:zero].decode()
            require(name not in entries, "duplicate retained tree entry")
            entries[name] = (mode, raw[zero + 1:zero + 21].hex())
            offset = zero + 21
        require(part in entries, "missing original path: " + path)
        mode, current = entries[part]
        require(mode == (b"100644" if depth + 1 == len(parts) else b"40000"),
                "unexpected retained Git mode")
    return current


def original(root, snapshot, name):
    require(name in snapshot["blobs"], "missing original blob: " + name)
    row = snapshot["blobs"][name]
    if name in EVOLVING:
        require("archive" in row, "missing original evolving-file bytes")
        data = local(root, row["archive"]).read_bytes()
    else:
        require("archive" not in row, "protected input redirected to archive")
        data = local(root, name).read_bytes()
    require(git_oid("blob", data) == row["oid"] == tree_entry(snapshot, name),
            "original Git blob mismatch: " + name)
    require(digest(data) == row["sha256"], "original SHA256 mismatch: " + name)
    return data


def history(root):
    snapshot = json.loads(local(root, SNAPSHOT).read_text())
    require(snapshot["schema"] == 1 and snapshot["parent"] == PARENT and
            snapshot["tree"] == TREE, "immutable P982 parent/tree changed")
    commit = base64.b64decode(snapshot["commit_base64"], validate=True)
    require(git_oid("commit", commit) == PARENT, "retained parent commit digest")
    require(commit.splitlines()[0] == ("tree " + TREE).encode(),
            "retained parent tree link")
    manifests = {name: json.loads(original(root, snapshot, name)) for name in MANIFESTS}
    protected = set(MANIFESTS)
    for name in [MANIFESTS[0], MANIFESTS[-1]]:
        for path, expected in manifests[name]["files"].items():
            require(digest(original(root, snapshot, path)) == expected,
                    "original recorded freeze mismatch: " + path)
            protected.add(path)
    source = local(root, SOURCE).read_bytes()
    require(digest(source) == SOURCE_SHA, "original P990 source manifest changed")
    for path, expected in json.loads(source)["files"].items():
        require(digest(local(root, path).read_bytes()) == expected,
                "original P990 source/oracle changed: " + path)
        protected.add(path)
    protected.update({SOURCE, SNAPSHOT, "tests/support/versioned_freeze.py",
                      "tests/p990/freeze_selftest.py"})
    protected.update(row["archive"] for row in snapshot["blobs"].values()
                     if "archive" in row)
    return protected


def verify(root, manifest):
    root = Path(root).resolve()
    current_name = str(Path(manifest))
    required = history(root)
    current = json.loads(local(root, current_name).read_text())
    require(current["schema"] == 1 and current["parent"] == PARENT,
            "current freeze schema/parent")
    generation = current["generation"]
    require(type(generation) is int and 1 <= generation <= 64,
            "current freeze generation")
    cursor, cursor_name = current, current_name
    seen = set()
    while True:
        require(cursor_name not in seen, "freeze chain cycle")
        seen.add(cursor_name)
        require(cursor["schema"] == 1 and cursor["parent"] == PARENT and
                cursor["generation"] == generation, "freeze chain generation")
        previous = cursor["previous"]
        if previous is None:
            require(generation == 1 and cursor_name == FIRST,
                    "freeze chain must retain P990 v1")
            break
        require(generation > 1, "unexpected freeze predecessor")
        previous_name = previous["path"]
        data = local(root, previous_name).read_bytes()
        require(digest(data) == previous["sha256"], "previous freeze digest")
        required.add(previous_name)
        cursor, cursor_name = json.loads(data), previous_name
        generation -= 1
    files = current["files"]
    require(required <= files.keys(), "current freeze omits protected inputs")
    for name, expected in files.items():
        require(digest(local(root, name).read_bytes()) == expected,
                "current freeze digest: " + name)
    return len(files)
