"""R1601 fixed-path data publisher V2, TEXT DRAFT.

Never executes compiler/tests and never changes a primary test result.
A zero publisher exit is NOT evidence availability: inspect R1601v1 STATUS/END.
"""
import base64
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import sys

MAX_FILES = 104
MAX_BYTES = 768 * 1024
CHUNK = 3072
LABEL = (
    r"(?:git-head|(?:p1|p2|c3|c4)-(?:check|observe|emit|cc|native)"
    r"|(?:c1|c2)-(?:check|observe))"
)
DATA_NAME = re.compile(
    r"(?:context\.json|summary\.json|SHA256SUMS|[0-9]{2}-" + LABEL +
    r"\.(?:start\.json|result\.json|stdout\.bin|stderr\.bin))"
)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def line(kind, payload):
    print("R1601v1 " + kind + " " +
          json.dumps(payload, sort_keys=True, separators=(",", ":")),
          flush=True)


def read_regular(path):
    # Nonblocking open is needed BEFORE fstat: an allowlisted FIFO must not
    # wait for a writer. No follow, regular type and one link precede any read.
    flags = os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK
    fd = os.open(path, flags)
    try:
        before = os.fstat(fd)
        if (not stat.S_ISREG(before.st_mode) or before.st_nlink != 1 or
                not 0 <= before.st_size <= MAX_BYTES):
            raise ValueError("data is not a bounded regular single-link file")

        def identity(info):
            return (info.st_dev, info.st_ino, info.st_mode, info.st_nlink,
                    info.st_size, info.st_mtime_ns, info.st_ctime_ns)

        chunks = []
        total = 0
        while True:
            data = os.read(fd, min(65536, MAX_BYTES + 1 - total))
            if not data:
                break
            chunks.append(data)
            total += len(data)
            if total > MAX_BYTES:
                raise ValueError("oversized data during read")
        after = os.fstat(fd)
        if (not stat.S_ISREG(after.st_mode) or after.st_nlink != 1 or
                identity(before) != identity(after) or total != after.st_size):
            raise ValueError("descriptor data/type/link count changed during read")
        named = os.stat(path, follow_symlinks=False)
        if (not stat.S_ISREG(named.st_mode) or named.st_nlink != 1 or
                identity(named) != identity(after)):
            raise ValueError("named data changed or no longer names opened file")
        return b"".join(chunks)
    finally:
        os.close(fd)


def emit_file(name, data, origin):
    parts = (len(data) + CHUNK - 1) // CHUNK
    line("FILE", {
        "name": name, "bytes": len(data), "sha256": sha(data),
        "parts": parts, "origin": origin,
    })
    for index in range(parts):
        piece = data[index * CHUNK:(index + 1) * CHUNK]
        line("DATA", {
            "name": name, "index": index,
            "base64": base64.b64encode(piece).decode("ascii"),
        })
    line("END_FILE", {"name": name, "parts": parts})


def publish(mode):
    if mode not in {"full", "partial"}:
        line("STATUS", {"status": "INVALID_MODE"})
        return
    project = Path(__file__).resolve().parents[2]
    build = project / ("build" if mode == "full" else "build-frontend")
    raw = build / "r1601-raw"
    if build.is_symlink() or raw.is_symlink():
        line("STATUS", {"status": "SYMLINK_ROOT_REFUSED"})
        return
    if not raw.is_dir():
        line("STATUS", {
            "status": "NO_RAW_DIRECTORY", "mode": mode,
            "meaning": "test may not have started; no evidence claimed",
        })
        return
    names = sorted(path.name for path in raw.iterdir())
    if len(names) > MAX_FILES or any(not DATA_NAME.fullmatch(name)
                                    for name in names):
        line("STATUS", {"status": "UNEXPECTED_FILE_SET"})
        return
    files = {}
    total = 0
    for name in names:
        data = read_regular(raw / name)
        total += len(data)
        if total > MAX_BYTES:
            line("STATUS", {"status": "BUNDLE_OVER_LIMIT", "bytes": total})
            return
        files[name] = data

    runner_manifest = "MISSING"
    if "SHA256SUMS" in files:
        manifest = files["SHA256SUMS"]
        if not manifest.endswith(b"\n"):
            raise ValueError("runner manifest missing final LF")
        expected = {}
        for item in manifest.decode("ascii").splitlines():
            match = re.fullmatch(r"([0-9a-f]{64})  ([A-Za-z0-9_.-]+)", item)
            if not match or match[2] in expected or match[2] == "SHA256SUMS":
                raise ValueError("runner manifest malformed")
            expected[match[2]] = match[1]
        if set(expected) != set(files).difference({"SHA256SUMS"}):
            raise ValueError("runner manifest file set mismatch")
        if any(sha(files[name]) != value for name, value in expected.items()):
            raise ValueError("runner manifest hash mismatch")
        runner_manifest = "VERIFIED"

    context = {}
    summary = {}
    if "context.json" in files:
        context = json.loads(files["context.json"].decode("ascii"))
    if "summary.json" in files:
        summary = json.loads(files["summary.json"].decode("ascii"))
    # This is a derived transport inventory, explicitly not a recovered file.
    inventory = "".join(
        sha(files[name]) + " " + str(len(files[name])) + " " + name + "\n"
        for name in names
    ).encode("ascii")
    metadata = {
        "mode": mode, "repo": os.environ.get("GITHUB_REPOSITORY"),
        "run_id": os.environ.get("GITHUB_RUN_ID"),
        "attempt": os.environ.get("GITHUB_RUN_ATTEMPT"),
        "job_key": os.environ.get("GITHUB_JOB"),
        "job_label": os.environ.get("R1601_JOB_LABEL"),
        "event_sha": os.environ.get("GITHUB_SHA"),
        "checkout_sha": summary.get("checkout_sha"),
        "runner_manifest": runner_manifest,
        "runner_status": summary.get("status", "MISSING"),
        "suite_completed": summary.get("completed", False),
        "context_environment_matches": context.get("environment") == {
            key: os.environ.get(key) for key in (
                "GITHUB_REPOSITORY", "GITHUB_RUN_ID", "GITHUB_RUN_ATTEMPT",
                "GITHUB_JOB", "GITHUB_SHA", "GITHUB_EVENT_NAME", "GITHUB_REF",
            )
        },
        "payload_files": len(names), "payload_bytes": total,
        "transport_inventory_sha256": sha(inventory),
        "transport_inventory_bytes": len(inventory),
    }
    line("BEGIN", metadata)
    emit_file("transport.manifest", inventory, "publisher_derived")
    for name in names:
        emit_file(name, files[name], "captured_runner_file")
    line("END", metadata)


def main():
    try:
        if len(sys.argv) != 2:
            line("STATUS", {"status": "EXPECTED_FULL_OR_PARTIAL"})
        else:
            publish(sys.argv[1])
    except Exception as error:
        line("STATUS", {
            "status": "PUBLISHER_DATA_ERROR",
            "error_type": type(error).__name__,
            "message": str(error)[:200],
            "meaning": "raw availability unverified; primary suite unchanged",
        })
    # Never manufacture a test failure merely to expose captured output.
    # Missing/truncated transport is NOT_RETRIEVED even on a green job.
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
