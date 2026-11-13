"""Exact selected source and fail-closed rejection through the ordinary CLI."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

compiler, directory = map(Path, sys.argv[1:3])
oracle = json.loads((directory / "oracle-current.json").read_text())
results = []
with tempfile.TemporaryDirectory(prefix="p1393-cli-") as temp:
    output = Path(temp) / "preserved.c"
    for name, row in oracle["files"].items():
        source = directory / "sources" / name
        assert hashlib.sha256(source.read_bytes()).hexdigest() == row["SHA256"]
        check = subprocess.run([str(compiler), "--check", str(source)],
                               capture_output=True)
        positive = name.startswith("P1-")
        assert check.returncode == (0 if positive else 3), (name, check.stderr)
        assert not check.stdout
        if not positive:
            code = (b"P3-STALE-POINTER" if name.startswith("N1-")
                    else b"ALLOCATED-DOMAIN-MISMATCH" if name.startswith("N2-")
                    else b"P5-SCOPE-OBLIGATION")
            assert code in check.stderr, (name, check.stderr)
            output.write_bytes(b"original named output\n")
            before = set(Path(temp).iterdir())
            denied = subprocess.run([str(compiler), "--emit-c17-to", str(output),
                                     str(source)], capture_output=True)
            assert denied.returncode == 3 and not denied.stdout
            assert output.read_bytes() == b"original named output\n"
            assert set(Path(temp).iterdir()) == before
        else:
            emitted = subprocess.run([str(compiler), str(source)],
                                     capture_output=True)
            assert emitted.returncode == 0 and not emitted.stderr, (name, emitted.stderr)
            assert emitted.stdout.startswith(b"/* Draft17.30 bounded five-root C17;")
        results.append({"source": name, "sha256": row["SHA256"],
                        "CHECK": check.returncode,
                        "classification": "accepted" if positive else "rejected",
                        "stderr": check.stderr.decode(),
                        "negative_named_output_unchanged": not positive})
print(json.dumps(results, indent=2))

repo=directory.resolve().parents[1]
for name,expected in {
    "I-canonical-six.nl":"04a5f00e7692173226dbc8af0fd5d3f2ef8e142017d9873f66510cc215d96cfe",
    "U-proposed-nine.nl":"4b5f8c99817c1a4ca33e6bb5d2d928ed6283f7e57f8f8fadac584dbc420ebf62",
}.items():
    r=subprocess.run([str(compiler),str(repo/"tests/p1387/sources"/name)],capture_output=True)
    assert r.returncode==0 and not r.stderr
    assert hashlib.sha256(r.stdout).hexdigest()==expected,name
    print("old6/exact9 emitted C byte identity PASS",name,expected)
