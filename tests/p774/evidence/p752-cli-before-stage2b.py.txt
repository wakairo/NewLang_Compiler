#!/usr/bin/env python3
"""Stage1 source/check admission and strict no-mixed-output contract."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile

compiler, probe, owner_probe = sys.argv[1:]
r = Path(__file__).resolve().parent
expected = {
    'negative_duplicate_field': 'AVS-DECL-REGISTRATION',
    'negative_duplicate_fn': 'P11-DUPLICATE-FUNCTION',
    'negative_fn_collision': 'P11-DUPLICATE-FUNCTION',
    'negative_h_fn_collision': 'P11-DUPLICATE-FUNCTION',
    'negative_late_definition': 'P8-BODY-RESULT',
    'negative_read_only': 'LOCAL-LOAN-PROFILE',
    'negative_real_read_ref': 'P3-TYPE-MISMATCH',
    'negative_reversed_h': 'AVS-DECL-PROFILE',
    'negative_same_nominal': 'REC-DECL-DUPLICATE',
    'negative_three_link': 'AVS-DECL-PROFILE',
    'negative_two_h': 'AVS-DECL-PROFILE',
    'negative_two_t': 'AVS-DECL-PROFILE',
    'negative_unknown_type': 'P3-UNKNOWN-TYPE',
    'negative_wrong_copy': 'P3-TYPE-MISMATCH',
    'negative_wrong_field_type': 'AVS-DECL-PROFILE',
    'negative_wrong_self': 'REC-SELF-TARGET',
    'negative_foreign_nominal': 'P5-DESTRUCTURE-TYPE',
    'negative_no_main': 'MIXED-UNIT-MAIN-PROFILE',
}

def run(args):
    return subprocess.run(list(map(str,args)),capture_output=True)

cases=[]
for folder, manifest in [('sources','freeze.json'),('additional','additional-freeze.json')]:
    freeze=json.loads((r/manifest).read_text())
    for name,sha in freeze['sha256'].items():
        source=r/folder/name
        assert hashlib.sha256(source.read_bytes()).hexdigest()==sha
        cases.append(source)
original=json.loads((r/'history/p746-original-freeze.json').read_text())
for name in ['calibration_batch.nl','archive_window.nl']:
    source=r/'history'/name
    assert hashlib.sha256(source.read_bytes()).hexdigest()==original['sha256'][name]
    cases.append(source)
with tempfile.TemporaryDirectory(prefix='p752-cli-') as d:
    d=Path(d);target=d/'existing.c';fresh=d/'absent.c'
    for source in cases:
        negative=source.stem.startswith('negative_')
        checked=run([compiler,'--check',source]); emitted=run([compiler,source])
        assert checked.returncode==(3 if negative else 0),checked.stderr.decode()
        assert emitted.returncode==(3 if negative else 4),emitted.stderr.decode()
        assert not checked.stdout and not emitted.stdout
        if negative:
            code=expected[source.stem]
            assert ('['+code+']').encode() in checked.stderr,(source,checked.stderr)
        else:
            assert not checked.stderr and b'V1-BACKEND-UNSUPPORTED' in emitted.stderr
        for destination in [target,fresh]:
            sentinel=b'previous correct C output\n'
            if destination==target:destination.write_bytes(sentinel)
            else:assert not destination.exists()
            named=run([compiler,'--emit-c17-to',destination,source])
            assert named.returncode==emitted.returncode and not named.stdout
            assert destination.read_bytes()==sentinel if destination==target else not destination.exists()
            assert not list(d.glob(destination.name+'.*'))
        if not negative:
            if source.parent.name=='history':
                observed=run([owner_probe,source]);assert observed.returncode==0,observed.stderr.decode()
                assert b'PUBLIC producer=1 recipient=1 legacy_closed=1' in observed.stdout
                print(source.name,'historical owner-only receipts PASS; no joined proof')
            else:
                observed=run([probe,'positive',source]);assert observed.returncode==0,observed.stderr.decode()
                print(source.name,observed.stdout.decode().strip())
        print(source.name,'CHECK',checked.returncode,'C17',emitted.returncode,'zero C / named output preservation PASS')
print('MIXED NATIVE NOT TESTED / NOT AUTHORIZED; no generated guest C17')
