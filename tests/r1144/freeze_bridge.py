"""Authenticate R CMake wiring, then run unchanged protected freeze callers."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CURRENT = "tests/p990/current-freeze-v35.json"
CURRENT_SHA = "518851e3c35046ef1212684f6c8003b4413340b90574d772122c728dbf37f3e1"
PARENT_CMAKE_SHA = "98c0f59e372ea21f0e7b0c0ce345e5bacd07544f30c9bd686faaa2f50920c6d7"
SCRIPTS = {
    "baseline": ("tests/p982/freeze_test.py", 0),
    "protocol": ("tests/p990/freeze_selftest.py", 0),
    "p1088_evidence.unit": ("tests/p1088/evidence_test.py", 0),
    "p1107_evidence_history.unit": ("tests/p1117/history_latest.py", 1),
    "p1117_evidence_history.unit": ("tests/p1117/history_latest.py", 1),
    "p1098_evidence_history.unit": ("tests/p1117/history_latest.py", 1),
    "p990_new_native.integration": ("tests/p990/run.py", 5),
    "p1045_two_original_native.integration": ("tests/p1055/p1045_forward.py", 3),
    "p1055_old_origin_0.integration": ("tests/p1055/run.py", 4),
    "p1058_closed_h_0.integration": ("tests/p1058/run.py", 4),
    "p1062_h_body_0.integration": ("tests/p1062/run.py", 4),
}


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def blob_oid(raw):
    return hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()


def git(root, *args, data=None):
    return subprocess.run(["git", "-C", str(root), *args], input=data,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          check=True).stdout


mode = sys.argv[1]
assert mode in SCRIPTS, "bounded original test command"
script, argc = SCRIPTS[mode]
arguments = sys.argv[2:]
assert len(arguments) == argc, "original argument count"
if mode.endswith("_evidence_history.unit"):
    assert arguments == [mode.split("_", 1)[0]], "original history family"
if mode in ("p1055_old_origin_0.integration", "p1058_closed_h_0.integration",
            "p1062_h_body_0.integration"):
    assert arguments[-1] == "0", "only original profile zero is projected"
if mode == "p990_new_native.integration":
    assert Path(arguments[3]).resolve() == ROOT / "tests/p990"
    assert arguments[4] == "new", "only original new profile is projected"

current_raw = (ROOT / CURRENT).read_bytes()
assert sha(current_raw) == CURRENT_SHA, "unchanged current freeze manifest"
current = json.loads(current_raw)
assert script in current["files"], "original entry script is protected"
r_manifest = json.loads((ROOT / "tests/r1144/harness-manifest.json").read_bytes())
for name, expected in r_manifest["files"].items():
    assert sha((ROOT / name).read_bytes()) == expected, "R input changed: " + name
parent_cmake = (ROOT / "tests/r1144/parent-CMakeLists.txt").read_bytes()
assert sha(parent_cmake) == PARENT_CMAKE_SHA
assert current["files"]["CMakeLists.txt"] == PARENT_CMAKE_SHA

# Preserve actual tracked records. Do not infer inventory by adding every file
# in the snapshot: original ambient/untracked-file controls must remain real.
assert git(ROOT, "rev-parse", "--show-object-format").strip() == b"sha1"
original_index = git(ROOT, "ls-files", "--stage", "-z")
records = []
indexed = {}
for record in original_index.split(b"\0"):
    if not record:
        continue
    metadata, name_raw = record.split(b"\t", 1)
    file_mode, oid, stage = metadata.split(b" ")
    assert stage == b"0", "unmerged actual checkout"
    name = os.fsdecode(name_raw)
    assert name not in indexed, "duplicate actual index path"
    indexed[name] = (file_mode, oid)
    records.append((file_mode, oid, name_raw))
assert indexed["CMakeLists.txt"][0] == b"100644"
actual_objects = Path(os.fsdecode(git(ROOT, "rev-parse", "--git-path", "objects").strip()))
if not actual_objects.is_absolute():
    actual_objects = ROOT / actual_objects
actual_objects = actual_objects.resolve(strict=True)
assert "\n" not in str(actual_objects)

with tempfile.TemporaryDirectory(prefix="r1144-freeze-") as directory:
    staged = Path(directory)
    for name in sorted(set(current["files"]) | {CURRENT}):
        source = ROOT / name
        raw = source.read_bytes()
        file_mode, oid = indexed[name]
        assert file_mode in (b"100644", b"100755"), "protected regular file mode"
        assert blob_oid(raw).encode() == oid, "actual index content: " + name
        if name == "CMakeLists.txt":
            projected = parent_cmake
        else:
            projected = raw
            if name in current["files"]:
                assert sha(raw) == current["files"][name], "protected input: " + name
        target = staged / name
        target.parent.mkdir(parents=True, exist_ok=True)
        if name == "CMakeLists.txt":
            target.write_bytes(projected)
        else:
            shutil.copyfile(source, target)
        target.chmod(0o755 if file_mode == b"100755" else 0o644)
        assert target.read_bytes() == projected, "authenticated snapshot: " + name

    # A private repository/index supplies normal Git inventory semantics. It
    # reads original objects; only the authenticated parent CMake OID changes.
    git(staged, "init", "--quiet", "--template=")
    alternate = staged / ".git/objects/info/alternates"
    alternate.write_text(str(actual_objects) + "\n")
    parent_oid = git(staged, "hash-object", "-w", "--stdin", data=parent_cmake).strip()
    assert parent_oid.decode() == blob_oid(parent_cmake)
    projected_index = b"".join(
        file_mode + b" " + (parent_oid if name == b"CMakeLists.txt" else oid) +
        b" 0\t" + name + b"\0"
        for file_mode, oid, name in records
    )
    git(staged, "update-index", "-z", "--index-info", data=projected_index)
    assert git(staged, "ls-files", "--stage", "-z") == projected_index

    # Only this explicit protected input root is remapped. Compiler, probe,
    # build, host compiler and .deps arguments retain their original values.
    if mode == "p990_new_native.integration":
        arguments[3] = str(staged / "tests/p990")
    result = subprocess.run([sys.executable, str(staged / script), *arguments],
                            check=False)
    if result.returncode != 0:
        print("R1144 original projected command exit:", mode, result.returncode)
        raise SystemExit(result.returncode)
print("R1144 original protected command PASS:", mode,
      "protected", len(current["files"]), "original manifest", CURRENT_SHA)
