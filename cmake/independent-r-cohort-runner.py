"""T993 reviewed CHECK-only CTest runner. No native/ownership proof is inferred."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys


def main():
    if len(sys.argv) != 5:
        raise SystemExit("T993 runner invalid argument count")
    cli, mode, filename, digest = sys.argv[1:]
    if mode not in ("check_accept", "check_reject"):
        raise SystemExit("T993 runner unknown mode")
    if os.environ.get("T993_FORCE_FAIL") == "1":
        raise SystemExit("T993 intentional forced fail")
    content = Path(filename).read_bytes()
    if hashlib.sha256(content).hexdigest() != digest:
        raise SystemExit("T993 source SHA drift since CMake registration")
    try:
        result = subprocess.run([cli, "--check", filename], capture_output=True, timeout=30)
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise SystemExit(f"T993 CHECK execution failure: {exc}") from exc
    if mode == "check_accept":
        if result.returncode != 0 or result.stderr:
            raise SystemExit(f"T993 CHECK expected accept; rc={result.returncode} stderr={result.stderr[:180]!r}")
    else:
        if result.returncode not in (2, 3, 4) or not result.stderr.startswith(b"error("):
            raise SystemExit(f"T993 CHECK expected compiler diagnostic rejection; rc={result.returncode} stderr={result.stderr[:180]!r}")
    print(f"T993_REAL_CTEST_EXECUTED mode={mode} sha256={digest} rc={result.returncode}", flush=True)
    print("T993 CHECK-only, NOT native/C17/OOM/proof validation", flush=True)


if __name__ == "__main__":
    main()
