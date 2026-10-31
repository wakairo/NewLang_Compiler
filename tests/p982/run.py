"""Real CLI, unchanged emitted C17, passive original carrier observation."""
from pathlib import Path
import hashlib
import json
import os
import resource
import signal
import subprocess
import sys
import tempfile

cli, cc, sanitizer, directory = sys.argv[1:]
root = Path(directory)
oracle = json.loads((root / "oracle-v1.json").read_text())


def run(args, **kw):
    return subprocess.run(list(map(str, args)), capture_output=True, **kw)


def ok(r):
    assert r.returncode == 0 and not r.stderr, (r.returncode, r.stdout, r.stderr)


def quota():
    signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
    resource.setrlimit(resource.RLIMIT_FSIZE, (128, 128))


with tempfile.TemporaryDirectory(prefix="p982-") as temp:
    temp = Path(temp)
    for name, expected in oracle["positive_profiles"].items():
        source = root / "sources" / name
        ok(run([cli, "--check", source]))
        result = run([cli, source])
        ok(result)
        assert result.stdout
        emitted = temp / (name + ".c")
        emitted.write_bytes(result.stdout)
        c_sha = hashlib.sha256(result.stdout).hexdigest()
        artifact = os.environ.get("P982_EVIDENCE_DIR")
        if artifact:
            dest = Path(artifact)
            dest.mkdir(parents=True, exist_ok=True)
            (dest / (name + ".c")).write_bytes(result.stdout)
        target = temp / "named.c"
        sentinel = b"P982 previous checked output\n"
        target.write_bytes(sentinel)
        ok(run([cli, "--emit-c17-to", target, source]))
        assert target.read_bytes() == result.stdout
        flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
        if sanitizer != "none":
            flags += ["-fsanitize=" + sanitizer, "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
        for opt in ["-O0", "-O2"]:
            raw = temp / "raw"
            ok(run([cc, *flags, opt, emitted, "-o", raw]))
            rr = run([raw])
            ok(rr)
            assert not rr.stdout
            observed = temp / "observed"
            ok(run([cc, *flags, opt, emitted, root.parent / "integration/two_heap_platform.c",
                    "-Wl,--wrap=malloc", "-Wl,--wrap=free", "-o", observed,
                    '-DNEWLANG_HEAP_OBSERVER="' + str((root / "observer.h").resolve()) + '"',
                    "-DEXTRA=0", *["-D" + k + "=" + str(v) for k, v in expected.items()]]))
            for fail, count in [("1", 0), ("2", 1), ("0", 2)]:
                rr = run([observed], env=dict(os.environ, NEWLANG_TEST_FAIL_CALL=fail,
                         ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                         UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))
                ok(rr)
                text = rr.stdout.decode().strip()
                assert "allocations=" + str(count) + " frees=" + str(count) in text
                values = [[expected["T00"], expected["T01"]], [expected["T10"], expected["T11"]]]
                old = values[expected["CALL_ROOT"]][expected["I0"]]
                values[expected["CALL_ROOT"]][expected["I0"]] = expected["NEXT"]
                values[1 - expected["CALL_ROOT"]][expected["I1"]] = old
                if count == 2:
                    final = "T=%d,%d/%d,%d old=%d calls=1" % (*values[0], *values[1], old)
                    assert final in text, (text, final)
                else:
                    assert "T=0,0/0,0 old=0 calls=0" in text
                print(json.dumps({"source": name, "C_sha256": c_sha, "cc": cc, "opt": opt,
                                  "sanitizer": sanitizer, "allocation_successes": count,
                                  "observation": text}), flush=True)
        target.write_bytes(sentinel)
        st = target.stat()
        rejected = run([cli, "--emit-c17-to", target, source], preexec_fn=quota)
        assert rejected.returncode == 1 and b"OUTPUT" in rejected.stderr and not rejected.stdout
        assert target.read_bytes() == sentinel and (st.st_dev, st.st_ino) == (target.stat().st_dev, target.stat().st_ino)
        assert not list(temp.glob("*.tmp*"))
        ok(run([cli, "--emit-c17-to", target, source]))
        assert target.read_bytes() == result.stdout
        print("P982 REACHED OUTPUT FAILURE AND EXACT RETRY", name, flush=True)
    for name in oracle["negative_sources"]:
        source = root / "sources" / name
        target = temp / "named.c"
        sentinel = b"P982 preserve on invalid source\n"
        target.write_bytes(sentinel)
        st = target.stat()
        for args in [[cli, "--check", source], [cli, source], [cli, "--emit-c17-to", target, source]]:
            result = run(args)
            assert result.returncode == 3 and not result.stdout
            assert target.read_bytes() == sentinel and (st.st_dev, st.st_ino) == (target.stat().st_dev, target.stat().st_ino)
            assert not list(temp.glob("*.tmp*"))
        print("P982 INVALID SOURCE AND NAMED PRESERVATION", name, flush=True)
