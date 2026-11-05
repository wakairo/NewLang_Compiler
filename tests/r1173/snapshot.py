"""R-owned, fixed-tree Git context; invoked by Actions CMake configure only."""
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import stat
import subprocess
import sys
import zlib

PIN = "870e4f0972abb9725b7cf9f38d09178e51ff1280"
TREE = "5db3576a09f2627f8975a21295419dee6433a629"


def object_bytes(kind, payload):
    return (kind + " " + str(len(payload)) + "\0").encode() + payload


def main():
    source, target = (Path(x).resolve() for x in sys.argv[1:3])
    own = source / "tests/r1173"
    manifest = json.loads((own / "fixed-tree.json").read_text())
    if manifest["truncated"] or manifest["sha"] != TREE:
        raise RuntimeError("fixed tree manifest identity")
    if target.exists():
        raise RuntimeError("fixed context already exists; never reuse a dirty index")
    env = {k: v for k, v in os.environ.items() if not k.startswith("GIT_")}
    env.update(GIT_CONFIG_NOSYSTEM="1", GIT_CONFIG_GLOBAL=os.devnull)
    subprocess.run(["git", "init", "--quiet", str(target)], env=env, check=True)
    gitdir = target / ".git"

    def git(*args):
        return subprocess.check_output(["git", "-C", str(target), *args], env=env)

    def put(kind, payload, expected):
        raw = object_bytes(kind, payload)
        actual = hashlib.sha1(raw).hexdigest()
        if actual != expected:
            raise RuntimeError(f"fixed {kind} object mismatch {expected} != {actual}")
        dest = gitdir / "objects" / expected[:2] / expected[2:]
        if not dest.exists():
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(zlib.compress(raw))

    entries = manifest["tree"]
    leaves = []
    children = {}
    expected_trees = {"": TREE}
    for entry in entries:
        path = PurePosixPath(entry["path"])
        if path.is_absolute() or ".." in path.parts or not path.parts:
            raise RuntimeError("unsafe fixed path")
        parent = str(path.parent)
        if parent == ".":
            parent = ""
        children.setdefault(parent, []).append(entry)
        if entry["type"] == "tree":
            expected_trees[str(path)] = entry["sha"]
            continue
        if entry["type"] != "blob" or entry["mode"] not in ("100644", "100755", "120000"):
            raise RuntimeError("unsupported fixed tree leaf")
        origin = source / str(path)
        if str(path) == "CMakeLists.txt":
            data = (own / "fixed-CMakeLists.txt").read_bytes()
        elif entry["mode"] == "120000":
            if not origin.is_symlink():
                raise RuntimeError(f"fixed symlink mode mismatch: {path}")
            data = os.fsencode(os.readlink(origin))
        else:
            if not origin.is_file() or origin.is_symlink():
                raise RuntimeError(f"fixed regular mode mismatch: {path}")
            executable = bool(origin.stat().st_mode & stat.S_IXUSR)
            if executable != (entry["mode"] == "100755"):
                raise RuntimeError(f"fixed executable mode mismatch: {path}")
            data = origin.read_bytes()
        put("blob", data, entry["sha"])
        dest = target / str(path)
        dest.parent.mkdir(parents=True, exist_ok=True)
        if entry["mode"] == "120000":
            os.symlink(os.fsdecode(data), dest)
        else:
            dest.write_bytes(data)
            dest.chmod(0o755 if entry["mode"] == "100755" else 0o644)
        leaves.append(entry)
    for directory in sorted(expected_trees, key=lambda p: -p.count("/") - bool(p)):
        contents = []
        for entry in sorted(children.get(directory, []), key=lambda e: (
                PurePosixPath(e["path"]).name + ("/" if e["type"] == "tree" else "")).encode()):
            name = PurePosixPath(entry["path"]).name
            mode = "40000" if entry["type"] == "tree" else entry["mode"]
            contents.append((mode + " " + name + "\0").encode() + bytes.fromhex(entry["sha"]))
        put("tree", b"".join(contents), expected_trees[directory])
    put("commit", (own / "fixed-commit.raw").read_bytes(), PIN)
    (gitdir / "refs/heads/fixed-r1173-input").write_text(PIN + "\n")
    (gitdir / "HEAD").write_text("ref: refs/heads/fixed-r1173-input\n")
    (gitdir / "shallow").write_text(PIN + "\n")
    git("read-tree", "--no-sparse-checkout", TREE)
    expected_index = sorted((e["mode"] + " " + e["sha"] + " 0\t" + e["path"]).encode()
                            for e in leaves)
    actual_index = sorted(x for x in git("ls-files", "--stage", "-z").split(b"\0") if x)
    if actual_index != expected_index:
        raise RuntimeError("canonical stage-0 index mismatch")
    if git("rev-parse", "HEAD").decode().strip() != PIN:
        raise RuntimeError("fixed HEAD mismatch")
    if git("rev-parse", "HEAD^{tree}").decode().strip() != TREE:
        raise RuntimeError("fixed HEAD tree mismatch")
    if git("status", "--porcelain=v1", "--untracked-files=all"):
        raise RuntimeError("fixed working tree/index is not clean")
    receipt = {"schema": "R1173_FIXED_GIT_V1", "commit": PIN, "tree": TREE,
               "leaves": len(leaves), "trees": len(expected_trees),
               "index_sha256": hashlib.sha256(b"\0".join(actual_index)).hexdigest(),
               "source_head": subprocess.check_output(
                   ["git", "-C", str(source), "rev-parse", "HEAD"], env=env).decode().strip(),
               "source_tree": subprocess.check_output(
                   ["git", "-C", str(source), "rev-parse", "HEAD^{tree}"], env=env).decode().strip(),
               "status": "VERIFIED_CANONICAL_OBJECTS_INDEX_AND_WORKTREE",
               "boundary": "one genuine fixed commit; parent history not materialized"}
    receipt_path = target.parent / "r1173-fixed-context.json"
    receipt_path.write_text(json.dumps(receipt, indent=2) + "\n")
    print("R1173 FIXED-GIT " + json.dumps(receipt, sort_keys=True), flush=True)


if __name__ == "__main__":
    main()
