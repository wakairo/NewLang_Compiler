"""Observe a caller IF fed by the actual checked widening result."""
import json
from pathlib import Path
import re
import runpy
import sys
from tempfile import TemporaryDirectory


HERE = Path(__file__).resolve().parent
SUPPORT = runpy.run_path(str(HERE.parent / "p1875" / "run.py"))
run, state = SUPPORT["run"], SUPPORT["state"]
OBSERVATIONS = json.loads((HERE / "observations.json").read_bytes())
CALLS = {
    b"sample_slots": (b"uint8_t", b"size_t"),
    b"slots_count": (b"size_t", b"uint8_t"),
    b"pick_label": (b"uint8_t", b"size_t"),
}


def caller_locals(data, source):
    entry = re.search(rb"^int main\(void\)\n\{\n    (nl_fn_\d+)\(\);$", data, re.M)
    assert entry
    body = re.search(rb"^static void " + entry[1] + rb"\(void\)\n\{\n(.*?)^\}", data, re.M | re.S)
    assert body
    matches = re.findall(rb"^    const (size_t|uint8_t|_Bool) (nl_\w+_\d+) = ([^;]+);$", body[1], re.M)
    declarations = {name: (type_name, expression) for type_name, name, expression in matches}
    locals_in_order = [name for name in declarations if name.startswith(b"nl_local_")]
    main = source.read_bytes().split(b"fn main() -> unit {\n", 1)[1]
    lets = re.findall(rb"^    let (\w+) = (.+);$", main, re.M)
    assert len(lets) == len(locals_in_order)
    names, calls = {}, []

    def operand(text):
        literal = re.fullmatch(rb"usize\((\d+)\)", text)
        return b"((size_t)UINTMAX_C(" + literal[1] + b"))" if literal else names[text]

    def check_comparison(test, emitted):
        left, operator, right = re.fullmatch(rb"(.+) (==|<) (.+)", test).groups()
        if operator == b"==":
            assert emitted == b"(" + operand(left) + b" == " + operand(right) + b")"
        else:
            order = re.fullmatch(rb"\((nl_less_left_(\d+)) < (nl_less_right_\2)\)", emitted)
            assert order
            assert declarations[order[1]] == (b"size_t", operand(left))
            assert declarations[order[3]] == (b"size_t", operand(right))
            left_position = body[1].index(b"const size_t " + order[1] + b" =")
            right_position = body[1].index(b"const size_t " + order[3] + b" =")
            assert left_position < right_position < body[1].index(emitted)

    for (name, init), local in zip(lets, locals_in_order):
        type_name, emitted = declarations[local]
        branch = re.fullmatch(rb"if \((.+)\) \{ u8\((\d+)\) \} else \{ u8\((\d+)\) \}", init)
        call = re.fullmatch(rb"(sample_slots|slots_count|pick_label)\((\w+)\)", init)
        if branch:
            c_if = re.fullmatch(rb"\((.+) \? (\d+) : (\d+)\)", emitted)
            assert type_name == b"uint8_t" and c_if and c_if.groups()[1:] == branch.groups()[1:]
            if branch[1] in names:
                assert c_if[1] == names[branch[1]] and declarations[c_if[1]][0] == b"_Bool"
            else:
                check_comparison(branch[1], c_if[1])
        elif call:
            result, formal = CALLS[call[1]]
            assert type_name == result and emitted.startswith(b"nl_call_")
            staged_type, staged_call = declarations[emitted]
            target = re.fullmatch(rb"(nl_fn_\d+)\(" + names[call[2]] + rb"\)", staged_call)
            assert staged_type == result and target
            assert re.search(rb"^static " + result + b" " + target[1] +
                             rb"\(" + formal + rb" nl_local_\d+\)$", data, re.M)
            calls.append(emitted)
        elif init in names:
            assert (type_name, emitted) == (declarations[names[init]][0], names[init])
        elif b" == " in init or b" < " in init:
            assert type_name == b"_Bool"
            check_comparison(init, emitted)
        names[name] = local
    assert calls == [name for name in declarations if name.startswith(b"nl_call_")]
    return body, names


def observation(data, source, case):
    body, names = caller_locals(data, source)
    locals_ = [names[name] for name in case["names"].encode().split()]
    fmt = b",".join([b"%ju"] * len(locals_))
    values = b", ".join(b"(uintmax_t)" + local for local in locals_)
    observe = b'    printf("' + fmt + b'\\n", ' + values + b");\n"
    caller = body[1]
    end = caller.rfind(b"    return;\n")
    if end == -1:
        end = len(caller)
    caller = caller[:end] + observe + caller[end:]
    headers = b"#include <inttypes.h>\n#include <stdio.h>\n"
    tail = data[body.end(1):]
    if "constant" in case:
        return headers + data[:body.start(1)] + caller + tail
    seed = str(OBSERVATIONS["seed"]).encode()
    needle = rb"^    const size_t " + names[b"load"] + rb" = \(\(size_t\)UINTMAX_C\(" + seed + rb"\)\);$"
    caller, count = re.subn(needle, b"    const size_t " + names[b"load"] + b" = test_load;", caller, flags=re.M)
    assert count == 1 and tail.count(b"int main(void)") == 1
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
            data[:body.start(1)] + caller + tail.replace(b"int main(void)", b"static int compiled_main(void)", 1) +
            driver)


def exercise(root, compiler, cc, source, case):
    root.mkdir()
    check = run(root, "check", [compiler, "--check", source], 0)
    raw = root / "raw.c"
    emit = run(root, "emit", [compiler, "--emit-c17-to", raw, source], 0)
    assert not (check.stdout or check.stderr or emit.stdout or emit.stderr)
    data = raw.read_bytes()
    watched = root / "observed.c"
    watched.write_bytes(observation(data, source, case))
    usize = re.search(rb"_Static_assert\(SIZE_MAX == UINTMAX_C\((\d+)\)", data)
    assert usize
    if "constant" in case:
        rows = [(None, case["constant"])]
    else:
        inputs = [int(usize[1]) if n == "SIZE_MAX" else int(n) for n in OBSERVATIONS["runtime_inputs"]]
        rows = [(n, case["low"] if n < OBSERVATIONS["seed"] else case["high"]) for n in inputs]
    for opt in ("-O0", "-O2"):
        strict = [cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror", opt]
        plain, trace = root / ("untouched" + opt), root / ("observed" + opt)
        run(root, plain.name + ".build", [*strict, raw, "-o", plain], 0)
        untouched = run(root, plain.name + ".run", [plain], 0)
        assert not (untouched.stdout or untouched.stderr)
        run(root, trace.name + ".build", [*strict, watched, "-o", trace], 0)
        for value, row in rows:
            observed = run(root, f"{trace.name}.{value}.run",
                           [trace] if value is None else [trace, value], 0)
            assert observed.stdout == (",".join(map(str, row)) + "\n").encode()
            assert not observed.stderr
    print(source.stem, "strict C17 and native O0/O2")


def refusal(root, compiler, source, check_status, emit_status, code):
    root.mkdir()
    diagnostic = f"[{code}]".encode()
    checked = run(root, "check", [compiler, "--check", source], check_status)
    assert not checked.stdout and (diagnostic in checked.stderr if check_status else not checked.stderr)
    sentinel = b"/* P1923 prior output */\n"
    old, absent = root / "existing.c", root / "absent.c"
    old.write_bytes(sentinel)
    before = state(old)
    for target in (old, absent):
        emitted = run(root, target.stem, [compiler, "--emit-c17-to", target, source], emit_status)
        assert not emitted.stdout and diagnostic in emitted.stderr
    assert old.read_bytes() == sentinel and state(old) == before and not absent.exists()
    print(source.stem, "refusal and output preserved")


def suite(root, compiler, cc):
    source_dir = HERE / "source"
    for name, case in OBSERVATIONS["cases"].items():
        exercise(root / name, compiler, cc, source_dir / (name + ".nl"), case)
    for name, expected in OBSERVATIONS["refusals"].items():
        refusal(root / name, compiler, source_dir / (name + ".nl"), *expected)


if __name__ == "__main__":
    if len(sys.argv) == 4:
        raw = Path(sys.argv[3])
        raw.mkdir()
        suite(raw, *sys.argv[1:3])
    else:
        with TemporaryDirectory(prefix="p1923-caller-comparison-if-") as directory:
            suite(Path(directory), *sys.argv[1:3])
