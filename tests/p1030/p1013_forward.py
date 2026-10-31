"""P1030 forward C112 regression epoch; old source/oracle/runner remain immutable.

The newly eligible old sibling case is tested positively in P1030/run.py.
All other original C112 checks below are retained. Original oracle RED is saved.
"""
from pathlib import Path
import hashlib
import json
import os
import re
import resource
import shutil
import signal
import subprocess
import sys
import tempfile

cli, cc, sanitizer = sys.argv[1:]
cli = Path(cli).resolve()
data = Path(__file__).resolve().parents[1] / "p1013/production"
repo = data.parents[2]
oracle = json.loads(Path(__file__).with_name("p1013-forward-oracle-v1.json").read_bytes())
sha = lambda b: hashlib.sha256(b).hexdigest()
commands = []
evidence = os.environ.get("P1013_EVIDENCE_DIR")
archive = Path(tempfile.mkdtemp(prefix="P1013-", dir=evidence)) if evidence else None

def invoke(label, args, **kw):
    result = subprocess.run(list(map(str, args)), capture_output=True, timeout=80, **kw)
    commands.append({"label": label, "command": list(map(str, args)), "exit": result.returncode,
                     "stdout_sha256": sha(result.stdout), "stderr_sha256": sha(result.stderr)})
    if archive:
        (archive / (label + ".stdout")).write_bytes(result.stdout)
        (archive / (label + ".stderr")).write_bytes(result.stderr)
        (archive / "commands-FIRST.json").write_text(json.dumps(commands, indent=2) + "\n")
    return result

def ok(result):
    assert result.returncode == 0 and not result.stderr, (result.returncode, result.stdout, result.stderr)

def quota():
    signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
    resource.setrlimit(resource.RLIMIT_FSIZE, (128, 128))

def preserved(path, original, before):
    assert path.read_bytes() == original
    after = path.stat()
    assert (before.st_dev, before.st_ino) == (after.st_dev, after.st_ino)
    assert not list(path.parent.glob("*.tmp*"))

flags = ["-std=c17", "-Wall", "-Wextra", "-Werror", "-pedantic-errors"]
if sanitizer != "none":
    flags += ["-fsanitize=" + sanitizer, "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]

with tempfile.TemporaryDirectory(prefix="p1013-") as directory:
    temp = Path(directory)
    libraries = [cli.parent / ("libnewlang_" + n + ".a")
                 for n in ["semantic", "parser", "checked_c_node", "lexer", "checked", "syntax", "source", "diagnostic"]]
    assert all(p.is_file() for p in libraries)
    for name, source in [("public", data.parent / "stage0/probe.c"),
                         ("consumer", data / "probe.c")]:
        result = invoke("build-" + name, [cc, *flags, "-g", "-O0", "-fno-pie", "-no-pie",
                        "-I" + str(repo / "include"), source,
                        "-Wl,--start-group", *libraries, "-Wl,--end-group", "-o", temp / name])
        ok(result)
    positive = [repo / p for p in oracle["positive_sources"]]
    for index, source in enumerate(positive):
        label = source.stem
        ok(invoke(label + "-check", [cli, "--check", source]))
        public = invoke(label + "-public", [temp / "public", source, "observe"])
        ok(public)
        rows = [json.loads(line) for line in public.stdout.decode().splitlines() if line.startswith("{")]
        calls = [r for r in rows if r["stage"] == "public"]
        values = [r for r in rows if r["stage"] == "values"]
        assert len(calls) == len(values) == 2
        if "same" in label:
            assert calls[0]["function"] == calls[1]["function"]
        else:
            assert calls[0]["function"] != calls[1]["function"]
        emitted = invoke(label + "-emit", [cli, source]); ok(emitted)
        assert emitted.stdout
        c = temp / (label + ".c"); c.write_bytes(emitted.stdout)
        if archive: (archive / c.name).write_bytes(emitted.stdout)
        c_sha = sha(emitted.stdout)
        if source.name in oracle["unchanged_parent_C"]:
            assert c_sha == oracle["unchanged_parent_C"][source.name]
        symbols = re.findall(rb"nl_copy_state_(\d+)\(", emitted.stdout)
        if label.startswith("mixed"):
            expected = {}
            for call in calls:
                fn = str(call["function"]).encode()
                expected[fn] = expected.get(fn, 1) + 1
            assert {fn: symbols.count(fn) for fn in set(symbols)} == expected
            consumer = invoke(label + "-consumer", [temp / "consumer", source]); ok(consumer)
            print(consumer.stdout.decode(), end="", flush=True)
        print(json.dumps({"P1013": "public-current", "source": str(source.relative_to(repo)),
              "C_sha256": c_sha, "receipts": rows}), flush=True)
        named = temp / "named.c"; sentinel = b"P1013 preserve old complete C output\n"
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
        print("P1013 NAMED reached_error/exact_retry", label, flush=True)
        for opt in ["-O0", "-O2"]:
            executable = temp / "raw"
            ok(invoke(label + opt + "-raw-build", [cc, *flags, opt, c, "-o", executable]))
            executed = invoke(label + opt + "-raw", [executable]); ok(executed)
            assert not executed.stdout
            print("P1013 RAW", label, opt, sanitizer, c_sha, flush=True)
            if not label.startswith("mixed"): continue
            physical = temp / "physical"
            ok(invoke(label + opt + "-physical-build", [cc, *flags, opt, c,
                      repo / "tests/integration/two_heap_platform.c",
                      "-Wl,--wrap=malloc", "-Wl,--wrap=free",
                      '-DNEWLANG_HEAP_OBSERVER="' + str(data / "observer.h") + '"',
                      "-o", physical]))
            for failure, successes in [("1", 0), ("2", 1), ("0", 2)]:
                executed = invoke(label + opt + "-heap" + str(successes), [physical],
                                  env=dict(os.environ, NEWLANG_TEST_FAIL_CALL=failure))
                ok(executed)
                text = executed.stdout.decode()
                assert "old=41,67 new=67,109 sibling=173,173 calls=2 same_field=YES" in text
                assert f"allocations={successes} frees={successes}" in text
                assert f"live_tail={1 if successes == 2 else 0} original_H=YES teardown=YES" in text
                print(json.dumps({"P1013": "physical", "source": label, "cc": cc,
                      "optimization": opt, "sanitizer": sanitizer, "successes": successes,
                      "C_sha256": c_sha, "observation": text.strip()}), flush=True)
        if label.startswith("mixed"):
            oom = invoke(label + "-reached-emitter-OOM", [cli.parent / "p990_oom", source])
            ok(oom)
            assert b"P990 new_emitter_reached_OOM=" in oom.stdout
            print(oom.stdout.decode(), end="", flush=True)
    for relative, expected in oracle["negative_sources"].items():
        source = repo / relative; label = source.stem
        check = invoke(label + "-negative-check", [cli, "--check", source])
        assert check.returncode == expected["CHECK"]
        named = temp / "negative-existing.c"; sentinel = b"P1013 KEEP NEGATIVE OUTPUT\n"
        named.write_bytes(sentinel); before = named.stat()
        for mode in ["stdout", "fresh", "existing"]:
            target = temp / "negative-never.c" if mode == "fresh" else named
            args = [cli] + ([] if mode == "stdout" else ["--emit-c17-to", target]) + [source]
            result = invoke(label + "-negative-" + mode, args)
            assert result.returncode == expected["C17"] and not result.stdout
            if mode == "fresh": assert not target.exists()
            preserved(named, sentinel, before)
        print("P1013 NEGATIVE", label, expected, "unpublished=YES no_clobber=YES", flush=True)
print("P1030 FORWARD C112 retained-controls PASS; original unsupported_sibling oracle RED remains, eligible case validated separately", flush=True)
