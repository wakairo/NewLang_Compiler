"""Observe real guarded conversion and its arm-local Copy chain."""

from contextlib import nullcontext
import json
from pathlib import Path
import re
import runpy
import sys
from tempfile import TemporaryDirectory


here = Path(__file__).resolve().parent
support = runpy.run_path(str(here.parent / "p1882" / "run.py"))
run, refusal = support["run"], support["refusal"]
obs = json.loads((here / "observations.json").read_text(encoding="utf-8"))


def expect(pattern, data):
    match = re.search(pattern, data, re.M)
    assert match, pattern
    return match


def guard_dataflow(raw, case):
    helper = expect(rb"^static uint8_t (nl_fn_\d+)\(size_t (nl_local_\d+)\)\n"
                    rb"\{\n((?:[^\n]*\n)*?)^\}", raw)
    callee, formal, body = helper.groups()
    guard = expect(rb"^    const size_t (nl_less_left_(\d+)) = " + formal +
                   rb";\n    const size_t (nl_less_right_\2) = "
                   rb"\(\(size_t\)UINTMAX_C\((\d+)\)\);$", body)
    assert int(guard[4]) == case["bound"]
    condition = b"(" + guard[1] + b" < " + guard[3] + b")"
    fallback = str(case["fallback"]).encode()
    if case.get("direct"):
        assert body[guard.end():] == (
            b"\n    return (" + condition + b" ? (uint8_t)" + formal +
            b" : " + fallback + b");\n"
        )
    else:
        guarded = expect(rb"^    if \(" + re.escape(condition) +
                         rb"\) \{\n((?:[^\n]*\n)*?)^    \}\n    return (\d+);$", body)
        assert guard.end() < guarded.start() and guarded[2] == fallback
        values = re.findall(rb"^        const uint8_t (nl_local_\d+) = ([^;]+);$", guarded[1], re.M)
        assert len(values) == case["copies"] + 1
        expected = b"(uint8_t)" + formal
        for name, value in values:
            assert value == expected
            expected = name
        assert guarded[1].endswith(b"        return " + expected + b";\n")
        assert body.count(b"(uint8_t)" + formal) == 1
    return callee


def codec_dataflow(caller, expected):
    codec = expect(rb"^    const uint8_t (nl_local_\d+) = (\d+);\n"
                   rb"    \(void\)\1;\n"
                   rb"    const uint8_t (nl_local_\d+) = \(uint8_t\)\1;\n"
                   rb"    \(void\)\3;\n"
                   rb"    const uint8_t (nl_local_\d+) = \(uint8_t\)\3;\n"
                   rb"    \(void\)\4;\n"
                   rb"    const uint8_t (nl_local_\d+) = \4;$", caller)
    assert int(codec[2]) == expected
    return codec[5]


def observe(raw, case):
    entry = expect(rb"^int main\(void\)\n\{\n    (nl_fn_\d+)\(\);$", raw)
    main = expect(rb"^static void " + entry[1] +
                  rb"\(void\)\n\{\n((?:[^\n]*\n)*?)^\}", raw)
    caller = main[1]
    symbols = []
    if "bound" in case:
        callee = guard_dataflow(raw, case)
        call = expect(rb"^    const uint8_t (nl_call_\d+) = " + callee +
                      rb"\((nl_local_\d+)\);$", caller)
        seed = expect(rb"^    const size_t " + call[2] +
                      rb" = \(\(size_t\)UINTMAX_C\((\d+)\)\);$", caller)
        stored = expect(rb"^    const uint8_t (nl_local_\d+) = " + call[1] + rb";$", caller)
        copied = expect(rb"^    const uint8_t (nl_local_\d+) = " + stored[1] + rb";$", caller)
        assert int(seed[1]) == case["seed"]
        assert seed.end() < call.start() < stored.start() < copied.start()
        caller = (caller[:seed.start()] + b"    const size_t " + call[2] +
                  b" = test_input;" + caller[seed.end():])
        symbols.append(copied[1])
    if "codec" in case:
        symbols.append(codec_dataflow(caller, case["codec"]))
    before_return = caller.rindex(b"    return;\n")
    fields = b",".join([b"%ju"] * len(symbols))
    args = b", ".join(b"(uintmax_t)" + name for name in symbols)
    caller = (caller[:before_return] + b'    printf("' + fields + b'\\n", ' +
              args + b");\n" + caller[before_return:])
    observed = b"#include <inttypes.h>\n#include <stdio.h>\n"
    data = raw[:main.start(1)] + caller + raw[main.end(1):]
    if "bound" not in case:
        return observed + data, [None]
    assert data.count(b"int main(void)") == 1
    data = data.replace(b"int main(void)", b"static int compiled_main(void)", 1)
    driver = b"""
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    char *end;
    errno = 0;
    const uintmax_t value = strtoumax(argv[1], &end, 10);
    if (errno || end == argv[1] || *end || value > SIZE_MAX)
        return 2;
    test_input = (size_t)value;
    return compiled_main();
}
"""
    maximum = int(expect(rb"_Static_assert\(SIZE_MAX == UINTMAX_C\((\d+)\)", raw)[1])
    inputs = [maximum if x == "SIZE_MAX" else int(x) for x in obs["inputs"]]
    return (observed + b"#include <errno.h>\n#include <stddef.h>\n"
            b"static size_t test_input;\n" + data + driver), inputs


def positive(root, compiler, cc, source, case):
    check = run(root, "check", [compiler, "--check", source], 0)
    output = root / "raw.c"
    emitted = run(root, "emit", [compiler, "--emit-c17-to", output, source], 0)
    assert not any((check.stdout, check.stderr, emitted.stdout, emitted.stderr))
    c, inputs = observe(output.read_bytes(), case)
    witness = root / "observed.c"
    witness.write_bytes(c)
    for opt in ("-O0", "-O2"):
        flags = [cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror", opt]
        plain, instrumented = root / ("raw" + opt), root / ("observed" + opt)
        run(root, plain.name + ".build", flags + [output, "-o", plain], 0)
        status = run(root, plain.name + ".run", [plain], 0)
        assert not status.stdout and not status.stderr
        run(root, instrumented.name + ".build", flags + [witness, "-o", instrumented], 0)
        for value in inputs:
            arguments = [instrumented]
            expected = []
            if value is not None:
                arguments.append(value)
                expected.append(value if value < case["bound"] else case["fallback"])
            if "codec" in case:
                expected.append(case["codec"])
            status = run(root, f"{instrumented.name}.{value}.run", arguments, 0)
            assert status.stdout == (",".join(map(str, expected)) + "\n").encode()
            assert not status.stderr
    print(source.stem, len(inputs), "observations at each of O0/O2")


def suite(root, compiler, cc):
    for name, case in obs["positive"].items():
        dest = root / name
        dest.mkdir()
        positive(dest, compiler, cc, here / "source" / f"{name}.nl", case)
    for name, diagnostic in obs["negative"].items():
        dest = root / name
        dest.mkdir()
        refusal(dest, compiler, here / "source" / f"{name}.nl", diagnostic)


if __name__ == "__main__":
    if len(sys.argv) == 4:
        output = Path(sys.argv[3])
        output.mkdir()
        scope = nullcontext(output)
    else:
        scope = TemporaryDirectory(prefix="p1914-guarded-narrow-copy-")
    with scope as directory:
        suite(Path(directory), *sys.argv[1:3])
