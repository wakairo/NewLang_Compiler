"""Observe the authenticated widening call and downstream Copy dataflow."""

from contextlib import nullcontext
from pathlib import Path
import re
import runpy
import sys
from tempfile import TemporaryDirectory


here = Path(__file__).resolve().parent
support = runpy.run_path(str(here.parent / "p1882" / "run.py"))
run, refusal, state = (support[name] for name in ("run", "refusal", "state"))

HEADER = b"""#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
"""

DYNAMIC_INPUT = b"""
    char *end;
    errno = 0;
    const uintmax_t value = strtoumax(argv[1], &end, 10);
    if (errno || end == argv[1] || *end || value > SIZE_MAX)
        return 2;
    supplied_seed = (size_t)value;
"""

ALL_OCTETS = b"""
    if (strcmp(argv[1], "all-octets") == 0) {
        for (unsigned int i = 0; i <= UINT8_MAX; ++i) {
            volatile uint8_t actual = (uint8_t)i;
            if (WIDENING_FUNCTION(actual) != i)
                return 3;
        }
        puts("all 256 octets");
        return 0;
    }
"""


def match(pattern, data):
    matches = list(re.finditer(pattern, data, re.M))
    assert len(matches) == 1, pattern
    return matches[0]


def entry_block(raw):
    main = match(rb"^int main\(void\)\n\{\n    (nl_(?:qty_)?fn_\d+)\(\);$", raw)
    return match(rb"^static void " + main[1] +
                 rb"\(void\)\n\{\n((?:[^\n]*\n)*?)^\}", raw)


def widening_path(raw, entry, seed, priority, named_tail):
    helper = match(rb"^static size_t (nl_fn_\d+)\(uint8_t (nl_local_\d+)\)\n\{\n"
                   rb"((?:[^\n]*\n)*?)^\}", raw)
    body = helper[3]
    if named_tail:
        saved = match(rb"^    const size_t (nl_local_\d+) = \(size_t\)" + helper[2] + rb";$", body)
        result = match(rb"^    return " + saved[1] + rb";$", body)
        assert saved.end() < result.start()
    else:
        match(rb"^    return \(size_t\)" + helper[2] + rb";$", body)

    call = match(rb"^    const size_t (nl_call_\d+) = " + helper[1] +
                 rb"\((nl_local_\d+)\);$", entry)
    actual = match(rb"^    const uint8_t " + call[2] + rb" = (nl_call_\d+|255);$", entry)
    bound = match(rb"^    const size_t (nl_local_\d+) = " + call[1] + rb";$", entry)
    assert actual.end() < call.start() < bound.start()
    if seed is None:
        assert actual[1] == b"255"
    else:
        producer = match(rb"^    const uint8_t " + actual[1] +
                         rb" = nl_fn_\d+\((nl_local_\d+)\);$", entry)
        seed_binding = match(rb"^    const size_t " + producer[1] +
                             rb" = \(\(size_t\)UINTMAX_C\((\d+)\)\);$", entry)
        assert int(seed_binding[1]) == seed and seed_binding.end() < producer.start() < actual.start()
    if priority:
        consumer = match(rb"^    const uint8_t (nl_call_\d+) = nl_fn_\d+\(" +
                         bound[1] + rb"\);$", entry)
        saved = match(rb"^    const uint8_t (nl_local_\d+) = " + consumer[1] + rb";$", entry)
        observed = match(rb"^    const uint8_t (nl_local_\d+) = " + saved[1] + rb";$", entry)
        assert bound.end() < consumer.start() < saved.start() < observed.start()
    else:
        observed = match(rb"^    const size_t (nl_local_\d+) = " + bound[1] + rb";$", entry)
        assert bound.end() < observed.start()
    return helper[1], (call[2], bound[1], observed[1])


def observe(raw, seed, widening, priority, named_tail):
    block = entry_block(raw)
    entry = block[1]
    if widening:
        helper, observed = widening_path(raw, entry, seed, priority, named_tail)
        exhaustive = ALL_OCTETS.replace(b"WIDENING_FUNCTION", helper)
    else:
        bindings = re.findall(rb"^    (?:const )?size_t (nl_local_\d+) = .+;$", entry, re.M)
        assert bindings
        observed = (bindings[-1],)
        exhaustive = b""
    text = HEADER
    if seed is not None:
        text += b"static size_t supplied_seed;\n"
        binding = match(rb"^    const size_t (nl_local_\d+) = \(\(size_t\)UINTMAX_C\((\d+)\)\);$", entry)
        assert int(binding[2]) == seed
        entry = (entry[:binding.start()] + b"    const size_t " + binding[1] +
                 b" = supplied_seed;" + entry[binding.end():])
    at = entry.rindex(b"    return;")
    arguments = b", ".join(b"(uintmax_t)" + symbol for symbol in observed)
    formatters = b",".join([b"%ju"] * len(observed))
    entry = entry[:at] + b'    printf("' + formatters + b'\\n", ' + arguments + b");\n" + entry[at:]
    text += raw[:block.start(1)] + entry + raw[block.end(1):]
    assert text.count(b"int main(void)") == 1
    text = text.replace(b"int main(void)", b"static int compiled_main(void)", 1)
    driver = b"\nint main(int argc, char **argv)\n{\n"
    driver += b"    (void)argv;\n    if (argc != 2)\n        return 2;\n"
    return text + driver + exhaustive + (DYNAMIC_INPUT if seed is not None else b"") + b"    return compiled_main();\n}\n"


def positive(root, compiler, cc, name, seed, widening=False, priority=False, named_tail=False):
    source = here / "source" / f"{name}.nl"
    result = run(root, "check", [compiler, "--check", source], 0)
    assert not result.stdout and not result.stderr
    raw = root / "raw.c"
    result = run(root, "emit", [compiler, "--emit-c17-to", raw, source], 0)
    assert not result.stdout and not result.stderr
    data = raw.read_bytes()
    maximum = int(match(rb"_Static_assert\(SIZE_MAX == UINTMAX_C\((\d+)\)", data)[1])
    assert maximum >= 255
    if widening:
        assert b"_Static_assert(UINT8_MAX == 255" in data
    watched = root / "observed.c"
    watched.write_bytes(observe(data, seed, widening, priority, named_tail))
    values = (0, 1, 63, 64, 65, 127, 128, 255, maximum) if seed is not None else (None,)
    for opt in ("-O0", "-O2"):
        flags = [cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror", opt]
        plain, driver = root / ("raw" + opt), root / ("observed" + opt)
        run(root, plain.name + ".build", flags + [raw, "-o", plain], 0)
        result = run(root, plain.name + ".run", [plain], 0)
        assert not result.stdout and not result.stderr
        run(root, driver.name + ".build", flags + [watched, "-o", driver], 0)
        if widening:
            result = run(root, driver.name + ".octets.run", [driver, "all-octets"], 0)
            assert result.stdout == b"all 256 octets\n" and not result.stderr
        for value in values:
            octet = 255 if value is None or value >= 64 else 0
            if widening:
                observed = (7 if octet < 128 else 9) if priority else octet
                expected = (octet, octet, observed)
            else:
                expected = (64 if seed is None else octet,)
            result = run(root, f"{driver.name}.{value}.run",
                         [driver, "source" if value is None else str(value)], 0)
            assert result.stdout == (",".join(map(str, expected)) + "\n").encode() and not result.stderr
    print(name, len(values), "caller inputs at O0/O2", "and all 256 helper octets" if widening else "")


def unsupported(root, compiler, source):
    result = run(root, "check", [compiler, "--check", source], 0)
    assert not result.stdout and not result.stderr
    output = root / "out.c"
    sentinel = b"/* original C17 output */\n"
    output.write_bytes(sentinel)
    before = state(output)
    command = [compiler, "--emit-c17-to", output, source]
    result = run(root, "existing", command, 4)
    assert b"[V1-BACKEND-UNSUPPORTED]" in result.stderr and not result.stdout
    assert output.read_bytes() == sentinel and state(output) == before
    output.unlink()
    result = run(root, "absent", command, 4)
    assert b"[V1-BACKEND-UNSUPPORTED]" in result.stderr and not result.stdout
    assert not output.exists()
    print(source.stem, "existing CHECK success, C17 refusal and preserved output")


def main():
    compiler, cc = sys.argv[1:3]
    if len(sys.argv) == 4:
        target = Path(sys.argv[3])
        target.mkdir()
        scope = nullcontext(target)
    else:
        scope = TemporaryDirectory(prefix="p1897-ordinary-widening-")
    with scope as directory:
        root = Path(directory)
        for name, properties in (
            ("caller-local", (64,)),
            ("usize-call-control", (None,)),
            ("literal-actual", (None, True)),
            ("runtime-call", (64, True)),
            ("runtime-named-tail", (0, True, False, True)),
            ("priority-low", (0, True, True)),
            ("priority-high", (64, True, True)),
        ):
            case = root / name
            case.mkdir()
            positive(case, compiler, cc, name, *properties)
        for name, diagnostic in (
            ("wrong-actual", "P3-TYPE-MISMATCH"),
            ("nominal-reject", "NUMERIC-CONVERSION-TYPE"),
            ("unguarded-narrow", "NUMERIC-CONVERSION-PRECISION"),
            ("different-formal", "NUMERIC-CONVERSION-PRECISION"),
        ):
            case = root / name
            case.mkdir()
            refusal(case, compiler, here / "source" / f"{name}.nl", diagnostic)
        case = root / "unused-conversion"
        case.mkdir()
        unsupported(case, compiler, here / "source/unused-conversion.nl")


if __name__ == "__main__":
    main()
