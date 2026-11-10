"""Strict unmodified C first; then observe actual surviving T/H0/1/2."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

cli, owned, prefix, cc, sanitizer, destination = sys.argv[1:]
here = Path(__file__).resolve().parent
oracle = json.loads((here / "oracle.json").read_text())
source = here / "P-surviving-state.nl"
assert hashlib.sha256(source.read_bytes()).hexdigest() == \
    oracle["files"][source.name]["sha256"]
parent = Path(destination)
parent.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix="native-", dir=parent))
rows = []


def run(args, label):
    args = list(map(str, args))
    result = subprocess.run(args, capture_output=True)
    (out / (label + ".stdout")).write_bytes(result.stdout)
    (out / (label + ".stderr")).write_bytes(result.stderr)
    rows.append({"label": label, "argv": args, "status": result.returncode})
    (out / "commands.json").write_text(json.dumps(rows, indent=2) + "\n")
    assert result.returncode == 0, (label, result.returncode, result.stderr, out)
    return result.stdout


run([cli, "--check", source], "CHECK")
facts = run([owned, source, 71, 163, 179, 1], "owned").decode()
run([prefix, source], "prefix")
bindings = {n: int(s) for n, s in re.findall(r"BIND (\w+) (\d+)", facts)}
generated = run([cli, source], "C17")
c = out / "positive.c"
c.write_bytes(generated)
named = out / "named.c"
run([cli, "--emit-c17-to", named, source], "named")
assert named.read_bytes() == generated
text = generated.decode()
records = re.findall(r"nl_copy_record (nl_b_(\d+)_\d+) =", text)
assert len(records) == 2
retained, frame = records[0][0], int(records[0][1])
assert records[1][1] != records[0][1]
names = ["computed", "supplied", "previous_mark", "actual_current",
         "observed_sibling", "saved_old"]
for name in names:
    assert re.search(r"\buint8_t nl_b_" + str(frame) + "_" +
                     str(bindings[name]) + r" =", text)
assert text.index("nl_copy_record " + retained) < text.index("malloc(24)")
store = re.search(re.escape(retained) + r"\.f1=(nl_v_\d+);", text)
releases = list(re.finditer(r"free\(nl_v_\d+\.handle\);", text))
assert store and releases and all(m.end() < store.start() for m in releases)
incoming = "nl_b_" + str(frame) + "_" + str(bindings["supplied"])
assert re.search(r"uint8_t " + store[1] + r" = " + incoming + ";", text)
assert "uint8_t old=*target;*target=next;" in text
(out / "def-use.json").write_text(json.dumps({
    "pre_H_original": retained, "runtime_incoming_binding": incoming,
    "postmatch_source_store": store[0], "records_distinct": True,
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
    definitions = "#define P1295_ROOT_FRAME " + str(frame) + "\n"
    definitions += "#define P1295_SYMBOLS {" + ",".join(
        str(bindings[n]) for n in names) + "}\n"
    wrapper.write_text(definitions + "#define main p1295_guest_main\n" +
                       "#define NEWLANG_NODE_OBSERVER " +
                       json.dumps(str(here / "native_observer.h")) + "\n" +
                       "#define NEWLANG_HEAP_OBSERVER " +
                       json.dumps(str(here / "native_observer.h")) + "\n" +
                       "#include " + json.dumps(str(c)) + "\n#undef main\n" +
                       "int main(void){for(size_t q=0;q<3;++q){"
                       "memset(&p1295,0,sizeof(p1295));p1295.quota=q;"
                       "assert(p1295_guest_main()==0 && p1295.finished);"
                       "printf(\"H%zu frees=%zu old=101 current=%u sibling=213 "
                       "same-pre-H-original=1\\n\",q,p1295.frees,p1295.computed);"
                       "}return 0;}\n")
    physical = out / (opt + "-physical")
    run(flags + ["-" + opt, wrapper, "-o", physical], opt + "-physical-build")
    run([physical], opt + "-physical")
print("P1295 actual postmatch original/current/sibling PASS", out)
