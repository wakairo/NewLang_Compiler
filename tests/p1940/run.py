"""Check a readonly byte call followed by independent result and input Copies."""
from argparse import ArgumentParser
from contextlib import nullcontext
from pathlib import Path
import re
import runpy
from tempfile import TemporaryDirectory

from cases import cases, refusals


SUPPORT = runpy.run_path(str(Path(__file__).resolve().parents[1] / "p1923" / "run.py"))
run, state = (SUPPORT[name] for name in ("run", "state"))
LOCAL = rb"nl_local_\d+"
FUNCTION = rb"nl_fn_\d+"
BYTE = rb"uint8_t"


def caller(data, source):
    entry = re.search(rb"^int main\(void\)\n\{\n    (" + FUNCTION + rb")\(\);$", data, re.M)
    assert entry
    body = function(data, b"void", entry[1], b"void")
    declarations = list(re.finditer(
        rb"^    const (size_t|uint8_t|_Bool) (" + LOCAL + rb") = ([^;]+);$", body[1], re.M,
    ))
    main = source.read_bytes().split(b"fn main() -> unit {\n", 1)[1]
    names = re.findall(rb"^    let (\w+) = .+;$", main, re.M)
    assert len(names) == len(declarations)
    return body, dict(zip(names, declarations))


def function(data, result, name, arguments):
    body = re.search(rb"^static " + result + b" " + name + rb"\(" + arguments +
                     rb"\)\n\{\n(.*?)^\}", data, re.M | re.S)
    assert body
    return body


def before_return(body):
    end = body.rfind(b"    return;\n")
    return len(body) if end == -1 else end


def trace(label, symbols):
    fields = b",".join([b"%u"] * len(symbols))
    values = b", ".join(b"(unsigned)" + symbol for symbol in symbols)
    return b'    printf("' + label + b"," + fields + b'\\n", ' + values + b");\n"


def observe(data, source, oracle):
    body, names = caller(data, source)
    chosen = names[oracle["selection"].encode()]
    original = names[b"original_copy"]
    assert chosen[1] == original[1] == BYTE and original[3] == chosen[2]
    assert chosen.start() < original.start()
    writes = []
    variables = [chosen[2], original[2]]
    kind = oracle["kind"]
    if kind in ("scalar", "selector", "unit"):
        actual = names[oracle["actual"].encode()]
        assert actual[1] == BYTE
        if actual is not chosen:
            assert actual[3] == chosen[2] and chosen.start() < actual.start()
        if kind == "unit":
            (site,) = re.finditer(rb"^    (" + FUNCTION + rb")\((" + LOCAL + rb")\);$",
                                  body[1], re.M)
            assert site[2] == actual[2]
            callee, observations = observe_callee(data, site[1], b"void", False)
        else:
            bound, copied = names[b"returned"], names[b"returned_copy"]
            assert bound[1] == copied[1] == BYTE and copied[3] == bound[2]
            assert re.fullmatch(rb"nl_call_\d+", bound[3])
            (site,) = re.finditer(rb"^    const uint8_t " + bound[3] + rb" = (" +
                                  FUNCTION + rb")\((" + LOCAL + rb")\);$", body[1], re.M)
            assert site[2] == actual[2]
            assert site.end() < bound.start() < min(copied.start(), original.start())
            assert len({chosen[2], bound[2], copied[2], original[2]}) == 4
            callee, observations = observe_callee(data, site[1], BYTE, oracle["distinct"])
            variables = [chosen[2], actual[2], bound[2], copied[2], original[2]]
        assert body[1].count(site[1] + b"(") == 1
        assert actual.start() < site.start() < original.start()
        writes.append((callee.start(1) + observations[0], trace(b"callee", observations[1])))
    writes.append((body.start(1) + before_return(body[1]), trace(b"caller", variables)))
    for offset, addition in sorted(writes, reverse=True):
        data = data[:offset] + addition + data[offset:]
    return b"#include <stdio.h>\n" + data


def observe_callee(data, name, result, distinct):
    callee = function(data, result, name, rb"uint8_t " + LOCAL)
    signature = callee[0].split(b"\n", 1)[0]
    (formal,) = re.findall(LOCAL, signature)
    (copy,) = re.findall(rb"^    const uint8_t (" + LOCAL + rb") = " + formal + rb";$",
                         callee[1], re.M)
    if result == b"void":
        assert callee[1].count(b"    (void)" + copy + b";\n") == 2
        return callee, (before_return(callee[1]), [formal, copy])
    (ret,) = re.finditer(rb"^    return (" + LOCAL + rb");$", callee[1], re.M)
    if distinct:
        (code,) = re.findall(rb"^    const uint8_t (" + LOCAL + rb") = 87;$", callee[1], re.M)
        assert ret[1] == code and code != copy
    else:
        assert ret[1] == copy
    return callee, (ret.start(), [formal, copy, ret[1]])


def prediction(oracle):
    selected = oracle["selected"]
    if oracle["kind"] == "if-only":
        return f"caller,{selected},{selected}\n".encode()
    if oracle["kind"] == "unit":
        return f"callee,{selected},{selected}\ncaller,{selected},{selected}\n".encode()
    returned = oracle["returned"]
    return (f"callee,{selected},{selected},{returned}\n"
            f"caller,{selected},{selected},{returned},{returned},{selected}\n").encode()


def positive(directory, source, oracle, compiler, cc, sanitizer):
    raw, watched = directory / "raw.c", directory / "observed.c"
    checked = run(directory, "check", [compiler, "--check", source], 0)
    emitted = run(directory, "emit", [compiler, "--emit-c17-to", raw, source], 0)
    assert not (checked.stdout or checked.stderr or emitted.stdout or emitted.stderr)
    watched.write_bytes(observe(raw.read_bytes(), source, oracle))
    expected = prediction(oracle)
    flags = [cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror"]
    if sanitizer and sanitizer != "none":
        flags.extend(["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all"])
        if "clang" in Path(cc).name:
            flags.append("-Wno-error=gcc-install-dir-libstdcxx")
    for opt in ("-O0", "-O2"):
        for program, payload in ((raw, b""), (watched, expected)):
            executable = directory / (program.stem + opt)
            run(directory, executable.name + ".build", [*flags, opt, program, "-o", executable], 0)
            result = run(directory, executable.name + ".run", [executable], 0)
            assert result.stdout == payload and not result.stderr
            executable.unlink()
    print(source.stem, "C17 O0/O2", expected.decode().strip().replace("\n", " -> "))


def negative(directory, source, status, compiler):
    check, emit = status
    diagnostic = b"[P3-TYPE-MISMATCH]" if check else b"[V1-BACKEND-UNSUPPORTED]"
    checked = run(directory, "check", [compiler, "--check", source], check)
    assert not checked.stdout and (diagnostic in checked.stderr if check else not checked.stderr)
    named = directory / "named"
    named.mkdir()
    old, absent = named / "existing.c", named / "absent.c"
    sentinel = b"/* prior complete output */\n"
    old.write_bytes(sentinel)
    before = state(old)
    for target in (old, absent):
        failed = run(directory, target.stem, [compiler, "--emit-c17-to", target, source], emit)
        assert not failed.stdout and diagnostic in failed.stderr
    assert state(old) == before and old.read_bytes() == sentinel and list(named.iterdir()) == [old]
    print(source.stem, "CHECK", check, "C17", emit, "output preserved")


def suite(args, output):
    accepted = rejected = 0
    for name, text, oracle in cases():
        directory = output / name
        directory.mkdir()
        source = directory / (name + ".nl")
        source.write_bytes(text.encode())
        # CHECK's readonly receipt covers a normal chain of Copies. The same
        # caller with a literal-return helper has no such finished receipt.
        if oracle["kind"] == "scalar" and oracle["distinct"]:
            negative(directory, source, (0, 4), args.compiler)
            rejected += 1
        else:
            positive(directory, source, oracle, args.compiler, args.cc, args.sanitizer)
            accepted += 1
    for name, text, status in refusals():
        directory = output / name
        directory.mkdir()
        source = directory / (name + ".nl")
        source.write_bytes(text.encode())
        negative(directory, source, status, args.compiler)
        rejected += 1
    assert accepted == 60 and rejected == 27
    print(f"{accepted} positive sources, untouched and observed native O0/O2; "
          f"{rejected} refusals with output preserved")


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("compiler")
    parser.add_argument("cc")
    parser.add_argument("sanitizer")
    parser.add_argument("--raw", type=Path)
    args = parser.parse_args()
    if args.raw is None:
        work = TemporaryDirectory(prefix="p1940-if-scalar-result-")
    else:
        args.raw.mkdir()
        work = nullcontext(args.raw)
    with work as output:
        suite(args, Path(output))
