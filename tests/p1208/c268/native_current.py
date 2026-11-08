"""Same preserved C266 cohort; current selector reaches both child producers."""
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
from inputs import HERE, verify

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / 'tests/support'))
from legacy_audit import enabled
if enabled():
    verify()
compiler, mixed_probe, literal_probe, cc, sanitizer, destination = sys.argv[1:]
raw = Path(destination)
raw.mkdir(parents=True, exist_ok=False)
for script, probe, name in [('mixed_native.py', mixed_probe, 'mixed'),
                            ('literal_native.py', literal_probe, 'literal')]:
    subprocess.run([sys.executable, str(HERE / 'c268/run_current.py'),
                    str(HERE / 'c266' / script), compiler, probe, cc,
                    sanitizer, str(raw / name)], check=True)
print('C268 preserved C266 physical native cohort PASS12/config')
