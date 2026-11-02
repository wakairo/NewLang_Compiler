"""C180 retained pre-run snapshots and externally pinned failed v17.

Archived snapshots authenticate old inputs without requiring future legitimate
Production to keep the same hashes. Current inputs use the versioned lineage.
"""
from pathlib import Path
import hashlib
import json
import runpy
import sys
import zipfile

root = Path(__file__).resolve().parents[2]
data = root / "tests/p1088"
for name in ["freeze-v1.json", "freeze-v2.json", "freeze-negative-v1.json",
             "freeze-v3.json", "freeze-v4.json", "freeze-v5.json", "freeze-v6.json"]:
    frozen = json.loads((data / name).read_bytes())["files"]
    with zipfile.ZipFile(data / "evidence" / name.replace(".json", "-inputs.zip")) as archive:
        assert set(archive.namelist()) == set(frozen), name
        for path, expected in frozen.items():
            assert hashlib.sha256(archive.read(path)).hexdigest() == expected, (name, path)

logs = json.loads((data / "evidence/original-m-ci-exact-map.json").read_bytes())["files"]
with zipfile.ZipFile(data / "evidence/original-m-ci-exact.zip") as archive:
    assert set(archive.namelist()) == {job + ".log" for job in logs}
    for job, expected in logs.items():
        raw = archive.read(job + ".log")
        assert len(raw) == expected["bytes"]
        assert hashlib.sha256(raw).hexdigest() == expected["sha256"]

old_name = "tests/p990/current-freeze-v17.json"
old_raw = (root / old_name).read_bytes()
old_sha = "3fc39dc0cae85a051e77e814d4d54c3d5c33d2bea955ea5d9db1c2d900b9a0b6"
assert hashlib.sha256(old_raw).hexdigest() == old_sha
assert hashlib.sha1(b"blob " + str(len(old_raw)).encode() + b"\0" + old_raw).hexdigest() == "8110d494acae11de3114f3f8574d5cc2c833104c"
old = json.loads(old_raw)
assert old["generation"] == 17 and len(old["files"]) == 2754
current_name = runpy.run_path(str(root / "tests/p982/freeze_test.py"))["CURRENT"]
current = json.loads((root / current_name).read_bytes())
assert current["generation"] >= 18
assert old["files"].keys() <= current["files"].keys()
assert current["files"][old_name] == old_sha
while current["generation"] > 18:
    current = json.loads((root / current["previous"]["path"]).read_bytes())
assert current["previous"] == {"path": old_name, "sha256": old_sha}
assert current["stage"].isascii()
print("P1088 archived pre-run inputs, fixed failed-v17 bytes/coverage and forward lineage PASS")
