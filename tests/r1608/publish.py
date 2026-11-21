"""Publish saved R1608 bytes only. Never run a child or evaluate captured data."""
import base64
import os
import signal
import time
import sys

from rawio import (CHUNK_BYTES, TOTAL_PUBLISH_BYTES, allowlist, canonical,
                   open_fixed_root, read_regular_at, sha)

PREFIX = "R1608V1 "

def emit(record):
    sys.stdout.write(PREFIX + canonical(record).decode("ascii"))
    sys.stdout.flush()

def payload(name, data):
    count = (len(data) + CHUNK_BYTES - 1) // CHUNK_BYTES
    digest = sha(data)
    emit({"kind": "file", "name": name, "bytes": len(data),
          "sha256": digest, "chunks": count})
    for index in range(count):
        chunk = data[index * CHUNK_BYTES:(index + 1) * CHUNK_BYTES]
        emit({"kind": "chunk", "name": name, "index": index,
              "data": base64.b64encode(chunk).decode("ascii")})
    emit({"kind": "file_end", "name": name, "bytes": len(data),
          "sha256": digest})
    return digest

def alarm(_signum, _frame):
    # Terminal deadline: no exception handler, cleanup, or buffered flush follows.
    os._exit(124)

def main():
    signal.signal(signal.SIGALRM, alarm)
    # Keep the alarm armed through all output and interpreter finalization.
    signal.alarm(30)
    deadline = time.monotonic() + 30
    mode = sys.argv[1] if len(sys.argv) == 2 else ""
    meta = {
        "protocol": "R1608V1",
        "mode": mode,
        "repository": os.environ.get("GITHUB_REPOSITORY", ""),
        "run_id": os.environ.get("GITHUB_RUN_ID", ""),
        "run_attempt": os.environ.get("GITHUB_RUN_ATTEMPT", ""),
        "job_key": os.environ.get("GITHUB_JOB", ""),
        "matrix": os.environ.get("R1608_MATRIX", ""),
        "event_sha": os.environ.get("GITHUB_SHA", ""),
        "event_name": os.environ.get("GITHUB_EVENT_NAME", ""),
    }
    emit({"kind": "header", **meta})
    entries, errors, total = [], [], 0
    fd = None
    root_error = None
    try:
        fd = open_fixed_root(os.environ.get("GITHUB_WORKSPACE", ""), mode)
    except (OSError, ValueError) as error:
        root_error = type(error).__name__ + ":" + str(error)
        errors.append("raw_root_unavailable")
    try:
        for name, cap in sorted(allowlist().items()):
            if fd is None:
                entries.append({"name": name, "state": "unavailable",
                                "reason": "raw_root_unavailable"})
                continue
            try:
                if time.monotonic() >= deadline:
                    alarm(None, None)
                data, _identity = read_regular_at(fd, name, cap)
                if total + len(data) > TOTAL_PUBLISH_BYTES:
                    raise ValueError("total payload byte cap")
            except FileNotFoundError:
                entries.append({"name": name, "state": "missing"})
                continue
            except (OSError, ValueError) as error:
                entries.append({"name": name, "state": "error",
                                "reason": type(error).__name__ + ":" + str(error)})
                errors.append(name)
                continue
            total += len(data)
            digest = payload(name, data)
            entries.append({"name": name, "state": "present",
                            "bytes": len(data), "sha256": digest})
    finally:
        if fd is not None:
            os.close(fd)
    manifest = canonical({
        "metadata": meta, "entries": entries, "payload_bytes": total,
        "errors": errors, "root_error": root_error,
        "measurement_status": "requires_raw_reconstruction_and_classification",
    })
    digest = payload("TRANSPORT_MANIFEST.json", manifest)
    emit({"kind": "end", "manifest_sha256": digest,
          "payload_bytes": total, "transport_complete": not errors,
          "measurement_success": "not_inferred"})
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
