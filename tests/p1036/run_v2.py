"""C137 finite three-source-current/original T-H/native/OOM measurements."""
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

cli, cc, sanitizer, *selected = sys.argv[1:]
assert not selected or len(selected) == 1 and selected[0] in ["0", "1", "2"]
selection = int(selected[0]) if selected else None
cli = Path(cli).resolve()
data = Path(__file__).resolve().parent
repo = data.parents[1]
oracle = json.loads((data / "oracle-v1.json").read_bytes())
old = json.loads((data / "old-C-identity-v1.json").read_bytes())
sha = lambda b: hashlib.sha256(b).hexdigest()
commands = []
evidence = os.environ.get("P1036_EVIDENCE_DIR")
archive = Path(tempfile.mkdtemp(prefix="P1036-", dir=evidence)) if evidence else None


def invoke(label, args, **kw):
    planned_signal = kw.pop("planned_signal", None)
    result = subprocess.run(list(map(str, args)), capture_output=True, timeout=180, **kw)
    commands.append({"label": label, "command": list(map(str, args)), "exit": result.returncode,
                     "stdout_SHA256": sha(result.stdout), "stderr_SHA256": sha(result.stderr)})
    if archive:
        (archive / (label + ".stdout")).write_bytes(result.stdout)
        (archive / (label + ".stderr")).write_bytes(result.stderr)
        (archive / "commands-FIRST.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode >= 0 or result.returncode == planned_signal, ("signal: stop candidate", label, result.returncode, result.stderr)
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

with tempfile.TemporaryDirectory(prefix="p1036-") as directory:
    temp = Path(directory)
    libraries = [cli.parent / ("libnewlang_" + n + ".a") for n in [
        "semantic", "parser", "checked_c_node", "lexer", "checked", "syntax", "source", "diagnostic"]]
    assert all(p.is_file() for p in libraries)
    for name in ["current", "consumer", "oom"]:
        wraps = ["-Wl,--wrap=" + n for n in ["malloc", "calloc", "realloc"]] if name == "oom" else []
        ok(invoke("build-" + name, [cc, *flags, "-g", "-O0", "-fno-pie", "-no-pie",
                  "-I" + str(repo / "include"), data / (name + ".c"),
                  "-Wl,--start-group", *libraries, "-Wl,--end-group", *wraps, "-o", temp / name]))
    profiles = [(data / "sources" / name, expected["first_index"], expected)
                for name, expected in oracle["positive"].items()]
    compat = oracle["compatibility_forward_plan"]
    profiles.append((repo / compat["old_source"], 2,
                     {"old":[41,67,109],"next":[67,109,151],"sibling":[173,173,173],"indices":[0,0,0]}))
    if selection is not None:
        profiles = [profiles[selection]]
    for source, profile, expected in profiles:
        if profile == 2:
            assert sha(source.read_bytes()) == compat["source_SHA256"]
        name = source.name
        label = source.stem + ("-forward" if profile == 2 else "")
        expected_physical = "old=" + ",".join(map(str,expected["old"])) + " new=" + ",".join(map(str,expected["next"])) + " sibling=" + ",".join(map(str,expected["sibling"])) + " calls=3"
        indices = ",".join(map(str,expected["indices"]))
        ok(invoke(label + "-CHECK", [cli, "--check", source]))
        public = invoke(label + "-public-current",
                        [temp / "current", source, profile]); ok(public)
        assert public.stdout.count(b"body=0 parameters=0 event=0 GET=YES") == 3
        assert b"P1036 C17 status=0 published=YES" in public.stdout
        print(public.stdout.decode(), end="", flush=True)
        emitted = invoke(label + "-C17", [cli, source]); ok(emitted)
        assert emitted.stdout
        c = temp / (label + ".c"); c.write_bytes(emitted.stdout)
        c_sha = sha(emitted.stdout)
        if archive:
            (archive / (label + ".c.data")).write_bytes(emitted.stdout)
        calls = re.findall(rb"function=(\d+)", public.stdout)
        assert len(calls) == len(set(calls)) == 3
        for function in calls:
            assert emitted.stdout.count(b"nl_copy_state_" + function + b"(") == 2
        bindings = re.findall(rb"P1036 RESULT ordinal=2 binding=(\d+)", public.stdout)
        frames = re.findall(rb"NL_MIXED_COPY_BIND\((\d+),\d+,&nl_b_\d+_\d+\.f0", emitted.stdout)
        assert len(bindings) == len(frames) == 1
        third_result = b"nl_b_" + frames[0] + b"_" + bindings[0]
        assert re.search(rb"uint8_t\s+" + third_result + rb"\s*=", emitted.stdout)
        definitions = ["-DP1036_FIRST_INDEX=" + str(profile),
                       "-DP1036_THIRD_RESULT=" + third_result.decode()]
        named = temp / "named.c"
        sentinel = b"P1036 keep previous complete named output\n"
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
            print("P1036 RAW", name, opt, sanitizer, c_sha,
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
                print(json.dumps({"P1036": "physical", "source": name, "cc": cc,
                      "opt": opt, "sanitizer": sanitizer, "guest_successes": successes,
                      "C_SHA256": c_sha, "observations": text}), flush=True)
        # Calibration changes only a defined u8 return in an emitted-C copy.
        # Stores/hooks stay correct, proving the caller-result check adds evidence.
        if sanitizer == "none":
            function = calls[2]
            pattern = (rb"(static uint8_t nl_copy_state_" + function +
                       rb"\([^\n]+;)return old;(\})")
            mutant_text, replacements = re.subn(pattern, rb"\1return next;\2", emitted.stdout)
            assert replacements == 1
            mutant = temp / "wrong-third-return.c"; mutant.write_bytes(mutant_text)
            calibrated = temp / "wrong-third-return"
            ok(invoke(label + "-calibration-build", [cc, *flags, "-O0", mutant,
                      repo / "tests/integration/two_heap_platform.c",
                      "-Wl,--wrap=malloc", "-Wl,--wrap=free",
                      '-DNEWLANG_HEAP_OBSERVER="' + str(data / "observer.h") + '"',
                      *definitions, "-o", calibrated]))
            def no_core():
                resource.setrlimit(resource.RLIMIT_CORE, (0,0))
            detected = invoke(label + "-planned-wrong-return-rejection", [calibrated],
                              env=dict(os.environ, NEWLANG_TEST_FAIL_CALL="1"),
                              preexec_fn=no_core, planned_signal=-signal.SIGABRT)
            assert detected.returncode == -signal.SIGABRT and b"P1036_THIRD_RESULT" in detected.stderr
            print("P1036 OBSERVER CALIBRATION third-return-next detected=YES defined_u8_mutant=YES", name, flush=True)
        negative = invoke(label + "-metadata-current-owner", [temp / "consumer", source]); ok(negative)
        assert negative.stdout.count(b"P1036 ACTUAL REJECT") == 21
        assert negative.stdout.count(b"P1036 THIRD STALE REJECT") == 3
        assert b"P1036 THIRD RESULT REJECT incoming_is_not_old=YES" in negative.stdout
        assert negative.stdout.count(b"P1036 H REJECT") == 4
        assert b"A_destroyed_unrestored=YES B_current_exact_C_destroy=YES" in negative.stdout
        print(negative.stdout.decode(), end="", flush=True)
        oom = invoke(label + "-reached-OOM", [temp / "oom", source]); ok(oom)
        assert b"P1036 OOM SUMMARY reached=" in oom.stdout
        print(oom.stdout.decode(), end="", flush=True)
        print("P1036 NAMED bytes/dev/ino/no-temp/reached-error/exact-retry PASS", name, flush=True)
    if selection in (None, 0):
        for name, expected in oracle["negative_sources"].items():
            source = data / "sources" / name
            checked = invoke(source.stem + "-negative-CHECK", [cli, "--check", source])
            assert checked.returncode == expected["CHECK"] and not checked.stdout
            if checked.returncode:
                assert b"error(semantic)" in checked.stderr
            named = temp / "negative-existing.c"
            sentinel = b"P1036 rejected source must preserve named bytes\n"
            named.write_bytes(sentinel); before = named.stat()
            for mode in ["stdout", "fresh", "existing"]:
                target = temp / "negative-never.c" if mode == "fresh" else named
                args = [cli] + ([] if mode == "stdout" else ["--emit-c17-to", target]) + [source]
                rejected = invoke(source.stem + "-negative-" + mode, args)
                assert rejected.returncode == expected["C17"] and not rejected.stdout
                if mode == "fresh":
                    assert not target.exists()
                preserved(named, sentinel, before)
            print("P1036 SOURCE REJECT", name, expected, "no_clobber=YES", flush=True)
        for relative, expected in old["sources"].items():
            source = repo / relative
            assert sha(source.read_bytes()) == expected["source_SHA256"]
            result = invoke("old-C-" + relative.replace("/", "_"), [cli, source]); ok(result)
            assert sha(result.stdout) == expected["C_SHA256"], relative
        print("P1036 original unchanged C byte identities PASS", len(old["sources"]), flush=True)
        history = invoke("history-v8-v9-counterexamples", [sys.executable, data / "history_tests.py"])
        ok(history)
        assert history.stdout.count(b"P1036 HISTORY REJECT") == 5
        assert history.stdout.count(b"P1030 HISTORY REJECT") == 5
        print(history.stdout.decode(), end="", flush=True)
print("P1036 C137 bounded selected-profile", selection, "current/C17/original-T-H/guest0-1-2/OOM/negative/named PASS; global source/old-C/history in profile0", flush=True)
