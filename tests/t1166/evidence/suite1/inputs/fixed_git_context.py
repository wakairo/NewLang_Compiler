"""T1166: one fixed commit/tree, one owned temporary Git/Python snapshot.

No network, shared object stores, linked worktrees, source index writes, or
compiler execution. Callers supply an already available object repository and
an already fixed file copy. Only regular Git modes 100644/100755 are supported.
"""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import subprocess
import tempfile
import uuid


class SnapshotError(RuntimeError):
    pass


def git_environment(environ=None):
    """Discard ambient Git routing/config; retain every non-Git variable."""
    env = dict(os.environ if environ is None else environ)
    env = {key: value for key, value in env.items() if not key.startswith("GIT_")}
    env.update(GIT_CONFIG_GLOBAL=os.devnull, GIT_CONFIG_SYSTEM=os.devnull,
               GIT_CONFIG_NOSYSTEM="1", GIT_ATTR_NOSYSTEM="1",
               GIT_NO_REPLACE_OBJECTS="1", GIT_NO_LAZY_FETCH="1",
               GIT_OPTIONAL_LOCKS="0",
               GIT_TERMINAL_PROMPT="0")
    return env


def object_id(kind, data):
    return hashlib.sha1(kind.encode("ascii") + b" " + str(len(data)).encode("ascii")
                        + b"\0" + data).hexdigest()


def checked_directory(value):
    path = Path(value).absolute()
    if not path.is_dir() or path.is_symlink() or path != path.resolve():
        raise SnapshotError("noncanonical or symlink directory: " + str(path))
    return path


def relative_file(name):
    parts = PurePosixPath(name).parts
    if (not parts or name != "/".join(parts) or name.startswith("/")
            or any(part in (".", "..", ".git") for part in parts)
            or "\\" in name or "\0" in name):
        raise SnapshotError("unsafe Git path: " + repr(name))
    return parts


def leaf_bytes(root, name):
    path = root
    parts = relative_file(name)
    for part in parts[:-1]:
        path /= part
        if not stat.S_ISDIR(path.lstat().st_mode) or path.is_symlink():
            raise SnapshotError("non-directory/symlink input parent: " + name)
    path /= parts[-1]
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW)
    with os.fdopen(fd, "rb") as stream:
        info = os.fstat(stream.fileno())
        if not stat.S_ISREG(info.st_mode):
            raise SnapshotError("nonregular input: " + name)
        mode = "100755" if info.st_mode & 0o111 else "100644"
        return mode, stream.read()


def ordinary_command(argv, *, cwd, env, input=None):
    return subprocess.run(argv, cwd=cwd, env=env, input=input,
                          capture_output=True, timeout=60, check=False)


class FixedGitContext:
    """Synchronous owner. All consumers finish before close()/context exit.

    Git/Python operations assume exclusive access to this owned context; no
    concurrent adversary or background consumer is supported. The source is
    read-only. Failure to prove cleanup ownership raises and leaves the path.
    """

    @classmethod
    def create(cls, source_repo, copy_root, parent, commit, tree, *,
               environ=None, command=ordinary_command):
        if not all(re.fullmatch(r"[0-9a-f]{40}", value) for value in (commit, tree)):
            raise SnapshotError("full SHA-1 commit/tree required")
        source_repo, copy_root, parent = map(
            checked_directory, (source_repo, copy_root, parent))
        obj = cls()
        obj.commit, obj.tree = commit, tree
        obj.environ = git_environment(environ)
        obj.command = command
        obj.git = shutil.which("git", path=obj.environ.get("PATH"))
        if not obj.git:
            raise SnapshotError("Git executable unavailable")
        obj.root = Path(tempfile.mkdtemp(prefix="t1166-", dir=parent))
        info = obj.root.lstat()
        obj.identity = (info.st_dev, info.st_ino)
        obj.token = uuid.uuid4().hex
        obj.marker = obj.root / "owner.json"
        obj.marker.write_text(json.dumps({"token": obj.token}) + "\n")
        obj.work = obj.root / "work"
        obj.gitdir = obj.root / "git"
        obj.closed = False
        try:
            template = obj.root / "empty-template"
            template.mkdir()
            obj._git(parent, "init", "--quiet", "--object-format=sha1",
                     "--template=" + str(template),
                     "--separate-git-dir=" + str(obj.gitdir), str(obj.work))
            actual_root = obj._git(source_repo, "rev-parse", "--show-toplevel").strip()
            if Path(os.fsdecode(actual_root)).resolve() != source_repo:
                raise SnapshotError("source repository root mismatch")
            commit_bytes = obj._git(source_repo, "cat-file", "commit", commit)
            if (object_id("commit", commit_bytes) != commit
                    or commit_bytes.splitlines()[0] != ("tree " + tree).encode("ascii")):
                raise SnapshotError("fixed commit/tree identity mismatch")
            raw = obj._git(source_repo, "ls-tree", "-r", "-t", "-z",
                           "--full-tree", tree)
            obj.inventory = {}
            objects = {commit, tree}
            for record in raw.split(b"\0"):
                if not record:
                    continue
                header, name = record.split(b"\t", 1)
                mode, kind, oid = header.decode("ascii").split()
                name = name.decode("utf-8")
                relative_file(name)
                if not re.fullmatch(r"[0-9a-f]{40}", oid):
                    raise SnapshotError("invalid tree object ID")
                objects.add(oid)
                if kind == "tree" and mode == "040000":
                    continue
                if kind != "blob" or mode not in ("100644", "100755"):
                    raise SnapshotError("unsupported Git mode/type: " + name)
                if name in obj.inventory:
                    raise SnapshotError("duplicate Git leaf: " + name)
                obj.inventory[name] = (mode, oid)
            if not obj.inventory:
                raise SnapshotError("empty fixed inventory")
            # An explicit finite object closure; no revision traversal, fetch,
            # alternates, hardlinks, ref mutation, or source pack writes.
            object_lines = ("\n".join(sorted(objects)) + "\n").encode("ascii")
            pack = obj._git(source_repo, "pack-objects", "--stdout", input=object_lines)
            obj._git(obj.work, "index-pack", "--stdin", input=pack)
            # Verify the transferred bytes too: object filenames alone are not
            # a hash check. Batch reads stay within this owned object store.
            batch = obj._git(obj.work, "cat-file", "--batch", input=object_lines)
            offset = 0
            for expected in sorted(objects):
                end = batch.index(b"\n", offset)
                oid, kind, size = batch[offset:end].decode("ascii").split()
                offset = end + 1
                size = int(size)
                data = batch[offset:offset + size]
                offset += size
                if (oid != expected or object_id(kind, data) != expected
                        or len(data) != size or batch[offset:offset + 1] != b"\n"):
                    raise SnapshotError("transferred object identity mismatch")
                offset += 1
            if offset != len(batch):
                raise SnapshotError("extra transferred object output")
            (obj.gitdir / "HEAD").write_text(commit + "\n", encoding="ascii")
            if any(line.startswith(b"parent ") for line in commit_bytes.split(b"\n\n", 1)[0].splitlines()):
                (obj.gitdir / "shallow").write_text(commit + "\n", encoding="ascii")
            for name, (mode, oid) in obj.inventory.items():
                actual_mode, data = leaf_bytes(copy_root, name)
                if actual_mode != mode or object_id("blob", data) != oid:
                    raise SnapshotError("fixed copy mode/blob mismatch: " + name)
                target = obj.work.joinpath(*relative_file(name))
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(data)
                target.chmod(0o755 if mode == "100755" else 0o644)
            obj._git(obj.work, "read-tree", "--no-sparse-checkout", tree)
            obj.verify()
            obj.receipt = {"commit": commit, "tree": tree,
                           "leaves": len(obj.inventory), "object_count": len(objects),
                           "pack_bytes": len(pack), "copied_bytes": sum(
                               (obj.work / name).stat().st_size for name in obj.inventory),
                           "work": str(obj.work), "gitdir": str(obj.gitdir),
                           "index": str(obj.gitdir / "index"),
                           "inventory": obj.inventory}
            (obj.root / "receipt.json").write_text(json.dumps(obj.receipt, indent=2) + "\n")
            return obj
        except BaseException:
            obj.close()
            raise

    def _git(self, cwd, *args, input=None):
        argv = [self.git, "--no-replace-objects", "-c", "core.fsmonitor=false",
                "-c", "core.untrackedCache=false", "-C", str(cwd), *args]
        result = self.command(argv, cwd=str(cwd), env=self.environ, input=input)
        if result.returncode:
            raise SnapshotError("Git failed (%d): %s" % (result.returncode, " ".join(args)))
        return result.stdout

    def verify(self):
        if self.closed:
            raise SnapshotError("context already closed")
        locations = (("--show-toplevel", self.work),
                     ("--absolute-git-dir", self.gitdir),
                     ("--git-common-dir", self.gitdir),
                     ("--git-path", self.gitdir / "index"))
        for flag, expected in locations:
            extra = ("index",) if flag == "--git-path" else ()
            observed = self._git(self.work, "rev-parse", flag, *extra).strip()
            if Path(os.fsdecode(observed)).resolve() != expected:
                raise SnapshotError("Git context path mismatch: " + flag)
        if (self.gitdir / "objects/info/alternates").exists():
            raise SnapshotError("shared object store is forbidden")
        if self._git(self.work, "rev-parse", "HEAD").strip().decode() != self.commit:
            raise SnapshotError("detached HEAD mismatch")
        observed = {}
        for record in self._git(self.work, "ls-files", "--stage", "-z").split(b"\0"):
            if not record:
                continue
            header, name = record.split(b"\t", 1)
            mode, oid, stage = header.decode("ascii").split()
            name = name.decode("utf-8")
            if stage != "0" or name in observed:
                raise SnapshotError("nonzero/duplicate index stage")
            observed[name] = (mode, oid)
        if observed != self.inventory:
            raise SnapshotError("index inventory/mode/blob mismatch")
        for name, (mode, oid) in self.inventory.items():
            actual_mode, data = leaf_bytes(self.work, name)
            if actual_mode != mode or object_id("blob", data) != oid:
                raise SnapshotError("working mode/blob mismatch: " + name)
        return len(observed)

    def require_tracked(self, names):
        self.verify()
        for name in names:
            relative_file(name)
            if name not in self.inventory:
                raise SnapshotError("untracked prospective input: " + name)

    def run(self, argv):
        """Run one caller-supplied command unchanged, with isolated Git env."""
        self.verify()
        result = self.command(list(argv), cwd=str(self.work), env=self.environ)
        self.verify()
        return result

    def close(self):
        if self.closed:
            return
        try:
            info = self.root.lstat()
            valid = (stat.S_ISDIR(info.st_mode) and not self.root.is_symlink()
                     and (info.st_dev, info.st_ino) == self.identity
                     and not self.marker.is_symlink()
                     and json.loads(self.marker.read_text()) == {"token": self.token})
        except (OSError, ValueError):
            valid = False
        if not valid:
            raise SnapshotError("cleanup ownership mismatch; retained: " + str(self.root))
        if not shutil.rmtree.avoids_symlink_attacks:
            raise SnapshotError("safe recursive cleanup unavailable; retained: " + str(self.root))
        shutil.rmtree(self.root)
        self.closed = True

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()
