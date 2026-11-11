"""CURRENT successor: all old meaning/scope failures and output faults.

The legal historical two-input-call sources now have owned/current positive
registrations. The original runners, sources, oracles and FIRST remain intact.
"""
import hashlib
from pathlib import Path
import re
import subprocess
import sys
import tempfile


compiler, injector, parent, family = sys.argv[1:5]
root = Path(__file__).resolve().parent.parent / family
cases = {
    "p1339": ({
        "N-argument-type": b"P3-TYPE-MISMATCH",
        "N-ended-ref": b"P8-EXIT-DEPENDENCY",
        "N-formal-write": b"LOCAL-LOAN-PROFILE",
    }, ["A-input67", "B-input109"]),
    "p1343": ({
        "N-wrong-argument": b"P3-TYPE-MISMATCH",
        "N-ended-ref": b"P8-EXIT-DEPENDENCY",
        "N-extra-caller": b"P14-LOOP-PRECISION",
    }, ["A-caller-live", "B-index1"]),
    "p1351": ({
        "N-wrong-type": b"P8-BODY-RESULT",
        "N-ended-loan": b"P8-EXIT-DEPENDENCY",
        "N-two-inner": b"USIZE-ADDITION-PRECISION",
        "N-extra-wrapper": b"USIZE-ADDITION-PRECISION",
    }, ["A-wrapper", "B-wrapper-index1"]),
    "p1347": ({
        "N-wrong-type": b"P3-TYPE-MISMATCH",
        "N-ended-loan": b"P8-EXIT-DEPENDENCY",
        "N-extra-original": b"P14-LOOP-PRECISION",
        "N-third-call": b"USIZE-ADDITION-PRECISION",
    }, ["A-updated-caller", "B-reverse-field"]),
}
negative, positives = cases[family]
Path(parent).mkdir(parents=True, exist_ok=True)
work = Path(tempfile.mkdtemp(prefix="negative-", dir=parent))


def run(label, command):
    result = subprocess.run(list(map(str, command)), capture_output=True,
                            timeout=60)
    (work / (label + ".stdout")).write_bytes(result.stdout)
    (work / (label + ".stderr")).write_bytes(result.stderr)
    return result


def snapshot(path):
    stat = path.stat()
    return path.read_bytes(), stat.st_ino, stat.st_mtime_ns


for name, code in negative.items():
    target = work / (name + ".c")
    target.write_bytes(b"CURRENT preserve rejected output\n")
    before = snapshot(target)
    result = run(name,
                 [compiler, "--emit-c17-to", target, root / (name + ".nl")])
    assert result.returncode == 3 and not result.stdout and code in result.stderr
    assert snapshot(target) == before
    assert not list(work.glob(target.name + ".tmp.*"))
    print(name, "semantic rejection/no-clobber PASS", flush=True)

for name in positives:
    source = root / (name + ".nl")
    target = work / (name + ".c")
    census = run(name + ".census", [injector, source, target, 0])
    match = re.fullmatch(rb"events=(\d+) fired=0 status=0\n", census.stdout)
    assert census.returncode == 0 and match and not census.stderr
    total = int(match[1])
    original = target.read_bytes()
    original_sha = hashlib.sha256(original).hexdigest()
    assert total > 4
    points = sorted({1, 2, total // 2, total})
    assert len(points) <= 4
    for ordinal in points + ["write", "flush"]:
        target.write_bytes(b"CURRENT preserve bytes inode mtime\n")
        before = snapshot(target)
        label = name + ".fault-" + str(ordinal)
        result = run(label, [injector, source, target, ordinal])
        match = re.fullmatch(rb"events=(\d+) fired=1 status=(\d+)\n",
                             result.stdout)
        assert result.returncode == 0 and match and result.stderr
        status = int(match[2])
        if isinstance(ordinal, str):
            assert status == 1 and b"V0-C-OUTPUT" in result.stderr
        else:
            assert int(match[1]) >= ordinal
            assert status == 1 or (
                status == 3 and b"error(host)[P3-OUT-OF-MEMORY]" in result.stderr)
        assert snapshot(target) == before
        assert not list(work.glob(target.name + ".tmp.*"))
        retry = run(label + ".retry",
                    [compiler, "--emit-c17-to", target, source])
        assert retry.returncode == 0 and not retry.stdout and not retry.stderr
        assert hashlib.sha256(target.read_bytes()).hexdigest() == original_sha
        print(name, "fault", ordinal, "census", total,
              "no-clobber/retry PASS", flush=True)

print(work, flush=True)
