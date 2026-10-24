"""R825 only: filesystem identity observations on preserved R803 inputs."""
import hashlib
from pathlib import Path
import re
import subprocess
import sys
import tempfile

SOURCES = {
    "one.nl": "eb2423e7b9eab273fe21b5eff11c7a808b9fd383e48542a7f7400ad2cc09505d",
    "two.nl": "e1f95235eac4c3fb029251ed8a1cf51ad9b0fef0c0ea2aac09a5a9e987423a61",
}
FIELDS = ("parser", "register", "entry")
SENTINEL = b"R825_existing_named_output\n"


def run(*args):
    return subprocess.run([str(arg) for arg in args], capture_output=True)


def only_target(directory, target):
    entries = sorted(directory.iterdir())
    assert entries == [target], ("unexpected temporary output", entries)


def metadata(process):
    found = re.search(
        rb"R825_META rc=(\d+) hit=(\d+) hit_phase=(\d+) calls=(\d+) "
        rb"parser=(\d+):(\d+) register=(\d+):(\d+) "
        rb"entry=(\d+):(\d+) tail=(\d+):(\d+)", process.stderr)
    assert found, ("missing fault trace", process.returncode, process.stderr)
    rc, hit, phase, calls, *ranges = map(int, found.groups())
    pairs = dict(zip(("parser", "register", "entry", "tail"),
                     zip(ranges[::2], ranges[1::2])))
    assert rc == process.returncode
    return hit, phase, calls, pairs


def one_source(compiler, probe, folder, name):
    source = folder / name
    actual_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    assert actual_hash == SOURCES[name], ("R803 source modified", name)
    stdout = run(compiler, source)
    assert stdout.returncode == 0 and not stdout.stderr and stdout.stdout
    with tempfile.TemporaryDirectory(prefix="r825-") as temp:
        directory = Path(temp)
        target = directory / "existing.c"
        target.write_bytes(SENTINEL)
        before_trace = target.stat()
        trace = run(probe, "trace", target, source)
        hit, phase, calls, pairs = metadata(trace)
        assert trace.returncode == 0 and hit == 0 and calls > 0
        assert not trace.stdout and target.read_bytes() == stdout.stdout
        only_target(directory, target)
        print("R825 baseline", name, "source_sha256", actual_hash,
              "allocations", calls, "phase ranges", pairs,
              "initial_inode", (before_trace.st_dev, before_trace.st_ino),
              flush=True)

        selected = {}
        for label in FIELDS:
            first, last = pairs[label]
            assert first < calls and last < calls and first <= last, (
                "required parser/issuer phase NOT TESTED", name, label, pairs)
            for position in (first, last):
                selected.setdefault(position, set()).add(label)
        selected.setdefault(calls - 1, set()).add("last-reached")
        failed = {label: 0 for label in FIELDS}
        for index, labels in sorted(selected.items()):
            target.unlink()
            target.write_bytes(SENTINEL)
            before = target.stat()
            old_bytes = target.read_bytes()
            assert old_bytes == SENTINEL
            injected = run(probe, index, target, source)
            hit, injected_phase, counted, _ = metadata(injected)
            assert hit == 1 and counted > index, (
                "injection was not reached", name, index, injected.stderr)
            assert injected.returncode != 0 and not injected.stdout, (
                "no actual failed CLI result", name, index, injected.returncode,
                injected.stdout, injected.stderr)
            after = target.stat()
            assert target.read_bytes() == old_bytes, (
                "named-output bytes changed", name, index)
            assert (before.st_dev, before.st_ino) == (
                after.st_dev, after.st_ino), (
                    "named-output identity changed", name, index,
                    (before.st_dev, before.st_ino),
                    (after.st_dev, after.st_ino))
            only_target(directory, target)
            for label in labels:
                if label in failed:
                    failed[label] += 1
            print("R825 injected failure", name, "index", index,
                  "phases", sorted(labels), "hit_phase", injected_phase,
                  "rc", injected.returncode,
                  "before", (before.st_dev, before.st_ino, old_bytes),
                  "after", (after.st_dev, after.st_ino, target.read_bytes()),
                  "temp_count", 0, flush=True)

            clean = run(compiler, "--emit-c17-to", target, source)
            assert clean.returncode == 0 and not clean.stdout and not clean.stderr, (
                "clean retry failed", name, index, clean.stderr)
            assert target.read_bytes() == stdout.stdout, (
                "clean retry differs from stdout C17", name, index)
            only_target(directory, target)
            print("R825 clean retry", name, "index", index,
                  "bytes", len(stdout.stdout), "temp_count", 0, flush=True)
        assert all(failed.values()), (
            "reached failed parser/issuer transaction missing", name, failed)


def main():
    compiler, probe, folder = sys.argv[1:]
    folder = Path(folder)
    for name in SOURCES:
        one_source(compiler, probe, folder, name)
    print("R825 bounded actual OOM inode check PASS", flush=True)


if __name__ == "__main__":
    main()
