"""Observe genuine named Copy statements and their independent C17 execution."""

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


def checked_records(proof, oracle):
    functions, bindings, emitted_layout = {}, {}, {}
    invocations = []
    for kind, *fields in map(str.split, proof.decode().splitlines()):
        if kind in ("FUNCTION", "BIND"):
            index, name = fields
            assert index.isdecimal() and name.isascii() and name.isidentifier()
            table = functions if kind == "FUNCTION" else bindings
            assert table.setdefault(index, name) == name
        elif kind == "READS":
            index, = fields
            invocations.append((index, [], []))
        elif kind == "READ":
            symbol, name, known, number, original, copied = fields
            assert symbol.isdecimal() and name.isascii() and name.isidentifier()
            assert known in ("0", "1") and int(original) < int(copied)
            assert known == "1" or number == "0"
            _, values, layout = invocations[-1]
            values.append([name, int(number) if known == "1" else None])
            layout.append((symbol, name))
    assert [[functions[index], values] for index, values, _ in invocations] == oracle
    for index, _, layout in invocations:
        emitted_layout.setdefault(index, layout)
    return functions, bindings, emitted_layout


def observe(pristine, functions, bindings, layout, trace):
    lines = pristine.decode().splitlines(keepends=True)
    insertions = {0: "#include <stdio.h>\n"}
    observed_bindings = {line.split()[1] for line in trace if line.startswith("BIND ")}
    reads = iter(())
    current = None
    for i, line in enumerate(lines):
        definition = re.fullmatch(
            r"static (?:void|size_t) nl_qty_fn_(\d+)\((?:void|size_t nl_local_(\d+))\)\n",
            line,
        )
        if definition:
            assert next(reads, None) is None
            current = functions[definition[1]]
            reads = iter(layout[definition[1]])
            if definition[2]:
                assert lines[i + 1] == "{\n"
                insertions[i + 2] = (
                    f'    printf("CALL {current} %zu\\n", nl_local_{definition[2]});\n'
                )
        copied = re.fullmatch(r"( +)\(void\)\(nl_local_(\d+)\);\n", line)
        if copied:
            symbol, name = next(reads)
            assert copied[2] == symbol
            insertions[i + 1] = f'{copied[1]}printf("READ {name} %zu\\n", nl_local_{symbol});\n'
        condition = re.fullmatch(r"( +)_Bool (nl_condition_\d+) = .*;\n", line)
        if condition:
            insertions[i + 1] = (
                f'{condition[1]}printf("COND %u\\n", (unsigned){condition[2]});\n'
            )
        binding = re.fullmatch(r"    size_t nl_local_(\d+) = .*;\n", line)
        if current == "main" and binding and bindings.get(binding[1]) in observed_bindings:
            name = bindings[binding[1]]
            insertions[i + 1] = f'    printf("BIND {name} %zu\\n", nl_local_{binding[1]});\n'
    assert next(reads, None) is None

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
        proof = harness.run([probe, "copy-trace", source], 0, case / "proof").stdout
        functions, bindings, layout = checked_records(proof, expected["owned"])
        emitted = harness.run([cli, "--emit-c17-to", pristine, source], 0, case / "emit")
        assert not emitted.stdout and not emitted.stderr
        observed = case / "observed.c"
        observed.write_bytes(
            observe(pristine.read_bytes(), functions, bindings, layout, expected["trace"])
        )
        native(case, (pristine, observed), compilers, expected["trace"])
        print(f"{name}: owned Copy layout / C17 / GCC and Clang native PASS")


if __name__ == "__main__":
    main()
