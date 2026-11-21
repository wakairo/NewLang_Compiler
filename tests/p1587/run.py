"""Check actual two-arm quantity results against fixed source and native traces."""
from collections import Counter
from hashlib import sha256
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parent
MODE, CLI, PROBE, OUTPUT = sys.argv[1:]
RAW = Path(OUTPUT) / str(time.time_ns())
RAW.mkdir(parents=True)
ORACLE = json.loads((ROOT / "oracle.json").read_bytes())
ORACLE.update(json.loads((ROOT / "supplemental.json").read_bytes()))
FREEZE = json.loads((ROOT / "freeze.json").read_bytes())
ENV = {
    **os.environ,
    "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
    "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1",
}
CHECK_FORKS = {
    "split-route": [[74, 25], [97, 25]],
    "route-return": [[7, 12], [31, 36]],
    "flat-control": [],
    "tail-choice": [[42, 91], [68, 91]],
}
UNKNOWN_ARITHMETIC = {"split-route": 5, "route-return": 5,
                      "flat-control": 0, "tail-choice": 2}
RUNS = []


def source(name):
    return ROOT / "source" / f"{name}.nl"


def run(command, expected=0, native=False):
    args = list(map(str, command))
    tag = RAW / f"{len(RUNS) + 1:03}"
    tag.with_suffix(".command.json").write_text(json.dumps(args) + "\n")
    result = subprocess.run(args, capture_output=True, timeout=75, env=ENV)
    RUNS.append({"command": args, "status": result.returncode, "native": native})
    tag.with_suffix(".stdout").write_bytes(result.stdout)
    tag.with_suffix(".stderr").write_bytes(result.stderr)
    tag.with_suffix(".status").write_text(f"{result.returncode}\n")
    assert result.returncode == expected, (tag, args, result.returncode, result.stderr)
    return result


def observe_c(pristine, checked, name):
    functions, bindings, forks, conditions, unknown = {}, {}, [], [], 0
    for row in checked.stdout.decode().splitlines():
        field, *values = row.split()
        if field == "FUNCTION":
            index, function_name = values
            assert functions.setdefault(index, function_name) == function_name
        elif field == "BIND":
            index, binding = values
            assert index not in bindings
            bindings[index] = binding
        elif field == "IF":
            assert values[1] == "1", row
            conditions.append(values[2] == "1")
        elif field == "FORK":
            forks.append([int(v) for v in values])
        elif field == "ARITH":
            unknown += values[1] == "0"
    assert forks == CHECK_FORKS[name]
    assert conditions == ORACLE[name].get("conditions", [])
    assert unknown == UNKNOWN_ARITHMETIC[name]

    # Insert reads of existing values. No source text or oracle value is
    # inserted into a guest expression or used to decide a branch.
    original = pristine.decode().splitlines(keepends=True)
    additions = {0: ["#include <stdio.h>\n"]}
    seen_functions = set()
    condition_sites, assignment_sites, tests = Counter(), Counter(), Counter()
    for index, line in enumerate(original, 1):
        function = re.fullmatch(
            r"static (?:size_t|void) nl_qty_fn_(\d+)\((?:size_t nl_local_(\d+)|void)\)\n", line
        )
        if function:
            code, parameter = function.groups()
            function_name = functions[code]
            assert code not in seen_functions and original[index] == "{\n"
            seen_functions.add(code)
            if parameter:
                additions[index + 1] = [
                    f'    printf("CALL {function_name} %zu\\n", nl_local_{parameter});\n'
                ]
            else:
                assert function_name == "main"
            continue
        condition = re.fullmatch(r"    _Bool nl_condition_(\d+) = .*;\n", line)
        selection = re.fullmatch(r"    if \(nl_condition_(\d+)\) \{\n", line)
        assignment = re.fullmatch(r"        nl_if_(\d+) = .*;\n", line)
        binding = re.fullmatch(r"    size_t nl_local_(\d+) = .*;\n", line)
        if condition:
            key = (function_name, condition[1])
            condition_sites[key] += 1
            additions[index] = [
                f'    printf("COND %u\\n", (unsigned)nl_condition_{condition[1]});\n'
            ]
        elif selection:
            tests[(function_name, selection[1])] += 1
        elif assignment:
            key = (function_name, assignment[1])
            arm = assignment_sites[key]
            assignment_sites[key] += 1
            additions[index] = [
                f'        printf("ARM {arm} %zu\\n", nl_if_{assignment[1]});\n'
            ]
        elif binding and function_name == "main" and binding[1] in bindings:
            label = bindings[binding[1]]
            additions[index] = [f'    printf("BIND {label} %zu\\n", nl_local_{binding[1]});\n']
    assert seen_functions == set(functions)
    assert condition_sites == tests and all(count == 1 for count in tests.values())
    assert condition_sites.keys() == assignment_sites.keys()
    assert all(count == 2 for count in assignment_sites.values())

    observed = []
    inserted = set()
    for boundary in range(len(original) + 1):
        for line in additions.get(boundary, ()):
            inserted.add(len(observed))
            observed.append(line)
        if boundary < len(original):
            observed.append(original[boundary])
    assert "".join(line for i, line in enumerate(observed) if i not in inserted).encode() == pristine
    return "".join(observed).encode()


def check_trace(result, name):
    expected = ORACLE[name]
    state, calls, conditions, arms = {}, [], [], []
    for line in result.stdout.decode().splitlines():
        kind, *values = line.split()
        if kind == "BIND":
            assert values[0] not in state
            state[values[0]] = int(values[1])
        elif kind == "CALL":
            calls.append([values[0], int(values[1])])
        elif kind == "COND":
            conditions.append(bool(int(values[0])))
        else:
            assert kind == "ARM"
            arms.append([int(value) for value in values])
    assert {key: state[key] for key in expected["entry_bindings"]} == expected["entry_bindings"]
    assert conditions == expected.get("conditions", [])
    assert calls == expected["runtime_calls"]
    choices = [0 if condition else 1 for condition in conditions]
    assert arms == [[arm, forks[arm]] for arm, forks in zip(choices, CHECK_FORKS[name], strict=True)]
    assert not result.stderr


def native():
    gcc = shutil.which("gcc")
    clang = shutil.which("clang-23") or shutil.which("clang")
    assert gcc and clang
    strict = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    for name in CHECK_FORKS:
        checked = run([CLI, "--check", source(name)])
        assert not checked.stdout and not checked.stderr
        graph = run([PROBE, "observe", source(name)])
        pristine = RAW / f"{name}.c"
        emitted = run([CLI, "--emit-c17-to", pristine, source(name)])
        assert not emitted.stdout and not emitted.stderr
        observed = RAW / f"{name}.observed.c"
        observed.write_bytes(observe_c(pristine.read_bytes(), graph, name))
        for cc in (gcc, clang):
            for text in (pristine, observed):
                exe = RAW / f"{text.stem}.{Path(cc).name}.bin"
                run([cc, *strict, "-O2", text, "-o", exe], native=True)
                result = run([exe], native=True)
                if text == pristine:
                    assert not result.stdout and not result.stderr
                else:
                    check_trace(result, name)
        if name in {"split-route", "route-return"}:
            for sanitizer in ("address", "undefined"):
                exe = RAW / f"{name}.{sanitizer}.bin"
                run([gcc, *strict, "-O1", f"-fsanitize={sanitizer}",
                     "-fno-omit-frame-pointer", "-fno-sanitize-recover=all",
                     "-no-pie", observed, "-o", exe], native=True)
                check_trace(run([exe], native=True), name)
    print("Pristine and add-only C17 on GCC/Clang; both actual choices, call order and results PASS")


def named_output(path):
    stat = path.stat()
    return path.read_bytes(), stat.st_ino, stat.st_mtime_ns


def negative():
    for name, checking, emission, diagnostic in (
        ("wrong-arm-type", 3, 3, b"error(semantic)[P13-IF-RESULT]"),
        ("zero-arm", 3, 3, b"error(semantic)[USIZE-REMAINDER-ZERO]"),
        ("unknown-plus", 3, 3, b"error(precision)[USIZE-ADDITION-PRECISION]"),
        ("direct-arm", 0, 4, b"V1-BACKEND-UNSUPPORTED"),
    ):
        checked = run([CLI, "--check", source(name)], checking)
        assert diagnostic in checked.stderr if checking else not checked.stderr
        output = RAW / f"{name}.c"
        output.write_bytes(b"named output sentinel\0\x81\n")
        before = named_output(output)
        result = run([CLI, "--emit-c17-to", output, source(name)], emission)
        assert diagnostic in result.stderr and named_output(output) == before
        print(name, checking, emission, diagnostic.decode(), "no-clobber PASS")


def main():
    for file, expected in FREEZE.items():
        data = (ROOT / file).read_bytes()
        assert expected == {"bytes": len(data), "sha256": sha256(data).hexdigest()}, file
    if MODE == "native":
        native()
    elif MODE == "negative":
        negative()
    elif MODE == "identity":
        for mode, case in (("identity", "split-route"), ("return-identity", "route-return")):
            result = run([PROBE, mode, source(case)])
            assert b"rejected; restore PASS" in result.stdout
            print(result.stdout.decode().strip())
    else:
        result = run([PROBE, MODE, source("split-route")])
        assert b"hits=1" in result.stdout and b"retry PASS" in result.stdout
        print(result.stdout.decode().strip())


try:
    main()
finally:
    (RAW / "commands.json").write_text(json.dumps(RUNS, indent=2) + "\n")
