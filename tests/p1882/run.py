"""Verify known named numeric conversions on ordinary checked source."""

from contextlib import nullcontext
from pathlib import Path
import re
import runpy
import sys
from tempfile import TemporaryDirectory


sources = Path(__file__).resolve().parent
support = runpy.run_path(str(sources.parent / "p1875" / "run.py"))
run, state, native = (support[name] for name in ("run", "state", "native"))


def positive(root, compiler, cc, source, targets, values, extra=None):
    checked = run(root, "check", [compiler, "--check", source], 0)
    output = root / "raw.c"
    emitted = run(root, "emit", [compiler, "--emit-c17-to", output, source], 0)
    assert not any((checked.stdout, checked.stderr, emitted.stdout, emitted.stderr))
    raw = output.read_bytes()
    casts = re.findall(
        rb"^    (?:const )?(uint8_t|size_t|uintptr_t) (nl_local_\d+) = \(\1\)nl_local_\d+;$",
        raw, re.M,
    )
    assert [target.decode() for target, _ in casts] == list(targets)
    observed = [symbol for _, symbol in casts]
    if extra is not None:
        extras = re.findall(extra, raw, re.M)
        assert len(extras) == 1
        observed.extend(extras)
    observed.extend(re.findall(rb"^    const _Bool (nl_local_\d+) = .*;$", raw, re.M))
    assert len(observed) == len(values)
    point = raw.rindex(b"\n}\n\nint main")
    specifiers = b",".join([b"%ju"] * len(observed))
    args = b", ".join(b"(uintmax_t)" + name for name in observed)
    watched = root / "observed.c"
    watched.write_bytes(b"#include <stdio.h>\n" + raw[:point] +
                        b'\n    printf("' + specifiers + b'\\n", ' + args +
                        b");" + raw[point:])
    expected = (",".join(map(str, values)) + "\n").encode("ascii")
    native(root, cc, output, b"")
    native(root, cc, watched, expected)
    print(source.stem, expected.decode("ascii").strip())


def refusal(root, compiler, source, diagnostic):
    expected = f"[{diagnostic}]".encode("ascii")
    result = run(root, "check", [compiler, "--check", source], 3)
    assert not result.stdout and expected in result.stderr
    output = root / "out.c"
    sentinel = b"/* prior numeric-conversion output */\n"
    output.write_bytes(sentinel)
    before = state(output)
    command = [compiler, "--emit-c17-to", output, source]
    result = run(root, "existing", command, 3)
    assert not result.stdout and expected in result.stderr
    assert output.read_bytes() == sentinel and state(output) == before
    output.unlink()
    result = run(root, "absent", command, 3)
    assert not result.stdout and expected in result.stderr and not output.exists()
    print(source.stem, diagnostic, "named output preserved")


def main():
    compiler, cc = sys.argv[1:3]
    if len(sys.argv) == 4:
        output = Path(sys.argv[3])
        output.mkdir()
        work = nullcontext(output)
    else:
        work = TemporaryDirectory(prefix="p1882-explicit-numeric-")
    with work as directory:
        root = Path(directory)
        successes = (
            ("widen-u8", ("size_t",), (42, 1), None),
            ("narrow-usize", ("uint8_t",), (255, 255),
             rb"^    const uint8_t (nl_local_\d+) = nl_local_\d+;$"),
            ("zero-roundtrip", ("size_t", "uint8_t"), (0, 0), None),
            ("octet-point-and-back", ("uintptr_t", "uint8_t", "size_t"),
             (255, 255, 255, 1), None),
            ("known-expression-roundtrip", ("uint8_t", "size_t"), (19, 19, 1), None),
            ("current-value", ("uint8_t", "size_t"), (9, 9, 7, 1),
             rb"^    const uint8_t (nl_local_\d+) = nl_loan_result_\d+;$"),
        )
        for name, targets, values, extra in successes:
            case = root / name
            case.mkdir()
            positive(case, compiler, cc, sources / (name + ".nl"), targets, values, extra)
        for name, diagnostic in (
            ("usize-range-reject", "NUMERIC-CONVERSION-RANGE"),
            ("coordinate-range-reject", "NUMERIC-CONVERSION-RANGE"),
            ("bool-reject", "NUMERIC-CONVERSION-TYPE"),
            ("ptr-reject", "NUMERIC-CONVERSION-TYPE"),
            ("addr-reject", "NUMERIC-CONVERSION-TYPE"),
            ("unknown-narrow-reject", "NUMERIC-CONVERSION-PRECISION"),
            ("compound-reject", "NUMERIC-CONVERSION-PROFILE"),
        ):
            case = root / name
            case.mkdir()
            refusal(case, compiler, sources / (name + ".nl"), diagnostic)


if __name__ == "__main__":
    main()
