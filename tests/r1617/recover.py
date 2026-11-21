#!/usr/bin/env python3
"""Strict DATA-ONLY recovery. Recovered bytes are never imported or executed."""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import time
import zlib

MAX_LOG = 84 * 1024 * 1024
MAX_COMPRESSED = 56 * 1024 * 1024
MAX_PAYLOAD = 72 * 1024 * 1024
MAX_RAW = 48 * 1024 * 1024
MAX_FILE = 8 * 1024 * 1024
MAX_FILES = 320
CHUNK = 4096

def sha(data):
    return hashlib.sha256(data).hexdigest()

def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError("duplicate JSON key")
        result[key] = value
    return result

def parse_json(text):
    return json.loads(text, object_pairs_hook=unique_object)

def encoded(value):
    return (json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n").encode()

def strict_path(text):
    if not isinstance(text, str) or len(text) > 240 \
            or not re.fullmatch(r"(raw|metadata)/[A-Za-z0-9._/-]+", text):
        raise ValueError("invalid payload path")
    parts = text.split("/")
    if len(parts) > 10 or any(x in ("", ".", "..") for x in parts):
        raise ValueError("invalid path component")
    return text

def write_new(path, data):
    path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "wb") as stream:
        stream.write(data)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("log")
    ap.add_argument("destination")
    ap.add_argument("--transport-sha256", required=True)
    ap.add_argument("--run-id", required=True)
    ap.add_argument("--run-attempt", required=True)
    ap.add_argument("--job-label", required=True)
    ap.add_argument("--checkout-commit", required=True)
    args = ap.parse_args()
    started = time.monotonic()
    if not re.fullmatch(r"[0-9a-f]{64}", args.transport_sha256):
        raise SystemExit("STOP expected transport hash")
    log_path = Path(args.log)
    if log_path.is_symlink() or not stat.S_ISREG(log_path.stat().st_mode) \
            or log_path.stat().st_size > MAX_LOG:
        raise SystemExit("STOP log shape/size")
    log = log_path.read_bytes().decode("utf-8")
    header = end = None
    chunks = []
    state = "before"
    timestamp = re.compile(r"^\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d(?:\.\d+)?Z ")
    for raw_line in log.splitlines():
        if time.monotonic() - started > 60:
            raise SystemExit("STOP recovery deadline")
        line = timestamp.sub("", raw_line, count=1)
        if line.startswith("R1617_TRANSPORT_BEGIN "):
            if state != "before":
                raise SystemExit("STOP repeated/overlapping transport")
            header = parse_json(line.removeprefix("R1617_TRANSPORT_BEGIN "))
            state = "inside"
        elif line.startswith("R1617_TRANSPORT_CHUNK "):
            if state != "inside":
                raise SystemExit("STOP chunk outside frame")
            match = re.fullmatch(r"R1617_TRANSPORT_CHUNK (\d+)/(\d+) ([A-Za-z0-9+/=]+)", line)
            if not match:
                raise SystemExit("STOP malformed chunk")
            index, count = int(match[1]), int(match[2])
            if index != len(chunks) + 1 or count != header["chunks"]:
                raise SystemExit("STOP missing/reordered/duplicate chunk")
            if len(match[3]) > CHUNK or (index < count and len(match[3]) != CHUNK):
                raise SystemExit("STOP chunk length")
            chunks.append(match[3])
            if len(chunks) > 20000:
                raise SystemExit("STOP chunk count")
        elif line.startswith("R1617_TRANSPORT_END "):
            if state != "inside":
                raise SystemExit("STOP unexpected end")
            end = line.removeprefix("R1617_TRANSPORT_END ")
            state = "after"
        elif "R1617_TRANSPORT_" in line:
            raise SystemExit("STOP altered framing/prefix")
    if state != "after" or header is None or len(chunks) != header["chunks"]:
        raise SystemExit("STOP incomplete transport")
    keys = {"schema", "transport_id", "encoding", "compressed_bytes",
            "payload_bytes", "payload_sha256", "raw_manifest_bytes",
            "raw_manifest_sha256", "chunks", "chunk_characters"}
    if set(header) != keys or header["schema"] != "r1617-transport-v1" \
            or header["encoding"] != "gzip-mtime0-base64" \
            or header["chunk_characters"] != CHUNK:
        raise SystemExit("STOP transport schema")
    compressed = base64.b64decode("".join(chunks), validate=True)
    if len(compressed) > MAX_COMPRESSED or len(compressed) != header["compressed_bytes"]:
        raise SystemExit("STOP compressed length")
    if sha(compressed) != args.transport_sha256 \
            or header["transport_id"] != args.transport_sha256 \
            or end != args.transport_sha256:
        raise SystemExit("STOP transport hash")
    decompressor = zlib.decompressobj(wbits=31)
    payload = decompressor.decompress(compressed, MAX_PAYLOAD + 1)
    if len(payload) > MAX_PAYLOAD or decompressor.unconsumed_tail:
        raise SystemExit("STOP decompression bound")
    payload += decompressor.flush(MAX_PAYLOAD + 1 - len(payload))
    if not decompressor.eof or decompressor.unused_data \
            or len(payload) != header["payload_bytes"] \
            or sha(payload) != header["payload_sha256"]:
        raise SystemExit("STOP incomplete/concatenated/mismatched gzip")
    obj = parse_json(payload.decode("utf-8"))
    if set(obj) != {"schema", "raw_manifest", "files"} \
            or obj["schema"] != "r1617-payload-v1":
        raise SystemExit("STOP payload schema")
    manifest = obj["raw_manifest"]
    raw_manifest = encoded(manifest)
    if len(raw_manifest) != header["raw_manifest_bytes"] \
            or sha(raw_manifest) != header["raw_manifest_sha256"]:
        raise SystemExit("STOP raw manifest hash")
    if manifest.get("schema") != "r1617-original-raw-v1" \
            or manifest.get("payload_is_data_only") is not True:
        raise SystemExit("STOP raw schema")
    env = manifest["selected_environment"]
    if env.get("GITHUB_RUN_ID") != args.run_id \
            or env.get("GITHUB_RUN_ATTEMPT") != args.run_attempt \
            or env.get("GITHUB_REPOSITORY") != "wakairo/NewLang_Compiler":
        raise SystemExit("STOP run binding mismatch")
    rows = manifest["files"]
    if len(rows) != manifest["file_count"] or len(rows) != len(obj["files"]) \
            or len(rows) > MAX_FILES:
        raise SystemExit("STOP raw file count")
    recovered = []
    names = set()
    total = 0
    for expected, item in zip(rows, obj["files"]):
        if set(expected) != {"path", "bytes", "sha256"} \
                or set(item) != {"path", "base64"}:
            raise SystemExit("STOP file schema")
        name = strict_path(expected["path"])
        if name in names or item["path"] != name:
            raise SystemExit("STOP duplicate/reordered file")
        names.add(name)
        data = base64.b64decode(item["base64"], validate=True)
        total += len(data)
        if len(data) > MAX_FILE or total > MAX_RAW \
                or len(data) != expected["bytes"] or sha(data) != expected["sha256"]:
            raise SystemExit("STOP raw file length/hash")
        recovered.append((name, data))
    if total != manifest["raw_bytes"]:
        raise SystemExit("STOP raw total")
    metadata = next((data for name, data in recovered
                     if name == "metadata/metadata.json"), None)
    identity_status = "METADATA_MISSING_NO_TEST_CLAIM"
    if metadata is not None:
        info = parse_json(metadata.decode("utf-8"))
        if info.get("label") != args.job_label \
                or info.get("checkout_commit") != args.checkout_commit:
            raise SystemExit("STOP exact checkout/job-label mismatch")
        identity_status = "CHECKOUT_METADATA_MATCHED_EXTERNAL_API_RECEIPT_STILL_REQUIRED"
    elif env.get("GITHUB_SHA") != args.checkout_commit:
        raise SystemExit("STOP declared event SHA mismatch without metadata")
    if time.monotonic() - started > 60:
        raise SystemExit("STOP recovery deadline before writes")
    destination = Path(args.destination).absolute()
    if destination.exists() or destination.is_symlink():
        raise SystemExit("STOP recovery destination exists")
    destination.mkdir(parents=True, mode=0o700, exist_ok=False)
    for name, data in recovered:
        # Always a non-executable .data file, including source/scripts/binaries.
        write_new(destination / (name + ".data"), data)
    write_new(destination / "raw-manifest.json.data", raw_manifest)
    write_new(destination / "transport-manifest.json.data", encoded(header))
    write_new(destination / "recovery-index.json", encoded({
        "schema": "r1617-data-recovery-v1",
        "transport_id": args.transport_sha256, "files": len(recovered),
        "bytes": total, "suffix": ".data", "permissions": "0600",
        "executed_recovered_bytes": False, "identity_status": identity_status,
        "remaining_evidence_gate":
            "approved API numeric job ID, full attempt-job pagination, all steps, exact tree/parents and run receipt",
    }))
    print("R1617 DATA RECOVERED; no recovered byte executed")

if __name__ == "__main__":
    main()
