"""Exercise source-owned mixed bool/usize calls and typed C17 evaluation."""

import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import sys
import tempfile


FIXTURES = Path(__file__).resolve().parent
HARNESS = importlib.util.spec_from_file_location(
    "quantity_test_harness", FIXTURES.parent / "p1596" / "run.py"
)
harness = importlib.util.module_from_spec(HARNESS)
HARNESS.loader.exec_module(harness)

C_TYPES = {"size_t": "usize", "_Bool": "bool"}


def scalar(known, number):
    assert known in ("0", "1")
    assert known == "1" or number == "0"
    return int(number) if known == "1" else None


def checked_facts(proof, expected):
    functions, signatures = {}, {}
    pairs, bools, joins, linked = [], [], [], []
    for kind, *data in map(str.split, proof.decode().splitlines()):
        if kind == "FUNCTION":
            index, name = data
            assert index.isdecimal() and name.isascii() and name.isidentifier()
            assert functions.setdefault(index, name) == name
        elif kind == "RESULT":
            name, known, number = data
            pairs.append([name, [], scalar(known, number)])
        elif kind == "POSITION":
            name, rank, type_name, known, number, *_ = data
            assert pairs[-1][0] == name and int(rank) == len(pairs[-1][1])
            pairs[-1][1].append([type_name, scalar(known, number)])
        elif kind == "BOOL":
            name, binding, known, number = data
            bools.append([name, binding, scalar(known, number)])
        elif kind == "JOIN":
            name, value = data
            assert value == "Unknown"
            joins.append(name)
        elif kind == "EARLY_LINK":
            name, first, sibling = data
            assert int(first) < int(sibling)
            linked.append(name)
    assert pairs == expected.get("owned", []), pairs
    assert bools == expected.get("bools", []), bools
    assert joins == expected.get("joins", []), joins
    assert linked == expected.get("linked", []), linked
    for name, formals, _ in pairs:
        types = [kind for kind, _ in formals]
        assert signatures.setdefault(name, types) == types
    return functions, signatures


def typed_print(c_type, name):
    if c_type == "_Bool":
        return "%u", f"(unsigned){name}"
    assert c_type == "size_t"
    return "%zu", name


def observe(pristine, functions, signatures):
    lines = pristine.decode().splitlines(keepends=True)
    insertions = {0: "#include <stdio.h>\n"}
    for i, line in enumerate(lines):
        definition = re.fullmatch(
            r"static (?:void|size_t) nl_qty_fn_(\d+)\((.*)\)\n", line
        )
        if definition and definition[2] != "void":
            name = functions[definition[1]]
            params = [arg.split() for arg in definition[2].split(", ")]
            assert [C_TYPES[c_type] for c_type, _ in params] == signatures.get(name, ["usize"])
            assert all(re.fullmatch(r"nl_local_\d+", symbol) for _, symbol in params)
            formats, expressions = zip(*(typed_print(t, v) for t, v in params), strict=True)
            layout, arguments = " ".join(formats), ", ".join(expressions)
            assert lines[i + 1] == "{\n"
            insertions[i + 2] = f'    printf("CALL {name} {layout}\\n", {arguments});\n'
        actual = re.fullmatch(r"( +)(size_t|_Bool) (nl_actual_\d+_([01])) = .*;\n", line)
        if actual:
            indent, c_type, symbol, rank = actual.groups()
            fmt, expression = typed_print(c_type, symbol)
            insertions[i + 1] = (
                f'{indent}printf("ARG {rank} {C_TYPES[c_type]} {fmt}\\n", {expression});\n'
            )
        condition = re.fullmatch(r"( +)_Bool (nl_condition_\d+) = .*;\n", line)
        if condition:
            insertions[i + 1] = (
                f'{condition[1]}printf("COND %u\\n", (unsigned){condition[2]});\n'
            )
        discard = re.fullmatch(r"( +)\(void\)\((nl_local_\d+)\);\n", line)
        if discard:
            insertions[i + 1] = f'{discard[1]}printf("DISCARD %zu\\n", {discard[2]});\n'

    observed, added = [], set()
    for i, line in enumerate(lines):
        if i in insertions:
            added.add(len(observed))
            observed.append(insertions[i])
        observed.append(line)
    assert len(added) == len(insertions)
    assert "".join(line for i, line in enumerate(observed) if i not in added).encode() == pristine
    return "".join(observed).encode()


def native(case, sources, compilers, trace):
    for cc in compilers:
        for source, expected in zip(sources, ([], trace), strict=True):
            tag = f"{source.stem}-{Path(cc).name}"
            binary = case / f"{tag}.bin"
            harness.run(
                [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                 "-O2", source, "-o", binary], 0, case / f"{tag}-compile"
            )
            result = harness.run([binary], 0, case / f"{tag}-native")
            assert not result.stderr
            actual = result.stdout.decode().splitlines()
            assert actual == expected, actual


def main():
    cli, probe, evidence = sys.argv[1:]
    for row in (FIXTURES / "SHA256SUMS").read_text().splitlines():
        expected, path = row.split()
        assert hashlib.sha256((FIXTURES / path).read_bytes()).hexdigest() == expected, path
    oracle = json.loads((FIXTURES / "oracle.json").read_bytes())
    compilers = (shutil.which("gcc"), shutil.which("clang-23") or shutil.which("clang"))
    assert all(compilers), compilers
    Path(evidence).mkdir(parents=True, exist_ok=True)
    raw = Path(tempfile.mkdtemp(dir=evidence))
    print(f"evidence={raw}", flush=True)

    for name, expected in oracle.items():
        case = raw / name
        case.mkdir()
        source = FIXTURES / "source" / f"{name}.nl"
        pristine = case / "pristine.c"
        checked = harness.run([cli, "--check", source], expected.get("check", 0), case / "check")
        if "emit" in expected:
            diagnostic = expected["diagnostic"].encode()
            if expected["check"]:
                assert diagnostic in checked.stderr
            else:
                assert not checked.stderr
            pristine.write_bytes(b"keep original output\0\x81\n")
            before = harness.file_state(pristine)
            emitted = harness.run(
                [cli, "--emit-c17-to", pristine, source], expected["emit"], case / "emit"
            )
            assert diagnostic in emitted.stderr and before == harness.file_state(pristine)
            print(f"{name}: refused; named output preserved")
            continue

        assert not checked.stdout and not checked.stderr
        proof = harness.run([probe, "trace", source], 0, case / "proof").stdout
        functions, signatures = checked_facts(proof, expected)
        emitted = harness.run([cli, "--emit-c17-to", pristine, source], 0, case / "emit")
        assert not emitted.stdout and not emitted.stderr
        observed = case / "observed.c"
        observed.write_bytes(observe(pristine.read_bytes(), functions, signatures))
        native(case, (pristine, observed), compilers, expected["trace"])
        print(f"{name}: typed owned arguments / C17 / GCC and Clang native PASS")


if __name__ == "__main__":
    main()
