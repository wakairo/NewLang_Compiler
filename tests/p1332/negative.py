"""Meaning/scope rejection and finite real CLI resource/output failures."""
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

compiler, injector, parent = sys.argv[1:4]
root = Path(__file__).resolve().parent
Path(parent).mkdir(parents=True, exist_ok=True)
work = Path(tempfile.mkdtemp(prefix="negative-", dir=parent))


def run(label, command):
    result = subprocess.run(list(map(str, command)), capture_output=True, timeout=60)
    (work / (label + ".stdout")).write_bytes(result.stdout)
    (work / (label + ".stderr")).write_bytes(result.stderr)
    return result


def snapshot(path):
    s = path.stat()
    return path.read_bytes(), s.st_ino, s.st_mtime_ns


for name, code in [("N-type", b"P3-TYPE-MISMATCH"),
                   ("N-ended-ref", b"P8-EXIT-DEPENDENCY"),
                   ("N-current-tail", b"V1-BACKEND-UNSUPPORTED"),
                   ("N-explicit-return", None)]:
    target = work / (name + ".c")
    target.write_bytes(b"P1332 original named output\n")
    before = snapshot(target)
    result = run(name, [compiler, "--emit-c17-to", target, root / (name + ".nl")])
    assert not result.stdout and result.returncode in (3, 4), result
    if code:
        assert code in result.stderr, result
        assert result.returncode == (4 if name == "N-current-tail" else 3), result
    assert snapshot(target) == before and not list(work.glob(target.name + ".tmp.*"))
    print(name, "status", result.returncode, "no-clobber PASS", flush=True)

for name in ["A-callee-local", "B-index1"]:
    source = root / (name + ".nl")
    target = work / (name + ".c")
    result = run(name + ".census", [injector, source, target, 0])
    match = re.fullmatch(rb"events=(\d+) fired=0 status=0\n", result.stdout)
    assert result.returncode == 0 and match and not result.stderr, result
    total = int(match[1])
    original = target.read_bytes()
    assert total > 4
    points = sorted({1, 2, total // 2, total - 1, total})
    # Combined A/B selection stays within the eight-point CLI budget.
    points = points[:3] + points[-1:]
    for ordinal in points + ["write", "flush"]:
        target.write_bytes(b"P1332 preserve bytes inode mtime\n")
        before = snapshot(target)
        label = name + ".fault-" + str(ordinal)
        result = run(label, [injector, source, target, ordinal])
        match = re.fullmatch(rb"events=(\d+) fired=1 status=(\d+)\n", result.stdout)
        assert result.returncode == 0 and match and result.stderr, result
        status = int(match[2])
        oom = b"error(host)[P3-OUT-OF-MEMORY]" in result.stderr
        assert status == 1 or (status == 3 and oom), result
        if isinstance(ordinal, str):
            assert status == 1 and b"V0-C-OUTPUT" in result.stderr, result
        else:
            assert int(match[1]) >= ordinal, result
        assert before == snapshot(target) and not list(work.glob(target.name + ".tmp.*"))
        retry = run(label + ".retry", [compiler, "--emit-c17-to", target, source])
        assert retry.returncode == 0 and not retry.stdout and not retry.stderr, retry
        assert target.read_bytes() == original
        print(name, "fault", ordinal, "census", total, "status", status,
              "no-clobber retry PASS", flush=True)
(work / "summary.json").write_text(json.dumps({"CLI_OOM": 8, "write": 2, "flush": 2,
    "source_negatives": 4, "original_inode_bytes_mtime": True, "retry_byte_identical": True}, indent=2) + "\n")
print(work, flush=True)
