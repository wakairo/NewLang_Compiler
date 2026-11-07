"""P-owned ordinary CLI gate. Oracle/source and each FIRST remain immutable."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

compiler, output, cc = sys.argv[1:]
here = Path(__file__).resolve().parent
out = Path(output)
out.mkdir()
manifest = json.loads((here / 'source-freeze-v1.json').read_bytes())
for name, expected in manifest['files'].items():
    assert hashlib.sha256((here / name).read_bytes()).hexdigest() == expected, name
oracle = json.loads((here / 'oracle-v1.json').read_bytes())
summary = []
for name in sorted(oracle['positive']) + sorted(oracle['negative']):
    positive = name in oracle['positive']
    source = here / 'sources' / name
    stem = source.stem
    checked = subprocess.run([compiler, '--check', str(source)], capture_output=True)
    (out / (stem + '.check.stdout')).write_bytes(checked.stdout)
    (out / (stem + '.check.stderr')).write_bytes(checked.stderr)
    target = out / (stem + '.c')
    target.write_bytes(b'P1217 original named C17 output\n')
    before = (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns)
    emitted = subprocess.run([compiler, '--emit-c17-to', str(target), str(source)], capture_output=True)
    (out / (stem + '.emit.stdout')).write_bytes(emitted.stdout)
    (out / (stem + '.emit.stderr')).write_bytes(emitted.stderr)
    after = (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns)
    row = dict(source=name, positive=positive, check=checked.returncode, emit=emitted.returncode,
               check_diagnostic=checked.stderr.decode(), emit_diagnostic=emitted.stderr.decode(),
               no_clobber=after == before if emitted.returncode else None,
               generated_sha256=hashlib.sha256(target.read_bytes()).hexdigest() if not emitted.returncode else None)
    summary.append(row)
    (out / 'FIRST-summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(name, 'CHECK', checked.returncode, 'C17', emitted.returncode, flush=True)
    if positive:
        assert checked.returncode == emitted.returncode == 0, row
        strict = subprocess.run([cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-fsyntax-only', str(target)], capture_output=True)
        (out / (stem + '.syntax.stdout')).write_bytes(strict.stdout)
        (out / (stem + '.syntax.stderr')).write_bytes(strict.stderr)
        assert strict.returncode == 0, strict.stderr.decode()
    else:
        assert checked.returncode != 0 and emitted.returncode != 0 and before == after, row
print('P1217 source CHECK / C17 strict syntax / refusal no-clobber PASS')
