"""Exercise the nominal addr data path on ordinary lexical u8 roots."""

from contextlib import nullcontext
from pathlib import Path
import re
import runpy
import sys
from tempfile import TemporaryDirectory


support = runpy.run_path(str(Path(__file__).resolve().parents[1] / "p1654" / "run.py"))
run, state = support["run"], support["state"]


def native(root, cc, c, expected):
    for optimization in ("-O0", "-O2"):
        executable = root / (c.stem + optimization)
        run(root, executable.name + ".build", [
            cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror",
            optimization, c, "-o", executable,
        ], 0)
        result = run(root, executable.name + ".run", [executable], 0)
        assert result.stdout == expected and not result.stderr


def positive(root, compiler, cc, source):
    checked = run(root, "addr.check", [compiler, "--check", source], 0)
    output = root / "addr.c"
    emitted = run(root, "addr.emit", [compiler, "--emit-c17-to", output, source], 0)
    assert not any((checked.stdout, checked.stderr, emitted.stdout, emitted.stderr))
    data = output.read_bytes()
    assert len(re.findall(rb"const uintptr_t nl_local_\d+ = \(uintptr_t\)\(const void \*\)", data)) == 3
    results = list(re.finditer(
        rb"^ +const _Bool (nl_local_\d+) = \(nl_local_\d+ (==|!=) nl_local_\d+\);$",
        data, re.M,
    ))
    assert [match[2] for match in results] == [b"==", b"!=", b"==", b"!="]
    at = results[-1].end()
    observed = root / "observed.c"
    observed.write_bytes(
        b"#include <stdio.h>\n" + data[:at] + b'\n    printf("%d,%d,%d,%d\\n", ' +
        b", ".join(match[1] for match in results) + b");" + data[at:]
    )
    native(root, cc, output, b"")
    native(root, cc, observed, b"1,0,0,1\n")
    print("addr comparison observations 1,0,0,1")


def refusals(root, compiler, source):
    raw = source.read_text(encoding="utf-8")
    ptrs, _ = raw.split("    let x_addr = addr(p);\n", 1)
    addresses, _ = raw.split("    let same_eq =", 1)
    end = "    unit\n}\n"
    cases = {
        "non-pointer": (ptrs + "    let bad = addr(x);\n" + end, "ADDR-PTR-TYPE"),
        "mixed": (
            addresses + "    let size = usize(7);\n    let bad = x_addr == size;\n" + end,
            "ADDR-COMPARISON-TYPE",
        ),
        "quantity-inequality": (
            "fn main() -> unit { let x = usize(7); let y = usize(9); "
            "let bad = x != y; unit }\n", "ADDR-COMPARISON-TYPE",
        ),
        "pointer-equality": (ptrs + "    let bad = p == p;\n" + end, "USIZE-EQUALITY-TYPE"),
        "address-not-pointer": (
            addresses + "    loan_read_ptr(x_addr) { |s| unit };\n" + end,
            "P3-PTR-REQUIRED",
        ),
        "ended-root": (
            "fn main() -> unit {\n"
            "    let p = { let x = u8(7); loan_read(x) { |r| ptr_from_ref(r) } };\n"
            "    let observed = addr(p);\n" + end, "P3-STALE-POINTER",
        ),
    }
    for name, (text, diagnostic) in cases.items():
        case = root / f"{name}.nl"
        case.write_text(text, encoding="utf-8")
        check = run(root, name + ".check", [compiler, "--check", case], 3)
        expected = f"[{diagnostic}]".encode("ascii")
        assert not check.stdout and expected in check.stderr
        output_dir = root / name
        output_dir.mkdir()
        output = output_dir / "out.c"
        sentinel = b"/* existing named output */\n"
        output.write_bytes(sentinel)
        before = state(output)
        command = [compiler, "--emit-c17-to", output, case]
        denied = run(root, name + ".existing", command, 3)
        assert not denied.stdout and expected in denied.stderr
        assert output.read_bytes() == sentinel and state(output) == before
        assert list(output_dir.iterdir()) == [output]
        output.unlink()
        absent = run(root, name + ".absent", command, 3)
        assert not absent.stdout and expected in absent.stderr
        assert not list(output_dir.iterdir())
        print(name, diagnostic, "named outputs preserved")


if __name__ == "__main__":
    compiler, cc, source = sys.argv[1:4]
    if len(sys.argv) == 5:
        output = Path(sys.argv[4])
        output.mkdir()
        work = nullcontext(output)
    else:
        work = TemporaryDirectory(prefix="p1875-local-addr-")
    with work as directory:
        root = Path(directory)
        positive(root, compiler, cc, Path(source))
        refusals(root, compiler, Path(source))
