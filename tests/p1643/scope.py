"""Check fail-closed scope around the private one-Return receipt."""

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile


EXPECTED = {
    "reversed-early-return.nl": (
        "af72154a34bc05d7704085e85c0b0d2f61212a0577c4b83a198dde4f457a6616"
    ),
    "unsupported-one-return.nl": (
        "159ffca652ad4acce4e3b085de0b04b3015a26493dfb52956c062c1811fc0c10"
    ),
}


def run(command, expected, evidence):
    result = subprocess.run(command, capture_output=True, check=False)
    evidence.with_suffix(".command").write_text(
        "\n".join(map(str, command)) + "\n", encoding="utf-8"
    )
    evidence.with_suffix(".stdout").write_bytes(result.stdout)
    evidence.with_suffix(".stderr").write_bytes(result.stderr)
    evidence.with_suffix(".status").write_text(
        f"{result.returncode}\n", encoding="ascii"
    )
    assert result.returncode == expected, (command, result.returncode, result.stderr)
    return result


def identity(path):
    info = path.stat()
    return {
        "device": info.st_dev,
        "inode": info.st_ino,
        "mode": info.st_mode,
        "size": info.st_size,
        "mtime_ns": info.st_mtime_ns,
    }


def reject(cli, source, raw):
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    assert source_hash == EXPECTED[source.name]
    checked = run([cli, "--check", source], 0, raw / f"{source.stem}-check")
    assert not checked.stdout and not checked.stderr

    diagnostic = b"error(cli)[V1-BACKEND-UNSUPPORTED]:"
    standard = run([cli, source], 4, raw / f"{source.stem}-stdout")
    assert not standard.stdout and standard.stderr.startswith(diagnostic)

    output_dir = raw / source.stem
    output_dir.mkdir()
    target = output_dir / "keep.c"
    sentinel = b"keep p1643 output\n\x00untouched\n"
    target.write_bytes(sentinel)
    before = identity(target)
    (raw / f"{source.stem}-before.bin").write_bytes(sentinel)
    (raw / f"{source.stem}-before.json").write_text(
        json.dumps(before), encoding="ascii"
    )
    named = run(
        [cli, "--emit-c17-to", target, source],
        4,
        raw / f"{source.stem}-named",
    )
    after = identity(target)
    actual = target.read_bytes()
    (raw / f"{source.stem}-after.bin").write_bytes(actual)
    (raw / f"{source.stem}-after.json").write_text(
        json.dumps(after), encoding="ascii"
    )
    assert not named.stdout and named.stderr.startswith(diagnostic)
    assert actual == sentinel and before == after
    assert list(output_dir.iterdir()) == [target]
    print(
        f"{source.name} sha256={source_hash} CHECK=0 C17=4 "
        "stdout=0 named=no-clobber PASS"
    )


def main():
    cli, evidence_arg, *sources = sys.argv[1:]
    root = Path(evidence_arg)
    root.mkdir(parents=True, exist_ok=True)
    raw = Path(tempfile.mkdtemp(dir=root))
    print(f"evidence={raw}", flush=True)
    for source in map(Path, sources):
        reject(cli, source, raw)


if __name__ == "__main__":
    main()
