"""C268 source/CLI/strict-C gate; never executes a generated guest."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile

here = Path(__file__).resolve().parent
mode, compiler, gcc, clang, destination = sys.argv[1:]
raw = Path(destination)
raw.mkdir(parents=True, exist_ok=False)
sha = lambda data: hashlib.sha256(data).hexdigest()
frozen = json.loads((here / 'source-freeze-v1.json').read_bytes())
for name, expected in frozen['files'].items():
    assert sha((here / name).read_bytes()) == expected, name
if mode == 'baseline':
    assert sha(Path(compiler).read_bytes()) == frozen['binary_sha256']
oracle = json.loads((here / 'oracle-v1.json').read_bytes())
rows = []


def run(command, label):
    command = list(map(str, command))
    result = subprocess.run(command, capture_output=True, timeout=120)
    (raw / (label + '.out')).write_bytes(result.stdout)
    (raw / (label + '.err')).write_bytes(result.stderr)
    rows.append(dict(command=command, label=label, status=result.returncode,
                     stdout_sha256=sha(result.stdout), stderr_sha256=sha(result.stderr)))
    (raw / 'observations.json').write_text(json.dumps(rows, indent=2) + '\n')
    return result


for name, meta in oracle['cases'].items():
    source = here / 'sources' / (name + '.nl')
    check = run([compiler, '--check', source], name + '-check')
    emitted = run([compiler, source], name + '-c17')
    assert check.returncode == meta[mode + '_check'], rows[-2]
    assert emitted.returncode == meta[mode + '_emit'], rows[-1]
    if emitted.returncode:
        assert not emitted.stdout
        with tempfile.TemporaryDirectory(prefix='p1208-c268-') as directory:
            target = Path(directory) / 'out.c'
            target.write_bytes(b'C268 original named output\n')
            before = (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns)
            result = run([compiler, '--emit-c17-to', target, source], name + '-named')
            assert result.returncode == emitted.returncode and not result.stdout
            assert (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns) == before
            assert list(Path(directory).iterdir()) == [target]
        print(name, 'pre-output rejection/no-clobber PASS', flush=True)
        continue
    code = raw / (name + '.c')
    code.write_bytes(emitted.stdout)
    for cc, label in [(gcc, 'gcc'), (clang, 'clang')]:
        result = run([cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                      '-fsyntax-only', code], name + '-' + label + '-strict')
        expected = meta.get('baseline_strict_compile', 0) if mode == 'baseline' else 0
        assert (result.returncode == 0) == (expected == 0), rows[-1]
    print(name, 'actual CHECK/C17/strict compile recorded; native0', flush=True)
print('C268', mode, 'gate complete; no generated C was executed')
