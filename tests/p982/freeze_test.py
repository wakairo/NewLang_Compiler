from pathlib import Path
import hashlib
import json

root = Path(__file__).resolve().parents[2]
for manifest in ["source-freeze-v1.json", "harness-freeze-v3.json"]:
    data = json.loads((root / "tests/p982" / manifest).read_text())
    for name, expected in data["files"].items():
        assert hashlib.sha256((root / name).read_bytes()).hexdigest() == expected, name
print("P982 frozen independent source/oracle/production/harness hashes PASS")
