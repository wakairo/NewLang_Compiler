"""C226 independent source/current and raw/passive native observations."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile

p = Path(__file__).resolve().parent
compiler, probe, gcc, clang, sanitizer, output = sys.argv[1:]
for name, expected in json.loads((p / "freeze-v4.json").read_bytes()).items():
    assert hashlib.sha256((p / name).read_bytes()).hexdigest() == expected, name
oracle = json.loads((p / "oracle-v2.json").read_bytes())
Path(output).mkdir(parents=True, exist_ok=True)
raw = Path(tempfile.mkdtemp(prefix="c226-", dir=output))
rows = []


def run(command, label):
    command = list(map(str, command))
    result = subprocess.run(command, capture_output=True, timeout=120)
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
    sentinel = b"C226 immutable named output\n"
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

    # Callbacks only read the actual formal, return, store operands and Copy.
    # They do not replace any source expression, argument, result or store.
    insert(r"(static uint8_t nl_fn_" + str(meta["function"]) +
           r"\(size_t nl_local_" + str(meta["formal"]) + r"\)\n\{\n)",
           lambda m: m[1] + "    c226_actual(nl_local_" +
           str(meta["formal"]) + ");\n")
    insert(r"(    const uint8_t nl_call_" + str(meta["call"]) +
           r" = [^;]+;\n)",
           lambda m: m[1] + "    c226_tag(nl_call_" + str(meta["call"]) + ");\n")
    store = "        *nl_local_" + str(meta["ref"]) + " = nl_local_" + str(meta["incoming"]) + ";\n"
    assert source.count(store) == 1
    source = source.replace(store,
        "        c226_before_store(nl_local_" + str(meta["ref"]) +
        ", nl_local_" + str(meta["incoming"]) + ");\n" + store +
        "        c226_after_store(nl_local_" + str(meta["ref"]) + ");\n")
    insert(r"(    const uint8_t nl_local_" + str(meta["old_again"]) +
           r" = nl_local_" + str(meta["old"]) + r";\n)",
           lambda m: m[1] + "    c226_old(nl_local_" + str(meta["old_again"]) + ");\n")
    insert(r"(int main\(void\)\n\{\n    nl_fn_" + str(meta["entry"]) + r"\(\);\n)",
           lambda m: m[1] + "    c226_finish();\n")
    return source


native = 0
for name, expected in oracle["positive_cases"].items():
    source = p / "sources" / (name + ".nl")
    result = run([compiler, "--check", source], name + "-check")
    assert not result.returncode and not result.stdout and not result.stderr
    result = run([probe, source, "--describe"], name + "-owned-current")
    assert not result.returncode and not result.stderr, result.stderr
    lines = [line.removeprefix("C226_SYMBOLS ") for line in result.stdout.decode().splitlines()
             if line.startswith("C226_SYMBOLS ")]
    assert len(lines) == 1
    meta = json.loads(lines[0])
    assert meta["field"] == expected["write_index"]
    result = run([compiler, source], name + "-emit")
    assert not result.returncode and not result.stderr, result.stderr
    code = result.stdout.decode()
    named(source, name, 0, result.stdout)
    observed = passive(code, meta)
    for variant, text in [("raw", code), ("passive", observed)]:
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
                              "-DC226_TRACE={" + ",".join(map(str, expected["trace"])) + "}",
                              "-DC226_RESULT=" + str(expected["result"]),
                              "-DC226_TAG=" + str(expected["tag"]),
                              "-DC226_WRITE_INDEX=" + str(expected["write_index"]),
                              "-DC226_FIELD0=" + str(expected["values"][0]),
                              "-DC226_FIELD1=" + str(expected["values"][1])]
                result = run([cc, *flags, cfile, "-o", executable], label + "-build")
                assert not result.returncode, result.stderr
                result = run([executable], label + "-native")
                assert not result.returncode and not result.stderr, result.stderr
                if variant == "raw":
                    assert not result.stdout
                else:
                    assert result.stdout == b"C226 runtime result/Copy/predicate/original/scoped store/old/current/sibling PASS\n"
                native += 1
                executable.unlink()

for name, expected in oracle["controls"].items():
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

for name in oracle["negative_cases"]:
    source = p / "sources" / (name + ".nl")
    for mode, command in [("check", [compiler, "--check", source]),
                          ("emit", [compiler, source])]:
        result = run(command, name + "-" + mode)
        assert result.returncode == 3 and not result.stdout and result.stderr
    named(source, name, 3)
print("C226 finite source/current/raw-passive/native/negative/no-clobber PASS", native,
      "native observations; sanitizer=" + sanitizer)
