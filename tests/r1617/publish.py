#!/usr/bin/env python3
"""Data-only raw evidence publisher. No subprocess, eval, imports from payload."""
import argparse
import base64
import gzip
import hashlib
import json
import os
from pathlib import Path
import stat
import time

MAX_FILE = 8 * 1024 * 1024
MAX_RAW = 48 * 1024 * 1024
MAX_FILES = 320
MAX_PAYLOAD = 72 * 1024 * 1024
MAX_COMPRESSED = 56 * 1024 * 1024
CHUNK = 4096

def sha(data):
    return hashlib.sha256(data).hexdigest()

def encoded(value):
    return (json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n").encode()

def put(path, data):
    with path.open("xb") as out:
        out.write(data)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--raw", required=True)
    ap.add_argument("--metadata", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()
    started = time.monotonic()
    destination = Path(args.out).absolute()
    if destination.exists() or destination.is_symlink():
        raise SystemExit("STOP transport destination preexists")
    destination.mkdir(parents=True, exist_ok=False)
    entries, files, total = [], [], 0
    presence = {}
    for prefix, root in (("raw", Path(args.raw)), ("metadata", Path(args.metadata))):
        if root.is_symlink():
            raise SystemExit("STOP symlink evidence root")
        presence[prefix] = root.exists()
        if not root.exists():
            continue
        if not root.is_dir():
            raise SystemExit("STOP evidence root not directory")
        for path in sorted(root.rglob("*")):
            if time.monotonic() - started > 60:
                raise SystemExit("STOP publisher deadline")
            st = path.lstat()
            if stat.S_ISDIR(st.st_mode):
                continue
            if not stat.S_ISREG(st.st_mode) or st.st_size > MAX_FILE:
                raise SystemExit("STOP nonregular/oversized evidence")
            relative = path.relative_to(root).as_posix()
            if any(p in ("", ".", "..") for p in relative.split("/")):
                raise SystemExit("STOP invalid evidence path")
            with path.open("rb") as stream:
                content = stream.read(MAX_FILE + 1)
            if len(content) != st.st_size or len(content) > MAX_FILE:
                raise SystemExit("STOP changed/oversized evidence")
            total += len(content)
            if len(files) >= MAX_FILES or total > MAX_RAW:
                raise SystemExit("STOP publisher emergency raw cap")
            name = prefix + "/" + relative
            entries.append({"path": name, "bytes": len(content), "sha256": sha(content)})
            files.append({"path": name,
                          "base64": base64.b64encode(content).decode("ascii")})
    for entry in entries:
        prefix, relative = entry["path"].split("/", 1)
        root = Path(args.raw) if prefix == "raw" else Path(args.metadata)
        path = root / relative
        if path.is_symlink() or path.stat().st_size != entry["bytes"]:
            raise SystemExit("STOP raw file changed before sealing")
        with path.open("rb") as stream:
            check = stream.read(MAX_FILE + 1)
        if sha(check) != entry["sha256"]:
            raise SystemExit("STOP raw bytes changed before sealing")
    current_names = []
    for prefix, root in (("raw", Path(args.raw)), ("metadata", Path(args.metadata))):
        if root.exists():
            for path in sorted(root.rglob("*")):
                if path.is_symlink():
                    raise SystemExit("STOP late symlink")
                if not path.is_dir():
                    current_names.append(prefix + "/" + path.relative_to(root).as_posix())
    if current_names != [entry["path"] for entry in entries]:
        raise SystemExit("STOP raw file set changed before sealing")
    manifest = {
        "schema": "r1617-original-raw-v1", "roots_present": presence,
        "raw_bytes": total, "file_count": len(entries), "files": entries,
        "selected_environment": {k: os.environ.get(k) for k in (
            "GITHUB_REPOSITORY", "GITHUB_SHA", "GITHUB_RUN_ID",
            "GITHUB_RUN_ATTEMPT", "GITHUB_JOB", "GITHUB_EVENT_NAME")},
        "payload_is_data_only": True,
        "no_claim_from_missing_raw": not presence["raw"],
        "driver_raw_budget": 32 * 1024 * 1024,
        "publisher_emergency_budget": MAX_RAW,
    }
    raw_manifest = encoded(manifest)
    payload = encoded({"schema": "r1617-payload-v1",
                       "raw_manifest": manifest, "files": files})
    if len(payload) > MAX_PAYLOAD:
        raise SystemExit("STOP payload budget")
    compressed = gzip.compress(payload, compresslevel=6, mtime=0)
    if len(compressed) > MAX_COMPRESSED:
        raise SystemExit("STOP compressed budget")
    transport_id = sha(compressed)
    text = base64.b64encode(compressed).decode("ascii")
    chunks = [text[i:i + CHUNK] for i in range(0, len(text), CHUNK)]
    transport = {
        "schema": "r1617-transport-v1", "transport_id": transport_id,
        "encoding": "gzip-mtime0-base64", "compressed_bytes": len(compressed),
        "payload_bytes": len(payload), "payload_sha256": sha(payload),
        "raw_manifest_bytes": len(raw_manifest),
        "raw_manifest_sha256": sha(raw_manifest),
        "chunks": len(chunks), "chunk_characters": CHUNK,
    }
    frame = [
        "R1617_TRANSPORT_BEGIN " + json.dumps(transport, sort_keys=True,
                                             separators=(",", ":"))
    ]
    frame += [f"R1617_TRANSPORT_CHUNK {i + 1}/{len(chunks)} {part}"
              for i, part in enumerate(chunks)]
    frame.append("R1617_TRANSPORT_END " + transport_id)
    framed_bytes = ("\n".join(frame) + "\n").encode("ascii")
    put(destination / "raw-manifest.json", raw_manifest)
    put(destination / "transport-manifest.json", encoded(transport))
    put(destination / "payload.json.gz", compressed)
    put(destination / "transport.txt", framed_bytes)
    if time.monotonic() - started > 60:
        raise SystemExit("STOP publisher deadline before publication")
    print(framed_bytes.decode("ascii"), end="", flush=True)

if __name__ == "__main__":
    main()
