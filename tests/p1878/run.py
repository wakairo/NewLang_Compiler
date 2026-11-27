"""Ordinary nominal address and coordinate data, including named output."""

from contextlib import nullcontext
from pathlib import Path
import re
import runpy
import sys
from tempfile import TemporaryDirectory


sources = Path(__file__).resolve().parent
support = runpy.run_path(str(sources.parent / "p1875" / "run.py"))
run, state, native = (support[name] for name in ("run", "state", "native"))


def positive(root, compiler, cc, name, values):
    source = sources / (name + ".nl")
    checked = run(root, "check", [compiler, "--check", source], 0)
    output = root / "raw.c"
    emitted = run(root, "emit", [compiler, "--emit-c17-to", output, source], 0)
    assert not any((checked.stdout, checked.stderr, emitted.stdout, emitted.stderr))
    data = output.read_bytes()
    assert b"_Static_assert(UINTPTR_MAX ==" in data
    scalars = re.finditer(
        rb"^    const (_Bool|uintptr_t|size_t) (nl_local_\d+) = (.*);$",
        data, re.M,
    )
    observations = [match for match in scalars if match[1] == b"_Bool" or
                    any(op in match[3] for op in (b" % ", b" + ", b" - "))]
    assert len(observations) == len(values)
    at = observations[-1].end()
    observed = root / "observed.c"
    arguments = b", ".join(b"(uintmax_t)" + match[2] for match in observations)
    specifiers = b",".join([b"%ju"] * len(observations))
    observed.write_bytes(b"#include <stdio.h>\n" + data[:at] +
                         b'\n    printf("' + specifiers + b'\\n", ' +
                         arguments + b");" + data[at:])
    expected = (",".join(map(str, values)) + "\n").encode("ascii")
    native(root, cc, output, b"")
    native(root, cc, observed, expected)
    print(name, expected.decode("ascii").strip())


def refusal(root, compiler, name, diagnostic):
    source = sources / (name + ".nl")
    expected = f"[{diagnostic}]".encode("ascii")
    checked = run(root, "check", [compiler, "--check", source], 3)
    assert not checked.stdout and expected in checked.stderr
    output = root / "output.c"
    sentinel = b"/* p1878 named output */\n"
    output.write_bytes(sentinel)
    before = state(output)
    command = [compiler, "--emit-c17-to", output, source]
    emitted = run(root, "existing", command, 3)
    assert not emitted.stdout and expected in emitted.stderr
    assert output.read_bytes() == sentinel and state(output) == before
    output.unlink()
    emitted = run(root, "absent", command, 3)
    assert not emitted.stdout and expected in emitted.stderr and not output.exists()
    print(name, diagnostic, "named output preserved")


def main():
    compiler, cc = sys.argv[1:3]
    if len(sys.argv) == 4:
        output = Path(sys.argv[3])
        output.mkdir()
        work = nullcontext(output)
    else:
        work = TemporaryDirectory(prefix="p1878-address-data-")
    with work as directory:
        root = Path(directory)
        for name, values in (
            ("coordinate-roundtrip", (1,)),
            ("coordinate-comparison", (1, 0, 0, 1)),
            ("coordinate-residue-literal", (0, 1)),
            ("coordinate-residue-known-literal", (5,)),
            ("coordinate-offset-known", (17, 13, 1)),
        ):
            case = root / name
            case.mkdir()
            positive(case, compiler, cc, name, values)
        for name, diagnostic in (
            ("coordinate-mixed-equality", "UINTPTR-COMPARISON-TYPE"),
            ("coordinate-as-pointer", "P3-PTR-REQUIRED"),
            ("coordinate-mixed-plus", "UINTPTR-ARITHMETIC-TYPE"),
            ("coordinate-residue", "USIZE-REMAINDER-SOURCE-PRECISION"),
            ("coordinate-residue-zero-literal", "UINTPTR-REMAINDER-ZERO"),
            ("coordinate-offset-unknown", "UINTPTR-ARITHMETIC-PRECISION"),
            ("coordinate-offset-underflow", "UINTPTR-OFFSET-RANGE"),
        ):
            case = root / name
            case.mkdir()
            refusal(case, compiler, name, diagnostic)


if __name__ == "__main__":
    main()
