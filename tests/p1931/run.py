"""Measure checked caller comparisons against the original C operands."""
from pathlib import Path
import re
import runpy
import sys
from tempfile import TemporaryDirectory

sys.dont_write_bytecode = True
from cases import cases, mixed_producers, refusals

HERE = Path(__file__).resolve().parent
SUPPORT = runpy.run_path(str(HERE.parent / "p1875" / "run.py"))
run, state = SUPPORT["run"], SUPPORT["state"]
CALLS = {
    b"sample_octet": (b"uint8_t", b"size_t"),
    b"read_slots": (b"uint8_t", b"size_t"),
    b"quantity_from_octet": (b"size_t", b"uint8_t"),
}


def observe(data, source, observed_names):
    entry = re.search(rb"^int main\(void\)\n\{\n    (nl_fn_\d+)\(\);$", data, re.M)
    assert entry
    body = re.search(rb"^static void " + entry[1] + rb"\(void\)\n\{\n(.*?)^\}", data, re.M | re.S)
    assert body
    matches = re.findall(rb"^    const (size_t|uint8_t|_Bool) (nl_\w+_\d+) = ([^;]+);$", body[1], re.M)
    declarations = {name: (type_name, value) for type_name, name, value in matches}
    assert len(declarations) == len(matches)
    locals_ = [name for name in declarations if name.startswith(b"nl_local_")]
    main = source.read_bytes().split(b"fn main() -> unit {\n", 1)[1]
    lets = re.findall(rb"^    let (\w+) = (.+);$", main, re.M)
    assert len(lets) == len(locals_)
    names, calls, staged = {}, [], set()

    def operand(text):
        literal = re.fullmatch(rb"usize\((\d+)\)", text)
        return b"((size_t)UINTMAX_C(" + literal[1] + b"))" if literal else names[text]

    def comparison(expression, emitted):
        left, operator, right = re.fullmatch(rb"(.+) (==|<) (.+)", expression).groups()
        if operator == b"==":
            assert emitted == b"(" + operand(left) + b" == " + operand(right) + b")"
            return
        pair = re.fullmatch(rb"\((nl_less_left_(\d+)) < (nl_less_right_\2)\)", emitted)
        assert pair
        a, b = pair[1], pair[3]
        assert a not in staged and b not in staged
        staged.update((a, b))
        assert declarations[a] == (b"size_t", operand(left))
        assert declarations[b] == (b"size_t", operand(right))
        assert body[1].index(b"const size_t " + a) < body[1].index(b"const size_t " + b) < body[1].index(emitted)

    for (name, init), local in zip(lets, locals_, strict=True):
        type_name, value = declarations[local]
        select = re.fullmatch(rb"if \((.+)\) \{ u8\((\d+)\) \} else \{ u8\((\d+)\) \}", init)
        call = re.fullmatch(rb"(\w+)\((\w+)\)", init)
        if select:
            target = re.fullmatch(rb"\((.+) \? (\d+) : (\d+)\)", value)
            assert type_name == b"uint8_t" and target and target.groups()[1:] == select.groups()[1:]
            if select[1] in names:
                assert target[1] == names[select[1]] and declarations[target[1]][0] == b"_Bool"
            else:
                comparison(select[1], target[1])
        elif call and call[1] in CALLS:
            returned, parameter = CALLS[call[1]]
            function = re.fullmatch(rb"(nl_fn_\d+)\(" + names[call[2]] + rb"\)", declarations[value][1])
            assert declarations[value][0] == type_name == returned and function
            assert re.search(rb"^static " + returned + b" " + function[1] + rb"\(" + parameter +
                             rb" nl_local_\d+\)$", data, re.M)
            calls.append(value)
        elif call and call[1] == b"usize" and call[2] in names:
            assert type_name == b"size_t" and declarations[names[call[2]]][0] == b"uint8_t"
            assert value == b"(size_t)" + names[call[2]]
        elif init in names:
            assert (type_name, value) == (declarations[names[init]][0], names[init])
        elif b" == " in init or b" < " in init:
            assert type_name == b"_Bool"
            comparison(init, value)
        names[name] = local
    assert calls == [name for name in declarations if name.startswith(b"nl_call_")]
    assert len(re.findall(rb"\bnl_fn_\d+\(", body[1])) == len(calls)
    assert staged == {name for name in declarations if name.startswith(b"nl_less_")}

    locals_to_print = [names[name.encode()] for name in observed_names.split()]
    fmt = b",".join([b"%ju"] * len(locals_to_print))
    values = b", ".join(b"(uintmax_t)" + value for value in locals_to_print)
    trace = b'    printf("' + fmt + b'\\n", ' + values + b");\n"
    end = body[1].rfind(b"    return;\n")
    at = body.end(1) if end == -1 else body.start(1) + end
    return b"#include <inttypes.h>\n#include <stdio.h>\n" + data[:at] + trace + data[at:]


def native(root, cc, source, expected, sanitizer):
    for optimization in ("-O0", "-O2"):
        executable = root / (source.stem + optimization)
        flags = [cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror", optimization]
        if sanitizer and sanitizer != "none":
            flags += ["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all"]
        run(root, executable.name + ".build", [*flags, source, "-o", executable], 0)
        result = run(root, executable.name + ".run", [executable], 0)
        assert result.stdout == expected and not result.stderr
        executable.unlink()


def positive(root, compiler, cc, source, names, expected, sanitizer):
    check = run(root, "check", [compiler, "--check", source], 0)
    raw = root / "raw.c"
    emitted = run(root, "emit", [compiler, "--emit-c17-to", raw, source], 0)
    assert not (check.stdout or check.stderr or emitted.stdout or emitted.stderr)
    traced = root / "observed.c"
    traced.write_bytes(observe(raw.read_bytes(), source, names))
    native(root, cc, raw, b"", sanitizer)
    native(root, cc, traced, (",".join(map(str, expected)) + "\n").encode(), sanitizer)
    print(source.stem, "strict C17 O0/O2", ",".join(map(str, expected)), flush=True)


def refusal(root, compiler, source, check_status, emit_status, code):
    diagnostic = f"[{code}]".encode()
    check = run(root, "check", [compiler, "--check", source], check_status)
    assert not check.stdout and (diagnostic in check.stderr if check_status else not check.stderr)
    old, absent = root / "existing.c", root / "absent.c"
    sentinel = b"/* P1931 prior complete C17 output */\n"
    old.write_bytes(sentinel)
    before = state(old)
    for target in (old, absent):
        emitted = run(root, target.stem, [compiler, "--emit-c17-to", target, source], emit_status)
        assert not emitted.stdout and diagnostic in emitted.stderr
    assert state(old) == before and old.read_bytes() == sentinel and not absent.exists()
    print(source.stem, "refusal and output preserved", flush=True)


def suite(root, compiler, cc, sanitizer):
    for name, source, oracle in cases():
        directory = root / name
        directory.mkdir()
        nl = directory / (name + ".nl")
        nl.write_text(source)
        names = "octet quantity reading prior_equal "
        row = [oracle["value"]] * 3 + [oracle["equal_before"]]
        if oracle["condition"] == "named":
            names += "compare flag "
            row += [oracle["branch"]] * 2
        names += "chosen saved later_less summary"
        row += [oracle["selected"]] * 2 + [oracle["less_after"]] * 2
        positive(directory, compiler, cc, nl, names, row, sanitizer)
    witness = root / "adopted-p1907-caller-if"
    witness.mkdir()
    positive(witness, compiler, cc, HERE.parent / "p1907/source/caller-if.nl",
             "octet slots exact next", [219, 219, 1, 23], sanitizer)
    migrated = root / "mixed-producers"
    migrated.mkdir()
    nl = migrated / "mixed-producers.nl"
    nl.write_text(mixed_producers(), encoding="utf-8")
    positive(migrated, compiler, cc, nl,
             "octet quantity reading bound prior_equal compare flag chosen saved later_less summary",
             [207, 207, 207, 207, 1, 0, 0, 163, 163, 0, 0], sanitizer)
    for name, (source, expected) in refusals().items():
        directory = root / name
        directory.mkdir()
        nl = directory / (name + ".nl")
        nl.write_text(source)
        refusal(directory, compiler, nl, *expected)


if __name__ == "__main__":
    compiler, cc, sanitizer = sys.argv[1:4]
    if len(sys.argv) == 5:
        raw = Path(sys.argv[4])
        raw.mkdir()
        suite(raw, compiler, cc, sanitizer)
    else:
        with TemporaryDirectory(prefix="p1931-caller-local-widening-if-") as directory:
            suite(Path(directory), compiler, cc, sanitizer)
