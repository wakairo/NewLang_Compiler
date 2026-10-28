"""Offline P982 historical Git provenance plus prospective current freezes.

The original manifests never describe today's changed production. Their exact
bytes and original blob paths are authenticated against the pinned parent tree.
Unchanged protected inputs are checked both historically and currently. A new
manifest generation changes current expectations without editing earlier ones.

C107 pins the three reviewed P990 generations independently of mutable
successor manifests. Changes to these literals or verifier are trusted-source
changes requiring Coordination review and independent F follow-up; a manifest
cannot authorize them. This is not protection against changing trusted code.
"""
import base64
import hashlib
import json
import re
import stat
from pathlib import Path

PARENT = "c981ff742865731decb434c2382a6fb418c83de5"
TREE = "aa01141aa821f4433725aa3b533cccd268cb064f"
SNAPSHOT = "tests/p990/lineage/parent-snapshot-v1.json"
FIRST = "tests/p990/current-freeze-v1.json"
SOURCE = "tests/p990/source-freeze-v1.json"
SOURCE_SHA = "f51773f9709413ae5eb0486e88876bf4a54e51d2b691e71ab90ca70a729ac0da"
# Externally reviewed authority: P995 8f8ac04c8b91d164d93a0d45639efce3efe36568,
# tree 312cf43cae2ce27e34f677a5214243991e2ca514; Issue999/C107, F81 Gate B.
# Neither these SHA256/blob OIDs nor their paths come from a later manifest.
BASELINES = {
    1: (FIRST,
        "e1f07f8a30fe927d374c76fd802c8c10d861090d6abe25e6ef090ee65b99aab4",
        "cd6e30902968e65c8ecf580dd404b3247035e7ec"),
    2: ("tests/p990/current-freeze-v2.json",
        "e6bf940e5b5e7ccbcc43ae269f51bfda5dc5f5638306916dd81d64df81785989",
        "f3a79d52250879e39b5abece92343097603e6c1a"),
    3: ("tests/p990/current-freeze-v3.json",
        "d636d2772584a16256ace830ecd4e82e352d0b8d80bf63172fa1ecaedf6829b8",
        "9f81177516e7e1f178e3cdf7e9a99aaaf19923ca"),
}
# Reviewed T998 4c3ed993fa8a94c7a2850ff98677127d96c31a6b,
# tree a0c648cc10305462cf902530decdc06e8f92552b. No R source enrollment.
TOOLING_BLOBS = {
    ".github/workflows/compiler-ci.yml": "23655288e60f0d3d08a12c9a542346bf31829a08",
    ".github/workflows/compiler-frontend-ci.yml": "0c2fac71ee2d67d4f3009b4031946d0dec3560b5",
    "cmake/independent-r-cohort-runner.py": "e82beffa4bc292e5dcf560130778bc4be05f9d10",
    "cmake/independent-r-manifest.py": "38564c5661b9efe04c37f865a1a0efe29f7b7eae",
    "cmake/independent-r-tests-hook.cmake": "a1063d203068bd02bee807cb141cbe74b37fad83",
    "tests/t991/approved-tests.cmake": "2b8438f52ee4d29cd8bc2ccc07ef99953a385533",
    "tests/t991/runner.py": "1614ef0168488a3afdd92f3416499ec34dc5ec21",
}
EVIDENCE = "tests/p990/evidence/SHA256SUMS"
EVIDENCE_SHA = "0df6bddf6e4e3b6ce0e290aa5de074825e9c7a3f5ed8e140dacf9e174dddccb5"
EVIDENCE_OID = "24205baf64cb7ddb4307d34508667566aa8531b3"
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


def relative_name(name):
    require(isinstance(name, str) and name and "\\" not in name and "\0" not in name,
            "invalid freeze path")
    require(all(part not in ("", ".", "..") for part in name.split("/")),
            "nonlocal freeze path: " + name)
    return name


def local(root, name):
    relative_name(name)
    path = root
    parts = name.split("/")
    for index, part in enumerate(parts):
        path = path / part
        try:
            mode = path.lstat().st_mode
        except OSError as error:
            raise FreezeError("missing freeze input: " + name) from error
        require(not stat.S_ISLNK(mode), "symlink freeze path: " + name)
        require(stat.S_ISREG(mode) if index + 1 == len(parts) else stat.S_ISDIR(mode),
                "nonregular freeze path: " + name)
    return path


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, "duplicate freeze JSON key: " + key)
        result[key] = value
    return result


def invalid_constant(value):
    raise FreezeError("invalid freeze JSON constant: " + value)


def json_data(data):
    try:
        return json.loads(data, object_pairs_hook=unique_object,
                          parse_constant=invalid_constant)
    except (ValueError, UnicodeError) as error:
        raise FreezeError("invalid freeze JSON") from error


def valid_digest(value):
    return isinstance(value, str) and re.fullmatch("[0-9a-f]{64}", value) is not None


def manifest_data(root, name):
    data = local(root, name).read_bytes()
    for fixed_name, sha, oid in BASELINES.values():
        if name == fixed_name:
            require(digest(data) == sha, "historical freeze SHA256: " + name)
            require(git_oid("blob", data) == oid, "historical freeze Git blob: " + name)
    doc = json_data(data)
    fields = {"schema", "parent", "generation", "previous", "stage", "files"}
    require(isinstance(doc, dict) and doc.keys() == fields, "freeze manifest fields")
    require(type(doc["schema"]) is int and doc["schema"] == 1 and
            doc["parent"] == PARENT, "current freeze schema/parent")
    generation = doc["generation"]
    require(type(generation) is int and 1 <= generation <= 64,
            "current freeze generation")
    if generation in BASELINES:
        require(name == BASELINES[generation][0], "historical freeze path")
    require(isinstance(doc["stage"], str), "freeze manifest stage")
    previous = doc["previous"]
    if previous is not None:
        require(isinstance(previous, dict) and previous.keys() == {"path", "sha256"},
                "freeze predecessor fields")
        relative_name(previous["path"])
        require(valid_digest(previous["sha256"]), "freeze predecessor SHA256 format")
    files = doc["files"]
    require(isinstance(files, dict) and files, "freeze files map")
    for path, expected in files.items():
        relative_name(path)
        require(valid_digest(expected), "freeze SHA256 format: " + path)
    return doc, data


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


def reviewed_history(root):
    required = set(TOOLING_BLOBS)
    for name, oid in TOOLING_BLOBS.items():
        require(git_oid("blob", local(root, name).read_bytes()) == oid,
                "immutable T997 blob mismatch: " + name)
    baseline = None
    for name, _, _ in BASELINES.values():
        baseline, _ = manifest_data(root, name)
        required.add(name)
        required.update(baseline["files"])
    # Original source/oracle/recorded JSON remains immutable; current code and
    # observers/runners can evolve under a reviewed new current generation.
    for name, expected in baseline["files"].items():
        if name.startswith("tests/p990/") and name.endswith((".json", ".nl")):
            require(digest(local(root, name).read_bytes()) == expected,
                    "original P990 frozen input changed: " + name)
    data = local(root, EVIDENCE).read_bytes()
    require(digest(data) == EVIDENCE_SHA and git_oid("blob", data) == EVIDENCE_OID,
            "original P990 evidence manifest changed")
    required.add(EVIDENCE)
    for line in data.decode().splitlines():
        expected, name = line.split("  ", 1)
        name = "tests/p990/evidence/" + name
        require(digest(local(root, name).read_bytes()) == expected,
                "original P990 evidence changed: " + name)
        required.add(name)
    return required


def _verify(root, manifest):
    current_name = relative_name(manifest)
    required = history(root)
    required.update(reviewed_history(root))
    current, _ = manifest_data(root, current_name)
    generation = current["generation"]
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
        older, data = manifest_data(root, previous_name)
        require(digest(data) == previous["sha256"], "previous freeze digest")
        missing = older["files"].keys() - cursor["files"].keys()
        require(not missing, "freeze coverage lost: " + ", ".join(sorted(missing)))
        require(cursor["files"].get(previous_name) == previous["sha256"],
                "successor must protect predecessor manifest")
        required.update(older["files"])
        required.add(previous_name)
        cursor, cursor_name = older, previous_name
        generation -= 1
    files = current["files"]
    require(required <= files.keys(), "current freeze omits protected inputs")
    for name, expected in files.items():
        require(digest(local(root, name).read_bytes()) == expected,
                "current freeze digest: " + name)
    return len(files)


def verify(root, manifest):
    try:
        return _verify(Path(root).resolve(), manifest)
    except (OSError, ValueError, TypeError, KeyError, UnicodeError) as error:
        raise FreezeError("invalid freeze data: " + str(error)) from error
