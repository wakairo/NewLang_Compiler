"""C127 first representative: fixed compiler, public current, exact refusal."""
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
sha = lambda b: hashlib.sha256(b).hexdigest()
commands = []


def invoke(label, args):
    result = subprocess.run(list(map(str, args)), capture_output=True, timeout=60)
    (out / (label + ".stdout")).write_bytes(result.stdout)
    (out / (label + ".stderr")).write_bytes(result.stderr)
    commands.append({"label": label, "command": list(map(str, args)), "exit": result.returncode,
                     "stdout_SHA256": sha(result.stdout), "stderr_SHA256": sha(result.stderr)})
    (out / "commands-FIRST.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode >= 0, ("signal: stop candidate", label, result.returncode)
    return result


def ok(result):
    assert result.returncode == 0 and not result.stderr, (
        result.returncode, result.stdout, result.stderr)


source = data / "sources/pair_before_heap.nl"
flags = ["-std=c17", "-Wall", "-Wextra", "-Werror", "-pedantic-errors", "-g", "-O0", "-fno-pie", "-no-pie",
         "-I" + str(repo / "include")]
libraries = [cli.parent / ("libnewlang_" + n + ".a") for n in [
    "semantic", "parser", "checked_c_node", "lexer", "checked", "syntax", "source", "diagnostic"]]
assert all(p.is_file() for p in libraries)
ok(invoke("first-source-CHECK", [cli, "--check", source]))
ok(invoke("build-public-probe", [cc, *flags, data / "probe.c", "-Wl,--start-group", *libraries,
                               "-Wl,--end-group", "-o", out / "public"]))
current = invoke("first-public-current", [out / "public", source]); ok(current)
assert current.stdout.count(b"body=0 parameters=0 event=0") == 2
emitted = invoke("first-C17", [cli, source])
assert emitted.returncode == 4 and not emitted.stdout
assert b"V1-BACKEND-UNSUPPORTED" in emitted.stderr
target = out / "existing-output.bin"
sentinel = b"P1030 preserve original complete named output\n"
target.write_bytes(sentinel); before = target.stat()
named = invoke("first-named", [cli, "--emit-c17-to", target, source])
assert named.returncode == 4 and not named.stdout
after = target.stat()
assert target.read_bytes() == sentinel and (before.st_dev, before.st_ino) == (after.st_dev, after.st_ino)
assert not list(out.glob("*.tmp*"))

# Only the unchanged original emitter TU is instrumented. No proof/node is
# repaired; the first actual reject's return PC identifies its private guard.
ok(invoke("build-original-instrumented-emitter", [cc, *flags, "-finstrument-functions", "-c",
          repo / "src/checked_c_node.c", "-o", out / "original-node.o"]))
ok(invoke("build-instrumented-probe", [cc, *flags, data / "probe.c", out / "original-node.o",
          "-Wl,--start-group", *libraries, "-Wl,--end-group", "-o", out / "instrumented"]))
symbols = invoke("debug-symbols", ["nm", "-n", out / "instrumented"]); ok(symbols)
reject = None
for line in symbols.stdout.decode().splitlines():
    fields = line.split()
    if len(fields) != 3 or fields[2] != "reject":
        continue
    mapped = invoke("reject-map-" + fields[0], ["addr2line", "-f", "-e", out / "instrumented", fields[0]])
    ok(mapped)
    if str(repo / "src/checked_c_node.c").encode() in mapped.stdout:
        reject = fields[0]
        break
assert reject
trace = invoke("first-original-reject", [out / "instrumented", source, reject]); ok(trace)
pc = re.search(rb"FIRST_PRIVATE_REJECT return_address=(0x[0-9a-f]+)", trace.stdout)
assert pc
address = pc[1].decode()
mapped = invoke("first-reject-location", ["addr2line", "-f", "-e", out / "instrumented",
                                       address, hex(int(address, 16) - 1)])
ok(mapped)
result = {"source": str(source.relative_to(repo)), "source_SHA256": sha(source.read_bytes()),
          "compiler_SHA256": sha(cli.read_bytes()), "CHECK": 0,
          "public_body_parameters_event_current": [0, 0], "CLI_C17": 4,
          "C_published": False, "named_bytes_dev_ino_no_temp": "preserved",
          "first_original_reject_return_PC": address,
          "first_original_reject_location": mapped.stdout.decode().splitlines(),
          "no_production_edit": True, "R_input_access": False}
(out / "result-FIRST.json").write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps(result, indent=2), flush=True)
