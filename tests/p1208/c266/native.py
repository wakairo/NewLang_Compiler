"""C266 additional guest budget: two mixed cases + one adopted literal case."""
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
here = Path(__file__).resolve().parent
from inputs import verify

verify()
compiler, mixed_probe, literal_probe, cc, sanitizer, destination = sys.argv[1:]
raw = Path(destination)
raw.mkdir(parents=True, exist_ok=False)
for script, probe, name in [('mixed_native.py', mixed_probe, 'mixed'),
                            ('literal_native.py', literal_probe, 'literal')]:
    subprocess.run([sys.executable, str(here / script), compiler, probe, cc,
                    sanitizer, str(raw / name)], check=True)
print('C266 targeted guest native PASS12/config; global GCC+Clang cap24')
