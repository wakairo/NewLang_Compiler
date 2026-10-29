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
calibration = json.loads((root / "oracle-v2.json").read_text())


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
        layered = json.loads((root / "negative-layered-v2.json").read_text())["profiles"]
        original = json.loads((root / "negative-layered-v1.json").read_text())["profiles"]
        assert original["negative_duplicate_return.nl"] == {"CHECK": 0, "C17": 4}
        normalized = run([cli, root / "sources/negative_duplicate_return.nl"])
        ok(normalized)
        direct = run([cli, root / "sources/before_direct.nl"])
        ok(direct)
        assert normalized.stdout == direct.stdout
        print("P990 V1 ORACLE RED RETAINED: expected4/actual0; one-issued-return "
              "normalization is separately covered by frozen V2 controls", flush=True)
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
                wanted = expected["CHECK" if mode == "check" else "C17"]
                if result.returncode != wanted:
                    artifact = os.environ.get("P990_EVIDENCE_DIR")
                    if artifact:
                        failure = Path(artifact)
                        failure.mkdir(parents=True, exist_ok=True)
                        (failure / (name + "-" + mode + "-RED.stdout")).write_bytes(result.stdout)
                        (failure / (name + "-" + mode + "-RED.stderr")).write_bytes(result.stderr)
                    raise AssertionError((name, mode, wanted, result.returncode))
                assert not result.stdout
                preserved(target, sentinel, before)
            print("P990 NEGATIVE SOURCE/NAMED", name,
                  expected["CHECK"], expected["C17"], flush=True)
        sys.exit(0)
    assert group in ["new", "p986"]
    if group == "new":
        profiles = [(root / "sources" / name, name, row)
                    for name, row in oracle["positive_profiles"].items()]
        profiles += [(root / name, Path(name).name, row)
                     for name, row in calibration["calibration_controls"].items()]
    else:
        profiles = [(root / "p986" / name, name, row)
                    for name, row in oracle["P986_unchanged_controls"].items()]
    for source, name, expected in profiles:
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

# C112's new P-owned source/oracle suite uses this existing test registration.
# The original P990 inputs, checks and expected observations above are retained.
if group == "new":
    additional = run([sys.executable, root.parent / "p1030/p1013_forward.py",
                      cli, cc, sanitizer])
    ok(additional)
    print(additional.stdout.decode(), end="", flush=True)
    bounded_unit = run([sys.executable, root.parent / "p1020/run.py",
                       cli, cc, sanitizer])
    ok(bounded_unit)
    print(bounded_unit.stdout.decode(), end="", flush=True)
    dual_fields = run([sys.executable, root.parent / "p1030/run.py",
                      cli, cc, sanitizer])
    ok(dual_fields)
    print(dual_fields.stdout.decode(), end="", flush=True)
    two_originals = run([sys.executable, root.parent / "p1045/run.py",
                        cli, cc, sanitizer])
    ok(two_originals)
    print(two_originals.stdout.decode(), end="", flush=True)
