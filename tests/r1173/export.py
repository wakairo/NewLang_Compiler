"""Lossless, bounded raw export through the natural Actions CTest log."""
import base64
import hashlib
import io
from pathlib import Path
import sys
import tarfile

root = Path(sys.argv[1])
if not root.is_dir():
    print("R1173 RAW NOT_RUN: no semantic evidence directory (FAST inventory is not a test)")
    raise SystemExit(0)
buffer = io.BytesIO()
with tarfile.open(fileobj=buffer, mode="w:gz") as archive:
    for path in sorted(root.rglob("*")):
        if path.is_file() and not path.is_symlink():
            archive.add(path, arcname=str(path.relative_to(root)), recursive=False)
raw = buffer.getvalue()
if len(raw) > 4 * 1024 * 1024:
    raise SystemExit("R1173 RAW export exceeds predeclared 4MiB bound")
print("R1173_RAW_BEGIN sha256=" + hashlib.sha256(raw).hexdigest() + " bytes=" + str(len(raw)), flush=True)
encoded = base64.b64encode(raw).decode("ascii")
for offset in range(0, len(encoded), 3000):
    print("R1173_RAW " + encoded[offset:offset + 3000], flush=True)
print("R1173_RAW_END", flush=True)
