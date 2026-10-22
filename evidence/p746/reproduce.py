#!/usr/bin/env python3
"""Reproduce the P746 first admission boundary; never synthesize guest state.
Run against newlangc and phase_probe built from the fixed compiler/evidence.
Writes a new result directory, leaving frozen source/baseline records intact.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--compiler', required=True)
p.add_argument('--probe', required=True)
p.add_argument('--out', required=True, type=Path)
a = p.parse_args()
a.out.mkdir(parents=True, exist_ok=False)
r = Path(__file__).resolve().parent
cases = []
for folder, manifest in [('sources', 'freeze.json'),
                         ('calibration', 'calibration-freeze.json')]:
    frozen = json.loads((r / manifest).read_text())
    for name, expected in frozen['sha256'].items():
        source = r / folder / name
        assert hashlib.sha256(source.read_bytes()).hexdigest() == expected
        cases.append(source)
rows = []
for source in cases:
    for mode, args in [('check', ['--check']), ('c17', [])]:
        proc = subprocess.run([a.compiler, *args, str(source)], capture_output=True)
        assert proc.returncode == 3 and not proc.stdout
        assert b'[AVS-DECL-PROFILE]' in proc.stderr
        rows.append(dict(source=source.name, mode=mode, exit=proc.returncode,
                         stdout_bytes=len(proc.stdout), stderr=proc.stderr.decode()))
    probe = subprocess.run([a.probe, str(source)], capture_output=True)
    assert probe.returncode == 0 and not probe.stderr
    (a.out / (source.stem + '.phase.txt')).write_bytes(probe.stdout)
    destination = a.out / 'existing.c'
    sentinel = b'P746 preserve pre-existing output\n'
    destination.write_bytes(sentinel)
    named = subprocess.run([a.compiler, '--emit-c17-to', str(destination), str(source)],
                           capture_output=True)
    assert named.returncode == 3 and not named.stdout
    assert destination.read_bytes() == sentinel
    assert not list(a.out.glob('existing.c.*'))
(a.out / 'cli.json').write_text(json.dumps(rows, indent=2) + '\n')
print(f'{len(cases)} frozen inputs: lexer EOF; repeatable parser unsupported; '
      'no partial tree; registration/owned/native NOT REACHED; output preserved')
