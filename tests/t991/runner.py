"""T991 synthetic CTest execution marker; NOT an independent R/native oracle."""
import os
import sys

if len(sys.argv) != 2 or sys.argv[1] not in ("accept", "reject"):
    raise SystemExit("T991 invalid synthetic test mode")
if os.environ.get("T991_FORCE_FAIL") == "1":
    raise SystemExit("T991 intentional forced failure")
if sys.argv[1] == "accept":
    assert (11 + 7) == 18
else:
    assert "bad" not in ("good", "ok")
print("T991_REAL_CTEST_EXECUTED mode=" + sys.argv[1])
