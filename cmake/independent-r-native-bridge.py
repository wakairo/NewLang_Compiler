"""T1014 NEW RECONSTRUCTION: unhooked, unapproved native receipt bridge preview.

NOT T1005 original ZIP bytes. NOT an R runner or a source-issued proof verifier.
Any integration requires an independently authorized C approval and new freeze.
"""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import re
import stat
import subprocess
import sys
import tempfile
from dataclasses import dataclass

HEADER = "NL_R_NATIVE_V1"
RECEIPT = "NL_R_NATIVE_RECEIPT_V1"
MANIFEST_PATH = "tests/r/approved-native-manifest.txt"
MAX_MANIFEST = 8192
MAX_SOURCE = 65536
MAX_AUX = 65536
MAX_RECEIPT = 32768
MAX_CHILD_OUTPUT = 8192
MAX_CASES = 8
CHILD_TIMEOUT = 90
HEX64 = re.compile(r"[0-9a-f]{64}\Z", re.ASCII)
HEX40 = re.compile(r"[0-9a-f]{40}\Z", re.ASCII)
COHORT = re.compile(r"r[1-9][0-9]{2,5}\Z", re.ASCII)
CASE = re.compile(r"(r[1-9][0-9]{2,5})_[a-z][a-z0-9_]{1,39}\Z", re.ASCII)
LEAF = r"[a-z][a-z0-9_]{0,63}"
FILE = re.compile(rf"tests/(r[1-9][0-9]{{2,5}})/sources/({LEAF})\.nl\Z", re.ASCII)
AUX = re.compile(r"tests/(r[1-9][0-9]{2,5})/native/(runner\.py|observer\.py|oracle\.json)\Z", re.ASCII)


@dataclass(frozen=True)
class Approval:
    """Reviewed OUTSIDE the R-controlled descriptor; never self-signed by it."""
    cohort: str
    manifest_sha256: str
    case_ids: tuple[str, ...]
    head: str
    parent: str


# DO NOT populate in a T preview or take approvals from argv, env, or manifest.
APPROVED: dict[str, Approval] = {}


class BridgeStop(ValueError):
    pass


def stop(why: str) -> None:
    raise BridgeStop("T1014 STOP: " + why)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def strict_regular(root: Path, rel: str, limit: int) -> bytes:
    """No traversal/symlinks/hardlinks or file-type ambiguity, path-by-path."""
    if not isinstance(rel, str) or not rel or rel.startswith("/") or "\\" in rel:
        stop("absolute or malformed relative path")
    parts = rel.split("/")
    if any(x in ("", ".", "..") for x in parts):
        stop("path traversal/alias")
    current = root
    for index, part in enumerate(parts):
        current = current / part
        try:
            entry = current.lstat()
        except (OSError, ValueError) as exc:
            stop("missing or inaccessible path: " + repr(exc))
        if stat.S_ISLNK(entry.st_mode):
            stop("symlink path component")
        if index < len(parts) - 1 and not stat.S_ISDIR(entry.st_mode):
            stop("non-directory component")
    if not stat.S_ISREG(entry.st_mode) or entry.st_nlink != 1:
        stop("nonregular or hard-linked file")
    if entry.st_size > limit:
        stop("oversized input")
    if not current.resolve().is_relative_to(root.resolve()):
        stop("outside checkout")
    data = current.read_bytes()
    if len(data) > limit or len(data) != entry.st_size:
        stop("unstable or oversized file")
    return data


def strict_json(raw: bytes) -> dict:
    if len(raw) > MAX_RECEIPT:
        stop("oversized receipt")
    def no_duplicates(pairs):
        obj = {}
        for key, value in pairs:
            if key in obj:
                stop("duplicate JSON key")
            obj[key] = value
        return obj
    try:
        obj = json.loads(raw.decode("utf-8"), object_pairs_hook=no_duplicates,
                         parse_constant=lambda x: stop("invalid JSON constant"))
    except (UnicodeError, json.JSONDecodeError) as exc:
        stop("invalid JSON receipt: " + str(exc))
    if not isinstance(obj, dict):
        stop("receipt is not object")
    return obj


def parse(root: Path, approvals: dict[str, Approval] | None = None):
    """Return verified manifest; no permission if external approval absent."""
    root = root.resolve(strict=True)
    raw = strict_regular(root, MANIFEST_PATH, MAX_MANIFEST)
    if not raw or b"\r" in raw or b"\x00" in raw or not raw.endswith(b"\n"):
        stop("manifest must be nonempty, LF-only, terminated")
    try:
        lines = raw.decode("ascii").splitlines()
    except UnicodeDecodeError:
        stop("manifest must be ASCII")
    if not lines or lines.pop(0) != HEADER:
        stop("wrong native manifest header")
    if len(lines) < 5 or len(lines) > 4 + MAX_CASES:
        stop("missing header/aux/cases or >8 cases")
    if lines[0].split("|")[:1] != ["cohort"]:
        stop("missing cohort")
    first = lines[0].split("|")
    if len(first) != 2 or not COHORT.fullmatch(first[1]):
        stop("bad cohort")
    cohort = first[1]
    ap = (APPROVED if approvals is None else approvals).get(cohort)
    if not isinstance(ap, Approval) or ap.cohort != cohort:
        stop("missing EXTERNAL C approval")
    if not HEX40.fullmatch(ap.head) or not HEX40.fullmatch(ap.parent):
        stop("unreviewed head/parent syntax")
    if not HEX64.fullmatch(ap.manifest_sha256) or sha(raw) != ap.manifest_sha256:
        stop("manifest SHA differs from external C approval")
    files = {}
    for kind, expected_leaf in zip(("runner", "observer", "oracle"),
                                   ("runner.py", "observer.py", "oracle.json")):
        fields = lines.pop(1).split("|")
        if len(fields) != 3 or fields[0] != kind or not HEX64.fullmatch(fields[2]):
            stop("malformed or reordered native auxiliary record")
        match = AUX.fullmatch(fields[1])
        if not match or match.group(1) != cohort or match.group(2) != expected_leaf:
            stop("wrong auxiliary path")
        if sha(strict_regular(root, fields[1], MAX_AUX)) != fields[2]:
            stop("auxiliary SHA mismatch")
        files[kind] = (fields[1], fields[2])
    rows = []
    seen = set()
    for line in lines[1:]:
        fields = line.split("|")
        if len(fields) != 5 or fields[0] != "case":
            stop("bad case row/embedded command")
        _, name, mode, path, expected_sha = fields
        match, file_match = CASE.fullmatch(name), FILE.fullmatch(path)
        if (not match or match.group(1) != cohort or
            not file_match or file_match.group(1) != cohort or
            mode not in ("positive", "negative") or
            not HEX64.fullmatch(expected_sha)):
            stop("bad case name/mode/source/SHA")
        if name in seen or path in (r[2] for r in rows):
            stop("duplicate case or source")
        seen.add(name)
        if sha(strict_regular(root, path, MAX_SOURCE)) != expected_sha:
            stop("source SHA mismatch")
        rows.append((name, mode, path, expected_sha))
    if not 1 <= len(rows) <= MAX_CASES or tuple(x[0] for x in rows) != ap.case_ids:
        stop("ordered case IDs differ from external C approval")
    return cohort, ap, files, rows, sha(raw)


def receipt_stage(stages, key):
    item = stages.get(key)
    if not isinstance(item, dict) or item.get("status") != "pass" or item.get("observed") is not True:
        stop("unobserved or unsuccessful stage: " + key)
    if not HEX64.fullmatch(str(item.get("evidence_sha256", ""))):
        stop("missing stage evidence digest: " + key)
    return item


def validate_receipt(raw: bytes, cohort: str, ap: Approval, rows, manifest_sha: str):
    """Strict completeness/typing ONLY, not physical authenticity of R claims."""
    r = strict_json(raw)
    if set(r) != {"schema", "cohort", "head", "parent", "manifest_sha256", "cases"}:
        stop("unexpected or missing receipt root keys")
    if (r["schema"], r["cohort"], r["head"], r["parent"], r["manifest_sha256"]) != (
        RECEIPT, cohort, ap.head, ap.parent, manifest_sha):
        stop("receipt identity/approval mismatch")
    if not isinstance(r["cases"], list) or len(r["cases"]) != len(rows):
        stop("missing/extra receipt cases")
    for item, (name, mode, _path, source_sha) in zip(r["cases"], rows):
        if not isinstance(item, dict) or set(item) != {"id", "mode", "source_sha256", "stages"}:
            stop("malformed receipt case")
        if (item["id"], item["mode"], item["source_sha256"]) != (name, mode, source_sha):
            stop("missing, reordered, or altered receipt case")
        stages = item["stages"]
        if not isinstance(stages, dict):
            stop("stages missing")
        if mode == "positive":
            keys = {"check", "owned_current", "old_copy", "emitted_c17_original",
                    "gcc_o0", "gcc_o2", "clang_o0", "clang_o2", "guest0", "guest1",
                    "guest2", "asan", "ubsan", "teardown"}
            if set(stages) != keys:
                stop("positive stages incomplete/extra")
            for key in keys:
                stage = receipt_stage(stages, key)
                if key == "owned_current" and stage.get("source_issued") is not True:
                    stop("no source-issued original receipt claim")
                if key == "old_copy" and any(type(stage.get(v)) is not int or
                                             not 0 <= stage[v] <= 255 for v in ("old", "current")):
                    stop("missing actual old/current Copy values")
                if key == "emitted_c17_original" and stage.get("unchanged") is not True:
                    stop("original C17 unverified")
                if key in ("guest0", "guest1", "guest2"):
                    expected = int(key[-1])
                    if stage.get("reached") is not True or any(
                        type(stage.get(x)) is not int or stage[x] != expected
                        for x in ("allocation_successes", "frees")
                    ):
                        stop("guest failure-world not actually reached or count mismatch")
        else:
            if set(stages) != {"check_reject", "no_clobber"}:
                stop("negative stages incomplete/extra")
            reject = receipt_stage(stages, "check_reject")
            if (reject.get("reason") != "semantic" or
                not re.fullmatch(r"[A-Z][A-Z0-9-]{2,70}", str(reject.get("code", ""))) or
                reject.get("safety_reject") is not True):
                stop("generic parser/CLI rejection is not a semantic safety negative")
            if receipt_stage(stages, "no_clobber").get("unchanged") is not True:
                stop("negative no-clobber not witnessed")
    # A self-signed receipt cannot authenticate its own stage data or host witnesses.
    return "CLAIM_SCHEMA_COMPLETE__PHYSICAL_UNAUTHENTICATED"


def configure(root: Path, output: Path, approvals=None) -> int:
    """Emit an inert, T-templated optional CMake registration file."""
    root = root.resolve(strict=True)
    manifest = root / MANIFEST_PATH
    if not manifest.exists() and not manifest.is_symlink():
        output.write_text("# No native descriptor: 0 CTests\n", encoding="ascii")
        return 0
    cohort, _ap, _files, _rows, _digest = parse(root, approvals)
    name = cohort + "_native_suite.integration"
    # Only cohort (strict regex) is interpolated; paths and argv fixed by T.
    code = (
        "# T1014 UNHOOKED PREVIEW. T-owned generation, never raw manifest include.\n"
        f"if(TEST {name})\n"
        f'  message(FATAL_ERROR "T1014 duplicate registered CTest {name}")\n'
        "endif()\n"
        f'add_test(NAME {name} COMMAND "${{Python3_EXECUTABLE}}"\n'
        '  "${CMAKE_SOURCE_DIR}/cmake/independent-r-native-bridge.py" --run\n'
        '  "${CMAKE_SOURCE_DIR}" "$<TARGET_FILE:newlangc>")\n'
        f"set_tests_properties({name} PROPERTIES TIMEOUT {CHILD_TIMEOUT})\n"
    )
    output.write_text(code, encoding="ascii")
    return 1


def run(root: Path, compiler: str, approvals=None) -> str:
    """Invoke separately reviewed runner; never execute missing/unapproved R."""
    root = root.resolve(strict=True)
    cohort, ap, files, rows, manifest_digest = parse(root, approvals)
    runner = root / files["runner"][0]
    observer = root / files["observer"][0]
    oracle = root / files["oracle"][0]
    with tempfile.TemporaryDirectory(prefix="t1014-receipt-") as tmp:
        temp = Path(tmp)
        receipt = temp / "receipt.json"
        with (temp / "stdout").open("wb") as out, (temp / "stderr").open("wb") as err:
            try:
                child = subprocess.run(
                    [sys.executable, str(runner), "--native-suite", compiler,
                     str(root / MANIFEST_PATH), str(observer), str(oracle), str(receipt)],
                    stdin=subprocess.DEVNULL, stdout=out, stderr=err,
                    env={"PATH": os.environ.get("PATH", "/usr/bin:/bin"),
                         "LC_ALL": "C", "PYTHONDONTWRITEBYTECODE": "1"},
                    cwd=root, shell=False, timeout=CHILD_TIMEOUT, check=False,
                )
            except subprocess.TimeoutExpired:
                stop("child timeout")
        if (temp / "stdout").stat().st_size > MAX_CHILD_OUTPUT or (temp / "stderr").stat().st_size > MAX_CHILD_OUTPUT:
            stop("oversized child output")
        if child.returncode != 0:
            stop("child failed (exit/status)" + str(child.returncode))
        if not receipt.is_file() or receipt.is_symlink() or receipt.stat().st_size > MAX_RECEIPT:
            stop("missing, symlink or oversized native receipt (exit0 is insufficient)")
        response = validate_receipt(receipt.read_bytes(), cohort, ap, rows, manifest_digest)
        # Detect changed descriptor/aux/source after child before declaring schema complete.
        cohort2, ap2, files2, rows2, digest2 = parse(root, approvals)
        if (cohort2, ap2, files2, rows2, digest2) != (cohort, ap, files, rows, manifest_digest):
            stop("descriptor/source/runner/oracle mutated at runtime")
        return response


def main(argv: list[str]) -> int:
    if not argv:
        stop("mode needed")
    if argv[0] == "--configure" and len(argv) == 3:
        print("T1014 registered synthetic suite count:", configure(Path(argv[1]), Path(argv[2])))
    elif argv[0] == "--run" and len(argv) == 3:
        print(run(Path(argv[1]), argv[2]))
    else:
        stop("unsupported mode/arguments")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv[1:]))
    except (BridgeStop, OSError, ValueError) as exc:
        print(str(exc), file=sys.stderr)
        sys.exit(2)
