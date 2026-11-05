"""Run the frozen C247 cohort against the unchanged fixed candidate."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
here = Path(__file__).resolve().parent
compiler = Path(sys.argv[1])
raw = Path(sys.argv[2])
raw.mkdir(parents=True, exist_ok=False)

freeze = json.loads((here / "source-freeze-v1.json").read_bytes())
for name, expected in freeze["files"].items():
    actual = hashlib.sha256((root / name).read_bytes()).hexdigest()
    assert actual == expected, (name, actual, expected)
oracle = json.loads((here / "oracle-v1.json").read_bytes())

rows = []
first = None
for name, meta in oracle["cases"].items():
    source = here / "sources" / f"{name}.nl"
    commands = (
        ("check", [compiler, "--check", source]),
        ("emit", [compiler, source]),
    )
    for stage, command in commands:
        result = subprocess.run(
            [str(item) for item in command], capture_output=True, timeout=60
        )
        (raw / f"{name}-{stage}.out").write_bytes(result.stdout)
        (raw / f"{name}-{stage}.err").write_bytes(result.stderr)
        expected = meta[f"fixed_candidate_{stage}"]
        row = {
            "case": name,
            "category": meta["category"],
            "boundary": meta["boundary"],
            "stage": stage,
            "status": result.returncode,
            "expected": expected,
        }
        rows.append(row)
        (raw / "observations.json").write_text(
            json.dumps(rows, indent=2) + "\n"
        )
        print(name, stage, result.returncode, flush=True)
        if result.returncode != expected and first is None:
            first = row
            (raw / "FIRST-unexpected.json").write_text(
                json.dumps(row, indent=2) + "\n"
            )
        if result.returncode != 0:
            assert result.stdout == b""

    target = raw / f"{name}-named.c"
    sentinel = b"P1188 named-output sentinel\n"
    target.write_bytes(sentinel)
    before = target.stat()
    named = subprocess.run(
        [str(compiler), "--emit-c17-to", str(target), str(source)],
        capture_output=True,
        timeout=60,
    )
    (raw / f"{name}-named.out").write_bytes(named.stdout)
    (raw / f"{name}-named.err").write_bytes(named.stderr)
    expected = meta["fixed_candidate_emit"]
    row = {
        "case": name,
        "category": meta["category"],
        "boundary": meta["boundary"],
        "stage": "named",
        "status": named.returncode,
        "expected": expected,
    }
    rows.append(row)
    (raw / "observations.json").write_text(json.dumps(rows, indent=2) + "\n")
    print(name, "named", named.returncode, flush=True)
    if named.returncode != expected and first is None:
        first = row
        (raw / "FIRST-unexpected.json").write_text(
            json.dumps(row, indent=2) + "\n"
        )
    if named.returncode:
        after = target.stat()
        assert named.stdout == b"" and target.read_bytes() == sentinel
        assert (after.st_ino, after.st_size, after.st_mtime_ns) == (
            before.st_ino,
            before.st_size,
            before.st_mtime_ns,
        )
    else:
        assert target.read_bytes() == named.stdout

if first is not None:
    raise AssertionError(first)
print("P1188 fixed-candidate Gate0 cohort matches the frozen oracle")
