"""C137 exact PR1031 baseline; source/current first, native not yet measured."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

cli, cc, directory = sys.argv[1:]
cli = Path(cli).resolve()
data = Path(__file__).resolve().parent
repo = data.parents[1]
out = Path(directory)
out.mkdir(parents=True, exist_ok=False)
oracle = json.loads((data / "oracle-v1.json").read_bytes())
sha = lambda b: hashlib.sha256(b).hexdigest()
commands = []


def run(label, args):
    args = list(map(str, args))
    r = subprocess.run(args, capture_output=True, timeout=90)
    (out / (label + ".stdout")).write_bytes(r.stdout)
    (out / (label + ".stderr")).write_bytes(r.stderr)
    commands.append({"label": label, "command": args, "exit": r.returncode,
                     "stdout_SHA256": sha(r.stdout), "stderr_SHA256": sha(r.stderr)})
    (out / "commands-FIRST.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert r.returncode >= 0, (label, "unexpected signal, stop candidate", r.returncode)
    return r


def ok(r):
    assert r.returncode == 0 and not r.stderr, (r.returncode, r.stdout, r.stderr)


flags = ["-std=c17", "-Wall", "-Wextra", "-Werror", "-pedantic-errors", "-g", "-O0",
         "-fno-pie", "-no-pie", "-I" + str(repo / "include")]
libraries = [cli.parent / ("libnewlang_" + n + ".a") for n in [
    "semantic", "parser", "checked_c_node", "lexer", "checked", "syntax", "source", "diagnostic"]]
assert all(p.is_file() for p in libraries)
ok(run("build-public", [cc, *flags, data / "probe.c", "-Wl,--start-group", *libraries,
                      "-Wl,--end-group", "-o", out / "public"]))
ok(run("build-original-instrumented-emitter", [cc, *flags, "-finstrument-functions", "-c",
          repo / "src/checked_c_node.c", "-o", out / "node.o"]))
ok(run("build-instrumented", [cc, *flags, data / "probe.c", out / "node.o", "-Wl,--start-group",
          *libraries, "-Wl,--end-group", "-o", out / "instrumented"]))
symbols = run("symbols", ["nm", "-n", out / "instrumented"]); ok(symbols)
reject = None
for line in symbols.stdout.decode().splitlines():
    fields = line.split()
    if len(fields) == 3 and fields[2] == "reject":
        loc = run("reject-symbol-" + fields[0],
                  ["addr2line", "-f", "-e", out / "instrumented", fields[0]])
        ok(loc)
        if str(repo / "src/checked_c_node.c").encode() in loc.stdout:
            reject = fields[0]
            break
assert reject
results = []
for name, expected in oracle["positive"].items():
    source = data / "sources" / name
    label = source.stem
    ok(run(label + "-CHECK", [cli, "--check", source]))
    current = run(label + "-public-current", [out / "public", source, expected["first_index"]]); ok(current)
    assert current.stdout.count(b"body=0 parameters=0 event=0 GET=YES") == 3
    emitted = run(label + "-C17", [cli, source])
    assert emitted.returncode == 4 and not emitted.stdout and b"V1-BACKEND-UNSUPPORTED" in emitted.stderr
    target = out / "existing-output.bin"
    sentinel = b"P1036 preserve original named output\n"
    target.write_bytes(sentinel); before = target.stat()
    r = run(label + "-named", [cli, "--emit-c17-to", target, source])
    after = target.stat()
    assert r.returncode == 4 and not r.stdout and target.read_bytes() == sentinel
    assert (before.st_dev, before.st_ino) == (after.st_dev, after.st_ino)
    assert not list(out.glob("*.tmp*"))
    traced = run(label + "-first-private-reject",
                 [out / "instrumented", source, expected["first_index"], reject]); ok(traced)
    pc = re.search(rb"FIRST_PRIVATE_REJECT return_address=(0x[0-9a-f]+)", traced.stdout)
    assert pc
    loc = run(label + "-first-location", ["addr2line", "-f", "-e", out / "instrumented",
                                         hex(int(pc[1], 16) - 1)]); ok(loc)
    result = {"source": str(source.relative_to(repo)), "source_SHA256": sha(source.read_bytes()),
              "compiler_SHA256": sha(cli.read_bytes()), "CHECK": 0, "GET_actual": 3,
              "body_parameters_event_current": [0, 0, 0], "CLI_C17": 4, "no_C": True,
              "named_bytes_dev_ino_no_temp": "preserved", "first_reject_PC": pc[1].decode(),
              "first_private_reject": loc.stdout.decode().splitlines(), "native": "NOT TESTED"}
    results.append(result)
    (out / "results-FIRST.json").write_text(json.dumps(results, indent=2) + "\n")
    print(json.dumps(result), flush=True)
for name, expected in oracle["negative_sources"].items():
    source = data / "sources" / name
    for mode, args in [("CHECK", [cli, "--check", source]), ("C17", [cli, source])]:
        r = run(source.stem + "-" + mode, args)
        assert r.returncode == expected[mode] and not r.stdout, (name, mode, r.returncode, r.stderr)
        print("P1036 SOURCE REFUSAL", name, mode, r.returncode, r.stderr.decode().strip(), flush=True)
print("P1036 literal pinned-parent source/current/C17 baseline DONE; native/R NOT TESTED", flush=True)
