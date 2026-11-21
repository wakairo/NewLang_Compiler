#!/usr/bin/env python3
"""R1595 V2 DRAFT TEXT: publish saved data only; never run during PREP."""
import argparse
import base64
import hashlib
import json
import os
import stat

PREFIX = "R1595_LOG_V2 "
CASES = ("c02", "c03", "c01", "p01", "p02")
STAGES = ("public-observer", "cli-check", "emit-c17",
          "pristine-c17-compile", "pristine-native")
ROOT_LANES = {
    "build/r1595-records":
        ("FULL:GCC", "FULL:GCC Release", "FULL:Clang", "FULL:ASan", "FULL:UBSan"),
    "build-frontend/r1595-records": ("PARTIAL:frontend-c17",),
}
ENV_NAMES = (
    "GITHUB_REPOSITORY", "GITHUB_RUN_ID", "GITHUB_RUN_ATTEMPT", "GITHUB_SHA",
    "GITHUB_REF", "GITHUB_HEAD_REF", "GITHUB_JOB", "GITHUB_WORKFLOW",
)
MAX_FILE = 4 * 1024 * 1024
MAX_TOTAL = 16 * 1024 * 1024
CHUNK = 4096
DIR_FLAGS = os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW | os.O_CLOEXEC
FILE_FLAGS = os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK | os.O_CLOEXEC

def digest(data):
    return hashlib.sha256(data).hexdigest()

def canonical(value):
    return (json.dumps(value, ensure_ascii=True, sort_keys=True,
                       separators=(",", ":")) + "\n").encode("utf-8")

class Emitter:
    def __init__(self, context):
        self.bundle_id = digest(canonical(context))
        self.sequence = 0

    def emit(self, event, **fields):
        record = {"event": event, "bundle_id": self.bundle_id,
                  "sequence": self.sequence, **fields}
        print(PREFIX + canonical(record).decode("utf-8").rstrip("\n"),
              flush=True)
        self.sequence += 1

def fixed_paths():
    paths = ["pipeline-report.json"]
    for case in CASES:
        paths += [case + "/case-report.json", case + "/public-records.json"]
        for stage in STAGES:
            paths += [case + "/" + stage + suffix
                      for suffix in (".status.json", ".stdout.bin", ".stderr.bin")]
    return paths

def open_root(workspace, relative):
    # Anchor every path component; do not follow links or enumerate a directory.
    if (not workspace or not os.path.isabs(workspace) or
            os.path.normpath(workspace) != workspace):
        raise ValueError("WORKSPACE_PATH_NOT_ABSOLUTE_NORMALIZED")
    descriptor = os.open("/", DIR_FLAGS)
    try:
        for component in workspace.split("/")[1:] + relative.split("/"):
            if not component or component in (".", ".."):
                raise ValueError("INVALID_PATH_COMPONENT")
            child = os.open(component, DIR_FLAGS, dir_fd=descriptor)
            os.close(descriptor)
            descriptor = child
        return descriptor
    except Exception:
        os.close(descriptor)
        raise

def snapshot(info):
    return (info.st_dev, info.st_ino, info.st_mode, info.st_nlink,
            info.st_size, info.st_mtime_ns, info.st_ctime_ns)

def read_fixed(root_descriptor, relative, remaining):
    # Only the caller's fixed allowlist reaches here. No path from saved JSON
    # is opened; a sidecar's path is data to compare, never filesystem authority.
    directory = os.dup(root_descriptor)
    descriptor = None
    try:
        components = relative.split("/")
        for component in components[:-1]:
            child = os.open(component, DIR_FLAGS, dir_fd=directory)
            os.close(directory)
            directory = child
        descriptor = os.open(components[-1], FILE_FLAGS, dir_fd=directory)
        before = os.fstat(descriptor)
        if not stat.S_ISREG(before.st_mode) or before.st_nlink != 1:
            return {"state": "NOT_AVAILABLE",
                    "reason": "NONREGULAR_OR_MULTILINK"}, None
        limit = min(MAX_FILE, remaining)
        if before.st_size > limit:
            return {"state": "NOT_AVAILABLE", "reason": "BYTE_LIMIT",
                    "reported_bytes": before.st_size}, None
        chunks = []
        size = 0
        while size <= limit:
            piece = os.read(descriptor, min(65536, limit + 1 - size))
            if not piece:
                break
            chunks.append(piece)
            size += len(piece)
        after = os.fstat(descriptor)
        if size > limit:
            return {"state": "NOT_AVAILABLE", "reason": "BYTE_LIMIT"}, None
        if snapshot(before) != snapshot(after) or size != before.st_size:
            return {"state": "NOT_AVAILABLE", "reason": "CHANGED_DURING_READ"}, None
        data = b"".join(chunks)
        return {"state": "PRESENT", "bytes": size, "sha256": digest(data),
                "chunks": (size + CHUNK - 1) // CHUNK}, data
    except FileNotFoundError:
        return {"state": "NOT_AVAILABLE", "reason": "MISSING"}, None
    except OSError as error:
        return {"state": "NOT_AVAILABLE", "reason": "UNSAFE_OR_UNREADABLE",
                "errno": error.errno}, None
    finally:
        if descriptor is not None:
            os.close(descriptor)
        os.close(directory)

def sidecar_check(case, stage, blobs, absolute_root):
    stem = case + "/" + stage
    path = stem + ".status.json"
    raw = blobs.get(path)
    if raw is None:
        return {"path": path, "state": "NOT_AVAILABLE",
                "reason": "STATUS_NOT_AVAILABLE"}
    try:
        status_record = json.loads(raw.decode("utf-8"))
    except (UnicodeError, json.JSONDecodeError, RecursionError):
        return {"path": path, "state": "INCONSISTENT",
                "reason": "STATUS_JSON_INVALID"}
    if (not isinstance(status_record, dict) or
            status_record.get("schema") != "r1595.child.v2" or
            status_record.get("case") != case or
            status_record.get("label") != stage):
        return {"path": path, "state": "INCONSISTENT",
                "reason": "STATUS_IDENTITY_DIFFERENT"}
    unavailable = False
    for stream in ("stdout", "stderr"):
        stream_path = stem + "." + stream + ".bin"
        if status_record.get(stream + "_path") != absolute_root + "/" + stream_path:
            return {"path": path, "state": "INCONSISTENT",
                    "reason": "RECORDED_PATH_DIFFERENT"}
        stream_data = blobs.get(stream_path)
        if stream_data is None:
            unavailable = True
            continue
        if (type(status_record.get(stream + "_bytes")) is not int or
                status_record[stream + "_bytes"] != len(stream_data) or
                status_record.get(stream + "_sha256") != digest(stream_data)):
            return {"path": path, "state": "INCONSISTENT",
                    "reason": "RECORDED_BYTES_OR_HASH_DIFFERENT"}
    return {"path": path,
            "state": "NOT_AVAILABLE" if unavailable else "MATCH",
            "reason": "RAW_STREAM_MISSING" if unavailable else
                      "RECORDED_PATH_LENGTH_AND_SHA_MATCH"}

def main():
    parser = argparse.ArgumentParser(
        description="Data-only publisher of existing R1595 saved records")
    parser.add_argument("--root", choices=tuple(ROOT_LANES), required=True)
    parser.add_argument("--lane", required=True)
    args = parser.parse_args()
    metadata = {name: os.environ.get(name) for name in ENV_NAMES}
    context = {"schema": "r1595.log.v2", "root": args.root,
               "lane": args.lane, "available_environment_metadata": metadata}
    out = Emitter(context)
    out.emit("begin", **context, chunk_bytes=CHUNK,
             maximum_file_bytes=MAX_FILE, maximum_total_bytes=MAX_TOTAL,
             source_verdict="NOT_INTERPRETED", retrieved=False)
    manifest = []
    blobs = {}
    checks = []
    root_descriptor = None
    root_state = "NOT_AVAILABLE"
    root_reason = "NOT_OPENED"
    workspace = os.environ.get("GITHUB_WORKSPACE", "")
    try:
        if os.environ.get("GITHUB_ACTIONS") != "true":
            root_reason = "LOCAL_EXECUTION_FORBIDDEN"
        elif args.lane not in ROOT_LANES[args.root]:
            root_reason = "ROOT_LANE_MISMATCH"
        elif metadata["GITHUB_REPOSITORY"] != "wakairo/NewLang_Compiler":
            root_reason = "REPOSITORY_METADATA_MISMATCH"
        else:
            try:
                root_descriptor = open_root(workspace, args.root)
                root_state = "OPENED"
                root_reason = "FIXED_PATH_COMPONENTS_OPENED_WITHOUT_SYMLINKS"
            except FileNotFoundError:
                root_reason = "ROOT_MISSING"
            except ValueError:
                root_reason = "INVALID_WORKSPACE_PATH"
            except OSError:
                root_reason = "ROOT_UNSAFE_OR_UNREADABLE"
        out.emit("root", state=root_state, reason=root_reason)
        if root_descriptor is not None:
            total = 0
            for path in fixed_paths():
                info, data = read_fixed(root_descriptor, path, MAX_TOTAL - total)
                row = {"path": path, **info}
                manifest.append(row)
                out.emit("file", **row)
                if data is None:
                    continue
                blobs[path] = data
                total += len(data)
                for index, offset in enumerate(range(0, len(data), CHUNK)):
                    piece = data[offset:offset + CHUNK]
                    out.emit("chunk", path=path, index=index, offset=offset,
                             bytes=len(piece), sha256=digest(piece),
                             base64=base64.b64encode(piece).decode("ascii"))
            absolute_root = workspace + "/" + args.root
            for case in CASES:
                for stage in STAGES:
                    check = sidecar_check(case, stage, blobs, absolute_root)
                    checks.append(check)
                    out.emit("status_bytes", **check)
    finally:
        if root_descriptor is not None:
            os.close(root_descriptor)
    out.emit(
        "end", root_state=root_state, root_reason=root_reason,
        present_files=len(blobs),
        present_bytes=sum(len(data) for data in blobs.values()),
        unavailable_allowlisted_files=sum(row["state"] != "PRESENT"
                                         for row in manifest),
        manifest_sha256=digest(canonical(manifest)),
        manifest_encoding="sorted-key compact JSON array plus LF, UTF-8",
        status_byte_matches=sum(item["state"] == "MATCH" for item in checks),
        status_byte_inconsistencies=sum(item["state"] == "INCONSISTENT"
                                      for item in checks),
        status_bytes_not_available=sum(item["state"] == "NOT_AVAILABLE"
                                       for item in checks),
        pipeline_report="PRESENT" if "pipeline-report.json" in blobs else
                        "NOT_AVAILABLE",
        child_verdicts="UNCHANGED_OPAQUE_SAVED_DATA",
        retrieval="NOT_PERFORMED_BY_PUBLISHER",
        collector_must_bind_job_run_attempt_head_and_verify=True)
    # Completing a data report is not acceptance of its evidence. Missing,
    # inconsistent or incomplete required bytes make the future collector STOP.
    # Do not replace the already-completed CTest step's primary failure status.
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
