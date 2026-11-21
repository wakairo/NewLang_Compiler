"""Data-only reconstruction of one exact job-log publisher transcript."""
import argparse
import base64
import json
import os
from pathlib import Path
import re

from rawio import (CHUNK_BYTES, TOTAL_PUBLISH_BYTES, allowlist, canonical,
                   open_directory, read_regular_at, sha, write_new)

FRAME = re.compile(rb"^(?:\d{4}-\d{2}-\d{2}T[0-9:.]+Z )?R1608V1 (.*)$")

def input_bytes(path, cap):
    path = Path(path).absolute()
    fd = open_directory(path.parent)
    try:
        return read_regular_at(fd, path.name, cap)[0]
    finally:
        os.close(fd)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--log", required=True)
    parser.add_argument("--retrieval", required=True)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()
    log = input_bytes(args.log, 32 * 1024 * 1024)
    retrieval = json.loads(input_bytes(args.retrieval, 16384))
    if sha(log) != retrieval["decoded_log_sha256"]:
        raise SystemExit("retrieval log digest mismatch")
    for key in ("run_id", "run_attempt", "job_id"):
        if not re.fullmatch(r"[0-9]+", str(retrieval[key])):
            raise SystemExit("missing exact remote identity")
    expected = {key: str(retrieval[key]) for key in
                ("repository", "run_id", "run_attempt", "job_key",
                 "matrix", "event_sha", "mode", "event_name")}
    output = Path(args.out).absolute()
    os.mkdir(output, 0o700)
    out_fd = open_directory(output)
    allowed = allowlist()
    allowed["TRANSPORT_MANIFEST.json"] = 65536
    header, active, ending, manifest = None, None, None, None
    completed, published_total = {}, 0
    error = None
    try:
        write_new(out_fd, "RETRIEVAL.json", canonical(retrieval))
        for raw in log.splitlines():
            match = FRAME.fullmatch(raw.rstrip(b"\r"))
            if not match:
                continue
            if len(match.group(1)) > 16384:
                raise ValueError("frame line limit")
            record = json.loads(match.group(1))
            kind = record.get("kind")
            if ending is not None:
                raise ValueError("frame after end")
            if kind == "header":
                if header is not None or active is not None or completed:
                    raise ValueError("duplicate or late header")
                if record.get("protocol") != "R1608V1":
                    raise ValueError("protocol")
                for key, value in expected.items():
                    if str(record.get(key, "")) != value:
                        raise ValueError("remote metadata mismatch:" + key)
                header = {k: v for k, v in record.items() if k != "kind"}
                continue
            if header is None:
                raise ValueError("data without header")
            if kind == "file":
                name = record.get("name")
                size, chunks = record.get("bytes"), record.get("chunks")
                if (active is not None or name not in allowed or name in completed or
                        type(size) is not int or size < 0 or size > allowed[name] or
                        type(chunks) is not int or
                        chunks != (size + CHUNK_BYTES - 1) // CHUNK_BYTES or
                        not re.fullmatch(r"[0-9a-f]{64}", record.get("sha256", ""))):
                    raise ValueError("invalid file start")
                active = dict(record, buffer=bytearray(), next_index=0)
            elif kind == "chunk":
                if active is None or record.get("name") != active["name"] or \
                        record.get("index") != active["next_index"]:
                    raise ValueError("chunk identity/order")
                chunk = base64.b64decode(record.get("data", ""), validate=True)
                index = active["next_index"]
                remaining = active["bytes"] - len(active["buffer"])
                expected_size = min(CHUNK_BYTES, remaining)
                if len(chunk) != expected_size or index >= active["chunks"]:
                    raise ValueError("chunk length")
                active["buffer"].extend(chunk)
                active["next_index"] += 1
            elif kind == "file_end":
                if active is None or record.get("name") != active["name"]:
                    raise ValueError("file end identity")
                data = bytes(active["buffer"])
                if (active["next_index"] != active["chunks"] or
                        len(data) != active["bytes"] or sha(data) != active["sha256"] or
                        record.get("bytes") != len(data) or
                        record.get("sha256") != sha(data)):
                    raise ValueError("file count or digest")
                name = active["name"]
                if name != "TRANSPORT_MANIFEST.json":
                    published_total += len(data)
                    if published_total > TOTAL_PUBLISH_BYTES:
                        raise ValueError("total byte limit")
                write_new(out_fd, name, data)
                completed[name] = {"bytes": len(data), "sha256": sha(data)}
                if name == "TRANSPORT_MANIFEST.json":
                    manifest = json.loads(data)
                active = None
            elif kind == "end":
                if active is not None or manifest is None:
                    raise ValueError("end without complete manifest")
                if record.get("manifest_sha256") != completed["TRANSPORT_MANIFEST.json"]["sha256"]:
                    raise ValueError("manifest digest binding")
                ending = record
            else:
                raise ValueError("unknown frame kind")
        if active is not None:
            raise ValueError("incomplete file prefix")
        if header is None or ending is None or manifest is None:
            raise ValueError("incomplete publisher prefix")
        if manifest.get("metadata") != header:
            raise ValueError("manifest header identity")
        entries = manifest.get("entries", [])
        if (len(entries) != len(allowlist()) or
                [entry.get("name") for entry in entries] != sorted(allowlist())):
            raise ValueError("fixed allowlist manifest mismatch")
        present = {}
        for entry in entries:
            name, state = entry["name"], entry["state"]
            if state == "present":
                present[name] = {"bytes": entry["bytes"], "sha256": entry["sha256"]}
            elif state not in ("missing", "error", "unavailable"):
                raise ValueError("unknown manifest file state")
        actual = {k: v for k, v in completed.items() if k != "TRANSPORT_MANIFEST.json"}
        if (present != actual or manifest.get("payload_bytes") != published_total or
                ending.get("payload_bytes") != published_total):
            raise ValueError("manifest complete file set or byte count")
        if manifest.get("errors") or ending.get("transport_complete") is not True:
            raise ValueError("publisher declared incomplete transport")
    except BaseException as exc:
        error = type(exc).__name__ + ":" + str(exc)
        if active is not None:
            # Keep the exact decoded prefix; never represent it as a whole file.
            write_new(out_fd, active["name"] + ".partial", bytes(active["buffer"]))
    finally:
        report = {
            "transport_complete": error is None,
            "error": error, "retrieval_job_id": str(retrieval["job_id"]),
            "verified_files": completed, "verified_payload_bytes": published_total,
            "measurement_classification": "NOT_PERFORMED",
            "capability_or_product_pass": "NOT_INFERRED",
        }
        write_new(out_fd, "RECOVERY.json", canonical(report))
        os.close(out_fd)
    print(canonical(report).decode("ascii"), end="")
    return 0 if error is None else 1

if __name__ == "__main__":
    raise SystemExit(main())
