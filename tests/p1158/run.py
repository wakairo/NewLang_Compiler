"""C232 independent runtime condition and same-original normal join cohort."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile

p = Path(__file__).resolve().parent
compiler, probe, gcc, clang, sanitizer, output = sys.argv[1:]
for name, expected in json.loads((p / "freeze-v1.json").read_bytes()).items():
    assert hashlib.sha256((p / name).read_bytes()).hexdigest() == expected, name
oracle = json.loads((p / "oracle.json").read_bytes())
layers = {"controls": oracle["controls"], "negative": oracle["negative"]}
Path(output).mkdir(parents=True, exist_ok=True)
raw = Path(tempfile.mkdtemp(prefix="c232-", dir=output))
rows = []


def run(command, label):
    command = list(map(str, command))
    result = subprocess.run(command, capture_output=True, timeout=180)
    (raw / (label + ".out")).write_bytes(result.stdout)
    (raw / (label + ".err")).write_bytes(result.stderr)
    rows.append(dict(label=label, command=command, status=result.returncode))
    (raw / "observations.json").write_text(json.dumps(rows, indent=2) + "\n")
    print(label, result.returncode, flush=True)
    assert result.returncode >= 0, (label, result.stderr)
    return result


def named(source, label, expected, emitted=None):
    directory = raw / (label + "-named-directory")
    directory.mkdir()
    target = directory / "out.c"
    sentinel = b"C232 immutable named output\n"
    target.write_bytes(sentinel)
    before = target.stat()
    result = run([compiler, "--emit-c17-to", target, source], label + "-named")
    assert result.returncode == expected and not result.stdout, result.stderr
    assert sorted(x.name for x in directory.iterdir()) == ["out.c"]
    if expected:
        after = target.stat()
        assert target.read_bytes() == sentinel
        assert (after.st_ino, after.st_mtime_ns) == (before.st_ino, before.st_mtime_ns)
    else:
        assert not result.stderr and target.read_bytes() == emitted


def passive(source, meta):
    def insert(pattern, replacement):
        nonlocal source
        source, count = re.subn(pattern, replacement, source)
        assert count == 1, (pattern, count)

    # These callbacks read actual guest operands, Bool, place, and results.
    # No source expression, comparison, branch, store, or result is replaced.
    decision = str(meta["decision"])
    pattern = r"(    const _Bool nl_local_" + decision + r" = \(([^\n]+)\);\n)"
    match = re.search(pattern, source)
    assert match
    operands = re.fullmatch(r"(.+) (?:<|==) (.+)", match[2])
    assert operands
    insert(pattern, lambda m: "    c232_operands(" + operands[1] + ", " +
           operands[2] + ");\n" + m[1] +
           "    c232_condition(nl_local_" + decision + ");\n")
    insert(r"(    if \(nl_local_" + decision + r"\) \{\n)",
           lambda m: m[1] + "        c232_arm(0);\n")
    insert(r"(    \} else \{\n)", lambda m: m[1] + "        c232_arm(1);\n")
    store = r"(            \*(nl_local_[0-9]+) = ([^;]+);\n)"
    source, count = re.subn(store, lambda m:
        "            c232_before_store(" + m[2] + ", " + m[3] + ");\n" +
        m[1] + "            c232_after_store(" + m[2] + ");\n", source)
    assert count == 1
    insert(r"(        return;\n)", lambda m: "        c232_early();\n" + m[1])
    insert(r"(    const uint8_t nl_local_" + str(meta["old_copy"]) +
           r" = nl_local_" + str(meta["old"]) + r";\n)",
           lambda m: m[1] + "    c232_old(nl_local_" + str(meta["old_copy"]) + ");\n")
    insert(r"(int main\(void\)\n\{\n    nl_fn_" + str(meta["entry"]) + r"\(\);\n)",
           lambda m: m[1] + "    c232_finish();\n")
    return source


native = 0
for name, expected in oracle["positive"].items():
    source = p / "sources" / (name + ".nl")
    result = run([compiler, "--check", source], name + "-check")
    assert not result.returncode and not result.stdout and not result.stderr
    result = run([probe, source, "--describe"], name + "-owned-current")
    assert not result.returncode and not result.stderr, result.stderr
    lines = [line.removeprefix("C232_SYMBOLS ") for line in result.stdout.decode().splitlines()
             if line.startswith("C232_SYMBOLS ")]
    assert len(lines) == 1
    meta = json.loads(lines[0])
    assert meta["field"] == expected["index"] and meta["return_arm"] == expected["return_arm"]
    result = run([compiler, source], name + "-emit")
    assert not result.returncode and not result.stderr, result.stderr
    code = result.stdout.decode()
    named(source, name, 0, result.stdout)
    # Raw generated C is compiled/run before its passive derivative.
    for variant in ["raw", "passive"]:
        text = code if variant == "raw" else passive(code, meta)
        cfile = raw / (name + "-" + variant + ".c")
        cfile.write_text(text)
        for cc in dict.fromkeys([gcc, clang]):
            for optimization in ["-O0", "-O2"]:
                label = name + "-" + variant + "-" + Path(cc).name + optimization
                executable = raw / label
                flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", optimization]
                if sanitizer != "none":
                    flags += ["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all",
                              "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
                if variant == "passive":
                    flags += ['-DNEWLANG_LOOP_OBSERVER="' + str(p / "observe.h") + '"',
                              '-DNEWLANG_SCALAR_LOOP_OBSERVER="' + str(p / "observe.h") + '"',
                              "-DC232_TRACE={" + ",".join(map(str, expected["trace"])) + "}"]
                    for flag, key in [("RESULT", "result"), ("COMPARE", "compare"),
                                      ("TRUTH", "truth"), ("ARM", "selected_arm"),
                                      ("INDEX", "index"), ("UPDATED", "incoming"), ("RETURN_ARM", "return_arm"), ("EARLY", "early_return")]:
                        flags += ["-DC232_" + flag + "=" + str(int(expected[key]))]
                    flags += ["-DC232_FIELD0=" + str(expected["values"][0]),
                              "-DC232_FIELD1=" + str(expected["values"][1])]
                result = run([cc, *flags, cfile, "-o", executable], label + "-build")
                assert not result.returncode, result.stderr
                result = run([executable], label + "-native")
                assert not result.returncode and not result.stderr, result.stderr
                if variant == "raw":
                    assert not result.stdout
                else:
                    assert result.stdout == b"C232 real comparison/RETURN-or-normal/original/old/current/sibling PASS\n"
                native += 1
                executable.unlink()

for name, expected in {**layers["controls"], **layers["negative"]}.items():
    source = p / "sources" / (name + ".nl")
    result = run([compiler, "--check", source], name + "-check")
    assert result.returncode == expected["check"] and not result.stdout
    result = run([compiler, source], name + "-emit")
    assert result.returncode == expected["emit"]
    if expected["emit"]:
        assert not result.stdout
    else:
        assert not result.stderr
        baseline = (p / "evidence" / (name + "-golden.c17")).read_bytes()
        assert result.stdout == baseline, "old control generated C changed"
    named(source, name, expected.get("named", expected["emit"]), result.stdout)
print("C232 finite source/current/raw-passive/native/negative/no-clobber PASS", native,
      "native observations; sanitizer=" + sanitizer)
