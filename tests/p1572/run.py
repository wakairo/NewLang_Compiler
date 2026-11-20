"""Validate the frozen ordinary unit-entry scalar family."""

import fcntl
import hashlib
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
WORK = Path(OUTPUT) / str(time.time_ns())
WORK.mkdir(parents=True)
ORACLE = json.loads((ROOT / "oracle.json").read_bytes())
FREEZE = json.loads((ROOT / "freeze.json").read_bytes())
for filename, expected in FREEZE.items():
    path = ROOT / filename if filename == "oracle.json" else ROOT / "source" / filename
    data = path.read_bytes()
    assert {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()} == expected, path

ENV = {**os.environ, "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
       "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
sequence = 0


def charge(kinds, command):
    filename = os.environ.get("P1572_LEDGER")
    if not filename:
        return
    ledger = Path(filename)
    with ledger.with_suffix(".lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        record = json.loads(ledger.read_bytes())
        for kind, amount in kinds.items():
            total = record["counts"][kind] + amount
            assert total <= record["limits"][kind], (kind, total)
            record["counts"][kind] = total
            record["events"].append({
                "kind": kind, "charged": amount, "mode": MODE,
                "command": command, "raw": str(WORK),
            })
        ledger.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")


def run(command, success=True, native=False, oom=False):
    global sequence
    command = [str(part) for part in command]
    kinds = {"selected": 2 if oom else 1}
    if native:
        kinds["native"] = 1
    if oom:
        kinds["oom"] = 2  # Count injection and retry separately.
    charge(kinds, command)
    sequence += 1
    stem = WORK / f"{sequence:03}"
    stem.with_suffix(".command.json").write_text(json.dumps(command) + "\n")
    actual = subprocess.run(command, capture_output=True, timeout=90, env=ENV)
    stem.with_suffix(".stdout").write_bytes(actual.stdout)
    stem.with_suffix(".stderr").write_bytes(actual.stderr)
    stem.with_suffix(".status").write_text(f"{actual.returncode}\n")
    assert (actual.returncode == 0) == success, (command, actual, stem)
    return actual


def source(name):
    return ROOT / "source" / f"{name}.nl"


def observations(case, raw):
    lines = raw.decode().splitlines()
    assert {"SOURCE PASS", "CHECK PASS"} <= set(lines)
    assert f"OUTER result=1 world=1 plan=1 machine=1 calls={len(case['calls'])}" in lines
    helper_count = len({call[1] for call in case["calls"]})
    assert f"DEFINITION unknown={helper_count}" in lines
    expected_edges = int(case["exit"] == "return")
    assert f"EXIT {case['exit']} edges={expected_edges} payload=0" in lines

    bindings, direct, calls = {}, [], []
    for line in lines:
        fields = line.split()
        if fields[0] == "BIND":
            _, name, symbol, number, value = fields
            assert name not in bindings
            bindings[name] = tuple(map(int, (symbol, number, value)))
        elif fields[0] == "DIRECT":
            assert fields[-1] == "event=1"
            _, dest, op, numerator, lhs, rhs, result, *ids = fields[:-1]
            direct.append(([dest, op, numerator, int(lhs), int(rhs), int(result)],
                           tuple(map(int, ids))))
        elif fields[0] == "CALL":
            assert fields[-3:] == ["seal=1", "plan=1", "event=1"]
            _, dest, fn, op, actual, rhs, result, before, after, edge, *ids = fields[:-3]
            calls.append(([dest, fn, op, int(actual), int(rhs), int(result)],
                          tuple(map(int, ids))))
    assert {name: value[1] for name, value in bindings.items()} == case["bindings"]
    names_by_symbol = {values[0]: name for name, values in bindings.items()}
    assert len(names_by_symbol) == len(bindings)
    assert [spec for spec, _ in direct] == case["direct"]
    direct_results = set()
    for spec, ids in direct:
        dest, _, original, lhs, _, answer = spec
        symbol, original_id, borrowed, result = ids
        assert bindings[original] == (symbol, lhs, original_id)
        assert bindings[dest][1:] == (answer, result)
        assert 0 < original_id < borrowed < result
        assert result not in direct_results
        direct_results.add(result)
    actual_ids, formal_ids, result_ids = set(), set(), set()
    confirmed_calls = []
    helper_definitions = {}
    for spec, ids in calls:
        dest, fn, op, actual, divisor, answer = spec
        function, arg, formal, lhs, operand, event, result, source_symbol = ids
        source_name = names_by_symbol[source_symbol]
        assert bindings[source_name][1] == actual and bindings[source_name][2] < arg
        assert 0 < arg < operand < event <= result
        assert arg not in actual_ids and formal not in formal_ids and formal not in names_by_symbol
        assert result not in result_ids and result not in direct_results
        assert bindings[dest][1:] == (answer, result)
        actual_ids.add(arg)
        formal_ids.add(formal)
        result_ids.add(result)
        assert helper_definitions.setdefault(fn, (function, op, divisor)) == (function, op, divisor)
        confirmed_calls.append([dest, fn, source_name, actual, op, divisor, answer])
    assert confirmed_calls == case["calls"]
    return bindings, direct, calls, helper_definitions


def inspect_c(program, scan):
    bindings, direct, calls, definitions = scan
    assert "NewLang usize target mismatch" in program
    helpers = re.findall(r"^static size_t nl_qty_fn_(\d+)\(size_t nl_local_\d+\)\n\{\n(.*?)^\}",
                         program, re.M | re.S)
    assert len(helpers) == len(definitions)
    for (name, text), (fn, op, divisor) in zip(helpers, definitions.values(), strict=True):
        assert int(name) == fn
        assert re.findall(r" ([/%]) ", text) == [op]
        assert f" {op} ((size_t)UINTMAX_C({divisor}))" in text
    caller = re.search(r"^static void nl_qty_fn_(\d+)\(void\)\n\{\n(.*?)^\}", program, re.M | re.S)
    assert caller and caller[2].endswith("    return;\n")
    assert program.count(f"    nl_qty_fn_{caller[1]}();") == 1
    lines = caller[2].splitlines()
    actual_direct = [line for line in lines if " / " in line or " % " in line]
    expected_direct = [
        f"    size_t nl_local_{bindings[dest][0]} = nl_local_{bindings[origin][0]} "
        f"{op} ((size_t)UINTMAX_C({divisor}));"
        for (dest, op, origin, _, divisor, _), _ in direct
    ]
    assert actual_direct == expected_direct
    actual_calls = re.findall(r"^    size_t nl_local_(\d+) = nl_qty_fn_(\d+)\(nl_local_(\d+)\);$",
                              caller[2], re.M)
    expected_calls = [
        (str(bindings[spec[0]][0]), str(ids[0]), str(ids[-1]))
        for spec, ids in calls
    ]
    assert actual_calls == expected_calls


def positive():
    gcc = shutil.which("gcc")
    clang = shutil.which("clang-23") or shutil.which("clang")
    assert gcc and clang
    strict = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    for name in ("helper-tail", "turns-tail", "turns-return", "stock-mixed"):
        case = ORACLE[name]
        parsed = run([CLI, "--check", source(name)])
        assert not parsed.stdout and not parsed.stderr
        viewed = run([PROBE, "inspect", source(name)])
        assert not viewed.stderr
        scan = observations(case, viewed.stdout)
        original = WORK / f"{name}.c"
        emitted = run([CLI, "--emit-c17-to", original, source(name)])
        assert not emitted.stdout and not emitted.stderr
        raw = original.read_bytes()
        program = raw.decode("utf-8")
        inspect_c(program, scan)

        marker = "    return;\n}\n\nint main(void)\n"
        assert program.count(marker) == 1
        snippets = "".join(
            f'    printf("{key}=%zu\\n", nl_local_{scan[0][key][0]});\n'
            for key in case["bindings"]
        )
        observed = "#include <stdio.h>\n" + program.replace(marker, snippets + marker)
        assert observed.removeprefix("#include <stdio.h>\n").replace(snippets + marker, marker) == program
        modified = WORK / f"{name}.observed.c"
        modified.write_text(observed, encoding="utf-8")
        wanted = "".join(f"{key}={value}\n" for key, value in case["bindings"].items())
        builds = [("gcc", gcc, ["-O2"], modified, wanted),
                  ("clang", clang, ["-O2"], modified, wanted)]
        if name in {"turns-return", "stock-mixed"}:
            builds.append(("exact-c17", gcc, ["-O2"], original, ""))
        if name == "stock-mixed":
            builds.extend([
                ("asan", gcc, ["-O1", "-fsanitize=address", "-fno-omit-frame-pointer", "-no-pie"],
                 modified, wanted),
                ("ubsan", gcc, ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"],
                 modified, wanted),
            ])
        for label, compiler, flags, input_c, stdout in builds:
            exe = WORK / f"{name}.{label}"
            run([compiler, *strict, *flags, input_c, "-o", exe], native=True)
            result = run([exe], native=True)
            assert not result.stderr and result.stdout.decode() == stdout
            print(name, label, "PASS")
        assert original.read_bytes() == raw


def negative():
    for name in ("equal-helper", "divisor-zero", "operand-type", "dynamic-divisor", "invalid-return"):
        case = ORACLE[name]
        # The named nonzero divisor is legal source outside the literal profile.
        checked = run([CLI, "--check", source(name)], success=case["check"] == "PASS")
        assert not checked.stdout
        if "helper" in case:
            assert not checked.stderr
            excluded = run([PROBE, "excluded", source(name)])
            marker = f"EXCLUDED actual={case['direct']} helper={case['helper']} leaf seal=0 no-clobber=PASS"
            assert marker.encode() in excluded.stdout and not excluded.stderr
        else:
            assert case["check"].encode() in checked.stderr
        target = WORK / f"{name}.c"
        sentinel = b"P1572 existing named target\x00\n"
        target.write_bytes(sentinel)
        before = target.stat()
        result = run([CLI, "--emit-c17-to", target, source(name)], success=False)
        after = target.stat()
        assert case.get("emit", case["check"]).encode() in result.stderr and not result.stdout
        assert target.read_bytes() == sentinel
        assert (before.st_ino, before.st_mtime_ns) == (after.st_ino, after.st_mtime_ns)
        assert not list(WORK.glob(target.name + ".tmp*"))
        print(name, "named no-clobber PASS")


OOM_SOURCES = {
    "second-leaf-oom": "stock-mixed",
    "outer-nodes-oom": "turns-tail",
    "outer-world-oom": "turns-return",
    "staging-oom": "turns-return",
}
if MODE == "positive":
    positive()
elif MODE == "negative":
    negative()
elif MODE == "identity":
    observed = run([PROBE, MODE, source("turns-return")])
    assert not observed.stderr
    assert b"body-only, foreign receipt/RETURN and equal-valued substitutions rejected; restoration PASS" in observed.stdout
    print(observed.stdout.decode(), end="")
elif MODE in OOM_SOURCES:
    name = OOM_SOURCES[MODE]
    observed = run([PROBE, MODE, source(name)], oom=True)
    assert f"OOM {MODE} hit=1 no-clobber=PASS retry=PASS".encode() in observed.stdout
    assert (not observed.stderr if MODE == "staging-oom" else b"P3-OUT-OF-MEMORY" in observed.stderr)
    observations(ORACLE[name], observed.stdout)
    print(observed.stdout.decode(), end="")
else:
    raise AssertionError(MODE)
