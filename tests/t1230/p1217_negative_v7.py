"""Keep all original ten P1217 v6 negative probes; add T1230 CURRENT negatives."""
from pathlib import Path
import json
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
import p1217_current_v7 as prospective
from versioned_freeze import FreezeError

ROOT = Path(__file__).resolve().parents[2]

if __name__ == "__main__":
    report = prospective.verify_current(ROOT)
    report["original_v6_ten_negative_stdout_sha256"] = prospective.original_v6_negatives(ROOT)
    cases = {
        "original-escrow-tamper": {prospective.ORIGINAL_ROOT:
                                   (ROOT / prospective.ORIGINAL_ROOT).read_bytes() + b"# forged\n"},
        "current-root-tamper": {"CMakeLists.txt":
                               (ROOT / prospective.ORIGINAL_ROOT).read_bytes()},
        "p1217-registration-tamper": {"tests/p1217/CMakeLists.txt":
                                        (ROOT / prospective.ORIGINAL_P1217_CMAKE).read_bytes()},
        "v59-ancestor-tamper": {"tests/p990/current-freeze-v59.json": b"{}\n"},
    }
    for name, bytes_overrides in cases.items():
        try:
            prospective.verify_current(ROOT, bytes_overrides)
        except (FreezeError, KeyError, ValueError, TypeError):
            print("T1230 CURRENT tamper rejected", name, flush=True)
        else:
            raise SystemExit("T1230 STOP: CURRENT tamper accepted: " + name)
    print("T1230 original P1217 v6 ten negatives, original Git root, four CURRENT tamper negatives PASS "
          + json.dumps(report, sort_keys=True))
