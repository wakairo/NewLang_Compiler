"""Source negatives and selected real CLI OOM: named-output transaction."""
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

compiler, injector, parent = sys.argv[1:4]
root = Path(__file__).resolve().parent
Path(parent).mkdir(parents=True, exist_ok=True)
work = Path(tempfile.mkdtemp(prefix="run-", dir=parent))


def run(label, args):
    result = subprocess.run(list(map(str, args)), capture_output=True, timeout=60)
    (work / (label + ".stdout")).write_bytes(result.stdout)
    (work / (label + ".stderr")).write_bytes(result.stderr)
    return result


def snapshot(path):
    stat = path.stat()
    return path.read_bytes(), stat.st_ino, stat.st_mtime_ns


for name, status, diagnostic in [("N-ended-ref.nl", 3, b"P8-EXIT-DEPENDENCY"),
                                 ("N-typed-store.nl", 3, b"P3-TYPE-MISMATCH"),
                                 ("N-paired-helper.nl", 4, b"V1-BACKEND-UNSUPPORTED"),
                                 ("B-precision-boundary.nl", 3, b"P14-LOOP-PRECISION")]:
    target = work / (name + ".c")
    target.write_bytes(b"existing named output\n")
    before = snapshot(target)
    result = run(name, [compiler, "--emit-c17-to", target, root / name])
    assert result.returncode == status and not result.stdout and diagnostic in result.stderr, result
    assert before == snapshot(target) and not list(work.glob(target.name + ".tmp.*"))
    print(name, "status", status, "no-clobber PASS", flush=True)

for source in [root / "A-helper.nl"]:
    name = source.name
    target = work / (name + ".c")
    result = run(name + ".census", [injector, source, target, 0])
    match = re.fullmatch(rb"events=(\d+) fired=0 status=0\n", result.stdout)
    assert result.returncode == 0 and match and not result.stderr, result
    total = int(match[1])
    original = target.read_bytes()
    assert total > 4
    choices = sorted({1, 2, total // 2, total})
    for ordinal in choices + ["flush"]:
        target.write_bytes(b"P1326 preserve bytes inode and mtime\n")
        before = snapshot(target)
        label = name + ".oom-" + str(ordinal)
        result = run(label, [injector, source, target, ordinal])
        match = re.fullmatch(rb"events=(\d+) fired=1 status=(\d+)\n", result.stdout)
        assert result.returncode == 0 and match, result
        assert ordinal == "flush" or int(match[1]) >= ordinal, result
        # The adopted CLI renders CHECK's host OOM diagnostic as exit 3;
        # earlier/later driver resource failures use exit 1. Accept exit 3
        # only with the actual host OOM code, never a semantic diagnostic.
        status = int(match[2])
        check_oom = b"error(host)[P3-OUT-OF-MEMORY]" in result.stderr
        assert result.stderr and (status == 1 or (status == 3 and check_oom)), result
        if ordinal == "flush":
            assert status == 1 and b"V0-C-OUTPUT" in result.stderr, result
        assert before == snapshot(target) and not list(work.glob(target.name + ".tmp.*"))
        retry = run(label + ".retry", [compiler, "--emit-c17-to", target, source])
        assert retry.returncode == 0 and not retry.stdout and not retry.stderr, retry
        assert target.read_bytes() == original
        print(name, "OOM", ordinal, "of", total, "status", status,
              "no-clobber retry PASS", flush=True)
(work / "summary.json").write_text(json.dumps({"CLI_oom": 4, "flush_failure": 1, "semantic_negative": 2, "safe_family_refusal": 2,
    "named_bytes_inode_mtime": True, "temp_residue": False, "retries_byte_identical": True}, indent=2))
print(work, flush=True)
