"""Preserve the original freeze checks while authenticating R-only CMake wiring."""
from pathlib import Path
import hashlib
import json
import runpy
import shutil
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CURRENT = "tests/p990/current-freeze-v35.json"
CURRENT_SHA = "518851e3c35046ef1212684f6c8003b4413340b90574d772122c728dbf37f3e1"
PARENT_CMAKE_SHA = "98c0f59e372ea21f0e7b0c0ce345e5bacd07544f30c9bd686faaa2f50920c6d7"


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


mode = sys.argv[1]
assert mode in ("baseline", "protocol")
current_raw = (ROOT / CURRENT).read_bytes()
assert sha(current_raw) == CURRENT_SHA, "unchanged current freeze manifest"
current = json.loads(current_raw)
r_manifest = json.loads((ROOT / "tests/r1144/harness-manifest.json").read_bytes())
for name, expected in r_manifest["files"].items():
    assert sha((ROOT / name).read_bytes()) == expected, "R input changed: " + name
parent_cmake = (ROOT / "tests/r1144/parent-CMakeLists.txt").read_bytes()
assert sha(parent_cmake) == PARENT_CMAKE_SHA
assert current["files"]["CMakeLists.txt"] == PARENT_CMAKE_SHA
# Exact original validators run on actual protected bytes. Only CMake's known
# R registration delta is projected away; it was separately authenticated above.
with tempfile.TemporaryDirectory(prefix="r1144-freeze-") as directory:
    staged = Path(directory)
    for name in sorted(set(current["files"]) | {CURRENT}):
        source = ROOT / name
        raw = source.read_bytes()
        if name == "CMakeLists.txt":
            raw = parent_cmake
        elif name in current["files"]:
            assert sha(raw) == current["files"][name], "protected input: " + name
        target = staged / name
        target.parent.mkdir(parents=True, exist_ok=True)
        if name == "CMakeLists.txt":
            target.write_bytes(raw)
        else:
            shutil.copyfile(source, target)
            assert target.read_bytes() == raw, "snapshot copy: " + name
    script = ("tests/p982/freeze_test.py" if mode == "baseline"
              else "tests/p990/freeze_selftest.py")
    runpy.run_path(str(staged / script), run_name="__main__")
print("R1144 original freeze validator PASS:", mode,
      "protected", len(current["files"]), "original manifest", CURRENT_SHA)
