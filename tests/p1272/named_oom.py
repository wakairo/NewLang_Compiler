"""Real default CLI named-output OOM, inode/bytes/mtime and clean retry."""
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

probe, cli, destination = sys.argv[1:]
source = Path(__file__).resolve().parent / "sources/P-template-copy.nl"
parent = Path(destination)
parent.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix="run-", dir=parent))
target = out / "preserved.c"
baseline = subprocess.run([cli, str(source)], capture_output=True, check=True).stdout
sentinel = b"P1272 named output must survive actual core OOM\n"
rows = []
for phase in ["check", "emit"]:
    p = subprocess.run([probe, str(source), str(target), phase, "0"], capture_output=True)
    (out / (phase + "-census.stdout")).write_bytes(p.stdout)
    (out / (phase + "-census.stderr")).write_bytes(p.stderr)
    assert p.returncode == 0 and target.read_bytes() == baseline
    count = int(re.search(rb"allocations=(\d+)", p.stdout)[1])
    # Exhaust the actual CHECK/emitter call in the ordinary named CLI. This
    # is a fresh per-executable census, not a reused API ordinal catalogue.
    for ordinal in range(1, count + 1):
        target.write_bytes(sentinel)
        before = target.stat()
        p = subprocess.run([probe, str(source), str(target), phase, str(ordinal)], capture_output=True)
        (out / (phase + "-" + str(ordinal) + ".stdout")).write_bytes(p.stdout)
        (out / (phase + "-" + str(ordinal) + ".stderr")).write_bytes(p.stderr)
        assert p.returncode == 0, (phase, ordinal, p.returncode, p.stderr)
        after = target.stat()
        assert target.read_bytes() == sentinel
        assert (before.st_ino, before.st_size, before.st_mtime_ns) == (after.st_ino, after.st_size, after.st_mtime_ns)
        assert not list(out.glob("*.tmp*"))
        retry = subprocess.run([cli, "--emit-c17-to", str(target), str(source)], capture_output=True)
        assert retry.returncode == 0 and target.read_bytes() == baseline
    rows.append({"phase": phase, "exhausted": count, "clobbered": 0, "retry": count})
(out / "results.json").write_text(json.dumps(rows, indent=2) + "\n")
print("P1272 ordinary named CLI actual OOM/no-clobber/retry PASS", rows, out)
