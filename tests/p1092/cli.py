"""P-owned CHECK admission and named-output boundaries; no LOOP backend."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

compiler, root = map(Path, sys.argv[1:])
data = root / "tests/p1092"
frozen = json.loads((data / "source-freeze-v2.json").read_bytes())["files"]
for name, expected in frozen.items():
    assert hashlib.sha256((root / name).read_bytes()).hexdigest() == expected, name
positive = {0, 1, 2, 3, 4, 6, 15, 16, 17}
for source in sorted((data / "sources").glob("*.nl")):
    number = int(source.name[:2])
    check = subprocess.run([str(compiler), "--check", str(source)], capture_output=True)
    assert check.returncode == (0 if number in positive else 3), (source, check.returncode, check.stderr)
    with tempfile.TemporaryDirectory() as tmp:
        target = Path(tmp) / "out.c"
        sentinel = b"P1092 named output must survive rejection\n"
        target.write_bytes(sentinel)
        emit = subprocess.run([str(compiler), str(source), "-o", str(target)], capture_output=True)
        expected = 0 if number == 0 else 4 if number in positive else 3
        assert emit.returncode == expected, (source, emit.returncode, emit.stderr)
        if expected:
            assert target.read_bytes() == sentinel, source
    print("P1092 CLI", source.name, check.returncode, emit.returncode, "no-clobber")
