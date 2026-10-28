"""T993 offline fail-closed adversarial checks; no compiler or R cohort consulted."""
from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
VALIDATOR = ROOT / "cmake/independent-r-manifest.py"
RUNNER = ROOT / "cmake/independent-r-cohort-runner.py"
SOURCE_DIR = ROOT / "tests/t993/sources"
BASE = (ROOT / "tests/t993/approved-cohort-manifest.txt").read_text(encoding="ascii")
GOOD = (SOURCE_DIR / "good.nl").read_bytes()
BAD = (SOURCE_DIR / "bad.nl").read_bytes()


def run(case, text, expected, tweak=None):
    with tempfile.TemporaryDirectory(prefix="t993-guards-") as temp:
        root = Path(temp)
        sources = root / "tests/t993/sources"
        sources.mkdir(parents=True)
        (sources / "good.nl").write_bytes(GOOD)
        (sources / "bad.nl").write_bytes(BAD)
        manifest = root / "tests/t993/approved-cohort-manifest.txt"
        manifest.write_text(text, encoding="ascii")
        if tweak is not None:
            tweak(root)
        cp = subprocess.run(
            [sys.executable, str(VALIDATOR), str(root), str(manifest), str(root / "out.cmake")],
            capture_output=True, text=True, timeout=10,
        )
        assert cp.returncode == expected, (case, cp.returncode, cp.stdout, cp.stderr)
        if expected == 0:
            assert (root / "out.cmake").is_file(), case
        else:
            assert not (root / "out.cmake").exists(), case
        print(f"T993_GUARD_{case}=PASS rc={cp.returncode}", flush=True)


run("valid", BASE, 0)
run("duplicate_case", BASE + BASE.splitlines()[1] + "\n", 2)
run("old_test_name", BASE.replace("r993_synthetic_good", "p982_freeze.unit"), 2)
run("traversal", BASE.replace("tests/t993/sources/good.nl", "tests/t993/sources/../bad.nl"), 2)
run("absolute_path", BASE.replace("tests/t993/sources/good.nl", "/etc/passwd"), 2)
run("missing_source", BASE.replace("tests/t993/sources/good.nl", "tests/t993/sources/missing.nl"), 2)
run("sha_mismatch", BASE.replace(hashlib.sha256(GOOD).hexdigest(), "0" * 64), 2)
run("unsupported_mode", BASE.replace("|check_accept|", "|native_unsupported|"), 2)
run("oversized_manifest", BASE + "A" * 5000, 2)
run("cmake_command", BASE + 'message(FATAL_ERROR "untrusted")\n', 2)


def source_symlink(root):
    file = root / "tests/t993/sources/good.nl"
    file.unlink()
    file.symlink_to("/etc/passwd")


def manifest_symlink(root):
    file = root / "tests/t993/approved-cohort-manifest.txt"
    file.unlink()
    file.symlink_to("/etc/passwd")


run("source_symlink", BASE, 2, source_symlink)
run("manifest_symlink", BASE, 2, manifest_symlink)
# Intentionally fail the actual T-owned runner before any subprocess starts.
cp = subprocess.run(
    [sys.executable, str(RUNNER), "/does/not/exist", "check_accept",
     str(SOURCE_DIR / "good.nl"), hashlib.sha256(GOOD).hexdigest()],
    env={**os.environ, "T993_FORCE_FAIL": "1"}, capture_output=True, text=True, timeout=10,
)
assert cp.returncode != 0 and "forced fail" in cp.stderr, (cp.returncode, cp.stderr)
print("T993_GUARD_FORCED_FAILURE=PASS", flush=True)
