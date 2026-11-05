"""Restore R1185 C249 original bytes. Pure text/hash/archive work; executes nothing.

Usage: python restore.py ARCHIVE_DIRECTORY OUTPUT_DIRECTORY
       python restore.py ARCHIVE_DIRECTORY --verify-only
Only Python's standard library is used. No network or subprocess is invoked.
"""
import argparse
import base64
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import re
import tarfile


def sha(data):
    return hashlib.sha256(data).hexdigest()


def exact(data, expected, label):
    if len(data) != expected["bytes"] or sha(data) != expected["sha256"]:
        raise ValueError("bytes/hash mismatch: " + label)
    return data


def safe_name(value):
    name = PurePosixPath(value)
    if not value or name.is_absolute() or ".." in name.parts or str(name) != value:
        raise ValueError("unsafe archive path")
    return name


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("output", type=Path, nargs="?")
    parser.add_argument("--verify-only", action="store_true")
    args = parser.parse_args()
    root = args.archive
    manifest_bytes = (root / "manifest.json").read_bytes()
    manifest = json.loads(manifest_bytes)
    if manifest["schema"] != "R1185_C249_REVERSIBLE_ARCHIVE_V1":
        raise ValueError("unsupported manifest")
    rows, recipes = [], {}
    for part in manifest["file_index_parts"]:
        data = exact((root / safe_name(part["path"])).read_bytes(), part, part["path"])
        rows.extend(json.loads(data))
    for part in manifest["object_index_parts"]:
        data = exact((root / safe_name(part["path"])).read_bytes(), part, part["path"])
        for record in json.loads(data):
            key = record["sha256"]
            if key in recipes:
                raise ValueError("duplicate object identity")
            recipes[key] = record
    if len(rows) != manifest["file_count"] or len(recipes) != manifest["unique_object_count"]:
        raise ValueError("inventory count mismatch")
    if len({r["path"] for r in rows}) != len(rows):
        raise ValueError("duplicate original path")
    packed_parts = []
    for part in manifest["stored_object_bundle"]["parts"]:
        encoded = exact((root / safe_name(part["path"])).read_bytes(), part, part["path"])
        decoded = base64.b64decode(encoded, validate=True)
        exact(decoded, {"bytes": part["decoded_bytes"], "sha256": part["decoded_sha256"]}, part["path"] + " decoded")
        packed_parts.append(decoded)
    packed = exact(b"".join(packed_parts), manifest["stored_object_bundle"], "complete stored-object gzip")
    cache = {}
    with tarfile.open(fileobj=io.BytesIO(packed), mode="r:gz") as tar:
        for member in tar.getmembers():
            safe_name(member.name)
            if not member.isfile() or not member.name.startswith("objects/"):
                raise ValueError("nonobject member in storage bundle")
            key = member.name.removeprefix("objects/")
            if key in cache or key not in recipes or recipes[key]["source"]["kind"] != "stored":
                raise ValueError("unexpected/duplicate stored object")
            stream = tar.extractfile(member)
            if stream is None:
                raise ValueError("unreadable stored object")
            cache[key] = exact(stream.read(), recipes[key], key)
    if len(cache) != manifest["stored_object_count"]:
        raise ValueError("stored-object count mismatch")
    member_maps = {}
    resolving = set()

    def resolve(key):
        if key in cache:
            return cache[key]
        if key not in recipes or key in resolving:
            raise ValueError("missing/cyclic reconstruction dependency")
        resolving.add(key)
        record = recipes[key]
        source = record["source"]
        kind = source["kind"]
        if kind == "log_export":
            text = resolve(source["log_sha256"]).decode("utf-8", errors="strict")
            begins = list(re.finditer(r"R1185_RAW_BEGIN sha256=([0-9a-f]{64}) bytes=(\d+)", text))
            if len(begins) != 1 or begins[0].group(1) != key or int(begins[0].group(2)) != record["bytes"]:
                raise ValueError("original log export declaration mismatch")
            tail = text[begins[0].end():]
            if tail.count("R1185_RAW_END") != 1:
                raise ValueError("original log export framing mismatch")
            block = tail.split("R1185_RAW_END", 1)[0]
            encoded = "".join(re.findall(r"R1185_RAW ([A-Za-z0-9+/=]+)", block))
            data = base64.b64decode(encoded, validate=True)
        elif kind == "tar_member":
            archive = source["archive_sha256"]
            if archive not in member_maps:
                members = {}
                with tarfile.open(fileobj=io.BytesIO(resolve(archive)), mode="r:gz") as tar:
                    for member in tar.getmembers():
                        safe_name(member.name)
                        if not member.isfile() or member.name in members:
                            raise ValueError("invalid/duplicate original raw member")
                        stream = tar.extractfile(member)
                        if stream is None:
                            raise ValueError("unreadable original raw member")
                        value = stream.read()
                        digest = sha(value)
                        if len(value) != member.size:
                            raise ValueError("original raw member length mismatch")
                        members[member.name] = digest
                        if digest in recipes:
                            cache[digest] = exact(value, recipes[digest], digest)
                member_maps[archive] = members
            if member_maps[archive].get(source["member"]) != key or key not in cache:
                raise ValueError("original raw member identity mismatch")
            data = cache[key]
        elif kind == "base64":
            data = base64.b64encode(resolve(source["object_sha256"]))
        else:
            raise ValueError("unavailable reconstruction recipe")
        cache[key] = exact(data, record, key)
        resolving.remove(key)
        return cache[key]

    if not args.verify_only:
        if args.output is None or (args.output.exists() and any(args.output.iterdir())):
            raise ValueError("restoration needs a new or empty output directory")
        args.output.mkdir(parents=True, exist_ok=True)
    for row in rows:
        path = safe_name(row["path"])
        data = exact(resolve(row["sha256"]), row, row["path"])
        if not args.verify_only:
            target = args.output / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            target.chmod(row["mode"])
    print(json.dumps({"status": "ALL_ORIGINAL_FILE_BYTES_AND_HASHES_VERIFIED",
        "manifest_sha256": sha(manifest_bytes), "files": len(rows),
        "logical_bytes": sum(r["bytes"] for r in rows), "unique_objects": len(recipes),
        "whole_decoded_logs": manifest["evidence_counts"]["whole_decoded_logs"],
        "original_raw_gzip": manifest["evidence_counts"]["original_raw_gzip"],
        "mode": "verify-only" if args.verify_only else "restored",
        "semantic_execution": 0}, indent=2))


if __name__ == "__main__":
    main()
