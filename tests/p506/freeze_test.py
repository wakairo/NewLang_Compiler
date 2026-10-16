"""Keep the pre-execution sources and original P501 inputs immutable."""
from pathlib import Path
import hashlib
import json
import sys

root = Path(sys.argv[1])
manifest = json.loads((root / "frozen.json").read_text())
for name, digest in manifest["sources"].items():
    source = root / name
    assert hashlib.sha256(source.read_bytes()).hexdigest() == digest, name
for name in ("full_guard_true.nl", "full_guard_false.nl"):
    assert (root / name).read_bytes() == (root.parent / "p501" / name).read_bytes()
print("P506 frozen source SHA256s and original P501 source bytes: PASS")
