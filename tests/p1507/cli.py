"""Named output stays intact on semantic failure; the same path can retry."""
from pathlib import Path
import subprocess
import os
import sys
import tempfile

compiler, tests, base = sys.argv[1:]
Path(base).mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix="named-", dir=base))
target = out / "result.c"
sentinel = b"P1507 original named output\n"
target.write_bytes(sentinel)
for name, expected in [("N-wrong-domain.nl", 3), ("A-direct-zero.nl", 0)]:
    argv = [compiler, "--emit-c17-to", str(target), str(Path(tests) / name)]
    if os.environ.get("P1507_LOCAL_LEDGER"):
        argv = [sys.executable, "/workspace/p1507-evidence/run.py",
                "final-cli-" + name, *argv]
    r = subprocess.run(argv, capture_output=True)
    (out / (name + ".stdout")).write_bytes(r.stdout)
    (out / (name + ".stderr")).write_bytes(r.stderr)
    assert r.returncode == expected, (r.returncode, r.stderr)
    if expected:
        assert target.read_bytes() == sentinel
    else:
        assert b"int main(void)" in target.read_bytes()
    assert sorted(p.name for p in out.iterdir()) == sorted(
        ["result.c", "N-wrong-domain.nl.stdout", "N-wrong-domain.nl.stderr"] +
        ([] if expected else ["A-direct-zero.nl.stdout", "A-direct-zero.nl.stderr"]))
print("P1507 named no-clobber, no temporary leak and positive retry PASS")
