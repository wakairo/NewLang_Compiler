"""T1230 prospective CURRENT, retaining the authentic P1217 v6 + its historical negatives.

Not an issuer or a language semantic rule.  This adapter is a T-owned draft and
must not replace or mutate original source/oracle/FIRST/capsule/trust inputs.
"""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/support"))
sys.path.insert(0, str(ROOT / "tests/p1217"))
from versioned_freeze import FreezeError, digest, git_oid, require
import evidence_current_v6 as old_v6

ORIGINAL_ROOT = "tests/t1230/escrow/root-CMake-aa8dce.txt"
ORIGINAL_P1217_CMAKE = "tests/t1230/escrow/p1217-CMake-v6.txt"
OLD_ROOT_OID = "ffdc9cb6dfcc9dabe5dd8fabddde15edb41bbcbf"
OLD_ROOT_SHA = "04b2628ef1f636e0c28f535c8496ec499b5491c697f8a244ae82ca37113e5429"
OLD_P1217_CMAKE_OID = "de3acc8200a490aaf75eaabbc653c9046cef7a1e"
OLD_V59_OID = "78a04a10d509b137d8e906590e6cf4d72072ec4b"
OLD_V59_SHA = "228d16101791f78c6e7ad2011df7bdb2fe4db4611bb2bd9b570f6e9f32426f68"
CURRENT_V60 = "tests/p990/current-freeze-v60.json"
ROOT_PATCH = (b"    # T1230: target only two measured near-90s CTests.\n"
              b"    set_tests_properties(\n"
              b"        p1055_old_origin_0.integration\n"
              b"        p990_new_native.integration\n"
              b"        PROPERTIES TIMEOUT 180)\n")


def get(root, name, overrides=None):
    if overrides and name in overrides:
        return overrides[name]
    path = root / name
    require(not path.is_symlink() and path.is_file(), "T1230 missing/symlink: " + name)
    return path.read_bytes()


def expected_root(old):
    # One uniquely located insertion, after all old 90/300 assignments.
    suffix = b"    endforeach()\nendif()\n"
    require(old.endswith(suffix), "T1230 original root ending")
    return old[:-len(suffix)] + suffix[:-len(b"endif()\n")] + ROOT_PATCH + b"endif()\n"


def expected_p1217(old):
    changes = ((b"tests/p1217/evidence_current_v6.py",
                b"tests/t1230/p1217_current_v7.py"),
               (b"tests/p1217/c277/negative_test.py",
                b"tests/t1230/p1217_negative_v7.py"))
    for before, after in changes:
        require(old.count(before) == 1, "T1230 original P1217 registration uniqueness")
        old = old.replace(before, after, 1)
    return old


def verify_current(root, overrides=None):
    """Check source-owned CURRENT against true historical root, not a rehashed guess."""
    root = Path(root).resolve()
    oldroot = get(root, ORIGINAL_ROOT, overrides)
    require(digest(oldroot) == OLD_ROOT_SHA and
            git_oid("blob", oldroot) == OLD_ROOT_OID, "T1230 original Git root escrow")
    oldp = get(root, ORIGINAL_P1217_CMAKE, overrides)
    require(git_oid("blob", oldp) == OLD_P1217_CMAKE_OID,
            "T1230 original P1217 v6 registration escrow")
    capsule = json.loads(get(root, old_v6.CAPSULE, overrides))
    baseline = capsule["snapshots"]["baseline"]
    require(baseline["files"]["CMakeLists.txt"]["oid"] == OLD_ROOT_OID and
            baseline["files"]["CMakeLists.txt"]["sha256"] == OLD_ROOT_SHA and
            old_v6.lookup(capsule, baseline["tree"], ["CMakeLists.txt"]) == OLD_ROOT_OID,
            "T1230 authentic original baseline Git tree membership")
    v59 = get(root, "tests/p990/current-freeze-v59.json", overrides)
    require(digest(v59) == OLD_V59_SHA and
            git_oid("blob", v59) == OLD_V59_OID,
            "T1230 immutable original v59 Git identity")
    old_files = json.loads(v59)["files"]
    require(len(old_files) == 4072 and
            old_files["CMakeLists.txt"] == OLD_ROOT_SHA and
            old_files["tests/p1217/CMakeLists.txt"] == digest(oldp),
            "T1230 original current history anchors")
    actualroot = get(root, "CMakeLists.txt", overrides)
    require(actualroot == expected_root(oldroot),
            "T1230 CURRENT root contains exactly two post-global exceptions")
    currentp = get(root, "tests/p1217/CMakeLists.txt", overrides)
    require(currentp == expected_p1217(oldp),
            "T1230 CURRENT P1217 evidence/tamper commands independently derived")
    current = json.loads(get(root, CURRENT_V60, overrides))
    require(current["generation"] == 60 and
            current["previous"] == {"path": "tests/p990/current-freeze-v59.json",
                                    "sha256": OLD_V59_SHA} and
            old_files.keys() <= current["files"].keys(),
            "T1230 prospective forward history inherits immutable v59")
    for name in ("CMakeLists.txt", "tests/p1217/CMakeLists.txt",
                 ORIGINAL_ROOT, ORIGINAL_P1217_CMAKE,
                 "tests/t1230/p1217_current_v7.py",
                 "tests/t1230/p1217_negative_v7.py"):
        require(current["files"].get(name) == digest(get(root, name, overrides)),
                "T1230 CURRENT v60 evidence digest: " + name)
    return {"original_root_git": OLD_ROOT_OID, "original_root_sha256": OLD_ROOT_SHA,
            "current_root_sha256": digest(actualroot),
            "protected_v59_count": len(old_files),
            "current_v60_count": len(current["files"])}


def original_view(root, directory):
    """Exact original root/P1217 registration for the unchanged legacy v6 tests."""
    root, target = Path(root).resolve(), Path(directory)
    (target / "tests").mkdir(parents=True)
    shutil.copytree(root / "tests/p1217", target / "tests/p1217")
    shutil.copytree(root / "tests/support", target / "tests/support")
    (target / "CMakeLists.txt").write_bytes(get(root, ORIGINAL_ROOT))
    (target / "tests/p1217/CMakeLists.txt").write_bytes(get(root, ORIGINAL_P1217_CMAKE))
    return target


def original_v6_positive(root):
    with tempfile.TemporaryDirectory(prefix="t1230-true-old-v6-") as directory:
        original_view(root, directory)
        result = old_v6.verify(directory)
    return result


def original_v6_negatives(root):
    with tempfile.TemporaryDirectory(prefix="t1230-true-old-negatives-") as directory:
        old = original_view(root, directory)
        env = dict(os.environ, PYTHONDONTWRITEBYTECODE="1", PYTHONHASHSEED="0",
                   PYTHONOPTIMIZE="0", LC_ALL="C")
        cmd = [sys.executable, str(old / "tests/p1217/c277/negative_test.py")]
        proc = subprocess.run(cmd, cwd=old, env=env, capture_output=True, timeout=60)
        require(proc.returncode == 0 and
                b"all10 negative/ambient controls PASS" in proc.stdout,
                "T1230 unchanged original v6 ten negative controls: " +
                proc.stderr.decode(errors="replace")[-500:])
        return digest(proc.stdout)


if __name__ == "__main__":
    try:
        state = verify_current(ROOT)
        state["original_v6_verifier"] = original_v6_positive(ROOT)
        try:
            old_v6.verify(ROOT)
        except FreezeError as err:
            require("baseline blob: CMakeLists.txt" in str(err) or
                    "baseline blob: tests/p1217/CMakeLists.txt" in str(err),
                    "T1230 original v6 should reject altered current CMake precisely")
            state["old_v6_rejects_new_current"] = str(err)
        else:
            raise FreezeError("T1230 STOP: old v6 unexpectedly accepted altered current")
    except (ValueError, KeyError, TypeError, OSError, FreezeError) as err:
        raise SystemExit("T1230 prospective historical evidence REFUSE: " + str(err))
    print("T1230 CURRENT and original-v6 positive PASS " + json.dumps(state, sort_keys=True))
