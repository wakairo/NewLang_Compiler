from pathlib import Path
import subprocess
import sys
import tempfile

probe, cli, source = sys.argv[1:]
with tempfile.TemporaryDirectory(prefix='p965-output-') as directory:
    out = Path(directory) / 'previous.c'
    sentinel = b'P965 original named output\n'
    out.write_bytes(sentinel)
    before = out.stat()
    r = subprocess.run([probe, source, str(out), 'attack'], capture_output=True)
    assert r.returncode == 4 and not r.stdout, (r.returncode, r.stderr)
    assert b'V1-BACKEND-UNSUPPORTED' in r.stderr, r.stderr
    assert out.read_bytes() == sentinel
    assert (out.stat().st_dev, out.stat().st_ino) == (before.st_dev, before.st_ino)
    assert sorted(p.name for p in Path(directory).iterdir()) == ['previous.c']
    r = subprocess.run([probe, source, str(out), 'healthy'], capture_output=True)
    raw = subprocess.run([cli, source], capture_output=True)
    assert r.returncode == raw.returncode == 0 and not r.stderr and not raw.stderr
    assert not r.stdout and out.read_bytes() == raw.stdout and raw.stdout
    print('P965 whole T IF arm CLI4/C0; unrestored A teardown/B lives; named bytes/dev/ino preserved; healthy retry exact C')
