"""Finite integration only: unchanged public +/- proofs and genuine Copy bool.
No new proof issuer. Supplementary sources frozen before execution in P605.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    compiler, probe, cc, directory, sanitizer = sys.argv[1:]
    root = Path(directory)
    helper_path = root.parent / 'p603' / 'integration_test.py'
    spec = importlib.util.spec_from_file_location('p603_integration_helper', helper_path)
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    for name, digest in json.loads((root / 'frozen.json').read_text())['adopted_proof_files'].items():
        assert hashlib.sha256((root.parent.parent / name).read_bytes()).hexdigest() == digest
    with tempfile.TemporaryDirectory(prefix='p605-integration-') as tmp:
        for name, case in json.loads((root / 'composition-frozen.json').read_text())['cases'].items():
            source = root / name
            assert hashlib.sha256(source.read_bytes()).hexdigest() == case['sha256']
            check = helper.run([compiler, '--check', str(source)])
            emitted = helper.run([compiler, str(source)])
            helper.require(check.returncode == emitted.returncode == 0 and not check.stdout and
                           not check.stderr and not emitted.stderr, 'integrated source/check/C17 failed')
            observed = helper.run([probe, str(source)])
            helper.require(observed.returncode == 0 and not observed.stderr, f'public +/- proof or bool observation failed: {observed}')
            lines = observed.stdout.splitlines()
            evidence = json.loads(lines[0])
            helper.require(evidence['add'] == case['add'] and evidence['sub'] == case['subtract'] and
                           evidence['bool'] == case['bool'] and evidence['authority'] == 0,
                           'wrong original arithmetic or bool values')
            bindings = {}
            for line in lines[1:]:
                _, symbol, kind, value = line.split()
                bindings[int(symbol)] = (kind, int(value))
            watched, _ = helper.observe(emitted.stdout, evidence, bindings)
            results = {k: helper.execute(Path(tmp), cc, name+k, text, 0, sanitizer)
                       for k, text in [('plain', emitted.stdout), ('observed', watched)]}
            helper.require(' + ' in emitted.stdout and ' - ' in emitted.stdout and ' < ' in emitted.stdout,
                           'original guest arithmetic/comparison missing')
            print(json.dumps(dict(source=name, public=evidence, c_sha256=hashlib.sha256(emitted.stdout.encode()).hexdigest(),
                                  native=results, sanitizer=sanitizer)))
    for name, case in json.loads((root / 'mixed-frozen.json').read_text())['cases'].items():
        source = root / name
        assert hashlib.sha256(source.read_bytes()).hexdigest() == case['sha256']
        for args in (['--check'], []):
            result = helper.run([compiler, *args, str(source)])
            helper.require(result.returncode == 3 and not result.stdout and
                           ('error(unsupported)' in result.stderr or 'error(syntax)' in result.stderr),
                           f'mixed operator admitted: {result}')
        print(json.dumps(dict(source=name, exit=3, diagnostic=result.stderr.strip(), c_bytes=0)))
    print('P605 adopted +/- public revalidation and < Copy composition FINITE; no new public proof or Product GO')


if __name__ == '__main__':
    main()
