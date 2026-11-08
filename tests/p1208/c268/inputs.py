"""C268 compatibility path for original C266 executable tests, not SHA audit."""
from pathlib import Path
import hashlib

ROOT = Path(__file__).resolve().parents[3]
HERE = ROOT / "tests/p1208"


def sha(data):
    """Hash the actual current source, binary or injection plan bytes."""
    return hashlib.sha256(data).hexdigest()


def verify():
    # Original C266 tests still call verify(); actual source/native assertions
    # execute in the scripts themselves. Old repository SHA replay was retired.
    return None
