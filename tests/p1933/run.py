"""Measure a selected byte at the unit callee and again after the call."""
from argparse import ArgumentParser
from contextlib import nullcontext
from pathlib import Path
import re
import runpy
from tempfile import TemporaryDirectory

from cases import cases, refusals


SUPPORT = runpy.run_path(str(Path(__file__).resolve().parents[1] / "p1923" / "run.py"))
caller_locals, run, state = (SUPPORT[name] for name in ("caller_locals", "run", "state"))


def trace(label, symbols):
    fields = b",".join([b"%u"] * len(symbols))
    values = b", ".join(b"(unsigned)" + symbol for symbol in symbols)
    return b'    printf("' + label + b"," + fields + b'\\n", ' + values + b");\n"


def before_return(body):
    end = body.rfind(b"    return;\n")
    return len(body) if end == -1 else end


def observe(data, source, oracle):
    body, names = caller_locals(data, source)
    selected, actual, later, original = (
        names[key.encode()] for key in (oracle["selected"], oracle["passed"], "later", "original")
    )
    sites = list(re.finditer(rb"^    (nl_fn_\d+)\((nl_local_\d+)\);$", body[1], re.M))
    writes = [(body.start(1) + before_return(body[1]),
               trace(b"caller", [selected, actual, later, original]))]
    if oracle["case"] != "if-only":
        (site,) = sites
        assert site[2] == actual

        def declaration(local):
            return body[1].index(b"const uint8_t " + local + b" = ")

        assert declaration(selected) <= declaration(actual) < site.start()
        assert site.end() < declaration(later) < declaration(original)
        callee = re.search(
            rb"^static void " + site[1] + rb"\(uint8_t (nl_local_\d+)\)\n\{\n(.*?)^\}",
            data, re.M | re.S,
        )
        assert callee
        (copy,) = re.findall(
            rb"^    const uint8_t (nl_local_\d+) = " + callee[1] + rb";$",
            callee[2], re.M,
        )
        assert callee[2].count(b"    (void)" + copy + b";\n") == 2
        writes.append((callee.start(2) + before_return(callee[2]),
                       trace(b"callee", [callee[1], copy])))
    else:
        assert not sites
    for at, observation in sorted(writes, reverse=True):
        data = data[:at] + observation + data[at:]
    return b"#include <stdio.h>\n" + data


def native(directory, raw, source, oracle, cc, sanitizer):
    value = str(oracle["value"]).encode()
    row = b"caller," + b",".join([value] * 4) + b"\n"
    if oracle["case"] != "if-only":
        row = b"callee," + value + b"," + value + b"\n" + row
    observed = directory / "observed.c"
    observed.write_bytes(observe(raw.read_bytes(), source, oracle))
    flags = [cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror"]
    if sanitizer and sanitizer != "none":
        flags.extend(["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all"])
        if "clang" in Path(cc).name:
            flags.append("-Wno-error=gcc-install-dir-libstdcxx")
    for opt in ("-O0", "-O2"):
        for program, expected in ((raw, b""), (observed, row)):
            label = program.stem + opt
            executable = directory / label
            run(directory, label + ".build", [*flags, opt, program, "-o", executable], 0)
            result = run(directory, label + ".run", [executable], 0)
            assert result.stdout == expected and not result.stderr
            executable.unlink()
    print(source.stem, "C17 O0/O2", row.decode().strip().replace("\n", " -> "))


def positive(directory, source, oracle, compiler, cc, sanitizer, gate):
    raw = directory / "raw.c"
    if gate is None:
        checked = run(directory, "check", [compiler, "--check", source], 0)
        emitted = run(directory, "emit", [compiler, "--emit-c17-to", raw, source], 0)
        assert not (checked.stdout or checked.stderr or emitted.stdout or emitted.stderr)
    else:
        raw.write_bytes((gate / source.stem / "emit.stdout").read_bytes())
    native(directory, raw, source, oracle, cc, sanitizer)


def negative(directory, source, expected, compiler):
    check, emit = expected
    diagnostic = b"[P3-TYPE-MISMATCH]" if check else b"[V1-BACKEND-UNSUPPORTED]"
    checked = run(directory, "check", [compiler, "--check", source], check)
    assert not checked.stdout and (diagnostic in checked.stderr if check else not checked.stderr)
    named = directory / "named"
    named.mkdir()
    old, absent = named / "existing.c", named / "absent.c"
    sentinel = b"/* original complete output */\n"
    old.write_bytes(sentinel)
    before = state(old)
    for target in (old, absent):
        emitted = run(directory, target.stem, [compiler, "--emit-c17-to", target, source], emit)
        assert not emitted.stdout and diagnostic in emitted.stderr
    assert state(old) == before and old.read_bytes() == sentinel and list(named.iterdir()) == [old]
    print(source.stem, "CHECK", check, "C17", emit, "output preserved")


def suite(args, output):
    cases_run = 0
    for name, text, oracle in cases():
        if args.gate is not None:
            if not (args.gate / name / "emit.status").read_bytes() == b"0\n":
                continue
        elif oracle.get("producer") == "local" and not args.local:
            continue
        directory = output / name
        directory.mkdir()
        source = directory / (name + ".nl")
        source.write_text(text, encoding="utf-8")
        positive(directory, source, oracle, args.compiler, args.cc, args.sanitizer, args.gate)
        cases_run += 1
    assert cases_run
    if args.gate is None:
        for name, text, expected in refusals():
            if name.startswith("local-") and not args.local:
                continue
            directory = output / name
            directory.mkdir()
            source = directory / (name + ".nl")
            source.write_text(text, encoding="utf-8")
            negative(directory, source, expected, args.compiler)
    print(cases_run, "positive sources, each with untouched and observed native O0/O2")


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("compiler")
    parser.add_argument("cc")
    parser.add_argument("sanitizer")
    parser.add_argument("--raw", type=Path)
    parser.add_argument("--gate", type=Path)
    parser.add_argument("--local", action="store_true")
    args = parser.parse_args()
    if args.raw is None:
        work = TemporaryDirectory(prefix="p1933-if-unit-copy-")
    else:
        args.raw.mkdir()
        work = nullcontext(args.raw)
    with work as output:
        suite(args, Path(output))
