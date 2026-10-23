#!/usr/bin/env python3
"""R769 calibrated launcher: sources are strictly frozen R765 bytes."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

compiler, probe, frozen, selected = sys.argv[1:]
manifest = json.loads(Path(frozen).read_text(encoding="utf-8"))
assert manifest["authority"]["candidate"] == "492652101a10bb573ffdfc020771334aa1dba375"
case_map = {c["name"]: c for c in manifest["cases"]}
assert len(case_map) == 14

def verify(item):
    raw = item["source"].encode("ascii")
    assert hashlib.sha256(raw).hexdigest() == item["source_sha256"]
    oracle = (json.dumps(item["oracle"], ensure_ascii=True,
                         separators=(",", ":")) + "\n").encode("ascii")
    assert hashlib.sha256(oracle).hexdigest() == item["oracle_sha256"]
    return raw

def invoke(*command):
    result = subprocess.run([str(v) for v in command], capture_output=True,
                            check=False)
    if result.returncode:
        print("EXEC", *command, "RETURN", result.returncode,
              "STDERR", result.stderr.decode(errors="replace"), flush=True)
    return result

for item in case_map.values():
    verify(item)

if selected == "negative_stages":
    counts = {}
    for item in manifest["cases"]:
        if item["oracle"]["kind"] == "positive":
            continue
        with tempfile.TemporaryDirectory(prefix="r769-refusal-") as t:
            path = Path(t) / "witness.nl"
            path.write_bytes(verify(item))
            checked = invoke(compiler, "--check", path)
            emitted = invoke(compiler, path)
            assert checked.returncode == 3 and emitted.returncode == 3
            assert not checked.stdout and not emitted.stdout
            diagnostic = checked.stderr.decode(errors="replace")
            tokens = re.findall(r"\[([A-Z][A-Z0-9_-]+)\]", diagnostic)
            code = tokens[0] if tokens else "UNCLASSIFIED"
            stage = ("parser" if code.startswith(("P2-", "P12-"))
                     else "semantic" if code.startswith(("P3-", "P4-", "AVS-", "REC-", "P5-"))
                     else "uncertain")
            counts[stage] = counts.get(stage, 0) + 1
            print("R769 REFUSAL", item["name"], "check=3 emit=3",
                  "diagnostic=", code, "stage=", stage,
                  "evidence=", diagnostic.strip().replace("\n", " ")[:240],
                  flush=True)
    assert sum(counts.values()) == 12
    print("R769 REFUSAL STAGE SUMMARY", counts, "unknown-not-inferred-safe")
    sys.exit(0)

modes = {
    "foreign_A": ("foreign", "H_first_mixed"),
    "foreign_B": ("foreign", "T_first_interposed"),
    "oom_A": ("oom", "H_first_mixed"),
    "oom_B": ("oom", "T_first_interposed"),
}
assert selected in modes, selected
mode, name = modes[selected]
with tempfile.TemporaryDirectory(prefix="r769-calibrated-") as temp:
    root = Path(temp)
    first = root / "independent_one.nl"
    first.write_bytes(verify(case_map[name]))
    if mode == "foreign":
        second = root / "independent_two.nl"
        second.write_bytes(verify(case_map[name]))
        # Distinct NLSource objects/semantic registries, even when all TypeIds
        # and nominal layouts coincide; no R765 oracle mutation.
        result = invoke(probe, "foreign", first, second)
    else:
        result = invoke(probe, "oom", first)
    print(result.stdout.decode(errors="replace"), end="", flush=True)
    assert result.returncode == 0, (name, result.returncode, result.stderr)
