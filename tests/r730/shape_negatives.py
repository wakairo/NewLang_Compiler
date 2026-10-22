"""New NON-BLIND R730 pretest-frozen unsupported scoped unit-statement sources."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

def run(*cmd):
    return subprocess.run([str(x) for x in cmd], capture_output=True)

def check(condition, message, observed=None):
    if not condition:
        msg = message
        if observed is not None:
            msg += f"\\nreturncode={observed.returncode} stdout={observed.stdout!r} stderr={observed.stderr!r}"
        raise AssertionError(msg)

def main():
    compiler = sys.argv[1]
    directory = Path(sys.argv[2])
    cases = json.loads((directory / "shapes.json").read_text())["cases"]
    with tempfile.TemporaryDirectory(prefix="r730_neg_") as temp:
        root = Path(temp)
        for case in cases:
            source = directory / case["file"]
            raw = source.read_bytes()
            check(len(raw) == case["bytes"], "pretest byte count differs")
            check(hashlib.sha256(raw).hexdigest() == case["sha256"], "pretest SHA differs")
            check(raw.endswith(b"\\n") and not raw.startswith(b"\\xef\\xbb\\xbf"), "encoding differs")
            semantic = run(compiler, "--check", source)
            check(semantic.returncode in (0, 3) and not semantic.stdout,
                  "unexpected source status or partial check output", semantic)
            emit = run(compiler, source)
            check(emit.returncode in (3, 4) and not emit.stdout,
                  "unsupported source unexpectedly emitted C17", emit)
            named = root / "already-exists.c"
            named.write_bytes(b"R730-NAMED-OUTPUT-SENTINEL")
            guard = run(compiler, "--emit-c17-to", named, source)
            check(guard.returncode in (3, 4) and named.read_bytes() == b"R730-NAMED-OUTPUT-SENTINEL",
                  "unsupported source overwrote named C17", guard)
            print("R730-NONBLIND-NEG", case["file"],
                  "CHECK", semantic.returncode, "EMIT", emit.returncode,
                  "NAMED", guard.returncode, "NO_PARTIAL", True)

if __name__ == "__main__":
    main()
