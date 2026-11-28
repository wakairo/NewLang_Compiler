"""Measure ordinary caller comparisons on the current widening results."""

from pathlib import Path
import json
import re
import runpy
import sys
from tempfile import TemporaryDirectory


HERE = Path(__file__).resolve().parent
support = runpy.run_path(str(HERE.parent / "p1875" / "run.py"))
run, state = support["run"], support["state"]
OBSERVATIONS = json.loads((HERE / "observations.json").read_text(encoding="utf-8"))


def caller_locals(data, source):
    # The entry and its locals are emitted in source order, with a typed C
    # temporary for each real call and for each ordered comparison operand.
    entry = re.search(rb"^int main\(void\)\n\{\n    (nl_fn_\d+)\(\);$", data, re.M)
    assert entry
    body = re.search(rb"^static void " + entry[1] +
                     rb"\(void\)\n\{\n((?:[^\n]*\n)*?)^\}", data, re.M)
    assert body
    declarations = dict(re.findall(
        rb"^    const (?:size_t|uint8_t|_Bool) (nl_\w+_\d+) = ([^;]+);$",
        body[1], re.M,
    ))
    locals_in_order = [name for name in declarations if name.startswith(b"nl_local_")]
    main = source.read_bytes().split(b"fn main() -> unit {\n", 1)[1]
    lets = re.findall(rb"^    let (\w+) = ([^;]+);$", main, re.M)
    assert len(lets) == len(locals_in_order)
    names = {}
    calls = []

    def operand(text):
        literal = re.fullmatch(rb"usize\((\d+)\)", text)
        return b"((size_t)UINTMAX_C(" + literal[1] + b"))" if literal else names[text]

    for (name, initializer), local in zip(lets, locals_in_order):
        emitted = declarations[local]
        comparison = re.fullmatch(rb"(.+) (==|<) (.+)", initializer)
        call = re.fullmatch(
            rb"(read_window|window_count|queue_mark|read_slots|widen_slots)\((\w+)\)",
            initializer,
        )
        widening = re.fullmatch(rb"usize\((\w+)\)", initializer)
        if comparison:
            left, op, right = comparison.groups()
            if op == b"==":
                assert emitted == b"(" + operand(left) + b" == " + operand(right) + b")"
            else:
                ordered = re.fullmatch(rb"\((nl_less_left_(\d+)) < (nl_less_right_\2)\)", emitted)
                assert ordered
                assert (declarations[ordered[1]], declarations[ordered[3]]) == (
                    operand(left), operand(right))
        elif call:
            assert emitted.startswith(b"nl_call_")
            invoked = re.fullmatch(rb"(nl_fn_\d+)\(" + names[call[2]] + rb"\)",
                                   declarations[emitted])
            assert invoked
            if call[1] in (b"window_count", b"widen_slots"):
                assert re.search(rb"^static size_t " + invoked[1] +
                                 rb"\(uint8_t nl_local_\d+\)$", data, re.M)
            calls.append(emitted)
        elif widening and widening[1] in names:
            assert emitted == b"(size_t)" + names[widening[1]]
        elif initializer in names:
            assert emitted == names[initializer]
        names[name] = local
    assert calls == [name for name in declarations if name.startswith(b"nl_call_")]
    return body, names


def observation(data, source, case):
    body, names = caller_locals(data, source)
    locals_to_print = [names[name] for name in case["names"].encode().split()]
    fields = b",".join([b"%ju"] * len(locals_to_print))
    args = b", ".join(b"(uintmax_t)" + name for name in locals_to_print)
    watch = b'    printf("' + fields + b'\\n", ' + args + b");\n"
    # Observe existing C locals just before the explicit unit return.
    at = body[1].rindex(b"    return;\n")
    caller = body[1][:at] + watch + body[1][at:]
    headers = b"#include <inttypes.h>\n#include <stdio.h>\n"
    tail = data[body.end(1):]
    if "constant" in case:
        return headers + data[:body.start(1)] + caller + tail
    threshold = str(case.get("threshold", 64)).encode()
    seed = re.compile(rb"^    const size_t " + names[b"load"] +
                      rb" = \(\(size_t\)UINTMAX_C\(" + threshold + rb"\)\);$", re.M)
    caller, count = seed.subn(b"    const size_t " + names[b"load"] + b" = test_load;", caller)
    assert count == 1
    assert tail.count(b"int main(void)") == 1
    tail = tail.replace(b"int main(void)", b"static int compiled_main(void)", 1)
    driver = b"""
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    char *end;
    errno = 0;
    const uintmax_t input = strtoumax(argv[1], &end, 10);
    if (errno || end == argv[1] || *end || input > SIZE_MAX)
        return 2;
    test_load = (size_t)input;
    return compiled_main();
}
"""
    return (headers + b"#include <errno.h>\n#include <stddef.h>\nstatic size_t test_load;\n" +
            data[:body.start(1)] + caller + tail + driver)


def exercise(root, compiler, cc, name, source=None):
    root.mkdir()
    source = source if source is not None else HERE / "source" / (name + ".nl")
    check = run(root, "check", [compiler, "--check", source], 0)
    c = root / "raw.c"
    emission = run(root, "emit", [compiler, "--emit-c17-to", c, source], 0)
    assert not any((check.stdout, check.stderr, emission.stdout, emission.stderr))
    raw = c.read_bytes()
    case = OBSERVATIONS["cases"][name]
    watched = root / "observed.c"
    watched.write_bytes(observation(raw, source, case))
    width = re.search(rb"_Static_assert\(SIZE_MAX == UINTMAX_C\((\d+)\)", raw)
    assert width
    inputs = [int(width[1]) if v == "SIZE_MAX" else int(v)
              for v in case.get("runtime_inputs", OBSERVATIONS["runtime_inputs"])]
    for opt in ("-O0", "-O2"):
        flags = [cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror", opt]
        plain, trace = root / ("raw" + opt), root / ("observed" + opt)
        run(root, plain.name + ".build", [*flags, c, "-o", plain], 0)
        result = run(root, plain.name + ".run", [plain], 0)
        assert not result.stdout and not result.stderr
        run(root, trace.name + ".build", [*flags, watched, "-o", trace], 0)
        threshold = case.get("threshold", 64)
        expected = ([(None, case["constant"])] if "constant" in case else
                    [(v, case["low"] if v < threshold else case["high"]) for v in inputs])
        for value, row in expected:
            args = [trace] if value is None else [trace, value]
            result = run(root, f"{trace.name}.{value}.run", args, 0)
            assert result.stdout == (",".join(map(str, row)) + "\n").encode()
            assert not result.stderr
    print(name, "C17 and native O0/O2")


def refusal(root, compiler, name, check_status, emit_status, diagnostic):
    root.mkdir()
    source = HERE / "source" / (name + ".nl")
    check = run(root, "check", [compiler, "--check", source], check_status)
    assert not check.stdout
    assert diagnostic in check.stderr if check_status else not check.stderr
    existing = root / "existing.c"
    absent = root / "absent.c"
    sentinel = b"/* P1907 prior output */\n"
    existing.write_bytes(sentinel)
    before = state(existing)
    for target in (existing, absent):
        result = run(root, target.stem, [compiler, "--emit-c17-to", target, source], emit_status)
        assert diagnostic in result.stderr and not result.stdout
    assert not absent.exists() and existing.read_bytes() == sentinel and state(existing) == before
    print(name, "refusal and output preserved")


def suite(root, compiler, cc):
    for name in OBSERVATIONS["cases"]:
        exercise(root / name, compiler, cc, name)
    for name in ("wrong-nominal", "local-wrong-nominal"):
        refusal(root / name, compiler, name, 3, 3, b"[USIZE-EQUALITY-TYPE]")
    refusal(root / "inline-operand", compiler, "inline-operand", 3, 3,
            b"[USIZE-EQUALITY-SOURCE-PRECISION]")
    refusal(root / "inline-actual", compiler, "inline-actual", 0, 4,
            b"[V1-BACKEND-UNSUPPORTED]")
    # The frozen caller-if source now reaches strict native in P1931.
    caller = root / "caller-if"
    caller.mkdir()
    source = HERE / "source" / "caller-if.nl"
    checked = run(caller, "check", [compiler, "--check", source], 0)
    emitted = run(caller, "emit", [compiler, source], 0)
    assert not (checked.stdout or checked.stderr or emitted.stderr) and emitted.stdout


if __name__ == "__main__":
    if len(sys.argv) == 4:
        raw = Path(sys.argv[3])
        raw.mkdir()
        suite(raw, *sys.argv[1:3])
    else:
        with TemporaryDirectory(prefix="p1907-widening-comparison-") as directory:
            suite(Path(directory), *sys.argv[1:3])
