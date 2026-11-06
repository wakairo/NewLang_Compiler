"""R1190 raw export via natural CTest logs; adapted from allowed plumbing only."""
import base64
import hashlib
import io
import json
from pathlib import Path
import sys
import tarfile


root = Path(sys.argv[1])
if not root.is_dir():
    print("R1190 RAW NOT_RUN: no semantic evidence directory; FAST is inventory only")
    raise SystemExit(0)
members = []
for path in sorted(root.rglob("*")):
    if path.is_file() and not path.is_symlink() and path != root / "MEMBER-INDEX.json":
        data = path.read_bytes()
        members.append({"path": str(path.relative_to(root)), "bytes": len(data),
                        "sha256": hashlib.sha256(data).hexdigest()})
(root / "MEMBER-INDEX.json").write_text(json.dumps(members, indent=2) + "\n")
buffer = io.BytesIO()
with tarfile.open(fileobj=buffer, mode="w:gz") as archive:
    for path in sorted(root.rglob("*")):
        if path.is_file() and not path.is_symlink():
            archive.add(path, arcname=str(path.relative_to(root)), recursive=False)
raw = buffer.getvalue()
if len(raw) > 4 * 1024 * 1024:
    raise SystemExit("R1190 RAW export exceeds predeclared 4 MiB bound; preserve FIRST")
print("R1190_RAW_BEGIN sha256=" + hashlib.sha256(raw).hexdigest()
      + " bytes=" + str(len(raw)), flush=True)
encoded = base64.b64encode(raw).decode("ascii")
for offset in range(0, len(encoded), 3000):
    print("R1190_RAW " + encoded[offset:offset + 3000], flush=True)
print("R1190_RAW_END", flush=True)
