"""Ordinary CLI -> unchanged C17 -> strict native and passive original worlds."""
from pathlib import Path
import hashlib
import json
import os
import resource
import signal
import subprocess
import sys
import tempfile

cli, cc, sanitizer, directory, group = sys.argv[1:]
root = Path(directory)
oracle = json.loads((root / "oracle-v1.json").read_text())


def run(args, **kw):
    return subprocess.run(list(map(str, args)), capture_output=True, **kw)


def ok(result):
    assert result.returncode == 0 and not result.stderr, (
        result.returncode, result.stdout, result.stderr)


def quota():
    signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
    resource.setrlimit(resource.RLIMIT_FSIZE, (128, 128))


def preserved(target, sentinel, before):
    assert target.read_bytes() == sentinel
    assert (before.st_dev, before.st_ino) == (target.stat().st_dev, target.stat().st_ino)
    assert not list(target.parent.glob("*.tmp*"))


with tempfile.TemporaryDirectory(prefix="p990-") as temp:
    temp = Path(temp)
    if group == "negative":
        layered = json.loads((root / "negative-layered-v1.json").read_text())["profiles"]
        for name, expected in layered.items():
            source = root / "sources" / name
            target = temp / "named.c"
            sentinel = b"P990 preserve previous complete output\n"
            target.write_bytes(sentinel)
            before = target.stat()
            for mode in ["check", "emit", "named"]:
                args = [cli] + (["--check"] if mode == "check" else
                               ["--emit-c17-to", target] if mode == "named" else []) + [source]
                result = run(args)
                assert result.returncode == expected["CHECK" if mode == "check" else "C17"]
                assert not result.stdout
                preserved(target, sentinel, before)
            print("P990 NEGATIVE SOURCE/NAMED", name,
                  expected["CHECK"], expected["C17"], flush=True)
        sys.exit(0)
    assert group in ["new", "p986"]
    profiles = oracle["positive_profiles"] if group == "new" else oracle["P986_unchanged_controls"]
    for name, expected in profiles.items():
        source = root / ("sources" if group == "new" else "p986") / name
        ok(run([cli, "--check", source]))
        result = run([cli, source])
        ok(result)
        assert result.stdout
        emitted = temp / (name + ".c")
        emitted.write_bytes(result.stdout)
        c_sha = hashlib.sha256(result.stdout).hexdigest()
        artifact = os.environ.get("P990_EVIDENCE_DIR")
        if artifact:
            destination = Path(artifact)
            destination.mkdir(parents=True, exist_ok=True)
            (destination / (name + ".c")).write_bytes(result.stdout)
        target = temp / "named.c"
        sentinel = b"P990 previous complete checked output\n"
        target.write_bytes(sentinel)
        ok(run([cli, "--emit-c17-to", target, source]))
        assert target.read_bytes() == result.stdout
        flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
        if sanitizer != "none":
            flags += ["-fsanitize=" + sanitizer, "-fno-omit-frame-pointer",
                      "-fno-sanitize-recover=all"]
        mixed = expected.get("mixed", True)
        for opt in ["-O0", "-O2"]:
            raw = temp / "raw"
            ok(run([cc, *flags, opt, emitted, "-o", raw]))
            executed = run([raw])
            ok(executed)
            assert not executed.stdout
            print("P990 RAW NATIVE", name, opt, sanitizer, c_sha, flush=True)
            if not mixed:
                continue
            observed = temp / "observed"
            definitions = ["T00", "T01", "T10", "T11", "CALL_ROOT", "I0", "I1",
                           "NEXT", "HV0", "HV1", "POST", "H_LOCAL", "HEAD_READ", "CALLER_READ"]
            ok(run([cc, *flags, opt, emitted,
                    root.parent / "integration/two_heap_platform.c",
                    "-Wl,--wrap=malloc", "-Wl,--wrap=free", "-o", observed,
                    '-DNEWLANG_HEAP_OBSERVER="' + str((root / "observer.h").resolve()) + '"',
                    *["-D" + key + "=" + str(expected[key]) for key in definitions]]))
            for fail, count in [("1", 0), ("2", 1), ("0", 2)]:
                executed = run([observed], env=dict(
                    os.environ, NEWLANG_TEST_FAIL_CALL=fail,
                    ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                    UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))
                ok(executed)
                text = executed.stdout.decode().strip()
                assert "allocations=" + str(count) + " frees=" + str(count) in text
                values = [[expected["T00"], expected["T01"]],
                          [expected["T10"], expected["T11"]]]
                old = values[expected["CALL_ROOT"]][expected["I0"]]
                values[expected["CALL_ROOT"]][expected["I0"]] = expected["NEXT"]
                values[1 - expected["CALL_ROOT"]][expected["I1"]] = old
                if not expected["POST"] or count == 2:
                    final = "T=%d,%d/%d,%d old=%d calls=1" % (*values[0], *values[1], old)
                    assert final in text, (text, final)
                else:
                    assert "T=0,0/0,0 old=0 calls=0" in text
                print(json.dumps(dict(source=name, group=group, C_sha256=c_sha, cc=cc,
                      opt=opt, sanitizer=sanitizer, allocation_successes=count,
                      observation=text)), flush=True)
        target.write_bytes(sentinel)
        before = target.stat()
        rejected = run([cli, "--emit-c17-to", target, source], preexec_fn=quota)
        assert rejected.returncode == 1 and b"OUTPUT" in rejected.stderr and not rejected.stdout
        preserved(target, sentinel, before)
        ok(run([cli, "--emit-c17-to", target, source]))
        assert target.read_bytes() == result.stdout
        print("P990 REACHED OUTPUT ERROR/EXACT RETRY", name, flush=True)
