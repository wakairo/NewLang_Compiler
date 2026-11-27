"""Read the real generated scalar Copy actual and source-ordered unit calls."""

from contextlib import nullcontext
from pathlib import Path
import re
import runpy
import sys
from tempfile import TemporaryDirectory


here = Path(__file__).resolve().parent
support = runpy.run_path(str(here.parent / "p1882" / "run.py"))
run, refusal, state = (support[name] for name in ("run", "refusal", "state"))
seeds = (0, 1, 17, 63, 64, 68, 255)


def matches(pattern, data):
    return list(re.finditer(pattern, data, re.M))


def one(pattern, data):
    found = matches(pattern, data)
    assert len(found) == 1, pattern
    return found[0]


def print_event(label, *symbols):
    fmt = b",".join([b"%u"] * len(symbols))
    args = b", ".join(b"(unsigned)" + name for name in symbols)
    return b'\n    printf("' + label + b":" + fmt + b'\\n", ' + args + b");"


def observe(raw, initial, ending):
    main = one(rb"^int main\(void\)\n\{\n    (nl_fn_\d+)\(\);$", raw)
    entry = one(rb"^static void " + main[1] +
                rb"\(void\)\n\{\n((?:[^\n]*\n)*?)^\}", raw)
    body = entry[1]
    bindings = matches(rb"^    const size_t (nl_local_\d+) = \(\(size_t\)UINTMAX_C\((\d+)\)\);$",
                       body)
    assert [int(row[2]) for row in bindings] == list(initial)
    edits = [
        (entry.start(1) + row.start(), entry.start(1) + row.end(),
         b"    const size_t " + row[1] + b" = inputs[" + str(index).encode() + b"];")
        for index, row in enumerate(bindings)
    ]
    if ending:
        helper = one(rb"^static void (nl_fn_\d+)\(uint8_t (nl_local_\d+)\)\n\{\n"
                     rb"((?:[^\n]*\n)*?)^\}", raw)
        local = one(rb"^    const uint8_t (nl_local_\d+) = " + helper[2] + rb";$", helper[3])
        assert (b"    return;\n" in helper[3]) == (ending == "return")
        assert (b"    (void)0;\n" in helper[3]) == (ending != "empty")
        edits.append((helper.start(3) + local.end(), helper.start(3) + local.end(),
                      print_event(b"call", helper[2], local[1])))
        calls = matches(rb"^    " + helper[1] + rb"\((nl_local_\d+)\);$", body)
        assert len(calls) == max(1, len(initial))
        previous = 0
        for call in calls:
            actual = one(rb"^    const uint8_t " + call[1] + rb" = (nl_call_\d+|239);$", body)
            copied = one(rb"^    const uint8_t (nl_local_\d+) = " + call[1] + rb";$", body)
            assert previous <= actual.start() < call.start() < copied.start()
            previous = copied.end()
            edits.append((entry.start(1) + copied.end(), entry.start(1) + copied.end(),
                          print_event(b"copy", copied[1])))
    else:
        locals_ = matches(rb"^    const uint8_t (nl_local_\d+) = .*;$", body)
        assert locals_
        copied = locals_[-1]
        edits.append((entry.start(1) + copied.end(), entry.start(1) + copied.end(),
                      print_event(b"copy", copied[1])))

    observed = raw
    for start, end, replacement in sorted(edits, reverse=True):
        observed = observed[:start] + replacement + observed[end:]
    assert observed.count(b"int main(void)") == 1
    observed = observed.replace(b"int main(void)", b"static int compiled_main(void)", 1)
    prefix = b"#include <errno.h>\n#include <inttypes.h>\n#include <stddef.h>\n#include <stdio.h>\n"
    driver = b"\nint main(int argc, char **argv)\n{\n    (void)argv;\n"
    length = str(len(initial)).encode()
    driver += b"    if (argc != " + str(len(initial) + 1).encode() + b")\n        return 2;\n"
    if initial:
        prefix += b"static size_t inputs[" + length + b"];\n"
        driver += b"""
    for (int i = 1; i < argc; ++i) {
        char *end;
        errno = 0;
        const uintmax_t value = strtoumax(argv[i], &end, 10);
        if (errno || end == argv[i] || *end || value > SIZE_MAX)
            return 2;
        inputs[i - 1] = (size_t)value;
    }
"""
    return prefix + observed + driver + b"    return compiled_main();\n}\n"


def expected(inputs, unit_call):
    octets = [11 if value < 64 else 239 for value in inputs] if inputs else [239]
    lines = []
    for value in octets:
        if unit_call:
            lines.append(f"call:{value},{value}\n")
        lines.append(f"copy:{value}\n")
    return "".join(lines).encode("ascii")


def positive(root, compiler, cc, name, initial, ending):
    source = here / "source" / f"{name}.nl"
    result = run(root, "check", [compiler, "--check", source], 0)
    assert not result.stdout and not result.stderr
    raw = root / "raw.c"
    result = run(root, "emit", [compiler, "--emit-c17-to", raw, source], 0)
    assert not result.stdout and not result.stderr
    data = raw.read_bytes()
    assert b"_Static_assert(UINT8_MAX == 255" in data
    observed = root / "observed.c"
    observed.write_bytes(observe(data, initial, ending))
    if initial:
        maximum = int(one(rb"_Static_assert\(SIZE_MAX == UINTMAX_C\((\d+)\)", data)[1])
        inputs = [(value,) for value in (*seeds, maximum)]
        if len(initial) == 2:
            inputs = [initial, initial[::-1], (63, 64), (maximum, 0)]
    else:
        inputs = [()]
    for opt in ("-O0", "-O2"):
        flags = [cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror", opt]
        plain, driver = root / ("raw" + opt), root / ("observed" + opt)
        run(root, plain.name + ".build", flags + [raw, "-o", plain], 0)
        result = run(root, plain.name + ".run", [plain], 0)
        assert not result.stdout and not result.stderr
        run(root, driver.name + ".build", flags + [observed, "-o", driver], 0)
        for test in inputs:
            suffix = ".".join(map(str, test)) or "source"
            result = run(root, f"{driver.name}.{suffix}.run",
                         [driver, *map(str, test)], 0)
            assert result.stdout == expected(test, bool(ending)) and not result.stderr
    print(name, len(inputs), "input vectors at O0/O2")


def unsupported(root, compiler, source):
    result = run(root, "check", [compiler, "--check", source], 0)
    assert not result.stdout and not result.stderr
    output = root / "out.c"
    original = b"/* original generated output */\n"
    output.write_bytes(original)
    before = state(output)
    command = [compiler, "--emit-c17-to", output, source]
    result = run(root, "existing", command, 4)
    assert b"[V1-BACKEND-UNSUPPORTED]" in result.stderr and not result.stdout
    assert state(output) == before and output.read_bytes() == original
    output.unlink()
    result = run(root, "absent", command, 4)
    assert b"[V1-BACKEND-UNSUPPORTED]" in result.stderr and not result.stdout
    assert not output.exists()
    print(source.stem, "CHECK success; backend refusal preserves both output states")


def main():
    compiler, cc = sys.argv[1:3]
    if len(sys.argv) == 4:
        target = Path(sys.argv[3])
        target.mkdir()
        scope = nullcontext(target)
    else:
        scope = TemporaryDirectory(prefix="p1900-unit-copy-")
    with scope as directory:
        root = Path(directory)
        for name, initial, ending in (
            ("runtime-statement", (17,), "return"),
            ("known-statement", (), "return"),
            ("ordered-statements", (17, 68), "return"),
            ("normal-unit-tail", (17,), "tail"),
            ("normal-empty-tail", (17,), "empty"),
            ("zero-arity-control", (17,), None),
            ("u8-result-control", (17,), None),
        ):
            case = root / name
            case.mkdir()
            positive(case, compiler, cc, name, initial, ending)
        case = root / "wrong-nominal"
        case.mkdir()
        refusal(case, compiler, here / "source/wrong-nominal.nl", "P3-TYPE-MISMATCH")
        for name in ("inline-actual", "nested-helper-body"):
            case = root / name
            case.mkdir()
            unsupported(case, compiler, here / "source" / f"{name}.nl")


if __name__ == "__main__":
    main()
