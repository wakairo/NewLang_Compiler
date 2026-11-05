"""Run the unchanged C246a oracle after authenticated forward Production changes."""
from pathlib import Path
import hashlib
import json
import runpy
import subprocess
import sys
import zipfile

root = Path(__file__).resolve().parents[2]
old = root / 'tests/p1184/c246a'
sha = lambda data: hashlib.sha256(data).hexdigest()
assert sha((old / 'freeze-v1.json').read_bytes()) == (
    '33a3febd5760810ca25f8346ed70b0fe5975e2f4c22b8d64be5e3bfb3371bb34')
freeze_bytes = (old / 'freeze-v2.json').read_bytes()
assert sha(freeze_bytes) == 'd2c1cf6fcdf011c7d82f48f86e97813591905cf1c8b017c82274f8e8e234a058'
frozen = json.loads(freeze_bytes)
packet = root / 'tests/p1188/evidence/c246a-inheritance-FIRST.zip'
assert sha(packet.read_bytes()) == '66c3185fb6fc4dbc8c9c9bb46195ef684a652ae4b3c85b8fe3bc9d7a3e965073'
with zipfile.ZipFile(packet) as archive:
    for name, expected in frozen['files'].items():
        data = (archive.read('fixed-old-current-loop.h')
                if name == 'src/current_loop_check_internal.h'
                else (root / name).read_bytes())
        assert sha(data) == expected, ('fixed predecessor', name)
# The old header pin is preserved above. The live successor must satisfy the
# complete externally anchored current history, including every Production
# file digest; an arbitrary current header is never accepted.
runpy.run_path(str(root / 'tests/p982/freeze_test.py'))
oracle = json.loads((old / 'oracle-v1.json').read_bytes())
assert sys.argv[2] in oracle['sources']
result = subprocess.run([sys.argv[1], str(root / sys.argv[2])], capture_output=True, text=True)
sys.stdout.write(result.stdout)
sys.stderr.write(result.stderr)
assert result.returncode == 0, result.returncode
assert result.stderr == ''
assert result.stdout.count('reject/out/sink/retry=PASS') == oracle['trials_per_source']
assert 'C246a trials=120 B-survive/cleanup/fresh-CHECK-retry=PASS\n' in result.stdout
