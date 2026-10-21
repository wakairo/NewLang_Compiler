"""P671 frozen semantic negatives: actual CLI + unchanged issuer rollback harness."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

compiler, gate = sys.argv[1:]
here = Path(__file__).resolve().parent
old = here.parent / 'p670'
prior = json.loads((old / 'freeze.json').read_text())['cases']
current = json.loads((here / 'freeze.json').read_text())['new_negatives']
for folder, cases in [(old, {k: v for k, v in prior.items() if 'trial_diagnostic' in v}),
                      (here, current)]:
    for name, oracle in cases.items():
        source = folder / name
        assert hashlib.sha256(source.read_bytes()).hexdigest() == oracle['sha256']
        code = oracle.get('trial_diagnostic', oracle.get('expected_diagnostic'))
        results = []
        for flags in [['--check'], []]:
            result = subprocess.run([compiler, *flags, str(source)], capture_output=True, text=True, timeout=60)
            assert result.returncode == 3 and not result.stdout and f'[{code}]' in result.stderr, result
            results.append({'exit': result.returncode, 'stderr': result.stderr.splitlines()})
        rollback = subprocess.run([gate, 'reject', str(source)], capture_output=True, text=True, timeout=60)
        assert rollback.returncode == 0, rollback
        print(json.dumps({'source': name, 'cli': results, 'rollback': rollback.stdout.splitlines()}))
