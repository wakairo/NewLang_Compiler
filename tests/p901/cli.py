"""P-owned calibrated source custody/CLI regressions; no R harness reuse."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile

compiler, root = sys.argv[1], Path(sys.argv[2])
freeze = json.loads((root / "freeze.json").read_text())
for name, digest in freeze["sha256"].items():
    source = root / "sources" / name
    assert hashlib.sha256(source.read_bytes()).hexdigest() == digest
    checked = subprocess.run([compiler, "--check", str(source)], capture_output=True)
    if name == "no_call.nl":
        assert checked.returncode == 0 and not checked.stdout and not checked.stderr
        continue
    assert checked.returncode == 3 and b"P6-JOIN-PRECISION" in checked.stderr
    emitted = subprocess.run([compiler, str(source)], capture_output=True)
    assert emitted.returncode == 3 and not emitted.stdout
    with tempfile.TemporaryDirectory() as temp:
        output = Path(temp) / "out.c"
        output.write_bytes(b"previous valid C17\n")
        result = subprocess.run([compiler, "--emit-c17-to", str(output), str(source)], capture_output=True)
        assert result.returncode == 3 and not result.stdout
        assert output.read_bytes() == b"previous valid C17\n"
        assert list(Path(temp).iterdir()) == [output]
    print(name, "CHECK3 / zero C17 / output preserved")
