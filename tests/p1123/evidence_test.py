from pathlib import Path
import hashlib
import json
import zipfile

data = Path(__file__).resolve().parent
freeze = json.loads((data / "freeze.json").read_bytes())
for name, digest in freeze["files"].items():
    assert hashlib.sha256((data / name).read_bytes()).hexdigest() == digest, name
with zipfile.ZipFile(data / "evidence" / "inputs-v3.zip") as archive:
    assert set(archive.namelist()) == set(freeze["files"])
    for name, digest in freeze["files"].items():
        assert hashlib.sha256(archive.read(name)).hexdigest() == digest, name
assert freeze["parent"] == "a1091b2672b0a4b1d37e2571ef97bf100882b60b"
assert freeze["before_first_revision_execution"] is True
print("P1123 frozen source/oracle/FIRST/RED and current harness bytes PASS")
