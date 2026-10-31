"""C118: actual source/current -> original C17, live T observation and faults."""
from pathlib import Path
import hashlib
import json
import os
import resource
import signal
import subprocess
import sys
import tempfile

cli, cc, sanitizer = sys.argv[1:]
cli = Path(cli).resolve()
data = Path(__file__).resolve().parent
repo = data.parents[1]
original = json.loads((data / "oracle-v1.json").read_bytes())
oracle = json.loads((data / "oracle-v2.json").read_bytes())
sha = lambda b: hashlib.sha256(b).hexdigest()
commands = []
evidence = os.environ.get("P1020_EVIDENCE_DIR")
archive = Path(tempfile.mkdtemp(prefix="P1020-", dir=evidence)) if evidence else None


def invoke(label, args, **kw):
    result = subprocess.run(list(map(str, args)), capture_output=True, timeout=240, **kw)
    commands.append({"label": label, "command": list(map(str, args)), "exit": result.returncode,
                     "stdout_sha256": sha(result.stdout), "stderr_sha256": sha(result.stderr)})
    if archive:
        (archive / (label + ".stdout")).write_bytes(result.stdout)
        (archive / (label + ".stderr")).write_bytes(result.stderr)
        (archive / "commands-FIRST.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode >= 0, ("signal: stop candidate", label, result.returncode,
                                    result.stdout, result.stderr)
    return result


def ok(result):
    assert result.returncode == 0 and not result.stderr, (
        result.returncode, result.stdout, result.stderr)


def quota():
    signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
    resource.setrlimit(resource.RLIMIT_FSIZE, (128, 128))


def preserved(path, sentinel, before):
    after = path.stat()
    assert path.read_bytes() == sentinel
    assert (before.st_dev, before.st_ino) == (after.st_dev, after.st_ino)
    assert not list(path.parent.glob("*.tmp*"))


flags = ["-std=c17", "-Wall", "-Wextra", "-Werror", "-pedantic-errors"]
if sanitizer != "none":
    flags += ["-fsanitize=" + sanitizer, "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]

with tempfile.TemporaryDirectory(prefix="p1020-") as directory:
    temp = Path(directory)
    libraries = [cli.parent / ("libnewlang_" + n + ".a")
                 for n in ["semantic", "parser", "checked_c_node", "lexer", "checked", "syntax", "source", "diagnostic"]]
    assert all(p.is_file() for p in libraries)
    for name in ["probe", "oom"]:
        wraps = ["-Wl,--wrap=" + n for n in ["malloc", "calloc", "realloc"]] if name == "oom" else []
        ok(invoke("build-" + name, [cc, *flags, "-g", "-O0", "-fno-pie", "-no-pie",
                  "-I" + str(repo / "include"), data / (name + ".c"),
                  "-Wl,--start-group", *libraries, "-Wl,--end-group", *wraps, "-o", temp / name]))
    for name, expected in original["positive"].items():
        source = data / "sources" / name
        label = source.stem
        ok(invoke(label + "-check", [cli, "--check", source]))
        proof = invoke(label + "-public-current", [temp / "probe", "positive", source])
        ok(proof)
        assert b"post=UNKNOWN unit=0 parameters=0" in proof.stdout
        assert proof.stdout.count(b"P1020 ARM") == (2 if expected["two_normal"] else 0)
        print(proof.stdout.decode(), end="", flush=True)
        result = invoke(label + "-emit", [cli, source]); ok(result)
        assert result.stdout
        c_sha = sha(result.stdout)
        if name in oracle["unchanged_parent_C"]:
            assert c_sha == oracle["unchanged_parent_C"][name]
        emitted = temp / (label + ".c"); emitted.write_bytes(result.stdout)
        if archive:
            (archive / emitted.name).write_bytes(result.stdout)
        named = temp / "named.c"
        sentinel = b"P1020 previous complete named output\n"
        named.write_bytes(sentinel)
        ok(invoke(label + "-named", [cli, "--emit-c17-to", named, source]))
        assert named.read_bytes() == result.stdout
        named.write_bytes(sentinel); before = named.stat()
        failed = invoke(label + "-reached-output-error", [cli, "--emit-c17-to", named, source],
                        preexec_fn=quota)
        assert failed.returncode == 1 and not failed.stdout and b"OUTPUT" in failed.stderr
        preserved(named, sentinel, before)
        ok(invoke(label + "-exact-retry", [cli, "--emit-c17-to", named, source]))
        assert named.read_bytes() == result.stdout
        for opt in ["-O0", "-O2"]:
            raw = temp / "raw"
            ok(invoke(label + opt + "-raw-build", [cc, *flags, opt, emitted, "-o", raw]))
            executed = invoke(label + opt + "-raw", [raw]); ok(executed)
            assert not executed.stdout
            print("P1020 RAW", name, cc, opt, sanitizer, c_sha,
                  "physical=INCONCLUSIVE(unit-exit-only)", flush=True)
            if not expected["two_normal"]:
                continue
            physical = temp / "physical"
            definitions = ["-DP1020_" + k.upper() + "=" + str(expected[k])
                           for k in ["index", "old", "new", "sibling"]]
            ok(invoke(label + opt + "-physical-build", [cc, *flags, opt, emitted,
                      '-DNEWLANG_UNIT_CALL_OBSERVER="' + str(data / "observer.h") + '"',
                      *definitions, "-o", physical]))
            observed = invoke(label + opt + "-physical", [physical]); ok(observed)
            text = observed.stdout.decode().strip()
            assert f"old={expected['old']} new={expected['new']} sibling={expected['sibling']}" in text
            assert "changes=1 reads=2" in text
            assert "same_original=YES live_original_reads=YES" in text
            print(json.dumps({"P1020": "physical", "source": name, "cc": cc,
                  "opt": opt, "sanitizer": sanitizer, "C_sha256": c_sha,
                  "observations": text}), flush=True)
        if expected["two_normal"]:
            rejected = invoke(label + "-metadata-foreign-owner", [temp / "probe", "negative", source])
            ok(rejected)
            assert rejected.stdout.count(b"P1020 METADATA REJECT") == 16
            assert rejected.stdout.count(b"P1020 VIEW REJECT") == 5
            assert b"A_destroyed_unrestored=YES B_current_and_destroy=YES" in rejected.stdout
            print(rejected.stdout.decode(), end="", flush=True)
        print("P1020 NAMED bytes/dev/ino/no-temp/reached-error/exact-retry", name, flush=True)
    for relative, expected in oracle["source_rejections"].items():
        source = data / relative
        label = source.stem
        checked = invoke(label + "-negative-check", [cli, "--check", source])
        assert checked.returncode == expected["CHECK"] and not checked.stdout
        assert expected["diagnostic"].encode() in checked.stderr if expected["CHECK"] else not checked.stderr
        sentinel = b"P1020 negative existing output must survive\n"
        named = temp / "negative-existing.c"; named.write_bytes(sentinel); before = named.stat()
        for mode in ["stdout", "fresh", "existing"]:
            target = temp / "negative-never.c" if mode == "fresh" else named
            args = [cli] + ([] if mode == "stdout" else ["--emit-c17-to", target]) + [source]
            failed = invoke(label + "-negative-" + mode, args)
            assert failed.returncode == expected["C17"] and not failed.stdout
            if mode == "fresh":
                assert not target.exists()
            preserved(named, sentinel, before)
        print("P1020 SOURCE REJECTION", relative, expected, "no_clobber=YES", flush=True)
    for name in oracle["OOM_sources"]:
        reached = invoke(Path(name).stem + "-reached-OOM", [temp / "oom", data / "sources" / name])
        ok(reached)
        assert reached.stdout.count(b"P1020 OOM SUMMARY") == 3
        print(reached.stdout.decode(), end="", flush=True)
print("P1020 C118 finite source/current/C17/physical/negative/OOM/named PASS", flush=True)
