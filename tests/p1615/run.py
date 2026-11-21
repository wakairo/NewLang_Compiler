"""Check ordinary positional Copy arguments through owned CHECK and native C17."""

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


def scalar(facts):
    assert facts["known"] in ("0", "1")
    value = int(facts["number"])
    if facts["known"] == "1":
        return value
    assert value == 0
    return None


def checked_facts(graph, expected):
    functions, bindings = {}, {}
    pairs, joins = [], []
    nested, discarded = [], []
    discards = {}
    for line in graph.decode().splitlines():
        kind, *fields = line.split()
        if kind in ("FUNCTION", "BIND"):
            index, name = fields
            assert index.isdecimal() and name.isascii() and name.isidentifier()
            table = functions if kind == "FUNCTION" else bindings
            assert table.setdefault(index, name) == name
        elif kind == "RESULT":
            name, *data = fields
            pairs.append([name, [], scalar(dict(field.split("=") for field in data))])
        elif kind == "POSITION":
            name, rank, *data = fields
            function, positions, _ = pairs[-1]
            assert function == name and int(rank) == len(positions)
            positions.append(scalar(dict(field.split("=") for field in data)))
        elif kind == "JOIN":
            name, _, fact = fields
            assert fact == "Unknown"
            joins.append(name)
        elif kind == "NESTED":
            name, *data = fields
            edge = dict(field.split("=") for field in data)
            assert int(edge["first"]) < int(edge["sibling"])
            nested.append(name)
        elif kind == "DISCARD":
            index, symbol, name, *data = fields
            assert symbol.isdecimal() and name.isascii() and name.isidentifier()
            assert discards.setdefault((index, symbol), name) == name
            discarded.append([functions[index], name, scalar(dict(field.split("=") for field in data))])
    assert pairs == expected.get("owned", []), pairs
    assert joins == expected.get("joins", []), joins
    assert nested == expected.get("nested", []), nested
    assert discarded == expected.get("discarded", []), discarded
    return functions, bindings, discards


def observe(original, functions, bindings, discards, observed_bindings):
    lines = original.decode().splitlines(keepends=True)
    inserted = {0: "#include <stdio.h>\n"}
    current = None
    for i, line in enumerate(lines):
        definition = re.fullmatch(
            r"static (?:void|size_t) nl_qty_fn_(\d+)\((.*)\)\n", line
        )
        if definition:
            current = definition[1]
            if definition[2] != "void":
                params = definition[2].split(", ")
                assert all(re.fullmatch(r"size_t nl_local_\d+", arg) for arg in params)
                names = ", ".join(decl.split()[1] for decl in params)
                formats = " %zu" * len(params)
                assert lines[i + 1] == "{\n"
                inserted[i + 2] = f'    printf("CALL {functions[current]}{formats}\\n", {names});\n'
        arg = re.fullmatch(r"( +)size_t (nl_actual_\d+_([01])) = .*;\n", line)
        if arg:
            inserted[i + 1] = f'{arg[1]}printf("ARG {arg[3]} %zu\\n", {arg[2]});\n'
        condition = re.fullmatch(r"( +)_Bool (nl_condition_\d+) = .*;\n", line)
        if condition:
            inserted[i + 1] = (
                f'{condition[1]}printf("COND %u\\n", (unsigned){condition[2]});\n'
            )
        binding = re.fullmatch(r"    size_t nl_local_(\d+) = .*;\n", line)
        if functions.get(current) == "main" and binding and bindings.get(binding[1]) in observed_bindings:
            name = bindings[binding[1]]
            inserted[i + 1] = f'    printf("BIND {name} %zu\\n", nl_local_{binding[1]});\n'
        discard = re.fullmatch(r"( +)\(void\)\(nl_local_(\d+)\);\n", line)
        if discard:
            name = discards[(current, discard[2])]
            inserted[i + 1] = f'{discard[1]}printf("DISCARD {name} %zu\\n", nl_local_{discard[2]});\n'

    instrumented, added = [], set()
    for index, line in enumerate(lines):
        if index in inserted:
            added.add(len(instrumented))
            instrumented.append(inserted[index])
        instrumented.append(line)
    assert len(added) == len(inserted)
    assert "".join(line for i, line in enumerate(instrumented) if i not in added).encode() == original
    return "".join(instrumented).encode()


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
    for manifest in ("SHA256SUMS", "reconcile.sha256"):
        for row in (FIXTURES / manifest).read_text().splitlines():
            expected, path = row.split()
            assert hashlib.sha256((FIXTURES / path).read_bytes()).hexdigest() == expected, path

    oracle = json.loads((FIXTURES / "oracle.json").read_bytes())
    reconcile = json.loads((FIXTURES / "reconcile-oracle.json").read_bytes())
    # Keep the original Gate0 expectation and source archived verbatim. The
    # private reconcile release promotes this one measured lawful case.
    assert oracle.keys() & reconcile.keys() == {"ordered-unused"}
    assert oracle["ordered-unused"]["emit"] == 4
    oracle.update(reconcile)
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
            pristine.write_bytes(b"keep existing named output\0\x81\n")
            before = harness.file_state(pristine)
            emitted = harness.run(
                [cli, "--emit-c17-to", pristine, source], expected["emit"], case / "emit"
            )
            assert diagnostic in emitted.stderr and before == harness.file_state(pristine)
            print(f"{name}: fail-closed; named output preserved")
            continue

        assert not checked.stderr and not checked.stdout
        graph = harness.run([probe, "trace", source], 0, case / "proof").stdout
        functions, bindings, discards = checked_facts(graph, expected)
        emitted = harness.run([cli, "--emit-c17-to", pristine, source], 0, case / "emit")
        assert not emitted.stderr and not emitted.stdout
        observed = case / "observed.c"
        observed_bindings = {row.split()[1] for row in expected["trace"] if row.startswith("BIND ")}
        observed.write_bytes(
            observe(pristine.read_bytes(), functions, bindings, discards, observed_bindings)
        )
        native(case, (pristine, observed), compilers, expected["trace"])
        print(f"{name}: owned / C17 / GCC and Clang native PASS")


if __name__ == "__main__":
    main()
