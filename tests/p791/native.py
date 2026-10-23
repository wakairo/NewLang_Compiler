"""Ordinary production parser, genuine registered proof, then physical C Array."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile

compiler, one, two, faults, cc, sanitizer, directory = sys.argv[1:]
r = Path(directory)
frozen = json.loads((r / 'freeze.json').read_text())
def run(args):
    return subprocess.run(list(map(str, args)), capture_output=True, text=True)
def require(p):
    assert p.returncode == 0, (p.args, p.returncode, p.stdout, p.stderr)
    return p

def module(name):
    spec = importlib.util.spec_from_file_location(name, r.parent / name / 'native_test.py')
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result

helpers = [module('p534'), module('p541')]
with tempfile.TemporaryDirectory(prefix='p791-') as tmp:
    tmp = Path(tmp)
    target = tmp / 'saved.c'
    for name, digest in frozen['source'].items():
        source = r / 'sources' / name
        assert hashlib.sha256(source.read_bytes()).hexdigest() == digest
        check = run([compiler, '--check', source])
        emitted = run([compiler, source])
        target.write_text('correct C sentinel')
        named = run([compiler, '--emit-c17-to', target, source])
        if name not in ('one.nl', 'two.nl'):
            assert check.returncode == emitted.returncode == named.returncode == 3
            assert not check.stdout and not emitted.stdout and not named.stdout
            assert target.read_text() == 'correct C sentinel'
            expected = frozen['oracle'][source.stem]
            if source.stem in ('arity3', 'arity5', 'wrong_type'):
                assert 'error(semantic)' in check.stderr and expected.split()[-1] in check.stderr
            else:
                assert 'error(unsupported)[ARRAY4-FORM-UNSUPPORTED]' in check.stderr
            print(name, check.stderr.strip())
            continue
        require(check); require(emitted); require(named)
        assert not check.stdout and not check.stderr and not emitted.stderr
        assert not named.stdout and target.read_text() == emitted.stdout
        probe = require(run([one if name == 'one.nl' else two, source]))
        public = json.loads(probe.stdout)
        index = 0 if name == 'one.nl' else 1
        assert public['public_validator']
        helper = helpers[index]
        observed = helper.observed(emitted.stdout, public)
        plain = helper.execute(tmp, cc, source.stem + '-plain', emitted.stdout, True, sanitizer)
        physical = helper.execute(tmp, cc, source.stem + '-observed', observed, True, sanitizer, True)
        print(json.dumps({'source': name, 'sha256': digest, 'check': 0,
                          'public': public, 'C17_sha256': hashlib.sha256(emitted.stdout.encode()).hexdigest(),
                          'plain': plain, 'physical': physical}))
    # Existing real atomic-output driver with exhaustive reached allocation sites,
    # failed fflush/fclose/rename, cleanup and recovery, on this new Array source.
    result = require(run([faults, r / 'sources/two.nl', target]))
    print(result.stdout.strip())
