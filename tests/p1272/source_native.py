"""P-owned normal CLI, actual owned, C17 def-use and passive native checks."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

cli, owned, cc, sanitizer, destination = sys.argv[1:]
here = Path(__file__).resolve().parent
oracle = json.loads((here / "oracle.json").read_text())
parent = Path(destination)
parent.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix="run-", dir=parent))
rows = []


def run(args, label, expected=0):
    args = [str(x) for x in args]
    p = subprocess.run(args, capture_output=True)
    (out / (label + ".stdout")).write_bytes(p.stdout)
    (out / (label + ".stderr")).write_bytes(p.stderr)
    rows.append({"label": label, "command": args, "status": p.returncode})
    (out / "commands.json").write_text(json.dumps(rows, indent=2) + "\n")
    assert p.returncode == expected, (label, p.returncode, p.stderr.decode(errors="replace"))
    return p.stdout


flags = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
if sanitizer != "none":
    flags += ["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
for name, item in oracle["sources"].items():
    source = here / "sources" / name
    assert hashlib.sha256(source.read_bytes()).hexdigest() == item["sha256"]
    stem = source.stem
    if not item["positive"]:
        run([cli, "--check", source], stem + "-CHECK", 3)
        assert not run([cli, source], stem + "-C17", 3)
        sentinel = out / (stem + ".named.c")
        sentinel.write_bytes(b"P1272 original named output\n")
        before = sentinel.stat()
        run([cli, "--emit-c17-to", sentinel, source], stem + "-named", 3)
        after = sentinel.stat()
        assert sentinel.read_bytes() == b"P1272 original named output\n"
        assert (before.st_ino, before.st_mtime_ns, before.st_size) == (after.st_ino, after.st_mtime_ns, after.st_size)
        continue
    run([cli, "--check", source], stem + "-CHECK")
    facts = run([owned, source, str(item["byte"])], stem + "-owned").decode()
    bindings = {name: int(symbol) for name, symbol in re.findall(r"BIND (\w+) (\d+)", facts)}
    symbols = [bindings[n] for n in ["sampled_quota", "retained_quota", "pending_quota"]]
    generated = run([cli, source], stem + "-C17")
    c = out / (stem + ".c")
    c.write_bytes(generated)
    named = out / (stem + "-named.c")
    run([cli, "--emit-c17-to", named, source], stem + "-named")
    assert named.read_bytes() == generated
    text = generated.decode()
    projected = re.findall(r"const uint8_t \* (nl_v_\d+) = &(nl_v_\d+)->f1;", text)
    assert len(projected) == 1
    loads = re.findall(r"uint8_t (nl_v_\d+) = \*" + projected[0][0] + r";", text)
    assert len(loads) == 1
    # Trace actual scalar variable copies to the second H constructor. A
    # matching literal or host result alone is deliberately insufficient.
    origins = {loads[0]}
    constructors = []
    variable = r"nl_(?:v_\d+|b_\d+_\d+)"
    assignment = re.compile(r"(" + variable + r") = (" + variable + r");")
    constructor = re.compile(r"nl_node (nl_v_\d+) = \{ (nl_v_\d+), (nl_v_\d+) \};")
    for line in text.splitlines():
        match = assignment.search(line)
        if match and match[2] in origins:
            origins.add(match[1])
        match = constructor.search(line)
        if match:
            constructors.append(match[3] in origins)
    assert constructors == [False, True]
    for opt in ["O0", "O2"]:
        raw = out / (stem + "-" + opt)
        run(flags + ["-" + opt, c, "-o", raw], stem + "-" + opt + "-raw-build")
        run([raw], stem + "-" + opt + "-raw")
        wrapper = out / (stem + "-observer-" + opt + ".c")
        wrapper.write_text(
            "#define P1272_BYTE " + str(item["byte"]) + "\n"
            "#define P1272_SYMBOLS {" + ",".join(map(str, symbols)) + "}\n"
            "#define main p1272_guest_main\n"
            "#define NEWLANG_NODE_OBSERVER " + json.dumps(str(here / "native_observer.h")) + "\n"
            "#define NEWLANG_HEAP_OBSERVER " + json.dumps(str(here / "native_observer.h")) + "\n"
            "#include " + json.dumps(str(c)) + "\n#undef main\n"
            "int main(void) { for(size_t q=0;q<3;++q){memset(&p1272,0,sizeof(p1272));"
            "p1272.quota=q;assert(p1272_guest_main()==0 && p1272.finished);"
            "printf(\"physical quota=%zu frees=%zu copies=%zu byte=%u\\n\",q,p1272.frees,p1272.copies,P1272_BYTE); } return 0;}\n"
        )
        executable = out / (stem + "-physical-" + opt)
        run(flags + ["-" + opt, wrapper, "-o", executable], stem + "-" + opt + "-physical-build")
        run([executable], stem + "-" + opt + "-physical")
print("P1272 normal CLI/owned/current/loan/C17/strict raw and passive physical guest0/1/2 PASS", out)
