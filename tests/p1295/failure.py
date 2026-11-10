"""Bounded real CHECK/emitter OOM and source/metadata refusal, never weak retry."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

mode, probe, cli, destination = sys.argv[1:]
here = Path(__file__).resolve().parent
oracle = json.loads((here / "oracle.json").read_text())
parent = Path(destination)
parent.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix=mode + "-", dir=parent))
rows = []


def run(args, label, expected=0):
    args = list(map(str, args))
    p = subprocess.run(args, capture_output=True)
    (out / (label + ".stdout")).write_bytes(p.stdout)
    (out / (label + ".stderr")).write_bytes(p.stderr)
    rows.append({"label": label, "argv": args, "status": p.returncode})
    (out / "commands.json").write_text(json.dumps(rows, indent=2) + "\n")
    assert p.returncode == expected, (label, p.returncode, p.stderr, out)
    return p


def unchanged(before, target, sentinel):
    after = target.stat()
    assert target.read_bytes() == sentinel
    assert (before.st_ino, before.st_size, before.st_mtime_ns) == \
        (after.st_ino, after.st_size, after.st_mtime_ns)
    assert not list(out.glob("*.tmp*"))


for name, receipt in oracle["files"].items():
    assert hashlib.sha256((here / name).read_bytes()).hexdigest() == \
        receipt["sha256"]
if mode == "negative":
    for name, diagnostic in [("N-undeclared-post-field.nl", "FIELD-UNKNOWN-FIELD"),
                             ("N-unclosed-front.nl", "P5-SCOPE-OBLIGATION")]:
        source = here / name
        checked = run([cli, "--check", source], name + "-check", 3)
        assert diagnostic.encode() in checked.stderr
        assert not run([cli, source], name + "-C17", 3).stdout
        target = out / (name + ".preserved.c")
        sentinel = b"P1295 invalid source must not overwrite this file\n"
        target.write_bytes(sentinel)
        before = target.stat()
        run([cli, "--emit-c17-to", target, source], name + "-named", 3)
        unchanged(before, target, sentinel)
    print("P1295 negative zero-C/named no-clobber PASS", out)
    sys.exit(0)

source = here / "P-surviving-state.nl"
phases = ["check"] if mode == "check" else ["emit"] if mode == "emit" \
    else ["check", "emit"]
for phase in phases:
    target = out / "preserved.c"
    if mode in ["check", "emit"]:
        census = run([probe, source, "census"] if phase == "check" else
                     [probe, source, phase, "census"], phase + "-census")
        count = int(re.search(rb"CENSUS phase=\w+ total=(\d+)",
                              census.stdout)[1])
        ordinals = [int(x) for x in re.findall(rb"SELECT (\d+)", census.stdout)]
        assert 0 < count <= (4096 if phase == "check" else 512)
        assert 0 < len(ordinals) <= (128 if phase == "check" else 512)
    else:
        assert mode == "named"
        baseline = run([cli, source], phase + "-baseline").stdout
        census = run([probe, source, target, phase, 0], phase + "-census")
        assert target.read_bytes() == baseline
        count = int(re.search(rb"allocations=(\d+)", census.stdout)[1])
        assert 0 < count <= (4096 if phase == "check" else 512)
        ordinals = (sorted(set(range(1, min(count, 8) + 1)) |
                           set(range(max(1, count - 7), count + 1)) |
                           {max(1, count // 2)}) if phase == "check"
                    else list(range(1, count + 1)))
    plan = {"phase": phase, "census": count, "selected": ordinals,
            "probe_sha256": hashlib.sha256(Path(probe).read_bytes()).hexdigest(),
            "injections_executed": 0}
    receipt = out / (phase + "-plan.json")
    receipt.write_text(json.dumps(plan, indent=2) + "\n")
    receipt.chmod(0o444)
    if mode in ["check", "emit"]:
        result = run([probe, source, "check" if mode == "check" else "emit"],
                     phase + "-faults")
        assert int(re.search(rb"selected=(\d+)", result.stdout)[1]) == len(ordinals)
    else:
        sentinel = b"P1295 preserve named output through real allocator failure\n"
        for ordinal in ordinals:
            target.write_bytes(sentinel)
            before = target.stat()
            result = run([probe, source, target, phase, ordinal],
                         phase + "-" + str(ordinal))
            assert b"fired=1" in result.stdout
            assert (b"OUT-OF-MEMORY" if phase == "check" else
                    b"error(cli)[NODE-C-RESOURCE]") in result.stderr
            unchanged(before, target, sentinel)
            run([cli, "--emit-c17-to", target, source],
                phase + "-" + str(ordinal) + "-retry")
            assert target.read_bytes() == baseline
    print("P1295", mode, phase, "census", count, "reached", len(ordinals),
          "rollback/no-clobber/retry PASS", out)
