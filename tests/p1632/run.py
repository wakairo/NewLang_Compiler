"""Exercise discarded Bool Copy uses and the original bindings' later uses."""

import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import sys
import tempfile

FIXTURES = Path(__file__).resolve().parent
P1623 = importlib.util.spec_from_file_location(
    "p1623_runner", FIXTURES.parent / "p1623" / "run.py"
)
typed = importlib.util.module_from_spec(P1623)
P1623.loader.exec_module(typed)
harness = typed.harness


CALLER = {
    "owned": [
        ["packed", [["usize", 101], ["bool", 1]], None],
        ["packed", [["usize", 101], ["bool", None]], None],
    ],
    "bools": [
        ["main", "permit", 1],
        ["main", "rule", None],
        ["main", "hold", None],
    ],
    "joins": ["packed", "packed"],
    "trace": [
        "DISCARD bool 1",
        "ARG 0 usize 101",
        "ARG 1 bool 1",
        "CALL packed 101 1",
        "COND 1",
        "LOCAL packed_count 12",
        "DISCARD bool 0",
        "ARG 0 usize 101",
        "ARG 1 bool 0",
        "CALL packed 101 0",
        "COND 0",
        "LOCAL leftovers 10",
        "LOCAL audit 1",
        "DISCARD usize 1",
    ],
}

HELPER = {
    "owned": [
        ["band", [["bool", 1], ["usize", 17]], None],
        ["band", [["bool", None], ["usize", 32]], None],
    ],
    "bools": [["band", "policy", 1], ["band", "policy", None]],
    "joins": ["band", "band"],
    "trace": [
        "ARG 0 bool 1",
        "CALL fold_groups 153",
        "ARG 1 usize 17",
        "CALL band 1 17",
        "DISCARD bool 1",
        "COND 1",
        "LOCAL first 2",
        "ARG 0 bool 0",
        "CALL fold_groups 288",
        "ARG 1 usize 32",
        "CALL band 0 32",
        "DISCARD bool 0",
        "COND 0",
        "LOCAL later 6",
        "DISCARD usize 2",
        "DISCARD usize 6",
    ],
}

REFUSALS = {
    "comparison-statement": "V1-BACKEND-UNSUPPORTED",
    "discard-missing-result": "P8-BODY-RESULT",
    "zero-after-discard": "USIZE-REMAINDER-ZERO",
}


def proof_locals(stdout, expected):
    locals_by_function = {}
    discards = []
    edges = []
    for kind, *fields in map(str.split, stdout.decode().splitlines()):
        if kind == "LOCAL":
            function, symbol, name = fields
            assert symbol.isdecimal() and name.isascii() and name.isidentifier()
            assert locals_by_function.setdefault((function, symbol), name) == name
        elif kind == "DISCARD":
            function, name, known, number, result_count, *_ = fields
            assert known in ("0", "1")
            discards.append([function, name, known == "1", int(number), int(result_count)])
        elif kind == "EDGE":
            edges.append(fields)
    assert b"PARSE 0 -\nREGISTER 0 -\nCHECK 0 -\n" in stdout
    assert b"ROOT custody=1 machine=1\n" in stdout
    assert discards == expected.get("boolDiscardUses", [])
    assert edges and edges[0][0] == "main" and edges[0][-3:] == [
        "seal=1", "world=1", "plan=1"
    ]
    assert all(edge[-3:] == ["seal=1", "world=0", "plan=1"] for edge in edges[1:])
    return locals_by_function


def observe(pristine, functions, signatures, locals_by_function, observed_names):
    lines = pristine.decode().splitlines(keepends=True)
    additions = {0: "#include <stdio.h>\n"}
    current = None
    types = {}
    for index, line in enumerate(lines):
        definition = re.fullmatch(
            r"static (?:void|size_t) nl_qty_fn_(\d+)\((.*)\)\n", line
        )
        if definition:
            current = functions[definition[1]]
            types.clear()
            if definition[2] == "void":
                continue
            params = [arg.split() for arg in definition[2].split(", ")]
            assert [typed.C_TYPES[t] for t, _ in params] == signatures.get(current, ["usize"])
            types.update({symbol: c_type for c_type, symbol in params})
            formats, values = zip(*(typed.typed_print(t, v) for t, v in params), strict=True)
            assert lines[index + 1] == "{\n"
            additions[index + 2] = (
                f'    printf("CALL {current} {" ".join(formats)}\\n", '
                f'{", ".join(values)});\n'
            )
            continue
        local = re.fullmatch(r"( +)(_Bool|size_t) (nl_local_(\d+)) = .*;\n", line)
        if local:
            indent, c_type, symbol, key = local.groups()
            types[symbol] = c_type
            name = locals_by_function.get((current, key))
            if current == "main" and name in observed_names:
                fmt, expression = typed.typed_print(c_type, symbol)
                additions[index + 1] = f'{indent}printf("LOCAL {name} {fmt}\\n", {expression});\n'
            continue
        discard = re.fullmatch(r"( +)\(void\)\((nl_local_\d+)\);\n", line)
        if discard:
            indent, symbol = discard.groups()
            c_type = types[symbol]
            fmt, expression = typed.typed_print(c_type, symbol)
            additions[index + 1] = (
                f'{indent}printf("DISCARD {typed.C_TYPES[c_type]} {fmt}\\n", {expression});\n'
            )
            continue
        actual = re.fullmatch(r"( +)(_Bool|size_t) (nl_actual_\d+_([01])) = .*;\n", line)
        if actual:
            indent, c_type, symbol, rank = actual.groups()
            fmt, expression = typed.typed_print(c_type, symbol)
            additions[index + 1] = (
                f'{indent}printf("ARG {rank} {typed.C_TYPES[c_type]} {fmt}\\n", {expression});\n'
            )
            continue
        condition = re.fullmatch(r"( +)_Bool (nl_condition_\d+) = .*;\n", line)
        if condition:
            indent, symbol = condition.groups()
            additions[index + 1] = f'{indent}printf("COND %u\\n", (unsigned){symbol});\n'

    output, added = [], set()
    for index, line in enumerate(lines):
        if index in additions:
            added.add(len(output))
            output.append(additions[index])
        output.append(line)
    assert len(added) == len(additions)
    assert "".join(line for i, line in enumerate(output) if i not in added).encode() == pristine
    return "".join(output).encode()


def main():
    cli, trace_probe, discard_probe, evidence = sys.argv[1:]
    for row in (FIXTURES / "SHA256SUMS").read_text().splitlines():
        expected, path = row.split()
        assert hashlib.sha256((FIXTURES / path).read_bytes()).hexdigest() == expected, path
    oracle = json.loads((FIXTURES / "oracle.json").read_bytes())
    cc = (shutil.which("gcc"), shutil.which("clang-23") or shutil.which("clang"))
    assert all(cc), cc
    Path(evidence).mkdir(parents=True, exist_ok=True)
    raw = Path(tempfile.mkdtemp(dir=evidence))
    print(f"evidence={raw}", flush=True)

    for name, expected in oracle.items():
        case = raw / name
        case.mkdir()
        source = FIXTURES / "source" / f"{name}.nl"
        pristine = case / "pristine.c"
        checked = harness.run([cli, "--check", source], expected["gate0Check"], case / "check")
        proof = harness.run([discard_probe, "inspect", source], 0, case / "proof")
        assert proof.stdout.startswith(b"PARSE 0 -\n")
        if expected["finalEmit"]:
            diagnostic = REFUSALS[name].encode()
            if expected["gate0Check"]:
                assert diagnostic in checked.stderr
                assert b"REGISTER 1 " + diagnostic in proof.stdout
                assert b"\nCHECK " not in proof.stdout
            else:
                assert b"ROOT custody=1 machine=0\n" in proof.stdout
            pristine.write_bytes(b"keep existing named output\0\x81\n")
            before = harness.file_state(pristine)
            emitted = harness.run([cli, "--emit-c17-to", pristine, source],
                                  expected["finalEmit"], case / "emit")
            assert diagnostic in emitted.stderr and before == harness.file_state(pristine)
            print(f"{name}: refused with the named output preserved")
            continue

        assert not checked.stdout and not checked.stderr
        locals_by_function = proof_locals(proof.stdout, expected)
        facts = CALLER if name.startswith("caller-") else HELPER
        graph = harness.run([trace_probe, "trace", source], 0, case / "typed")
        functions, signatures = typed.checked_facts(graph.stdout, facts)
        emitted = harness.run([cli, "--emit-c17-to", pristine, source], 0, case / "emit")
        assert not emitted.stdout and not emitted.stderr
        observed = case / "observed.c"
        observed.write_bytes(observe(pristine.read_bytes(), functions, signatures,
                                     locals_by_function, expected["native"]))
        trace = facts["trace"]
        if not expected.get("boolDiscardUses"):
            trace = [line for line in trace if not line.startswith("DISCARD bool ")]
        typed.native(case, (pristine, observed), cc, trace)
        print(f"{name}: typed Copy/reuse and strict GCC/Clang native PASS")


if __name__ == "__main__":
    main()
