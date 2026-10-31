"""C137 literal pinned-parent baseline, original actual source + public current."""
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
results = []


def invoke(label, args):
    args = list(map(str, args))
    r = subprocess.run(args, capture_output=True, timeout=90)
    (out / (label + ".stdout")).write_bytes(r.stdout)
    (out / (label + ".stderr")).write_bytes(r.stderr)
    commands.append({"label": label, "command": args, "exit": r.returncode,
                     "stdout_SHA256": sha(r.stdout), "stderr_SHA256": sha(r.stderr)})
    (out / "commands-FIRST.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert r.returncode >= 0, ("signal, stop candidate", label, r.returncode)
    return r


def ok(r):
    assert r.returncode == 0 and not r.stderr, (r.returncode, r.stdout, r.stderr)


flags = ["-std=c17", "-Wall", "-Wextra", "-Werror", "-pedantic-errors", "-g", "-O0",
         "-fno-pie", "-no-pie", "-I" + str(repo / "include")]
libraries = [cli.parent / ("libnewlang_" + n + ".a") for n in [
    "semantic", "parser", "checked_c_node", "lexer", "checked", "syntax", "source", "diagnostic"]]
assert all(p.is_file() for p in libraries)
ok(invoke("build-public-probe", [cc, *flags, data / "probe.c", "-Wl,--start-group",
          *libraries, "-Wl,--end-group", "-o", out / "public"]))
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
    mapped = invoke("reject-map-" + fields[0],
                    ["addr2line", "-f", "-e", out / "instrumented", fields[0]])
    ok(mapped)
    if str(repo / "src/checked_c_node.c").encode() in mapped.stdout:
        reject = fields[0]
        break
assert reject
for name, expected in oracle["profiles"].items():
    source = data / "sources" / name
    label = source.stem
    checked = invoke(label + "-CHECK", [cli, "--check", source]); ok(checked)
    public = invoke(label + "-public-current", [out / "public", source]); ok(public)
    assert public.stdout.count(b"body=0 parameters=0 event=0 GET=YES") == 2
    emitted = invoke(label + "-C17", [cli, source])
    assert emitted.returncode == expected["baseline_C17"] and not emitted.stdout
    assert b"V1-BACKEND-UNSUPPORTED" in emitted.stderr
    named = out / "existing-output.bin"
    sentinel = b"P1045 preserve original named bytes\n"
    named.write_bytes(sentinel); before = named.stat()
    result = invoke(label + "-named", [cli, "--emit-c17-to", named, source])
    assert result.returncode == 4 and not result.stdout
    after = named.stat()
    assert named.read_bytes() == sentinel and (before.st_dev, before.st_ino) == (after.st_dev, after.st_ino)
    assert not list(out.glob("*.tmp*"))
    traced = invoke(label + "-first-private-reject", [out / "instrumented", source, reject]); ok(traced)
    pc = re.search(rb"FIRST_PRIVATE_REJECT return_address=(0x[0-9a-f]+)", traced.stdout)
    assert pc
    address = pc[1].decode()
    location = invoke(label + "-first-location", ["addr2line", "-f", "-e", out / "instrumented",
                      address, hex(int(address, 16) - 1)]); ok(location)
    record = {"source": str(source.relative_to(repo)), "source_SHA256": sha(source.read_bytes()),
              "compiler_SHA256": sha(cli.read_bytes()), "CHECK": 0,
              "public_body_parameters_event_current": [0, 0], "CLI_C17": emitted.returncode,
              "C_published": False, "named_bytes_dev_ino_no_temp": "preserved",
              "first_original_reject_return_PC": address,
              "first_original_reject_location": location.stdout.decode().splitlines(),
              "R_input_access": False}
    results.append(record)
    (out / "results-FIRST.json").write_text(json.dumps(results, indent=2) + "\n")
    print(json.dumps(record), flush=True)
for name, expected in oracle["negatives"].items():
    source = data / "sources" / name
    for mode, args in [("CHECK", [cli, "--check", source]), ("C17", [cli, source])]:
        r = invoke(source.stem + "-" + mode, args)
        assert r.returncode == expected[mode] and not r.stdout
        if expected[mode] == 4:
            assert b"V1-BACKEND-UNSUPPORTED" in r.stderr
        print("P1045 SOURCE NEGATIVE", name, mode, r.returncode, r.stderr.decode().strip(), flush=True)
print("P1045 pinned parent baseline DONE; not candidate native/R evidence", flush=True)
