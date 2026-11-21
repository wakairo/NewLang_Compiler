"""Exercise genuine arithmetic actuals and observe their C17 execution."""

import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


def run(command, expected, raw):
    args = list(map(str, command))
    with raw.with_suffix(".command.json").open("x") as record:
        json.dump(args, record)
    result = subprocess.run(
        args, capture_output=True, timeout=90, env={**os.environ, "LC_ALL": "C"}
    )
    raw.with_suffix(".stdout").write_bytes(result.stdout)
    raw.with_suffix(".stderr").write_bytes(result.stderr)
    raw.with_suffix(".status").write_text(f"{result.returncode}\n")
    assert result.returncode == expected, (args, expected, result.stderr)
    return result


def observed_c(original, graph, expected):
    functions, bindings = {}, {}
    operations, calls_as_actuals, joins = [], 0, 0
    for line in graph.decode().splitlines():
        fields = line.split()
        if not fields:
            continue
        if fields[0] in ("FUNCTION", "BIND"):
            _, index, name = fields
            assert re.fullmatch(r"\d+", index) and re.fullmatch(r"\w+", name, re.ASCII)
            table = functions if fields[0] == "FUNCTION" else bindings
            assert table.setdefault(index, name) == name
        elif fields[0] == "ACTUAL":
            facts = dict(field.split("=") for field in fields[2:])
            assert facts["result"] == facts["formal"]
            if facts["op"] in ("/", "%", "+", "-"):
                operations.append([facts["op"], facts["known"] == "1", int(facts["number"])])
            elif facts["op"] == "call":
                calls_as_actuals += 1
        elif fields[0] == "JOIN":
            assert fields[-1] == "Unknown"
            joins += 1
    assert operations == expected.get("arithmetic_actuals", []), operations
    assert calls_as_actuals == expected.get("calls_as_actuals", 0), calls_as_actuals
    assert joins == len(expected.get("conditions", []))

    lines = original.decode().splitlines(keepends=True)
    inserted = {0: "#include <stdio.h>\n"}
    current = None
    for index, line in enumerate(lines):
        definition = re.fullmatch(
            r"static (?:void|size_t) nl_qty_fn_(\d+)\((?:void|size_t nl_local_(\d+))\)\n",
            line,
        )
        if definition:
            current = functions[definition[1]]
            assert lines[index + 1] == "{\n"
            if definition[2]:
                inserted[index + 2] = (
                    f'    printf("CALL {current} %zu\\n", nl_local_{definition[2]});\n'
                )
        condition = re.fullmatch(r"    _Bool nl_condition_(\d+) = .*;\n", line)
        if condition:
            inserted[index + 1] = (
                f'    printf("COND %u\\n", (unsigned)nl_condition_{condition[1]});\n'
            )
        binding = re.fullmatch(r"    size_t nl_local_(\d+) = .*;\n", line)
        if current == "main" and binding and binding[1] in bindings:
            name = bindings[binding[1]]
            if name in expected["bindings"]:
                inserted[index + 1] = f'    printf("BIND {name} %zu\\n", nl_local_{binding[1]});\n'
    copy, added = [], set()
    for index, line in enumerate(lines):
        if index in inserted:
            added.add(len(copy))
            copy.append(inserted[index])
        copy.append(line)
    assert len(added) == len(inserted)
    assert "".join(line for i, line in enumerate(copy) if i not in added).encode() == original
    return "".join(copy).encode()


def verify_trace(stdout, expected):
    calls, values, conditions = [], {}, []
    for kind, *fields in map(str.split, stdout.decode().splitlines()):
        if kind == "CALL":
            calls.append([fields[0], int(fields[1])])
        elif kind == "BIND":
            assert fields[0] not in values
            values[fields[0]] = int(fields[1])
        else:
            assert kind == "COND" and fields[0] in ("0", "1")
            conditions.append(fields[0] == "1")
    assert calls == expected["calls"], calls
    assert values == expected["bindings"], values
    assert conditions == expected.get("conditions", []), conditions


def verify_native(case, pristine, observed, expected, compilers):
    for cc in compilers:
        for source in (pristine, observed):
            prefix = case / f"{source.stem}-{Path(cc).name}"
            binary = prefix.with_suffix(".bin")
            run([cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                 "-O2", source, "-o", binary], 0, case / f"{prefix.name}-compile")
            result = run([binary], 0, case / f"{prefix.name}-native")
            assert not result.stderr
            if source == pristine:
                assert not result.stdout
            else:
                verify_trace(result.stdout, expected)


def file_state(path):
    state = path.stat()
    return (path.read_bytes(), state.st_ino, state.st_size, state.st_mtime_ns)


def main():
    cli, probe, evidence = sys.argv[1:]
    fixtures = Path(__file__).resolve().parent
    for row in (fixtures / "SHA256SUMS").read_text().splitlines():
        expected, path = row.split()
        assert hashlib.sha256((fixtures / path).read_bytes()).hexdigest() == expected, path
    expected_results = json.loads((fixtures / "oracle.json").read_bytes())
    compilers = (shutil.which("gcc"), shutil.which("clang-23") or shutil.which("clang"))
    assert all(compilers), compilers
    Path(evidence).mkdir(parents=True, exist_ok=True)
    raw = Path(tempfile.mkdtemp(dir=evidence))
    print(f"evidence={raw}", flush=True)
    for name, expected in expected_results.items():
        case = raw / name
        case.mkdir()
        source = fixtures / "source" / f"{name}.nl"
        pristine = case / "pristine.c"
        checked = run([cli, "--check", source], expected.get("check", 0), case / "check")
        if "emit" in expected:
            diagnostic = expected["diagnostic"].encode()
            if expected["check"]:
                assert diagnostic in checked.stderr
            else:
                assert not checked.stderr
            pristine.write_bytes(b"keep existing named output\0\x81\n")
            before = file_state(pristine)
            emitted = run([cli, "--emit-c17-to", pristine, source], expected["emit"],
                          case / "emit")
            assert diagnostic in emitted.stderr and before == file_state(pristine)
            print(f"{name}: fail-closed; named output preserved")
        else:
            assert not checked.stderr and not checked.stdout
            graph = run([probe, "strict", source], 0, case / "proof")
            emitted = run([cli, "--emit-c17-to", pristine, source], 0, case / "emit")
            assert not emitted.stderr and not emitted.stdout
            observed = case / "observed.c"
            observed.write_bytes(observed_c(pristine.read_bytes(), graph.stdout, expected))
            verify_native(case, pristine, observed, expected, compilers)
            print(f"{name}: owned / C17 / GCC and Clang native PASS")


if __name__ == "__main__":
    main()
