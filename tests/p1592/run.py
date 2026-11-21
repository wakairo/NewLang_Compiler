"""Record fixed-source C17/native and custody outcomes for nested quantity calls."""

from hashlib import sha256
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from time import time_ns


CLI, PROBE, OUTPUT = sys.argv[1:]
FIRST = Path(__file__).resolve().parent
RAW = Path(OUTPUT) / str(time_ns())
RAW.mkdir(parents=True)
ORACLE = json.loads((FIRST / "oracle.json").read_bytes())
BOUNDARIES = json.loads(Path(__file__).with_name("observed-boundaries.json").read_bytes())
CHAIN_EDGES = {"named-actuals": 0, "direct-actuals": 3, "branch-and-return": 6}
RUNS = []
ENV = {**os.environ, "LC_ALL": "C"}


def run(command, expected=0):
    args = list(map(str, command))
    record = RAW / f"{len(RUNS) + 1:03}"
    record.with_suffix(".command.json").write_text(json.dumps(args) + "\n")
    result = subprocess.run(args, env=ENV, capture_output=True, timeout=90)
    RUNS.append({"command": args, "status": result.returncode})
    record.with_suffix(".stdout").write_bytes(result.stdout)
    record.with_suffix(".stderr").write_bytes(result.stderr)
    record.with_suffix(".status").write_text(f"{result.returncode}\n")
    assert result.returncode == expected, (
        args, result.returncode, expected, result.stderr.decode(errors="replace")
    )
    return result


def observed_c(original, graph):
    functions, bindings = {}, {}
    for kind, *values in map(str.split, graph.decode().splitlines()):
        if kind == "FUNCTION":
            assert re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", values[1])
            assert functions.setdefault(values[0], values[1]) == values[1]
        if kind == "BIND":
            assert re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", values[1])
            assert values[0] not in bindings
            bindings[values[0]] = values[1]

    lines = original.decode().splitlines(keepends=True)
    additions = {0: ["#include <stdio.h>\n"]}
    function_names = set()
    function = None
    for index, line in enumerate(lines, 1):
        definition = re.fullmatch(
            r"static (?:void|size_t) nl_qty_fn_(\d+)\((?:void|size_t nl_local_(\d+))\)\n",
            line,
        )
        if definition:
            symbol, parameter = definition.groups()
            assert symbol in functions and symbol not in function_names
            function_names.add(symbol)
            function = functions[symbol]
            assert lines[index] == "{\n"
            if parameter:
                additions[index + 1] = [
                    f'    printf("CALL {function} %zu\\n", nl_local_{parameter});\n'
                ]
            continue
        condition = re.fullmatch(r"    _Bool nl_condition_(\d+) = .*;\n", line)
        if condition:
            additions[index] = [
                f'    printf("COND %u\\n", (unsigned)nl_condition_{condition[1]});\n'
            ]
        binding = re.fullmatch(r"    size_t nl_local_(\d+) = .*;\n", line)
        if function == "main" and binding and binding[1] in bindings:
            additions[index] = [
                f'    printf("BIND {bindings[binding[1]]} %zu\\n", nl_local_{binding[1]});\n'
            ]
    assert function_names == set(functions)

    copy, added = [], set()
    for index in range(len(lines) + 1):
        for line in additions.get(index, []):
            added.add(len(copy))
            copy.append(line)
        if index < len(lines):
            copy.append(lines[index])
    assert "".join(line for i, line in enumerate(copy) if i not in added).encode() == original
    return "".join(copy).encode()


def verify_trace(stdout, expected):
    calls, values, conditions = [], {}, []
    for kind, *parts in map(str.split, stdout.decode().splitlines()):
        if kind == "CALL":
            calls.append([parts[0], int(parts[1])])
        elif kind == "BIND":
            assert parts[0] not in values
            values[parts[0]] = int(parts[1])
        else:
            assert kind == "COND" and parts[0] in ("0", "1")
            conditions.append(parts[0] == "1")
    assert calls == expected["runtime_calls"], calls
    assert conditions == expected.get("conditions", []), conditions
    assert {name: values[name] for name in expected["bindings"]} == expected["bindings"], values


def native(name, pristine, graph, expected):
    facts = graph.decode().splitlines()
    actuals = [line for line in facts if line.startswith("ACTUAL ")]
    assert sum(" inner=1 " in line for line in actuals) == CHAIN_EDGES[name], actuals
    assert sum(line.startswith("IF-JOIN ") for line in facts) == len(expected.get("conditions", [])), facts
    if name == "branch-and-return":
        assert sum(line.endswith(" known=0/0") for line in actuals) == 4, actuals
    observed = RAW / f"{name}.observed.c"
    observed.write_bytes(observed_c(pristine.read_bytes(), graph))
    gcc = shutil.which("gcc")
    clang = shutil.which("clang-23") or shutil.which("clang")
    assert gcc and clang
    for cc in (gcc, clang):
        for source in (pristine, observed):
            binary = RAW / f"{source.stem}.{Path(cc).name}.bin"
            run([cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2",
                 source, "-o", binary])
            result = run([binary])
            assert not result.stderr
            if source == pristine:
                assert not result.stdout
            else:
                verify_trace(result.stdout, expected)


def file_state(path):
    stat = path.stat()
    return path.read_bytes(), stat.st_ino, stat.st_mtime_ns


def observe():
    for name, expected in ORACLE.items():
        source = FIRST / "source" / f"{name}.nl"
        out = RAW / f"{name}.c"
        wanted = BOUNDARIES.get(name, expected.get("final", {"check": 0, "emit": 0}))
        diagnostic = wanted.get("diagnostic", expected.get("diagnostic"))
        checked = run([CLI, "--check", source], wanted["check"])
        if wanted["emit"]:
            assert diagnostic and diagnostic.encode() in checked.stderr
            out.write_bytes(b"keep the named output\0\x81\n")
            before = file_state(out)
            emitted = run([CLI, "--emit-c17-to", out, source], wanted["emit"])
            assert diagnostic.encode() in emitted.stderr and file_state(out) == before
            print(f"{name} CHECK={wanted['check']} EMIT={wanted['emit']} no-clobber PASS")
        else:
            assert not checked.stdout and not checked.stderr
            graph = run([PROBE, "strict", source])
            emitted = run([CLI, "--emit-c17-to", out, source])
            assert not emitted.stdout and not emitted.stderr
            native(name, out, graph.stdout, expected)
            print(f"{name} CHECK/owned/C17/GCC+Clang native PASS")


def main():
    for row in (FIRST / "SHA256SUMS").read_text().splitlines():
        expected, path = row.split()
        assert sha256((FIRST / path).read_bytes()).hexdigest() == expected, path
    try:
        observe()
    finally:
        (RAW / "commands.json").write_text(json.dumps(RUNS, indent=2) + "\n")
        print(f"evidence={RAW} commands={len(RUNS)}")


if __name__ == "__main__":
    main()
