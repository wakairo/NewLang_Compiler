"""Actual unmodified CLI C17, then raw and passive strict native runs."""
import argparse
import hashlib
import json
import pathlib
import re
import subprocess
import tempfile

p = argparse.ArgumentParser()
p.add_argument("compiler")
p.add_argument("cc")
p.add_argument("source", type=pathlib.Path)
p.add_argument("output", type=pathlib.Path)
p.add_argument("sanitizer", choices=["none", "address", "undefined"])
p.add_argument("--raw", type=pathlib.Path)
p.add_argument("--only")
p.add_argument("--opt", choices=["0", "2"])
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
out = pathlib.Path(tempfile.mkdtemp(prefix="attempt-", dir=a.output))
oracle = json.loads((a.source / "oracle.json").read_text())["forward"]
receipt = []

def run(label, command, guest=False):
    r = subprocess.run([str(x) for x in command], capture_output=True)
    (out / (label + ".stdout")).write_bytes(r.stdout)
    (out / (label + ".stderr")).write_bytes(r.stderr)
    item = {"label": label, "argv": [str(x) for x in command],
            "status": r.returncode, "guest": guest,
            "stdout_sha256": hashlib.sha256(r.stdout).hexdigest(),
            "stderr_sha256": hashlib.sha256(r.stderr).hexdigest()}
    receipt.append(item)
    (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    if r.returncode:
        raise RuntimeError(str(item) + "\n" + r.stderr.decode(errors="replace"))
    return r

for name, contract in oracle.items():
    if contract["check"] or (a.only and not name.startswith(a.only)):
        continue
    stem = pathlib.Path(name).stem
    raw = a.raw / (stem + ".c") if a.raw else out / (stem + ".c")
    if not a.raw:
        run(stem + "-emit", [a.compiler, "--emit-c17-to", raw, a.source / name])
    data = raw.read_bytes()
    helper = re.findall(rb"static size_t nl_qty_fn_(\d+)\(size_t", data)
    caller = re.findall(rb"static size_t nl_qty_fn_(\d+)\(void\)", data)
    if len(helper) != 1 or len(caller) != 1:
        raise RuntimeError("missing genuine typed call/return")
    expected = ", ".join("((size_t)UINTMAX_C(%d))" % x for x in contract["outputs"])
    passive = out / (stem + "-passive.c")
    passive.write_text(
        '#define main nl_p1492_original_main\n#include ' + json.dumps(str(raw.resolve())) +
        '\n#undef main\n#include <inttypes.h>\n#include <stdio.h>\n#include <stdlib.h>\n'
        'int main(int argc, char **argv) {\n'
        '    static const size_t expected[] = {' + expected + '};\n'
        '    if (argc != (int)(sizeof(expected)/sizeof(*expected)) + 1) return 1;\n'
        '    for (int i = 1; i < argc; ++i) {\n'
        '        size_t input = (size_t)strtoumax(argv[i], NULL, 10);\n'
        '        size_t actual = nl_qty_fn_' + helper[0].decode() + '(input);\n'
        '        printf("input=%zu result=%zu\\n", input, actual);\n'
        '        if (actual != expected[i-1]) return 2;\n'
        '    }\n'
        '    size_t caller = nl_qty_fn_' + caller[0].decode() + '();\n'
        '    printf("caller=%zu\\n", caller);\n'
        '    return caller == ((size_t)UINTMAX_C(' + str(contract["main_result"]) + ')) ? 0 : 3;\n}\n')
    for opt in ([a.opt] if a.opt else ["0", "2"]):
        flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O" + opt]
        if a.sanitizer != "none":
            flags += ["-fsanitize=" + a.sanitizer, "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
        for mode, source in [("raw", raw), ("passive", passive)]:
            label = stem + "-O" + opt + "-" + mode
            exe = out / label
            run(label + "-compile", [a.cc, *flags, source, "-o", exe])
            run(label + "-run", [exe, *([str(x) for x in contract["inputs"]] if mode == "passive" else [])], True)
            if raw.read_bytes() != data:
                raise RuntimeError("raw C modified")
    print(stem, "raw SHA256", hashlib.sha256(data).hexdigest(), "PASS")
print("native executions", sum(x["guest"] for x in receipt), "receipt", out / "receipt.json")
