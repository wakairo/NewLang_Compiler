"""Source-level rejection and named-output atomicity, without guest execution."""
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile

compiler = sys.argv[1]
root = Path(__file__).resolve().parent
parent = Path(sys.argv[2])
parent.mkdir(parents=True, exist_ok=True)
work = Path(tempfile.mkdtemp(prefix="run-", dir=parent))
cases = [("foreign-domain", 3), ("ending-loan-ref", 3),
         ("unreleased-original", 3), ("wrong-continue", 3), ("control", 4)]
for name, expected in cases:
    source = root / (name + ".nl")
    if name == "control":
        checked = subprocess.run([compiler, "--check", str(source)], capture_output=True)
        assert checked.returncode == 0, checked.stderr.decode()
    target = work / (name + ".c")
    target.write_bytes(b"P1312 existing output must survive rejection\n")
    before = (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns)
    result = subprocess.run([compiler, "--emit-c17-to", str(target), str(source)],
                            capture_output=True)
    (work / (name + ".stdout")).write_bytes(result.stdout)
    (work / (name + ".stderr")).write_bytes(result.stderr)
    assert result.returncode == expected and not result.stdout, (name, result)
    assert result.stderr and before == (target.read_bytes(), target.stat().st_ino,
                                       target.stat().st_mtime_ns), name
    assert not list(work.glob(name + ".c.tmp.*")), name
    print(name, "exit", result.returncode, "no-clobber",
          hashlib.sha256(target.read_bytes()).hexdigest())
