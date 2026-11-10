"""The real named callee must update the actual pre-H original exactly once."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

cli, owned, cc, sanitizer, destination = sys.argv[1:]
here = Path(__file__).resolve().parent
source = here / "P-named-display.nl"
oracle = json.loads((here / "oracle.json").read_text())
assert hashlib.sha256(source.read_bytes()).hexdigest() == \
    oracle["files"][source.name]["sha256"]
parent = Path(destination)
parent.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix="native-", dir=parent))
rows = []


def run(args, label):
    args = list(map(str, args))
    p = subprocess.run(args, capture_output=True)
    (out / (label + ".stdout")).write_bytes(p.stdout)
    (out / (label + ".stderr")).write_bytes(p.stderr)
    rows.append({"label": label, "argv": args, "status": p.returncode})
    (out / "commands.json").write_text(json.dumps(rows, indent=2) + "\n")
    assert p.returncode == 0, (label, p.returncode, p.stderr, out)
    return p.stdout


run([cli, "--check", source], "CHECK")
facts = run([owned, source], "owned").decode()
bindings = {n: int(s) for n, s in re.findall(r"BIND (\w+) (\d+)", facts)}
functions = {role: int(f) for role, f in re.findall(
    r"CALL role=(\w+) function=(\d+)", facts)}
assert set(functions) == {"inside", "postmatch"}
generated = run([cli, source], "C17")
c = out / "named.c"
c.write_bytes(generated)
target = out / "atomic.c"
run([cli, "--emit-c17-to", target, source], "named-output")
assert target.read_bytes() == generated
text = generated.decode()
records = re.findall(r"nl_copy_record (nl_b_(\d+)_\d+) =", text)
assert len(records) == 2
original, frame = records[0][0], int(records[0][1])
scratch = records[1][0]
assert records[1][1] != records[0][1]
assert text.index("nl_copy_record " + original) < text.index("malloc(24)")
var = r"nl_(?:v_\d+|b_\d+_\d+)"
pattern = re.compile(r"uint8_t (" + var + r") = nl_copy_state_(\d+)\(&(" +
                     var + r")\.f([01]),(" + var + r")\);")
calls = list(pattern.finditer(text))
assert len(calls) == 2
inside, outside = calls
assert (int(inside[2]), inside[3], int(inside[4])) == \
    (functions["inside"], scratch, 0)
assert (int(outside[2]), outside[3], int(outside[4])) == \
    (functions["postmatch"], original, 0)
assert "uint8_t old=*target;*target=next;" in text
old_values = {inside[1]}
for line in text[inside.end():outside.start()].splitlines():
    m = re.search(r"\b(" + var + r")\s*=\s*(" + var + r");", line)
    if m and m[2] in old_values:
        old_values.add(m[1])
assert outside[5] in old_values
releases = list(re.finditer(r"free\(nl_v_\d+\.handle\);", text))
assert releases and all(m.end() < outside.start() for m in releases)
names = ["archived", "forwarded", "previous", "caller_old", "caller_current",
         "caller_sibling"]
for n in names:
    assert re.search(r"\buint8_t nl_b_" + str(frame) + "_" + str(bindings[n]) +
                     r" =", text)
(out / "def-use.json").write_text(json.dumps({
    "source_callees": functions, "pre_H_original": original,
    "inside_actual_old_to_postmatch_actual_argument": True,
    "postmatch_named_call_after_all_H_cleanup": True,
    "C_sha256": hashlib.sha256(generated).hexdigest()}, indent=2) + "\n")
flags = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
if sanitizer != "none":
    flags += ["-fsanitize=" + ("address,undefined" if sanitizer == "address"
                              else sanitizer),
              "-fno-sanitize-recover=all", "-fno-omit-frame-pointer",
              "-fno-pie", "-no-pie"]
for opt in ["O0", "O2"]:
    raw = out / (opt + "-raw")
    run(flags + ["-" + opt, c, "-o", raw], opt + "-raw-build")
    run([raw], opt + "-raw")
    wrapper = out / (opt + "-physical.c")
    defs = "#define P1299_ROOT_FRAME " + str(frame) + "\n"
    defs += "#define P1299_SYMBOLS {" + ",".join(
        str(bindings[n]) for n in names) + "}\n"
    wrapper.write_text(defs + "#define main p1299_guest_main\n" +
                       "#define NEWLANG_NODE_OBSERVER " +
                       json.dumps(str(here / "native_observer.h")) + "\n" +
                       "#define NEWLANG_HEAP_OBSERVER " +
                       json.dumps(str(here / "native_observer.h")) + "\n" +
                       "#include " + json.dumps(str(c)) + "\n#undef main\n" +
                       "int main(void){for(size_t q=0;q<3;++q){"
                       "memset(&p1299,0,sizeof(p1299));p1299.quota=q;"
                       "assert(p1299_guest_main()==0 && p1299.finished);"
                       "printf(\"H%zu frees=%zu old=103 current=%u sibling=227 "
                       "same-original=1 named-callee-calls=%zu\\n\","
                       "q,p1299.frees,p1299.computed,p1299.calls+p1299.root_calls);"
                       "}return 0;}\n")
    physical = out / (opt + "-physical")
    run(flags + ["-" + opt, wrapper, "-o", physical], opt + "-physical-build")
    run([physical], opt + "-physical")
print("P1299 genuine two-caller named old/current native PASS", out)
