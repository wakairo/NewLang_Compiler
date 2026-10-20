"""One frozen P program; this records a backend boundary, never native PASS."""
import hashlib
import json
import pathlib
import subprocess
import sys

cli, public_probe, locator, source_dir = map(pathlib.Path, sys.argv[1:5])
freeze = json.loads((source_dir / 'freeze.json').read_text())
expected = {'handoff.nl': (0, 4), 'wrong_quantity.nl': (3, 3),
            'extra_arm_local.nl': (0, 4), 'one_if_control.nl': (0, 4)}
for name, codes in expected.items():
    source = source_dir / name
    assert hashlib.sha256(source.read_bytes()).hexdigest() == freeze['source_sha256'][name]
    row = {'source': name, 'sha256': freeze['source_sha256'][name]}
    for stage, flags, code in zip(('check', 'c17'), (['--check'], []), codes):
        command = [str(cli), *flags, str(source)]
        result = subprocess.run(command, capture_output=True, timeout=60)
        assert result.returncode == code, (command, result.returncode, result.stderr)
        assert result.stdout == b'', (name, stage, result.stdout)
        diagnostic = result.stderr.decode()
        if code == 3:
            assert 'USIZE-LESS-THAN-TYPE' in diagnostic
            assert ':6:26-6:27:' in diagnostic
        elif code == 4:
            assert 'V1-BACKEND-UNSUPPORTED' in diagnostic
        else:
            assert not diagnostic
        row[stage] = {'exit': result.returncode, 'stdout_bytes': len(result.stdout),
                      'stderr': diagnostic}
    print(json.dumps(row))

for name in ('handoff.nl', 'one_if_control.nl'):
    for kind, tool in [('public', public_probe), ('private_locator', locator)]:
        result = subprocess.run([str(tool), str(source_dir / name)], capture_output=True,
                                timeout=60)
        assert result.returncode == 0, (kind, name, result.stderr)
        assert not result.stderr, result.stderr
        observations = [json.loads(line) for line in result.stdout.splitlines()]
        assert observations
        if kind == 'public':
            owned = bytes.fromhex(observations[0]['hex'])
            original = (source_dir / name).read_bytes()
            offset = original.find(owned)
            assert offset >= 0, 'owned body must match actual original source bytes'
            observations[0] = {'kind': 'owned_source', 'sha256': hashlib.sha256(owned).hexdigest(),
                               'bytes': len(owned), 'original_source_offset': offset,
                               'source_hex': owned.hex()}
        print(json.dumps({'source': name, 'kind': kind, 'exit': 0,
                          'observations': observations}))

# Recheck the same genuine source after all rejection subprocesses. This is
# CLI no-partial-output/repeatability evidence, not an in-process OOM proof.
result = subprocess.run([str(cli), '--check', str(source_dir / 'handoff.nl')],
                        capture_output=True, timeout=60)
assert result.returncode == 0 and result.stdout == b'' and result.stderr == b''
print(json.dumps({'result': 'P647_RECORDED_FIRST_BACKEND_BOUNDARY',
                  'ordinary_check': 0, 'ordinary_backend': 4, 'guest_c17_bytes': 0,
                  'guest_native': 'NOT_REACHED_NO_C17', 'post_failure_recheck': 0}))
