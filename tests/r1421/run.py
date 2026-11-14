"""R1421 immutable-FIRST SHA gate and independent public CHECK observer runner.

This only runs under the existing FULL/PARTIAL CTest cohorts, never R-FAST.
"""
import hashlib
from pathlib import Path
import subprocess
import sys

FROZEN = {
    "positive": (
        "author_route",
        1829,
        "8dfee230ee73d0d751755d65016799e340e5d91aa20e8fee357fe361473eddba",
    ),
    "wrong": (
        "wrong_domain",
        1443,
        "dccd19e56f371970e4be9a51e391b4b11c778d0aa798b62c9d5aa48e8fb04f0f",
    ),
    "ended": (
        "ended_original",
        1330,
        "33633b2261204d8492946667213280e598ea61015ce5dee458c800a0c9b6bf70",
    ),
}


def main() -> int:
    if len(sys.argv) != 4 or sys.argv[1] not in FROZEN:
        raise SystemExit("R1421 unexpected test arguments")
    case, executable, filename = sys.argv[1:]
    fn, required_bytes, required_sha = FROZEN[case]
    data = Path(filename).read_bytes()
    observed_sha = hashlib.sha256(data).hexdigest()
    if len(data) != required_bytes or observed_sha != required_sha:
        raise SystemExit(
            f"R1421 STOP FROZEN_SOURCE_DRIFT case={case} bytes={len(data)} "
            f"sha256={observed_sha}"
        )
    print(
        f"R1421 FIRST_PRESERVED case={case} fn={fn} "
        f"bytes={required_bytes} sha256={required_sha}",
        flush=True,
    )
    try:
        p = subprocess.run(
            [executable, case, filename, fn],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=90,
            check=False,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise SystemExit(f"R1421 OBSERVER_STOP {exc}") from exc
    print(p.stdout, end="", flush=True)
    if p.stderr:
        print(p.stderr, file=sys.stderr, end="", flush=True)
    if p.returncode != 0:
        raise SystemExit(
            f"R1421 STOP case={case} probe_exit={p.returncode} "
            "NO_NATIVE_RETRY"
        )
    if not any(
        line.startswith("R1421 result=NOT_REACHED")
        or line.startswith("R1421 positive_accepted=1")
        for line in p.stdout.splitlines()
    ):
        raise SystemExit("R1421 STOP MISSING_STAGE_CLASSIFICATION")
    if case != "positive" and "R1421 positive_accepted=1" in p.stdout:
        raise SystemExit("R1421 STOP NEGATIVE_ACCEPTED")
    print(
        f"R1421 C902 CHECK_ONLY_COMPLETE case={case} "
        "NO_NEW_NATIVE_OOM_LEAN no_author_truth_inference=1",
        flush=True,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
