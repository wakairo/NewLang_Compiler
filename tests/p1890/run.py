"""Drive the original scalar conversion entry with ordinary runtime inputs."""

from contextlib import nullcontext
from pathlib import Path
import re
import runpy
import sys
from tempfile import TemporaryDirectory


here = Path(__file__).resolve().parent
support = runpy.run_path(str(here.parent / "p1882" / "run.py"))
run, refusal = support["run"], support["refusal"]
sources = here / "source"


def observe(raw, original, narrow, widen):
    match = re.search(
        rb"^static uint8_t (nl_fn_\d+)\(size_t (nl_local_\d+)\)\n\{\n"
        rb"    \(void\)\2;\n    const size_t (nl_less_left_(\d+)) = \2;\n"
        rb"    const size_t (nl_less_right_\4) = \(\(size_t\)UINTMAX_C\((\d+)\)\);\n"
        rb"    return \(\(\3 < \5\) \? ([^;\n]+) : (\d+)\);\n\}$",
        raw, re.M,
    )
    assert match
    callee, formal = match[1], match[2]
    bound, true_value, false_value = match[6], match[7], match[8]
    if narrow:
        assert (bound, true_value, false_value) == (b"256", b"(uint8_t)" + formal, b"203")
    else:
        assert (bound, true_value, false_value) == (b"128", b"0", b"255")

    call = re.search(
        rb"^    const uint8_t (nl_call_\d+) = " + callee + rb"\((nl_local_\d+)\);$",
        raw, re.M,
    )
    assert call
    actual = re.search(rb"^    const size_t " + call[2] +
                       rb" = \(\(size_t\)UINTMAX_C\((\d+)\)\);$", raw, re.M)
    stored = re.search(rb"^    const uint8_t (nl_local_\d+) = " + call[1] + rb";$", raw, re.M)
    assert actual and stored and int(actual[1]) == original
    declaration = rb"const size_t (nl_local_\d+) = \(size_t\)" if widen else rb"const uint8_t (nl_local_\d+) = "
    observed = re.search(rb"^    " + declaration + stored[1] + rb";$", raw, re.M)
    assert observed and actual.end() < call.start() < stored.start() < observed.start() < match.start()
    limit = re.search(rb"_Static_assert\(SIZE_MAX == UINTMAX_C\((\d+)\)", raw)
    assert limit and int(limit[1]) >= 257

    # Vary only the emitted entry's input initializer. The selector, cast and
    # caller remain the ones emitted by the compiler.
    data = (raw[:actual.start()] + b"    const size_t " + call[2] + b" = driver_input;"
            + raw[actual.end():observed.end()]
            + b'\n    printf("%ju\\n", (uintmax_t)' + observed[1] + b");"
            + raw[observed.end():])
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
    driver_input = (size_t)value;
    return compiled_main();
}
"""
    headers = b"#include <errno.h>\n#include <inttypes.h>\n#include <stddef.h>\n#include <stdio.h>\n"
    return headers + b"static size_t driver_input;\n" + data + driver, int(limit[1])


def verify_case(case, compiler, cc, name, original, narrow, widen):
    source = sources / f"{name}.nl"
    raw_c, observed_c = case / "raw.c", case / "observed.c"
    checked = run(case, "check", [compiler, "--check", source], 0)
    emitted = run(case, "emit", [compiler, "--emit-c17-to", raw_c, source], 0)
    assert not any((checked.stdout, checked.stderr, emitted.stdout, emitted.stderr))
    observed, maximum = observe(raw_c.read_bytes(), original, narrow, widen)
    observed_c.write_bytes(observed)
    inputs = dict.fromkeys((original, 0, 1, 127, 128, 254, 255, 256, 257, maximum))
    for opt in ("-O0", "-O2"):
        flags = [cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror", opt]
        plain, probe = case / ("raw" + opt), case / ("observed" + opt)
        run(case, plain.name + ".build", flags + [raw_c, "-o", plain], 0)
        result = run(case, plain.name + ".run", [plain], 0)
        assert not result.stdout and not result.stderr
        run(case, probe.name + ".build", flags + [observed_c, "-o", probe], 0)
        for value in inputs:
            expected = (value if value < 256 else 203) if narrow else (0 if value < 128 else 255)
            result = run(case, f"{probe.name}.{value}.run", [probe, str(value)], 0)
            assert result.stdout == f"{expected}\n".encode("ascii") and not result.stderr
    print(name, len(inputs), "runtime inputs at each of O0/O2")


def main():
    compiler, cc = sys.argv[1:3]
    if len(sys.argv) == 4:
        output = Path(sys.argv[3])
        output.mkdir()
        scope = nullcontext(output)
    else:
        scope = TemporaryDirectory(prefix="p1890-runtime-conversion-")
    with scope as directory:
        root = Path(directory)
        for name, original, narrow, widen in (
            ("selector-control", 128, False, False),
            ("widen-low", 127, False, True),
            ("widen-high", 128, False, True),
            ("narrow-last", 255, True, False),
            ("narrow-false", 256, True, False),
            ("roundtrip-in-range", 254, True, True),
            ("roundtrip-false", 256, True, True),
        ):
            case = root / name
            case.mkdir()
            verify_case(case, compiler, cc, name, original, narrow, widen)
        for name, code in (
            ("unguarded", "NUMERIC-CONVERSION-PRECISION"),
            ("insufficient-bound", "NUMERIC-CONVERSION-PRECISION"),
            ("false-arm", "NUMERIC-CONVERSION-PRECISION"),
            ("different-current", "NUMERIC-CONVERSION-PRECISION"),
            ("shadowed-current", "NUMERIC-CONVERSION-PRECISION"),
            ("nominal-conversion-reject", "NUMERIC-CONVERSION-TYPE"),
        ):
            case = root / name
            case.mkdir()
            refusal(case, compiler, sources / f"{name}.nl", code)


if __name__ == "__main__":
    main()
