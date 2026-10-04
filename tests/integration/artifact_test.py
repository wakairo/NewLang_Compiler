"""Versioned policy, normative, closure, formal, and oracle files stay intact."""
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[2]
manifest = json.loads((root / "docs/reference/INPUT_ARTIFACTS.json").read_text())
for entry in manifest["snapshots"]:
    path = root / entry["path"]
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    if actual != entry["sha256"]:
        raise SystemExit(f"modified reference artifact: {entry['path']}")
print(f"artifacts: {len(manifest['snapshots'])} snapshot hashes verified")
