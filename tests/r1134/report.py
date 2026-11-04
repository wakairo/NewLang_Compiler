#!/usr/bin/env python3
"""Transport exact R1134 raw JSON after CTest; no test or compiler execution."""
import base64
import hashlib
import json
from pathlib import Path
import sys

def emit(kind, value):
    print("R1134_" + kind + " " + json.dumps(value, sort_keys=True,
                                            separators=(",", ":")), flush=True)

def main():
    root = Path(sys.argv[1]).resolve()
    path = root / "raw.json"
    if not path.is_file():
        emit("TRANSPORT_STATUS", {"status": "NOT_EXECUTED", "root": str(root)})
        return 0
    if path.is_symlink() or path.resolve().parent != root:
        raise RuntimeError("R1134 refuses foreign/symlink raw")
    data = path.read_bytes()
    payload = json.loads(data)
    sha = hashlib.sha256(data).hexdigest()
    encoded = base64.b64encode(data).decode("ascii")
    chunks = [encoded[index:index + 4096]
              for index in range(0, len(encoded), 4096)]
    emit("RAW_BEGIN", {"codec": "base64", "bytes": len(data), "sha256": sha,
                       "chunks": len(chunks), "success": payload["success"],
                       "first_failure": payload["first_failure"]})
    for index, chunk in enumerate(chunks):
        emit("RAW_CHUNK", {"index": index, "data": chunk})
    emit("RAW_END", {"sha256": sha, "chunks": len(chunks), "status": "COMPLETE"})
    emit("RESULT", {"success": payload["success"],
                    "first_failure": payload["first_failure"],
                    "error": payload["error"], "summary": payload["summary"],
                    "host_sanitizer": payload["host_sanitizer"],
                    "github": payload["github"]})
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
