"""Only the two preregistered invalid source variants; ordinary CLI, zero C."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

here = Path(__file__).resolve().parent
frozen = json.loads((here / 'freeze.json').read_text())
for name, expected in frozen['negative_expected'].items():
    source = here / name
    assert hashlib.sha256(source.read_bytes()).hexdigest() == frozen['sha256'][name]
    results = []
    for flags in [['--check'], []]:
        r = subprocess.run([sys.argv[1], *flags, str(source)], capture_output=True, text=True, timeout=60)
        assert r.returncode == 3 and not r.stdout and f"[{expected['diagnostic']}]" in r.stderr, r
        results.append({'exit': r.returncode, 'stdout_bytes': len(r.stdout), 'stderr': r.stderr.splitlines()})
    print(json.dumps({'source': name, 'results': results}))
