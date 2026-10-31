from pathlib import Path
import os, subprocess, sys, tempfile
probe, source, expected_text = sys.argv[1:]
expected = int(expected_text)
with tempfile.TemporaryDirectory(prefix='p974-named-') as directory:
    output = Path(directory) / 'previous.c'
    sentinel = b'P974 original output bytes\n'
    output.write_bytes(sentinel)
    before = output.stat()
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    failed = subprocess.run([probe, 'emit', source, str(output), 'invalid'],
                            capture_output=True, env=env)
    assert failed.returncode == 4, (failed.returncode, failed.stderr)
    assert b'V1-BACKEND-UNSUPPORTED' in failed.stderr and not failed.stdout
    assert output.read_bytes() == sentinel
    assert (output.stat().st_dev, output.stat().st_ino) == (before.st_dev, before.st_ino)
    assert not list(Path(directory).glob('*.tmp*'))
    healthy = subprocess.run([probe, 'emit', source, str(output), 'healthy'],
                             capture_output=True, env=env)
    assert healthy.returncode == expected and not healthy.stdout, healthy.stderr
    if expected == 0:
        assert not healthy.stderr
        assert output.read_bytes() != sentinel and output.stat().st_size > 0
    else:
        assert expected == 4 and b'V1-BACKEND-UNSUPPORTED' in healthy.stderr
        assert output.read_bytes() == sentinel
        assert (output.stat().st_dev, output.stat().st_ino) == (before.st_dev, before.st_ino)
    print('P974 real CLI current NULL functions: CLI4, no partial C, named bytes/dev/ino unchanged, restored-world cleanup, healthy baseline CLI' + expected_text)
