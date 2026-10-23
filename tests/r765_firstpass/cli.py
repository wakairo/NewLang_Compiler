#!/usr/bin/env python3
"""Independent R765 frozen source CLI gate; freeze is never rewritten."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

COMPILER, PROBE, MANIFEST, SELECTED = sys.argv[1:]
data = json.loads(Path(MANIFEST).read_bytes())
all_cases = {c["name"]: c for c in data["cases"]}
assert len(all_cases) == len(data["cases"]) == 14
case = all_cases[SELECTED]
source = case["source"].encode("ascii")
oracle = (json.dumps(case["oracle"], ensure_ascii=True, separators=(",", ":")) + "\n").encode("ascii")
assert hashlib.sha256(source).hexdigest() == case["source_sha256"], SELECTED
assert hashlib.sha256(oracle).hexdigest() == case["oracle_sha256"], SELECTED
assert data["authority"]["candidate"] == "492652101a10bb573ffdfc020771334aa1dba375"

def run(*args):
    return subprocess.run(list(map(str, args)), capture_output=True, check=False)

with tempfile.TemporaryDirectory(prefix="r765-firstpass-") as temp:
    p = Path(temp)
    src = p / (SELECTED + ".nl")
    src.write_bytes(source)
    check = run(COMPILER, "--check", src)
    ordinary = run(COMPILER, src)
    positive = case["oracle"]["kind"] == "positive"
    if positive:
        assert check.returncode == 0 and not check.stdout and not check.stderr, (SELECTED, check.returncode, check.stderr)
        assert ordinary.returncode == 4 and not ordinary.stdout and b"V1-BACKEND-UNSUPPORTED" in ordinary.stderr, (SELECTED, ordinary.returncode, ordinary.stderr)
        expect = ([0, 41, 86, 23, 1, 132, 41, 7] if SELECTED == "H_first_mixed"
                  else [1, 64, 17, 208, 0, 6, 64, 91])
        proof = run(PROBE, src, *expect)
        assert proof.returncode == 0, (SELECTED, proof.returncode, proof.stdout, proof.stderr)
        print(proof.stdout.decode(), end="")
    else:
        assert check.returncode == 3 and not check.stdout, (SELECTED, check.returncode, check.stdout, check.stderr)
        assert ordinary.returncode == 3 and not ordinary.stdout, (SELECTED, ordinary.returncode, ordinary.stdout, ordinary.stderr)
        print(SELECTED, "refusal", check.stderr.decode(errors="replace").strip())
    for existing in (True, False):
        target = p / ("existing.c" if existing else "new.c")
        original = b"ORIGINAL OUTPUT MUST SURVIVE\n"
        if existing:
            target.write_bytes(original)
        emitted = run(COMPILER, "--emit-c17-to", target, src)
        assert emitted.returncode == ordinary.returncode and not emitted.stdout, (SELECTED, emitted.returncode, emitted.stdout, emitted.stderr)
        if existing:
            assert target.read_bytes() == original
        else:
            assert not target.exists()
        assert not list(p.glob(target.name + ".*"))
    print(SELECTED, "SOURCE_SHA", case["source_sha256"], "CHECK", check.returncode,
          "EMIT", ordinary.returncode, "NAMED-SENTINEL=PASS")
