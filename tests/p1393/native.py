"""Actual emitted C: raw exit evidence and separate read-only observations.

The local matrix has 26 guests; normal CTest uses this host's seven guests.
No generated application byte is edited. Expected checked IDs are obtained
from the existing public checked-only probe, not injected into source CHECK.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("compiler", type=Path)
parser.add_argument("probe", type=Path)
parser.add_argument("directory", type=Path)
parser.add_argument("cc")
parser.add_argument("sanitizer")
parser.add_argument("--matrix", action="store_true")
parser.add_argument("--clang")
parser.add_argument("--output", type=Path)
args = parser.parse_args()
repo = args.directory.resolve().parents[1]
temporary = None
if args.output is None:
    temporary = tempfile.TemporaryDirectory(prefix="p1393-native-")
    root = Path(temporary.name)
else:
    root = args.output.resolve()
    root.mkdir()


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(label, command, env=None):
    (root / (label + ".attempt.json")).write_text(json.dumps(command) + "\n")
    p = subprocess.run(command, capture_output=True, env=env)
    (root / (label + ".stdout")).write_bytes(p.stdout)
    (root / (label + ".stderr")).write_bytes(p.stderr)
    assert p.returncode == 0, (label, p.returncode, p.stderr.decode())
    return p


source = args.directory / "sources/P1-C-free-current-tail-read-v2.nl"
assert sha(source) == "105fc934021586050ad875d05f251c65a30de70c34f330913bb1738022be11b4"
checked = run("checked", [str(args.probe), str(source)])
rows = {}
for line in checked.stdout.decode().splitlines():
    kind, *values = line.split()
    rows.setdefault(kind, []).append(list(map(int, values)))
assert {k: len(v) for k, v in rows.items()} == {
    "INIT": 5, "ROOT": 11, "FIELD": 11, "CHANGE": 9, "READ": 2}
assert all(len({v[i] for v in rows["INIT"]}) == 5 for i in range(4))
header = "".join(
    "static const size_t expected_" + k.lower() + "[" + str(len(v)) + "][" +
    str(len(v[0])) + "]={" +
    ",".join("{" + ",".join(map(str, row)) + "}" for row in v) + "};\n"
    for k, v in rows.items())
(root / "five_expect.h").write_text(header)
emitted = run("emit", [str(args.compiler), str(source)])
assert not emitted.stderr
code = emitted.stdout.decode()
assert code.count("=malloc(56);") == 5
assert len(re.findall(r"^\*nl_v_\d+=nl_v_\d+;", code, re.M)) == 9
calls = re.findall(r"NL_FIVE_ROOT\(nl_v_(\d+),nl_v_(\d+)->token,(\d+),(\d+),(\d+),(\d+)\)", code)
assert len(calls) == 11
for expected, call in zip(rows["ROOT"], calls):
    pointer, stable, r, inc, domain, scope, write = expected
    pv, sv, w, cr, ci, cs = map(int, call)
    assert (w, cr, ci, cs) == (write, r, inc, scope)
    assert re.search(r"nl_v_" + str(pv) + r"=nl_b_\d+_" + str(pointer) + ";", code)
    assert re.search(r"nl_v_" + str(sv) + r"=nl_b_\d+_" + str(stable) + ";", code)
# Exact emitted load -> forwarded loan -> binding Copy -> Some binder ->
# actual B ref -> second load. C temporaries and frame-qualified bindings are
# globally unique in this finite emitter; only preceding definitions count.
read_calls=re.findall(r"NL_FIVE_READ\(nl_v_(\d+),&nl_v_(\d+),(\d+),(\d+),(\d+),(\d+)\)",code)
assert len(read_calls)==2
for row,call in zip(rows["READ"],read_calls):
    field,result,r,index,fact,occ=map(int,call)
    assert (r,index,fact,occ)==(row[0],row[1],row[3],row[4])
    assert f"nl_option nl_v_{result}=*nl_v_{field};" in code

def origin(name,limit,seen=None):
    seen=set() if seen is None else seen
    assert name not in seen;seen.add(name)
    prefix=code[:limit]
    definitions=list(re.finditer(r"(?<![A-Za-z0-9_])"+re.escape(name)+r"=(nl_[vb]_\d+(?:_\d+)?)(\.ptr)?;",prefix))
    if not definitions:return name
    d=definitions[-1]
    base=origin(d[1],d.start(),seen)
    return ("payload",base) if d[2] else base

some=re.search(r"const nl_node \*(nl_b_\d+_\d+)=(nl_v_\d+)\.ptr;",code)
assert some
assert origin(some[2],some.start())=="nl_v_"+read_calls[0][1]
b_pointer="nl_v_"+calls[10][0]
b_root_hook=code.index("NL_FIVE_ROOT("+b_pointer+",")
assert origin(b_pointer,b_root_hook)==("payload","nl_v_"+read_calls[0][1])
assert code.rfind("free(",0,code.index("NL_FIVE_READ("))>=0
(root/"static-def-use.json").write_text(json.dumps({"read_hooks":read_calls,"Some_binder":some[1],"B_source_pointer":b_pointer,"authentic_chain":True})+"\n")
raw = root / "P1-raw.c"
raw.write_bytes(emitted.stdout)
original_C_hash = sha(raw)
records = []
(root / "native-input-freeze.json").write_text(json.dumps({
    "source_SHA256": sha(source), "raw_C_SHA256": original_C_hash,
    "checked_rows": rows, "checked_header_SHA256": sha(root / "five_expect.h"),
    "observer_SHA256": sha(args.directory / "observer.h"),
    "observer_memory_SHA256": sha(args.directory / "observer_memory.c"),
    "platform_SHA256": sha(repo / "tests/integration/five_platform.c"),
    "script_SHA256": sha(Path(__file__)),
    "plan": "local matrix26 or configured host7; each command logged before build/run",
    "source_values_never_injected": True,
    "status": "frozen before first application build/run"
}, indent=2) + "\n")


def native(label, cc, opt, observed, worlds, sanitizer="none", path=raw):
    command = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", opt]
    if sanitizer != "none":
        command += ["-g", "-fsanitize=" + sanitizer, "-fno-sanitize-recover=all",
                    "-fno-omit-frame-pointer"]
    if observed:
        command += ['-DNEWLANG_FIVE_OBSERVER="' +
                    str(args.directory.resolve() / "observer.h") + '"', "-I" + str(root)]
    else:
        command += ["-DNEWLANG_UNOBSERVED"]
    exe = root / label
    command += [str(path), str(repo / "tests/integration/five_platform.c")]
    if observed:
        command += [str(args.directory / "observer_memory.c")]
    command += ["-Wl,--wrap=malloc", "-Wl,--wrap=free", "-o", str(exe)]
    run(label + "-build", command)
    for failure in worlds:
        assert len(records) < 32
        name = label + ("-success" if failure is None else "-fail" + str(failure))
        env = dict(os.environ)
        env.pop("NEWLANG_TEST_FAIL_CALL", None)
        if failure is not None:
            env["NEWLANG_TEST_FAIL_CALL"] = str(failure)
        env.update(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        attempt = {"ordinal": len(records) + 1, "label": name,
                   "binary_SHA256": sha(exe), "source_C_SHA256": sha(path),
                   "observed": observed, "failed_site": failure,
                   "sanitizer": sanitizer}
        (root / (name + ".guest-start.json")).write_text(json.dumps(attempt) + "\n")
        # Every attempted application execution counts, including a failure.
        records.append(attempt)
        (root / "guest-ledger.json").write_text(json.dumps(records, indent=2) + "\n")
        result = run(name, [str(exe)], env)
        assert not result.stderr
        if observed:
            live = 5 if failure is None else failure - 1
            changes = 9 if failure is None else 0
            trace = result.stdout.decode()
            assert re.search(r"OBSERVED trials=\d+ allocations=" + str(live) +
                             " frees=" + str(live) + " changes=" + str(changes), trace)
            assert len(re.findall(r"^CHANGE ", trace, re.M)) == changes
            assert len(re.findall(r"^END ", trace, re.M)) == live
            assert len(re.findall(r"^READ ",trace,re.M)) == (2 if failure is None else 0)
            assert len(re.findall(r"^SOME ",trace,re.M)) == (1 if failure is None else 0)
        else:
            assert not result.stdout
        attempt.update(status=0, stdout_SHA256=hashlib.sha256(result.stdout).hexdigest(),
                       stderr_SHA256=hashlib.sha256(result.stderr).hexdigest())
        (root / "guest-ledger.json").write_text(json.dumps(records, indent=2) + "\n")


all_worlds = [1, 2, 3, 4, 5, None]
if args.matrix:
    assert args.clang and args.sanitizer == "none"
    native("gcc-raw-O0", args.cc, "-O0", False, all_worlds)
    native("gcc-passive-O0", args.cc, "-O0", True, all_worlds)
    for ccname, cc in [("gcc", args.cc), ("clang", args.clang)]:
        for opt in ["-O0", "-O2"]:
            if ccname == "gcc" and opt == "-O0":
                continue
            native(ccname + "-raw" + opt, cc, opt, False, [None])
            native(ccname + "-passive" + opt, cc, opt, True, [None])
    native("asan-passive-O0", args.clang, "-O0", True, all_worlds, "address")
    native("ubsan-passive-O0", args.clang, "-O0", True, [None], "undefined")
    kept = run("kept-nine-emit", [str(args.compiler), str(repo / "tests/p1387/sources/U-proposed-nine.nl")])
    six = root / "U-raw.c"
    six.write_bytes(kept.stdout)
    native("kept-nine-raw-O2", args.cc, "-O2", False, [None], path=six)
    assert len(records) == 26
else:
    native("host-raw-O0", args.cc, "-O0", False, [None], args.sanitizer)
    native("host-passive-O0", args.cc, "-O0", True, all_worlds, args.sanitizer)
    assert len(records) == 7
assert sha(raw) == original_C_hash
print(json.dumps({"source_SHA256": sha(source), "raw_C_SHA256": original_C_hash,
                  "genuine_checked_rows": rows, "guests": records,
                  "raw_unmodified": True, "no_program_value_injection": True,
                  "raw_dynamic_reads": "NOT_OBSERVED: legal optimization may eliminate unused reads",
                  "passive_memory": "instrumented operands/results plus separate-TU live A/B reads; not raw-machine timing"}, indent=2))
if temporary is not None:
    temporary.cleanup()
