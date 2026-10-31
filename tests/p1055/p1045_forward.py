"""C137 finite source/current/two-original T and original H C17 measurements."""
from pathlib import Path
import hashlib
import json
import os
import re
import resource
import signal
import subprocess
import sys
import tempfile

cli, cc, sanitizer = sys.argv[1:]
cli = Path(cli).resolve()
data = Path(__file__).resolve().parents[1] / "p1045"
repo = data.parents[1]
oracle = json.loads((data / "native-oracle-v1.json").read_bytes())
old = json.loads((data / "old-C-identity-v1.json").read_bytes())
sha = lambda b: hashlib.sha256(b).hexdigest()
commands = []
evidence = os.environ.get("P1045_EVIDENCE_DIR")
archive = Path(tempfile.mkdtemp(prefix="P1045-", dir=evidence)) if evidence else None


def invoke(label, args, **kw):
    result = subprocess.run(list(map(str, args)), capture_output=True, timeout=180, **kw)
    commands.append({"label": label, "command": list(map(str, args)), "exit": result.returncode,
                     "stdout_SHA256": sha(result.stdout), "stderr_SHA256": sha(result.stderr)})
    if archive:
        (archive / (label + ".stdout")).write_bytes(result.stdout)
        (archive / (label + ".stderr")).write_bytes(result.stderr)
        (archive / "commands-FIRST.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode >= 0, ("signal: stop candidate", label, result.returncode, result.stderr)
    return result


def ok(result):
    assert result.returncode == 0 and not result.stderr, (
        result.returncode, result.stdout, result.stderr)


def quota():
    signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
    resource.setrlimit(resource.RLIMIT_FSIZE, (128, 128))


def preserved(target, sentinel, before):
    assert target.read_bytes() == sentinel
    after = target.stat()
    assert (before.st_dev, before.st_ino) == (after.st_dev, after.st_ino)
    assert not list(target.parent.glob("*.tmp*"))


flags = ["-std=c17", "-Wall", "-Wextra", "-Werror", "-pedantic-errors"]
if sanitizer != "none":
    flags += ["-fsanitize=" + sanitizer, "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]

with tempfile.TemporaryDirectory(prefix="p1045-") as directory:
    temp = Path(directory)
    libraries = [cli.parent / ("libnewlang_" + n + ".a") for n in [
        "semantic", "parser", "checked_c_node", "lexer", "checked", "syntax", "source", "diagnostic"]]
    assert all(p.is_file() for p in libraries)
    for name in ["probe", "consumer", "oom"]:
        wraps = ["-Wl,--wrap=" + n for n in ["malloc", "calloc", "realloc"]] if name == "oom" else []
        ok(invoke("build-" + name, [cc, *flags, "-g", "-O0", "-fno-pie", "-no-pie",
                  "-I" + str(repo / "include"), data / (name + ".c"),
                  "-Wl,--start-group", *libraries, "-Wl,--end-group", *wraps, "-o", temp / name]))
    profiles = [(data / "sources-v2" / name, phase) for name, phase in oracle["profiles"].items()]
    for source, phase in profiles:
        name = source.name
        label = source.stem
        definitions = ["-DP1045_PHASE=" + str(phase)]
        ok(invoke(label + "-CHECK", [cli, "--check", source]))
        public = invoke(label + "-public-current",
                        [temp / "probe", source]); ok(public)
        assert public.stdout.count(b"body=0 parameters=0 event=0") == 2
        assert b"P1045 C17 status=0 published=YES" in public.stdout
        print(public.stdout.decode(), end="", flush=True)
        emitted = invoke(label + "-C17", [cli, source]); ok(emitted)
        assert emitted.stdout
        c = temp / (label + ".c"); c.write_bytes(emitted.stdout)
        c_sha = sha(emitted.stdout)
        if archive:
            (archive / (label + ".c.data")).write_bytes(emitted.stdout)
        calls = re.findall(rb"function=(\d+)", public.stdout)
        assert len(calls) == len(set(calls)) == 2
        for function in calls:
            assert emitted.stdout.count(b"nl_copy_state_" + function + b"(") == 2
        named = temp / "named.c"
        sentinel = b"P1045 keep previous complete named output\n"
        named.write_bytes(sentinel)
        ok(invoke(label + "-named", [cli, "--emit-c17-to", named, source]))
        assert named.read_bytes() == emitted.stdout
        named.write_bytes(sentinel); before = named.stat()
        failed = invoke(label + "-reached-output-error", [cli, "--emit-c17-to", named, source],
                        preexec_fn=quota)
        assert failed.returncode == 1 and not failed.stdout and b"OUTPUT" in failed.stderr
        preserved(named, sentinel, before)
        ok(invoke(label + "-exact-retry", [cli, "--emit-c17-to", named, source]))
        assert named.read_bytes() == emitted.stdout
        for opt in ["-O0", "-O2"]:
            raw = temp / "raw"
            ok(invoke(label + opt + "-raw-build", [cc, *flags, opt, c, "-o", raw]))
            result = invoke(label + opt + "-raw", [raw]); ok(result)
            assert not result.stdout
            print("P1045 RAW", name, opt, sanitizer, c_sha,
                  "physical=INCONCLUSIVE(unit-exit-only)", flush=True)
            observed = temp / "observed"
            ok(invoke(label + opt + "-physical-build", [cc, *flags, opt, c,
                      repo / "tests/integration/two_heap_platform.c",
                      "-Wl,--wrap=malloc", "-Wl,--wrap=free",
                      '-DNEWLANG_HEAP_OBSERVER="' + str(data / "observer.h") + '"',
                      *definitions, "-o", observed]))
            for failure, successes in [("1", 0), ("2", 1), ("0", 2)]:
                result = invoke(label + opt + "-guest" + str(successes), [observed],
                                env=dict(os.environ, NEWLANG_TEST_FAIL_CALL=failure,
                                         ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                                         UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))
                ok(result)
                text = result.stdout.decode().strip()
                t_reached = phase == 0 or successes == 2
                if t_reached:
                    assert "old=19,67 new=88,19 sibling=149,222 calls=2 T_observed=1" in text
                else:
                    assert "old=0,0 new=0,0 sibling=0,0 calls=0 T_observed=0" in text
                assert "separate_originals=YES live_original_reads=YES" in text
                assert f"allocations={successes} frees={successes}" in text
                assert f"live_tail={int(successes == 2)} original_H=YES teardown=YES" in text
                print(json.dumps({"P1045": "physical", "source": name, "cc": cc,
                      "opt": opt, "sanitizer": sanitizer, "guest_successes": successes,
                      "C_SHA256": c_sha, "observations": text}), flush=True)
        negative = invoke(label + "-metadata-current-owner", [temp / "consumer", source]); ok(negative)
        assert negative.stdout.count(b"P1045 ACTUAL REJECT") == 20
        assert negative.stdout.count(b"P1045 H REJECT") == 4
        assert b"A_destroyed_unrestored=YES B_current_exact_C_destroy=YES" in negative.stdout
        print(negative.stdout.decode(), end="", flush=True)
        oom = invoke(label + "-reached-OOM", [temp / "oom", source]); ok(oom)
        assert b"P1045 OOM SUMMARY reached=" in oom.stdout
        print(oom.stdout.decode(), end="", flush=True)
        print("P1045 NAMED bytes/dev/ino/no-temp/reached-error/exact-retry PASS", name, flush=True)
    for name, expected in oracle["source_rejections"].items():
        source = data / "sources-v2" / name
        checked = invoke(source.stem + "-negative-CHECK", [cli, "--check", source])
        assert checked.returncode == expected["CHECK"] and not checked.stdout
        if checked.returncode:
            assert expected["diagnostic"].encode() in checked.stderr
        named = temp / "negative-existing.c"
        sentinel = b"P1045 rejected source must preserve named bytes\n"
        named.write_bytes(sentinel); before = named.stat()
        for mode in ["stdout", "fresh", "existing"]:
            target = temp / "negative-never.c" if mode == "fresh" else named
            args = [cli] + ([] if mode == "stdout" else ["--emit-c17-to", target]) + [source]
            rejected = invoke(source.stem + "-negative-" + mode, args)
            assert rejected.returncode == expected["C17"] and not rejected.stdout
            if mode == "fresh":
                assert not target.exists()
            preserved(named, sentinel, before)
        print("P1045 SOURCE REJECT", name, expected, "no_clobber=YES", flush=True)
    for relative, expected in old["sources"].items():
        source = repo / relative
        assert sha(source.read_bytes()) == expected["source_SHA256"]
        result = invoke("old-C-" + relative.replace("/", "_"), [cli, source]); ok(result)
        assert sha(result.stdout) == expected["C_SHA256"], relative
    print("P1045 original unchanged C byte identities PASS", len(old["sources"]), flush=True)
    history = invoke("history-v8-v9-counterexamples", [sys.executable, Path(__file__).with_name("history_v8_forward.py")])
    ok(history)
    assert history.stdout.count(b"P1045 HISTORY REJECT") == 5
    print(history.stdout.decode(), end="", flush=True)
print("P1045 C137 bounded current/C17/two-original-T-H/guest0-1-2/OOM/negative/named/history PASS", flush=True)
