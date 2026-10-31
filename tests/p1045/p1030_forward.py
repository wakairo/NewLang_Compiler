"""C137 forward invocation: all original P1030 source/oracle/native checks unchanged."""
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
data = Path(__file__).resolve().parents[1] / "p1030"
repo = data.parents[1]
original = json.loads((data / "oracle-v1.json").read_bytes())
oracle = json.loads((data / "oracle-v2.json").read_bytes())
old = json.loads((data / "old-C-identity-v1.json").read_bytes())
compatibility = json.loads((data / "p1013-forward-oracle-v1.json").read_bytes())
sha = lambda b: hashlib.sha256(b).hexdigest()
commands = []
evidence = os.environ.get("P1030_EVIDENCE_DIR")
archive = Path(tempfile.mkdtemp(prefix="P1030-", dir=evidence)) if evidence else None


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

with tempfile.TemporaryDirectory(prefix="p1030-") as directory:
    temp = Path(directory)
    libraries = [cli.parent / ("libnewlang_" + n + ".a") for n in [
        "semantic", "parser", "checked_c_node", "lexer", "checked", "syntax", "source", "diagnostic"]]
    assert all(p.is_file() for p in libraries)
    for name in ["probe", "compat_probe", "consumer", "oom"]:
        wraps = ["-Wl,--wrap=" + n for n in ["malloc", "calloc", "realloc"]] if name == "oom" else []
        ok(invoke("build-" + name, [cc, *flags, "-g", "-O0", "-fno-pie", "-no-pie",
                  "-I" + str(repo / "include"), data / (name + ".c"),
                  "-Wl,--start-group", *libraries, "-Wl,--end-group", *wraps, "-o", temp / name]))
    profiles = [(data / "sources" / name, False) for name in original["positive"]]
    profiles.append((repo / "tests/p1013/production/unsupported_sibling.nl", True))
    for source, forward in profiles:
        if forward:
            assert sha(source.read_bytes()) == compatibility["transferred_positive"]["old_source_SHA256"]
        name = source.name
        label = source.stem + ("-forward" if forward else "")
        expected_physical = ("old=41,173 new=67,109 sibling=173,67 calls=2" if forward else
                             "old=203,58 new=97,203 sibling=58,97 calls=2")
        indices = "0,1" if forward else "1,0"
        definitions = (["-DP1030_T0=41", "-DP1030_T1=173", "-DP1030_FIRST_INDEX=0",
                        "-DP1030_NEXT0=67", "-DP1030_NEXT1=109", "-DP1030_H0=23",
                        "-DP1030_H1=199"] if forward else [])
        ok(invoke(label + "-CHECK", [cli, "--check", source]))
        public = invoke(label + "-public-current",
                        [temp / ("compat_probe" if forward else "probe"), source]); ok(public)
        assert public.stdout.count(b"body=0 parameters=0 event=0") == 2
        assert b"P1030 C17 status=0 published=YES" in public.stdout
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
        sentinel = b"P1030 keep previous complete named output\n"
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
            print("P1030 RAW", name, opt, sanitizer, c_sha,
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
                assert expected_physical in text
                assert f"indices={indices} same_original_T=YES live_original_reads=YES" in text
                assert f"allocations={successes} frees={successes}" in text
                assert f"live_tail={int(successes == 2)} original_H=YES teardown=YES" in text
                print(json.dumps({"P1030": "physical", "source": name, "cc": cc,
                      "opt": opt, "sanitizer": sanitizer, "guest_successes": successes,
                      "C_SHA256": c_sha, "observations": text}), flush=True)
        negative = invoke(label + "-metadata-current-owner", [temp / "consumer", source]); ok(negative)
        assert negative.stdout.count(b"P1030 ACTUAL REJECT") == 14
        assert negative.stdout.count(b"P1030 H REJECT") == 4
        assert b"A_destroyed_unrestored=YES B_current_exact_C_destroy=YES" in negative.stdout
        print(negative.stdout.decode(), end="", flush=True)
        oom = invoke(label + "-reached-OOM", [temp / "oom", source]); ok(oom)
        assert b"P1030 OOM SUMMARY reached=" in oom.stdout
        print(oom.stdout.decode(), end="", flush=True)
        print("P1030 NAMED bytes/dev/ino/no-temp/reached-error/exact-retry PASS", name, flush=True)
    for name, expected in oracle["source_rejections"].items():
        source = data / "sources" / name
        checked = invoke(source.stem + "-negative-CHECK", [cli, "--check", source])
        assert checked.returncode == expected["CHECK"] and not checked.stdout
        if checked.returncode:
            assert expected["diagnostic"].encode() in checked.stderr
        named = temp / "negative-existing.c"
        sentinel = b"P1030 rejected source must preserve named bytes\n"
        named.write_bytes(sentinel); before = named.stat()
        for mode in ["stdout", "fresh", "existing"]:
            target = temp / "negative-never.c" if mode == "fresh" else named
            args = [cli] + ([] if mode == "stdout" else ["--emit-c17-to", target]) + [source]
            rejected = invoke(source.stem + "-negative-" + mode, args)
            assert rejected.returncode == expected["C17"] and not rejected.stdout
            if mode == "fresh":
                assert not target.exists()
            preserved(named, sentinel, before)
        print("P1030 SOURCE REJECT", name, expected, "no_clobber=YES", flush=True)
    for relative, expected in old["sources"].items():
        source = repo / relative
        assert sha(source.read_bytes()) == expected["source_SHA256"]
        result = invoke("old-C-" + relative.replace("/", "_"), [cli, source]); ok(result)
        assert sha(result.stdout) == expected["C_SHA256"], relative
    print("P1030 original unchanged C byte identities PASS", len(old["sources"]), flush=True)
    history = invoke("history-v7-v8-counterexamples", [sys.executable, Path(__file__).resolve().parent / "history_v8_forward.py"])
    ok(history)
    assert history.stdout.count(b"P1030 HISTORY REJECT") == 5
    print(history.stdout.decode(), end="", flush=True)
print("P1030 C127 bounded current/C17/original-T-H/guest0-1-2/OOM/negative/named/history PASS", flush=True)

print("P1045 FORWARD P1030 all source/oracle/native checks retained; coherent v7-v8-v9 counterexample epoch", flush=True)
