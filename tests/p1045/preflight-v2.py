"""C137 intermediate C17 admission only; native/physical is NOT TESTED here."""
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
oracle = json.loads((data / "preflight-oracle-v2.json").read_bytes())
commands = []
sha = lambda b: hashlib.sha256(b).hexdigest()


def run(label, args):
    args = list(map(str, args))
    r = subprocess.run(args, capture_output=True, timeout=90)
    (out / (label + ".stdout")).write_bytes(r.stdout)
    (out / (label + ".stderr")).write_bytes(r.stderr)
    commands.append({"label": label, "command": args, "exit": r.returncode,
                     "stdout_SHA256": sha(r.stdout), "stderr_SHA256": sha(r.stderr)})
    (out / "commands-FIRST.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert r.returncode >= 0, (label, "signal, stop candidate", r.returncode)
    return r


def ok(r):
    assert r.returncode == 0 and not r.stderr, (r.returncode, r.stdout, r.stderr)


flags = ["-std=c17", "-Wall", "-Wextra", "-Werror", "-pedantic-errors", "-g", "-O0",
         "-fno-pie", "-no-pie", "-I" + str(repo / "include")]
libraries = [cli.parent / ("libnewlang_" + n + ".a") for n in [
    "semantic", "parser", "checked_c_node", "lexer", "checked", "syntax", "source", "diagnostic"]]
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
        location = run("reject-symbol-" + fields[0], ["addr2line", "-f", "-e", out / "instrumented", fields[0]])
        ok(location)
        if str(repo / "src/checked_c_node.c").encode() in location.stdout:
            reject = fields[0]
            break
assert reject
for name, expected in oracle["C17"].items():
    source = data / "sources-v2" / name
    label = source.stem
    ok(run(label + "-CHECK", [cli, "--check", source]))
    current = run(label + "-current", [out / "public", source]); ok(current)
    assert current.stdout.count(b"body=0 parameters=0 event=0 GET=YES") == 2
    emitted = run(label + "-C17", [cli, source])
    assert emitted.returncode == expected, (name, emitted.returncode, emitted.stderr)
    record = {"source": name, "CHECK": 0, "current": [0, 0], "CLI_C17": expected,
              "C_SHA256": sha(emitted.stdout), "physical": "NOT TESTED"}
    if expected:
        assert not emitted.stdout and b"V1-BACKEND-UNSUPPORTED" in emitted.stderr
        traced = run(label + "-first-private-reject", [out / "instrumented", source, reject]); ok(traced)
        pc = re.search(rb"FIRST_PRIVATE_REJECT return_address=(0x[0-9a-f]+)", traced.stdout)
        assert pc
        mapped = run(label + "-first-location", ["addr2line", "-f", "-e", out / "instrumented", hex(int(pc[1], 16) - 1)])
        ok(mapped)
        record["first_private_reject"] = mapped.stdout.decode().splitlines()
    else:
        assert emitted.stdout and not emitted.stderr
        (out / (label + ".c.data")).write_bytes(emitted.stdout)
    print(json.dumps(record), flush=True)
print("P1045 intermediate authentic-current/C17 preflight DONE; native NOT TESTED", flush=True)
