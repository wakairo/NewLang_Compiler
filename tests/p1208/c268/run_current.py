"""Run original C266 source/native behavior without obsolete history-SHA replay."""
from pathlib import Path
import runpy
import sys

sys.dont_write_bytecode = True
import inputs

# Original C266 source/native assertions stay mandatory; their historical-only
# verify() hook uses a compatibility no-op instead of the old hash protocol.
sys.modules["inputs"] = inputs
script = Path(sys.argv[1]).resolve()
assert script.parent == inputs.HERE / "c266"
sys.argv = [str(script), *sys.argv[2:]]
runpy.run_path(str(script), run_name="__main__")
