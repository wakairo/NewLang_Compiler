"""All frozen mixed and adopted literal source/proof/C17 controls; native0."""
from pathlib import Path
import json
import subprocess
import sys
import zipfile

sys.dont_write_bytecode = True
from inputs import ROOT, HERE, sha, verify

verify()
compiler, mixed_probe, literal_probe, census, destination = sys.argv[1:]
raw = Path(destination)
raw.mkdir(parents=True, exist_ok=False)
rows = []


def run(args, label, status=0):
    args = list(map(str, args))
    result = subprocess.run(args, capture_output=True, timeout=120)
    (raw / (label + '.out')).write_bytes(result.stdout)
    (raw / (label + '.err')).write_bytes(result.stderr)
    row = dict(label=label, args=args, status=result.returncode, expected=status,
               stdout_sha256=sha(result.stdout), stderr_sha256=sha(result.stderr))
    rows.append(row)
    (raw / 'observations.json').write_text(json.dumps(rows, indent=2) + '\n')
    if result.returncode != status:
        (raw / 'FIRST.json').write_text(json.dumps(row, indent=2) + '\n')
        raise AssertionError(row)
    if status:
        assert not result.stdout
    return result.stdout


receipt = json.loads((HERE / 'c266/C17-baseline-v1.json').read_bytes())
archive = ROOT / receipt['archive']
assert sha(archive.read_bytes()) == receipt['sha256']
with zipfile.ZipFile(archive) as z:
    assert set(z.namelist()) == set(receipt['files'])
    for name, expected in receipt['files'].items():
        assert sha(z.read(name)) == expected
    for suite, probe, cases in [
        ('p1208', mixed_probe, json.loads((HERE / 'oracle-v2.json').read_bytes())['profiles']),
        ('p1203', literal_probe, json.loads((ROOT / 'tests/p1203/oracle-v1.json').read_bytes())['cases'])
    ]:
        for name, meta in cases.items():
            source = ROOT / 'tests' / suite / 'sources' / (name + '.nl')
            status = 0 if meta.get('candidate_check', 0) == 0 else 3
            run([compiler, '--check', source], suite + '-' + name + '-check', status)
            code = run([compiler, source], suite + '-' + name + '-c17', status)
            if status:
                continue
            assert code == z.read(suite + '/' + name + '.c'), (suite, name, 'C17 byte preservation')
            proof = run([probe, source], suite + '-' + name + '-owned')
            assert b'public-body=YES private-current=YES canonical-root=YES' in proof
            print(suite, name, 'CHECK/owned/current/C17 bytes PASS', flush=True)
run([census, HERE / 'sources/03-left-final-return.nl'], 'mixed-census')
run([census, ROOT / 'tests/p1203/sources/01-many.nl'], 'literal-census')
for suite in ('p1208', 'p1203'):
    run([sys.executable, ROOT / 'tests' / suite / 'negative.py', compiler], suite + '-no-clobber')
print('C266 all mixed/literal source-owned/current/C17/negative/census PASS; native0')
