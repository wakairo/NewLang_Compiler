"""Run immutable existing producer assertions with the current input selector."""
from pathlib import Path
import runpy
import sys

sys.dont_write_bytecode = True
import inputs

inputs.verify()
sys.modules['inputs'] = inputs
script = Path(sys.argv[1]).resolve()
assert script.parent == inputs.HERE / 'c266'
sys.argv = [str(script), *sys.argv[2:]]
runpy.run_path(str(script), run_name='__main__')
