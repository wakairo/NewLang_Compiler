"""Check retained arm arithmetic and inspect the actual C17 execution."""

import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import sys
import tempfile


def common_runner():
    spec = importlib.util.spec_from_file_location(
        "quantity_native", Path(__file__).resolve().parent.parent / "p1596" / "run.py"
    )
    common = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(common)
    return common


def observe_arms(original):
    lines, added = [], set()
    for line in original.decode().splitlines(keepends=True):
        lines.append(line)
        result = re.fullmatch(
            r"( {8,})(?:size_t )?(nl_(?:if|local)_\d+) = .* ([/%]) .*;\n", line
        )
        if result:
            indent, variable, operator = result.groups()
            added.add(len(lines))
            lines.append(f'{indent}printf("ARM %c %zu\\n", \'{operator}\', {variable});\n')
    assert "".join(line for i, line in enumerate(lines) if i not in added).encode() == original
    return "".join(lines).encode()


def verify_native(raw, pristine, graph, expected, compilers, common):
    events = []
    for line in graph.decode().splitlines():
        if line.startswith("EVENT "):
            _, operator, known, number = line.split()
            events.append([operator, known == "1", int(number)])
    assert events == expected["arm_facts"], events
    code = pristine.read_bytes()
    observed = raw / "observed.c"
    observed.write_bytes(observe_arms(common.observed_c(code, graph, expected)))
    for cc in compilers:
        for c_file in (pristine, observed):
            stem = f"{c_file.stem}-{Path(cc).name}"
            binary = raw / f"{stem}.bin"
            common.run(
                [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                 "-O2", c_file, "-o", binary], 0, raw / f"{stem}-compile"
            )
            run = common.run([binary], 0, raw / f"{stem}-native")
            assert not run.stderr
            expected_trace = expected["trace"] if c_file == observed else []
            assert run.stdout.decode().splitlines() == expected_trace, run.stdout
    assert pristine.read_bytes() == code


def p1587_direct_arm(root):
    # The original oracle already describes the lawful result. Run the same
    # source through the required owned-arm path, using its frozen values.
    freeze = json.loads((root / "freeze.json").read_bytes())
    for path in ("oracle.json", "source/direct-arm.nl"):
        data = (root / path).read_bytes()
        assert freeze[path] == {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
    old = json.loads((root / "oracle.json").read_bytes())["direct-arm"]
    bindings = old["entry_bindings"]
    calls = old["runtime_calls"]
    return root / "source" / "direct-arm.nl", {
        "bindings": bindings,
        "calls": calls,
        "conditions": old["conditions"],
        "arm_facts": [["/", True, bindings["packed"]],
                      ["%", True, old["unselected_arm_result"]]],
        "trace": [
            f"COND {int(old['conditions'][0])}",
            f"ARM / {bindings['packed']}",
            *(f"BIND {name} {bindings[name]}" for name in ("packed", "held", "groups", "residue")),
            *(f"CALL {name} {value}" for name, value in calls),
            f"BIND measured {bindings['measured']}",
        ],
    }


def main():
    cli, probe, evidence = sys.argv[1:]
    fixtures = Path(__file__).resolve().parent
    for row in (fixtures / "SHA256SUMS").read_text().splitlines():
        expected, name = row.split()
        assert hashlib.sha256((fixtures / name).read_bytes()).hexdigest() == expected, name
    compilers = shutil.which("gcc"), shutil.which("clang-23") or shutil.which("clang")
    assert all(compilers), compilers
    Path(evidence).mkdir(parents=True, exist_ok=True)
    raw = Path(tempfile.mkdtemp(dir=evidence))
    print(f"evidence={raw}", flush=True)
    common = common_runner()
    own = json.loads((fixtures / "oracle.json").read_bytes())
    cases = [(fixtures / "source" / f"{name}.nl", expected) for name, expected in own.items()]
    cases.append(p1587_direct_arm(fixtures.parent / "p1587"))
    for source, expected in cases:
        name = source.stem
        case = raw / name
        case.mkdir()
        check = common.run([cli, "--check", source], expected.get("check", 0), case / "check")
        pristine = case / "pristine.c"
        emit = [cli, "--emit-c17-to", pristine, source]
        if "diagnostic" in expected:
            diagnostic = expected["diagnostic"].encode()
            assert diagnostic in check.stderr
            pristine.write_bytes(b"retain named C output\x00\x89\n")
            before = common.file_state(pristine)
            rejected = common.run(emit, expected["emit"], case / "emit")
            assert diagnostic in rejected.stderr and common.file_state(pristine) == before
        else:
            assert not check.stdout and not check.stderr
            graph = common.run([probe, "trace", source], 0, case / "proof")
            emitted = common.run(emit, 0, case / "emit")
            assert not graph.stderr and not emitted.stdout and not emitted.stderr
            verify_native(case, pristine, graph.stdout, expected, compilers, common)
        print(f"{name}: PASS", flush=True)


if __name__ == "__main__":
    main()
