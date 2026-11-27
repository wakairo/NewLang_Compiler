"""Exercise ordinary named byte and u8 conversions over the entire octet range."""

from contextlib import nullcontext
from pathlib import Path
import re
import runpy
import sys
from tempfile import TemporaryDirectory


sources = Path(__file__).resolve().parent
support = runpy.run_path(str(sources.parent / "p1882" / "run.py"))
run, native, positive, refusal = (
    support[name] for name in ("run", "native", "positive", "refusal")
)
octet_declaration = re.compile(
    rb"^    const uint8_t (nl_local_\d+) = (?:(\d+)|\(uint8_t\)(nl_local_\d+));$",
    re.M,
)


def range_case(root, compiler, cc, values, template):
    source = root / (root.name + ".nl")
    source.write_text(
        "fn main() -> unit {\n"
        + "".join(template.format(index=value, value=value) for value in values)
        + "    unit\n}\n",
        encoding="utf-8",
    )
    raw = root / "raw.c"
    checked = run(root, "check", [compiler, "--check", source], 0)
    emitted = run(root, "emit", [compiler, "--emit-c17-to", raw, source], 0)
    assert not any((checked.stdout, checked.stderr, emitted.stdout, emitted.stderr))
    data = raw.read_bytes()
    declarations = octet_declaration.findall(data)
    assert len(declarations) == 4 * len(values)
    observations = []
    for index, value in enumerate(values):
        incoming, encoded, decoded, restored = declarations[4 * index:4 * index + 4]
        assert incoming[1:] == (str(value).encode("ascii"), b"")
        assert encoded[1:] == (b"", incoming[0])
        assert decoded[1:] == (b"", encoded[0])
        assert restored[1:] == (b"", decoded[0])
        observed = b", ".join(
            b"(unsigned)" + symbol[0] for symbol in (incoming, encoded, decoded, restored)
        )
        observations.append(b'\n    printf("%u,%u,%u,%u\\n", ' + observed + b");")

    point = data.rindex(b"\n}\n\nint main")
    observed_c = root / "observed.c"
    observed_c.write_bytes(
        b"#include <stdio.h>\n" + data[:point] + b"".join(observations) + data[point:]
    )
    expected = "".join(f"{value},{value},{value},{value}\n" for value in values)
    native(root, cc, raw, b"")
    native(root, cc, observed_c, expected.encode("ascii"))
    print(values[0], "through", values[-1], "all codec values agree")


def main():
    compiler, cc = sys.argv[1:3]
    if len(sys.argv) == 4:
        output = Path(sys.argv[3])
        output.mkdir()
        work = nullcontext(output)
    else:
        work = TemporaryDirectory(prefix="p1885-byte-codec-")
    with work as directory:
        root = Path(directory)
        for name, targets, expected in (
            ("encode", ("uint8_t",), (255,)),
            ("round-trip", ("uint8_t", "uint8_t", "uint8_t"), (173, 173, 173)),
            ("decoded-consumer", ("uint8_t", "uint8_t", "size_t", "size_t"),
             (173, 173, 173, 173, 1)),
            ("existing-numeric", ("size_t",), (173, 1)),
        ):
            case = root / name
            case.mkdir()
            positive(case, compiler, cc, sources / f"{name}.nl", targets, expected)
        for name, diagnostic in (
            ("arithmetic-reject", "USIZE-ADDITION-TYPE"),
            ("address-reject", "ADDR-PTR-TYPE"),
            ("integer-reject", "NUMERIC-CONVERSION-TYPE"),
            ("wrong-source-reject", "NUMERIC-CONVERSION-TYPE"),
            ("literal-reject", "P5-EXPECTED-EXPRESSION"),
        ):
            case = root / name
            case.mkdir()
            refusal(case, compiler, sources / f"{name}.nl", diagnostic)
        template = (sources / "range-template.txt").read_text(encoding="utf-8")
        for start in range(0, 256, 32):
            case = root / f"range-{start:03d}-{start + 31:03d}"
            case.mkdir()
            range_case(case, compiler, cc, range(start, start + 32), template)


if __name__ == "__main__":
    main()
