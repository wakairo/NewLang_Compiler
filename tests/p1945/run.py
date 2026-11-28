"""Observe two real widening results feeding one ordinary caller IF."""
from argparse import ArgumentParser
from contextlib import nullcontext
from pathlib import Path
import re
import runpy
import sys
from tempfile import TemporaryDirectory

from fixtures import negatives, positives

sys.dont_write_bytecode = True
support = Path(__file__).resolve().parents[1] / "p1931"
sys.path.insert(0, str(support))
try:
    tests = runpy.run_path(str(support / "run.py"))
finally:
    sys.path.pop(0)
observe, run, state = (tests[name] for name in ("observe", "run", "state"))


def with_callee_trace(data, has_call):
    helpers = list(re.finditer(
        rb"^static size_t nl_fn_\d+\(uint8_t (nl_local_\d+)\)\n\{\n(.*?)^\}",
        data, re.S | re.M,
    ))
    if not has_call:
        assert not helpers
        return data
    (helper,) = helpers
    (result,) = re.finditer(rb"^    return (nl_local_\d+);$", helper[2], re.M)
    assert re.search(rb"^    const size_t " + result[1] + rb" = \(size_t\)" +
                     helper[1] + rb";$", helper[2], re.M)
    at = helper.start(2) + result.start()
    trace = (b'    printf("call,%u,%ju\\n", (unsigned)' + helper[1] +
             b", (uintmax_t)" + result[1] + b");\n")
    return data[:at] + trace + data[at:]


def positive(directory, source, oracle, compiler, cc, sanitizer):
    raw = directory / "raw.c"
    checked = run(directory, "check", [compiler, "--check", source], 0)
    emitted = run(directory, "emit", [compiler, "--emit-c17-to", raw, source], 0)
    assert not (checked.stdout or checked.stderr or emitted.stdout or emitted.stderr)
    observed = directory / "observed.c"
    traced = observe(raw.read_bytes(), source, oracle["names"])
    observed.write_bytes(with_callee_trace(traced, bool(oracle["calls"])))
    expected = "".join(f"call,{v},{v}\n" for v in oracle["calls"])
    expected += ",".join(map(str, oracle["values"])) + "\n"
    expected = expected.encode()
    flags = [cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror"]
    if sanitizer and sanitizer != "none":
        flags += ["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all"]
        if "clang" in Path(cc).name:
            flags.append("-Wno-error=gcc-install-dir-libstdcxx")
    for opt in ("-O0", "-O2"):
        for program, stdout in ((raw, b""), (observed, expected)):
            executable = directory / (program.stem + opt)
            run(directory, executable.name + ".build", [*flags, opt, program, "-o", executable], 0)
            result = run(directory, executable.name + ".run", [executable], 0)
            assert result.stdout == stdout and not result.stderr
            executable.unlink()
    print(source.stem, "C17 O0/O2", expected.decode().strip().replace("\n", " -> "), flush=True)


def negative(directory, source, status, compiler):
    check, emit, code = status
    diagnostic = f"[{code}]".encode()
    checked = run(directory, "check", [compiler, "--check", source], check)
    assert not checked.stdout and (diagnostic in checked.stderr if check else not checked.stderr)
    named = directory / "named"
    named.mkdir()
    old, absent = named / "existing.c", named / "absent.c"
    sentinel = b"/* complete prior C17 output */\n"
    old.write_bytes(sentinel)
    previous = state(old)
    for target in (old, absent):
        result = run(directory, target.stem, [compiler, "--emit-c17-to", target, source], emit)
        assert not result.stdout and diagnostic in result.stderr
    assert old.read_bytes() == sentinel and state(old) == previous and list(named.iterdir()) == [old]
    print(source.stem, "CHECK", check, "C17", emit, "output preserved", flush=True)


def suite(args, root):
    accepted = rejected = 0
    for name, text, oracle in positives():
        directory = root / name
        directory.mkdir()
        source = directory / (name + ".nl")
        source.write_bytes(text.encode())
        positive(directory, source, oracle, args.compiler, args.cc, args.sanitizer)
        accepted += 1
    for name, text, status in negatives():
        directory = root / name
        directory.mkdir()
        source = directory / (name + ".nl")
        source.write_bytes(text.encode())
        negative(directory, source, status, args.compiler)
        rejected += 1
    assert accepted == 104 and rejected == 12
    print(f"{accepted} positive sources and {rejected} output-preserving refusals")


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("compiler")
    parser.add_argument("cc")
    parser.add_argument("sanitizer")
    parser.add_argument("--raw", type=Path)
    args = parser.parse_args()
    if args.raw is None:
        work = TemporaryDirectory(prefix="p1945-two-widening-if-")
    else:
        args.raw.mkdir()
        work = nullcontext(args.raw)
    with work as output:
        suite(args, Path(output))
