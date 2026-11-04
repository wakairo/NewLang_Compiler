"""Run original freeze guards on a verified fixed-P snapshot/private index.

The working tree is first proven to be fixed P plus only R files and the
explicit CMake registration adapter. Original assertions and expected bytes
are never changed. The snapshot is test apparatus, not the measured compiler.
"""
from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
P_TREE = "6ebba6e90fc9e7a4db441663873a7780614647be"
CMAKE_BLOB = "5ddbfaa8a90097cdc1f787ca90075314178b4dff"
CMAKE_SHA = "b74082e37f9723516ecce2295c3a3ee36841af4be56c1bff5cfceccc07a6d98c"


def git(*args, env=None, data=None):
    return subprocess.check_output(
        ["git", "-C", str(ROOT), *args], env=env, input=data
    )


def main():
    assert len(sys.argv) >= 2
    original, arguments = sys.argv[1], sys.argv[2:]
    historical = {
        ("tests/p982/freeze_test.py", ()),
        ("tests/p990/freeze_selftest.py", ()),
        ("tests/p1088/evidence_test.py", ()),
        ("tests/p1117/history_latest.py", ("p1107",)),
        ("tests/p1117/history_latest.py", ("p1117",)),
        ("tests/p1117/history_latest.py", ("p1098",)),
    }
    native = {
        "tests/p990/run.py": 5,
        "tests/p1055/p1045_forward.py": 3,
        "tests/p1055/run.py": 4,
        "tests/p1058/run.py": 4,
        "tests/p1062/run.py": 4,
    }
    if original in native:
        assert len(arguments) == native[original]
        assert arguments[2] in {"none", "address", "undefined"}
        if original == "tests/p990/run.py":
            assert Path(arguments[3]) == ROOT / "tests/p990"
            assert arguments[4] == "new"
        else:
            assert len(arguments) == 3 or arguments[3] == "0"
        # Keep the actual R checkout's compiled CLI. Only historical source
        # paths are transported; no baseline compiler or guest is substituted.
        compiler = Path(arguments[0])
        assert compiler.is_absolute() and compiler.is_file()
    else:
        assert (original, tuple(arguments)) in historical
    baseline = (ROOT / "tests/r1151/CMakeLists.fixed.data").read_bytes()
    assert hashlib.sha256(baseline).hexdigest() == CMAKE_SHA
    with tempfile.TemporaryDirectory(prefix="r1151-freeze-") as tmp:
        tmp = Path(tmp)
        environment = os.environ.copy()
        environment["GIT_DIR"] = git(
            "rev-parse", "--absolute-git-dir"
        ).decode().strip()
        environment["GIT_INDEX_FILE"] = str(tmp / "private.index")
        environment["GIT_WORK_TREE"] = str(ROOT)
        git("read-tree", "HEAD", env=environment)
        tracked = git("ls-files", "-z", env=environment).split(b"\0")
        additions = [
            p for p in tracked if p.startswith(b"tests/r1151/")
        ]
        assert additions
        git("update-index", "--force-remove", "-z", "--stdin",
            env=environment, data=b"\0".join(additions) + b"\0")
        blob = git("hash-object", "-w", "--stdin", data=baseline).decode().strip()
        assert blob == CMAKE_BLOB
        git("update-index", "--cacheinfo", "100644", blob, "CMakeLists.txt",
            env=environment)
        actual = git("write-tree", env=environment).decode().strip()
        assert actual == P_TREE, ("non-R production or history drift", actual)
        snapshot = tmp / "snapshot"
        snapshot.mkdir()
        for name in git("ls-files", "-z", env=environment).split(b"\0"):
            if not name:
                continue
            relative = name.decode()
            source = ROOT / relative
            target = snapshot / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            if relative == "CMakeLists.txt":
                target.write_bytes(baseline)
            elif source.is_symlink():
                target.symlink_to(os.readlink(source))
            else:
                shutil.copy2(source, target)
        environment["GIT_WORK_TREE"] = str(snapshot)
        print("R1151 fixed-P snapshot tree", actual, "original guard", original,
              flush=True)
        forwarded = list(arguments)
        if original == "tests/p990/run.py":
            # Transport only the explicit source-data root. CLI, CC,
            # sanitizer and group are the original actual-R arguments.
            forwarded[3] = str(snapshot / "tests/p990")
        completed = subprocess.run(
            [sys.executable, str(snapshot / original), *forwarded],
            cwd=snapshot, env=environment, check=False
        )
        return completed.returncode


if __name__ == "__main__":
    raise SystemExit(main())
