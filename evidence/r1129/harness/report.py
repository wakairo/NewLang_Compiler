#!/usr/bin/env python3
"""Non-mutating CTest post-test transport of R-only raw into the job log."""
import base64
import gzip
import hashlib
import json
from pathlib import Path
import sys


def digest(data):
    return hashlib.sha256(data).hexdigest()


def emit(kind, value):
    print("R1129_" + kind + " " + json.dumps(value, sort_keys=True,
                                          separators=(",", ":")), flush=True)


def main():
    root = Path(sys.argv[1]).resolve()
    if not root.is_dir():
        emit("TRANSPORT_STATUS", {"status": "NOT_EXECUTED", "root": str(root)})
        return 0
    entries = []
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        if path.is_symlink() or not path.resolve().is_relative_to(root):
            raise RuntimeError("raw transport refuses a foreign/symlink path")
        relative = path.relative_to(root).as_posix()
        data = path.read_bytes()
        meta = {"path": relative, "bytes": len(data), "sha256": digest(data)}
        if path.suffix == ".exe":
            meta["codec"] = "hash-only-executable"
            meta["chunks"] = 0
            entries.append(meta)
            emit("FILE", meta)
            continue
        wire = gzip.compress(data, compresslevel=9, mtime=0) if len(data) > 16384 else data
        encoded = base64.b64encode(wire).decode("ascii")
        chunks = [encoded[i:i + 4096] for i in range(0, len(encoded), 4096)]
        meta.update({"codec": "gzip+base64" if len(data) > 16384 else "base64",
                     "wire_bytes": len(wire), "wire_sha256": digest(wire),
                     "chunks": len(chunks)})
        entries.append(meta)
        emit("FILE", meta)
        for index, chunk in enumerate(chunks):
            emit("CHUNK", {"path": relative, "index": index, "data": chunk})
        emit("END_FILE", {"path": relative, "chunks": len(chunks)})
    manifest = (json.dumps(entries, sort_keys=True, separators=(",", ":")) + "\n").encode()
    emit("TRANSPORT_END", {
        "status": "COMPLETE", "file_count": len(entries),
        "raw_bytes": sum(item["bytes"] for item in entries),
        "manifest_sha256": digest(manifest),
        "manifest_format": "UTF-8 compact sorted-key JSON array of FILE records, in log order, final LF",
        "binary_policy": "Executables retained in build only; exact size/SHA and command receipt transported.",
    })
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
