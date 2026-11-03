from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

phase, compiler, probe, root_arg, raw_arg, gcc, clang = sys.argv[1:]
assert phase in {"original", "repair"}
root = Path(root_arg).resolve()
raw = Path(raw_arg).resolve()
raw.mkdir(parents=True, exist_ok=True)

freeze = json.loads((root / "freeze.json").read_bytes())
for name, digest in freeze["files"].items():
    assert hashlib.sha256((root / name).read_bytes()).hexdigest() == digest
oracle = json.loads((root / "oracle.json").read_bytes())
assert oracle["authority"]["parent"] == (
    "5346a7e2ca2aacb4ed6ab50f3b09bb258b1e607c"
)

rows = []


def run(command, label):
    command = list(map(str, command))
    completed = subprocess.run(command, capture_output=True, timeout=120)
    prefix = raw / (f"{len(rows) + 1:03d}-{label}")
    prefix.with_suffix(".out").write_bytes(completed.stdout)
    prefix.with_suffix(".err").write_bytes(completed.stderr)
    rows.append(
        {
            "label": label,
            "command": command,
            "returncode": completed.returncode,
            "stdout_sha256": hashlib.sha256(completed.stdout).hexdigest(),
            "stderr_sha256": hashlib.sha256(completed.stderr).hexdigest(),
        }
    )
    (raw / "observations.json").write_text(json.dumps(rows, indent=2) + "\n")
    print(label, completed.returncode, flush=True)
    return completed


for name, case in oracle["cases"].items():
    source = root / "sources" / (name + ".nl")
    checked = run([compiler, "--check", source], name + "-check")
    emitted = run([compiler, source], name + "-c17")
    assert checked.returncode == 0 and not checked.stdout
    assert emitted.returncode == 0 and emitted.stdout and not emitted.stderr

    named_path = raw / (name + "-named.c")
    sentinel = b"P1117-C217 named output preserved\x00\xff\n"
    named_path.write_bytes(sentinel)
    named = run(
        [compiler, "--emit-c17-to", named_path, source], name + "-named"
    )
    assert named.returncode == 0 and not named.stdout and not named.stderr
    assert named_path.read_bytes() == emitted.stdout

    observed = run(
        [probe, phase, source, case["shape"]], name + "-lineage"
    )
    assert observed.returncode == 0 and not observed.stderr
    assert b"altered graphs were not emitted or executed" in observed.stdout

    text = emitted.stdout.decode()
    runtime = case["runtime"]
    arguments = ",".join(str(pair[0]) for pair in runtime)
    results = ",".join(str(pair[1]) for pair in runtime)
    header = "#include <assert.h>\n#include <stdio.h>\n#include <stdlib.h>\n"
    header += f"static const size_t p1117_c217_args[] = {{{arguments}}};\n"
    header += f"static const unsigned p1117_c217_results[] = {{{results}}};\n"
    header += r'''
static size_t p1117_c217_entered, p1117_c217_returned;
static void p1117_c217_finish(void) {
    assert(p1117_c217_entered == sizeof(p1117_c217_args) / sizeof(*p1117_c217_args));
    assert(p1117_c217_returned == p1117_c217_entered);
    puts("P1117-C217 genuine native results PASS");
}
static void p1117_c217_input(size_t value) {
    assert(p1117_c217_entered == p1117_c217_returned);
    assert(p1117_c217_entered < sizeof(p1117_c217_args) / sizeof(*p1117_c217_args));
    assert(value == p1117_c217_args[p1117_c217_entered]);
    if (!p1117_c217_entered) assert(atexit(p1117_c217_finish) == 0);
    ++p1117_c217_entered;
}
static uint8_t p1117_c217_output(uint8_t value) {
    assert(p1117_c217_returned + 1 == p1117_c217_entered);
    assert(value == p1117_c217_results[p1117_c217_returned]);
    ++p1117_c217_returned;
    return value;
}
'''
    pattern = (
        r"(static uint8_t nl_fn_\d+\((?:const )?size_t (nl_local_\d+)\)\n"
        r"\{\n)(.*?)(^\})"
    )

    def observe(match):
        body, returns = re.subn(
            r"\breturn ([^;\n]+);", r"return p1117_c217_output(\1);", match[3]
        )
        assert returns == 1
        return (
            match[1]
            + f"    p1117_c217_input({match[2]});\n"
            + body
            + match[4]
        )

    passive, count = re.subn(pattern, observe, text, flags=re.S | re.M)
    assert count == 1
    passive = passive.replace(
        "#include <stdint.h>",
        "#include <stdint.h>\n#include <stddef.h>\n" + header,
        1,
    )
    for variant, c_text in {"raw": text, "passive": passive}.items():
        c_path = raw / f"{name}-{variant}.c"
        c_path.write_text(c_text)
        for cc in dict.fromkeys([gcc, clang]):
            for optimization in ["-O0", "-O2"]:
                stem = f"{name}-{Path(cc).name}-{variant}{optimization}"
                executable = raw / stem
                built = run(
                    [
                        cc,
                        "-std=c17",
                        "-Wall",
                        "-Wextra",
                        "-Wpedantic",
                        "-Werror",
                        optimization,
                        c_path,
                        "-o",
                        executable,
                    ],
                    stem + "-build",
                )
                assert built.returncode == 0 and not built.stderr
                native = run([executable], stem + "-native")
                assert native.returncode == 0 and not native.stderr
                if variant == "passive":
                    assert b"genuine native results PASS" in native.stdout
                executable.unlink()

print(f"P1117-C217 {phase} observation complete", flush=True)
