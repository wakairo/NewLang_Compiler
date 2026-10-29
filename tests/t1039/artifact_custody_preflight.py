#!/usr/bin/env python3
"""T1039 SYNTHETIC ONLY: fail-closed upload-artifact name and raw-file preflight.

Does not access R original files, call GitHub, or upload anything.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
from string import Formatter

# Deliberately narrower than the upstream character set for portable custody.
PORTABLE = re.compile(r"[A-Za-z0-9][A-Za-z0-9._-]{0,159}\Z", re.ASCII)
SHA = re.compile(r"[0-9a-f]{40}\Z", re.ASCII)
RUN_ID = re.compile(r"[1-9][0-9]{0,19}\Z", re.ASCII)
ALLOWED_FIELDS = frozenset({"ref", "ref_slug", "run_id", "attempt", "sha"})
RESERVED = frozenset({"CON", "PRN", "AUX", "NUL", *(f"COM{i}" for i in range(1, 10)), *(f"LPT{i}" for i in range(1, 10))})
MAX_FILES = 128


class PreflightError(ValueError):
    pass


def safe_name(name):
    if (not PORTABLE.fullmatch(name) or name.endswith(".") or ".." in name
            or name.split(".", 1)[0].upper() in RESERVED):
        raise PreflightError(f"unsafe artifact name {name!r}")
    return name


def render(template, *, ref, run_id, attempt, sha):
    if not re.fullmatch(r"r-calibrated/r1016-pr1031(?:-v[12])?", ref):
        raise PreflightError("ref outside synthetic calibrated v1/v2 allowlist")
    if not RUN_ID.fullmatch(str(run_id)) or not 1 <= int(attempt) <= 99 or not SHA.fullmatch(sha):
        raise PreflightError("invalid run/attempt/SHA custody metadata")
    segments = list(Formatter().parse(template))
    fields = [field for _, field, _, _ in segments if field is not None]
    if any(field not in ALLOWED_FIELDS or fmt or conv for _, field, fmt, conv in segments if field is not None):
        raise PreflightError("unrecognized or executable template interpolation")
    if any(field not in fields for field in ("run_id", "attempt", "sha")):
        raise PreflightError("name must identify distinct run, attempt and immutable SHA")
    values = dict(ref=ref, ref_slug=ref.replace("/", "-"), run_id=str(run_id), attempt=str(attempt), sha=sha)
    try:
        name = template.format_map(values)
    except (ValueError, KeyError) as exc:
        raise PreflightError("malformed name template") from exc
    return safe_name(name)


def validate_names(template, records, existing=()):
    """Synthetic planned upload slots: duplicate names fail before any upload."""
    names = set(existing)
    outputs = []
    for record in records:
        name = render(template, **record)
        if name in names:
            raise PreflightError(f"duplicate name in one upload plan {name!r}")
        names.add(name)
        outputs.append(name)
    return outputs


def validate_files(raw_dir, *, required, expected_count):
    """Reject empty/missing/unreadable/symlink raw custody; record real SHA256s."""
    path = Path(raw_dir)
    if path.is_symlink() or not path.is_dir():
        raise PreflightError("raw output directory missing or symlink")
    files = sorted(path.iterdir())
    if not (1 <= len(files) <= MAX_FILES) or len(files) != expected_count:
        raise PreflightError(f"raw file count mismatch: got {len(files)}, expected {expected_count}")
    wanted = set(required)
    if not wanted or len(wanted) != len(required) or any(not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]{0,99}", n) for n in wanted):
        raise PreflightError("invalid required receipt filenames")
    if not wanted.issubset({f.name for f in files}):
        raise PreflightError("missing required raw custody receipt")
    digests = {}
    for f in files:
        if not PORTABLE.fullmatch(f.name) or f.name.endswith(".") or ".." in f.name:
            raise PreflightError(f"nonportable raw filename: {f.name!r}")
        if f.is_symlink() or not f.is_file() or f.stat().st_size == 0:
            raise PreflightError(f"nonregular, symlink or empty raw file: {f.name}")
        digests[f.name] = hashlib.sha256(f.read_bytes()).hexdigest()
    return digests


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("--template", required=True)
    cli.add_argument("--ref", required=True)
    cli.add_argument("--run-id", required=True)
    cli.add_argument("--attempt", type=int, required=True)
    cli.add_argument("--sha", required=True)
    cli.add_argument("--raw-dir", required=True)
    cli.add_argument("--expected-count", type=int, required=True)
    cli.add_argument("--required", action="append", required=True)
    a = cli.parse_args()
    try:
        name = render(a.template, ref=a.ref, run_id=a.run_id, attempt=a.attempt, sha=a.sha)
        digest_map = validate_files(a.raw_dir, required=a.required, expected_count=a.expected_count)
    except (PreflightError, OSError) as exc:
        cli.exit(2, f"T1039 SYNTHETIC PREFLIGHT RED: {exc}\n")
    print(json.dumps({"status": "SYNTHETIC PREFLIGHT PASS (not upload/native evidence)",
                      "name": name, "ref": a.ref, "run_id": a.run_id,
                      "attempt": a.attempt, "sha": a.sha, "raw_files": digest_map}, sort_keys=True))


if __name__ == "__main__":
    main()
