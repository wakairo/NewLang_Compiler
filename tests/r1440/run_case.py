#!/usr/bin/env python3
"""C958 immutable source check and same-CTest stdout capture (no guest execution)."""
import hashlib
import pathlib
import subprocess
import sys

EXPECTED = {
    "R1": (5101, "f1d1f05995e7a58f465482f81a983242f79d5b800bb7b0b5c8824af7732959cb"),
    "R2": (5113, "c69c64f8306f1a58a7b6e734fb30994525063cffebf97c1af93b0f3eab96b4a9"),
    "R3": (5332, "f13228ffb011f61073f523809d1c22857ba6108aeea887dbd39cebc08446b7cb"),
}


def main() -> int:
    if len(sys.argv) != 4 or sys.argv[2] not in EXPECTED:
        print("R1440_STOP=INVALID_RUNNER_ARGUMENTS", flush=True)
        return 2
    driver, case, filename = sys.argv[1:]
    print(f"R1440_BEGIN case={case}", flush=True)
    raw = pathlib.Path(filename).read_bytes()
    length, sha = EXPECTED[case]
    actual = hashlib.sha256(raw).hexdigest()
    print(f"R1440_SOURCE case={case} bytes={len(raw)} sha256={actual}", flush=True)
    if len(raw) != length or actual != sha or not raw.endswith(b"\n"):
        print("R1440_STOP=FROZEN_ORIGINAL_HASH_MISMATCH", flush=True)
        print(f"R1440_END case={case} status=STOP", flush=True)
        return 3
    completed = subprocess.run(
        [driver, filename, case], text=True, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, check=False,
    )
    print(completed.stdout, end="", flush=True)
    if completed.stderr:
        print("R1440_DRIVER_STDERR_BEGIN", flush=True)
        print(completed.stderr, end="", flush=True)
        print("R1440_DRIVER_STDERR_END", flush=True)
    print(f"R1440_END case={case} driver_exit={completed.returncode}", flush=True)
    return 0 if completed.returncode == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
