#!/usr/bin/env python3
"""Optional R-FAST source eligibility smoke. NOT a proof or R test verdict."""

import hashlib
import json
import os
from pathlib import Path
import re
import stat
import subprocess
import sys
import tempfile

BRANCH = re.compile(r"refs/heads/r-fast/r([1-9][0-9]{0,7})(?:-[a-z0-9][a-z0-9-]*)?")
BASENAME = re.compile(r"[a-z][a-z0-9_]{0,62}\.nl")
HEX_SHA = re.compile(r"[0-9a-f]{64}")
CODE = re.compile(r"[A-Z][A-Z0-9-]{2,79}")
FIRST_ERROR = re.compile(r"(?m)^.*?:\s*error\([a-z]+\)\[([A-Z][A-Z0-9-]+)\]:")
MAX_MANIFEST = 8192
MAX_SOURCE = 131072


class SmokeError(Exception):
    pass


def require(condition, message):
    if not condition:
        raise SmokeError(message)


def distinct_pairs(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, "duplicate manifest JSON key")
        result[key] = value
    return result


def checked_file(path, root, maximum):
    require(not path.is_symlink(), "symlink path is not allowed")
    require(path.is_file(), "required regular file missing")
    require(stat.S_ISREG(path.stat().st_mode), "not a regular file")
    require(path.resolve(strict=True).is_relative_to(root), "file escapes checkout")
    require(path.stat().st_size <= maximum, "file too large")
    return path.read_bytes()


def select_sources(root, ref):
    match = BRANCH.fullmatch(ref)
    if match is None:
        print("R-FAST SOURCE NOT TESTED (no r<issue> source opt-in)")
        return None
    issue = int(match.group(1))
    target = root / "tests" / ("r" + str(issue))
    require(not (root / "tests").is_symlink(), "symlink tests root")
    require(not target.is_symlink(), "symlink issue directory")
    manifest = target / "r-fast-source-smoke.json"
    if not manifest.exists() and not manifest.is_symlink():
        print("R-FAST SOURCE NOT TESTED (no data-only manifest)")
        return None
    raw = checked_file(manifest, root, MAX_MANIFEST)
    try:
        obj = json.loads(raw.decode("utf-8"), object_pairs_hook=distinct_pairs)
    except (UnicodeError, ValueError) as exc:
        raise SmokeError("invalid manifest JSON/UTF-8") from exc
    require(type(obj) is dict and set(obj) == {"issue", "sources"}, "manifest keys")
    require(type(obj["issue"]) is int and obj["issue"] == issue, "manifest issue mismatch")
    sources = obj["sources"]
    require(type(sources) is list and 1 <= len(sources) <= 2, "must opt into 1-2 sources")
    found = set()
    selected = []
    for entry in sources:
        require(type(entry) is dict and set(entry) ==
                {"file", "sha256", "expect_exit", "first_diagnostic"}, "source entry keys")
        name = entry["file"]
        require(type(name) is str and BASENAME.fullmatch(name) is not None,
                "source name must be one safe .nl basename")
        require(name not in found, "duplicate source name")
        found.add(name)
        digest = entry["sha256"]
        require(type(digest) is str and HEX_SHA.fullmatch(digest) is not None,
                "source SHA256 malformed")
        status = entry["expect_exit"]
        require(type(status) is int and status in (0, 3), "expected exit must be 0 or 3")
        code = entry["first_diagnostic"]
        require((code is None if status == 0 else
                 type(code) is str and CODE.fullmatch(code) is not None),
                "expected first diagnostic inconsistent with exit")
        source_path = target / name
        payload = checked_file(source_path, root, MAX_SOURCE)
        actual = hashlib.sha256(payload).hexdigest()
        require(actual == digest, "frozen source SHA256 mismatch for " + name)
        require(payload.endswith(b"\n") and b"\r" not in payload,
                "frozen source must use LF and trailing LF")
        selected.append((name, source_path, digest, status, code))
    return selected


def check_sources(root, ref, binary):
    cases = select_sources(root, ref)
    if cases is None:
        return
    require(not binary.is_symlink(), "compiler binary is symlink")
    require(binary.is_file() and os.access(binary, os.X_OK), "compiled newlangc unavailable")
    require(binary.resolve(strict=True).is_relative_to(root), "compiler escapes checkout")
    for name, path, sha, expected_exit, expected_code in cases:
        try:
            done = subprocess.run([str(binary), "--check", str(path)],
                                  cwd=root, shell=False, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE, timeout=20, check=False)
        except (OSError, subprocess.TimeoutExpired) as exc:
            raise SmokeError("newlangc execution failed or timed out for " + name) from exc
        try:
            stdout = done.stdout.decode("utf-8", errors="strict")
            stderr = done.stderr.decode("utf-8", errors="strict")
        except UnicodeDecodeError as exc:
            raise SmokeError("non-UTF8 CLI diagnostics for " + name) from exc
        first = FIRST_ERROR.search(stderr)
        first_code = first.group(1) if first else None
        print("R-FAST SOURCE observed file=" + name + " sha256=" + sha +
              " exit=" + str(done.returncode) +
              " first_diagnostic=" + str(first_code), flush=True)
        require(not stdout, "unexpected stdout for " + name)
        if expected_exit == 0:
            require(not stderr, "unexpected diagnostic on CHECK0 for " + name)
        else:
            require(first_code is not None, "missing first diagnostic for " + name)
        require(done.returncode == expected_exit and first_code == expected_code,
                "SOURCE SMOKE RED first gate mismatch for " + name +
                ": expected exit=" + str(expected_exit) +
                " first=" + str(expected_code))
    print("R-FAST SOURCE ELIGIBILITY PASS " + str(len(cases)) +
          " frozen CHECK(s); NO PUBLIC PROOF/NATIVE/NEGATIVE/OOM/SANITIZER/R PASS")


def self_test():
    with tempfile.TemporaryDirectory(prefix="t904-smoke-") as d:
        root = Path(d).resolve(strict=True)
        target = root / "tests" / "r904"
        target.mkdir(parents=True)
        ref = "refs/heads/r-fast/r904-t-only-pilot"
        require(select_sources(root, ref) is None, "manifest-absent should skip")
        manifest = target / "r-fast-source-smoke.json"
        def emit(name, sha):
            manifest.write_text(json.dumps({
                "issue": 904, "sources": [{"file": name, "sha256": sha,
                                         "expect_exit": 0, "first_diagnostic": None}]
            }) + "\n", encoding="utf-8")
        emit("../escape.nl", "0" * 64)
        try:
            select_sources(root, ref)
        except SmokeError:
            pass
        else:
            raise SmokeError("path traversal not refused")
        (target / "clean.nl").write_bytes(b"fn main()->unit{unit}\n")
        emit("clean.nl", "0" * 64)
        try:
            select_sources(root, ref)
        except SmokeError:
            pass
        else:
            raise SmokeError("wrong frozen SHA not refused")
        print("R-FAST STATIC MANIFEST NEGATIVES PASS: absent=NOT TESTED, path=STOP, SHA=STOP")


def main():
    try:
        if sys.argv[1:] == ["--self-test"]:
            self_test()
            return 0
        require(len(sys.argv) == 3, "usage: script REPO_ROOT BUILT_NEWLANGC")
        root = Path(sys.argv[1]).resolve(strict=True)
        require(root.is_dir(), "checkout not a directory")
        ref = os.environ.get("GITHUB_REF", "")
        binary = root / sys.argv[2]
        require(sys.argv[2] == "build-frontend/newlangc", "non-pinned binary argument")
        check_sources(root, ref, binary)
        return 0
    except (SmokeError, OSError) as exc:
        print("R-FAST SOURCE SMOKE STOP: " + str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
