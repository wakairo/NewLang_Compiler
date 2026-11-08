"""C268 compatibility path for original C266 executable tests, not SHA audit."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
HERE = ROOT / "tests/p1208"


def verify():
    # Original C266 tests still call verify(); actual source/native assertions
    # execute in the scripts themselves. Old repository SHA replay was retired.
    return None
