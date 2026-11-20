"""Validate the frozen ordinary helper graph and its native observations."""

from collections import Counter
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
    filename = os.environ.get("P1579_LEDGER")
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


UNKNOWN_EVENTS = {
    "parcels-flat": {},
    "parcels-extracted": {("packed", "/"): 1, ("packed", "%"): 1},
    "ledger-shared": {("parts", "/"): 6, ("parts", "%"): 6, ("basket", "%"): 2},
    "return-call": {("sieve", "/"): 2, ("sieve", "%"): 2},
}


def observations(name, raw):
    case = ORACLE[name]
    lines = raw.decode().splitlines()
    assert {"SOURCE PASS", "CHECK PASS"} <= set(lines)
    assert f"EXIT {case['exit']} edges={int(case['exit'] == 'return')} payload=0" in lines
    bindings, visits, events, unknown = {}, [], [], Counter()
    event_ids = set()
    for line in lines:
        fields = line.split()
        if not fields:
            continue
        if fields[0] == "BIND":
            _, label, symbol, number, value = fields
            assert label not in bindings
            bindings[label] = tuple(map(int, (symbol, number, value)))
        elif fields[0] == "VISIT":
            _, caller, fn, actual, result, edge, function, arg, formal, value, depth, *proof = fields
            assert proof == ["seal=1", "plan=1"]
            visits.append(([caller, fn, int(actual), int(result), edge],
                           tuple(map(int, (function, arg, formal, value, depth.removeprefix("depth="))))))
        elif fields[0] == "EVENT":
            _, function, op, lhs, rhs, answer, left, right, value, valid = fields
            assert valid == "valid=1"
            left, right, value = map(int, (left, right, value))
            assert 0 < left < right < value and value not in event_ids
            event_ids.add(value)
            events.append([function, op, int(lhs), int(rhs), int(answer)])
        elif fields[0] == "UNKNOWN":
            unknown[tuple(fields[1:])] += 1
        elif fields[0] == "COPY":
            assert 0 < int(fields[2]) < int(fields[3])
    assert unknown == UNKNOWN_EVENTS[name]
    assert f"DEFINITION unknown={sum(unknown.values())}" in lines
    assert {label: v[1] for label, v in bindings.items()} == case["bindings"]
    assert [spec for spec, _ in visits] == case["visits"]

    active, definitions = [], {}
    actuals, formals, results = set(), set(), set()
    main_calls = 0
    for spec, ids in visits:
        caller, callee, _, _, _ = spec
        function, actual, formal, result, depth = ids
        active = active[:depth - 1]
        assert depth == len(active) + 1
        assert caller == (active[-1][0] if active else "main")
        assert 0 < actual < result and actual not in actuals and formal not in formals
        if result in results:
            assert actual < result and any(prior == result for _, prior in active)
        active.append((callee, result))
        actuals.add(actual)
        formals.add(formal)
        results.add(result)
        if depth == 1:
            main_calls += 1
        assert definitions.setdefault(callee, (function, formal))[0] == function
    assert len({ids[0] for ids in definitions.values()}) == len(definitions)
    assert f"OUTER result=1 world=1 plan=1 machine=1 calls={main_calls}" in lines
    if "events" in case:
        assert events == case["events"]
    else:
        parts = []
        known = case["parts_known_independent"]
        for _, callee, actual, result, _ in case["visits"]:
            if callee == "parts":
                quotient = actual // 3
                parts.extend([["parts", "/", actual, 3, quotient],
                              ["parts", "%", quotient, 97, result],
                              *[["parts", *item] for item in known]])
        assert [event for event in events if event[0] == "parts"] == parts
        assert [event for event in events if event[0] == "basket"] == [
            ["basket", "%", 8, 5, 3], ["basket", "%", 2, 5, 2], ["basket", "%", 18, 5, 3],
        ]
        assert [event for event in events if event[0] == "main"] == [
            ["main", "%", 50, 6, 2], ["main", "+", 3, 2, 5],
        ]
    return bindings, definitions


def inspect_c(program, definitions):
    assert "NewLang usize target mismatch" in program
    helpers = re.findall(r"^static size_t nl_qty_fn_(\d+)\(size_t nl_local_(\d+)\)\n\{\n(.*?)^\}",
                         program, re.M | re.S)
    assert len(helpers) == len(definitions)
    emitted = set()
    for fn, formal, body in helpers:
        assert int(fn) not in emitted
        assert (int(fn), int(formal)) in definitions.values()
        callees = {int(n) for n in re.findall(r"\bnl_qty_fn_(\d+)\(", body)}
        assert callees <= emitted
        emitted.add(int(fn))
    caller = re.search(r"^static void nl_qty_fn_(\d+)\(void\)\n\{\n(.*?)^\}", program, re.M | re.S)
    assert caller and int(caller[1]) not in emitted and caller[2].endswith("    return;\n")
    assert program.count(f"    nl_qty_fn_{caller[1]}();") == 1
    assert {int(n) for n in re.findall(r"\bnl_qty_fn_(\d+)\(", caller[2])} <= emitted


def positive():
    gcc = shutil.which("gcc")
    clang = shutil.which("clang-23") or shutil.which("clang")
    assert gcc and clang
    strict = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    for name in UNKNOWN_EVENTS:
        case = ORACLE[name]
        checked = run([CLI, "--check", source(name)])
        assert not checked.stdout and not checked.stderr
        observed = run([PROBE, "inspect", source(name)])
        assert not observed.stderr
        bindings, definitions = observations(name, observed.stdout)
        pristine = WORK / f"{name}.c"
        emitted = run([CLI, "--emit-c17-to", pristine, source(name)])
        assert not emitted.stdout and not emitted.stderr
        raw = pristine.read_bytes()
        program = raw.decode("utf-8")
        inspect_c(program, definitions)

        marker = "    return;\n}\n\nint main(void)\n"
        assert program.count(marker) == 1
        prints = "".join(
            f'    printf("{key}=%zu\\n", nl_local_{bindings[key][0]});\n'
            for key in case["bindings"]
        )
        observed_c = "#include <stdio.h>\n" + program.replace(marker, prints + marker)
        reconstructed = observed_c.removeprefix("#include <stdio.h>\n").replace(prints + marker, marker)
        assert reconstructed.encode() == raw
        copy = WORK / f"{name}.observed.c"
        copy.write_text(observed_c, encoding="utf-8")
        wanted = "".join(f"{key}={value}\n" for key, value in case["bindings"].items())
        targets = [("gcc", gcc, ["-O2"], copy, wanted),
                   ("clang", clang, ["-O2"], copy, wanted)]
        if name in {"ledger-shared", "return-call"}:
            targets.append(("exact-c17", gcc, ["-O2"], pristine, ""))
        if name == "ledger-shared":
            targets.extend([
                ("asan", gcc, ["-O1", "-fsanitize=address", "-fno-omit-frame-pointer", "-no-pie"],
                 copy, wanted),
                ("ubsan", gcc, ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"],
                 copy, wanted),
            ])
        for label, compiler, flags, source_c, expected in targets:
            binary = WORK / f"{name}.{label}"
            run([compiler, *strict, *flags, source_c, "-o", binary], native=True)
            actual = run([binary], native=True)
            assert not actual.stderr and actual.stdout.decode() == expected
            print(name, label, "PASS")
        assert pristine.read_bytes() == raw


def negative():
    for name in ("equal-unrelated", "unknown-plus", "zero-inner", "named-divisor", "recursive-call"):
        case = ORACLE[name]
        diagnostic = case["check"]
        checked = run([CLI, "--check", source(name)], success=diagnostic == "PASS")
        assert not checked.stdout
        if diagnostic == "PASS":
            observed = run([PROBE, "inspect", source(name)])
            assert not checked.stderr and not observed.stderr
            assert b"OUTER result=0 world=0 plan=0 machine=0 calls=1" in observed.stdout
            assert f"VISIT main discard 28 {case['result']} tail ".encode() in observed.stdout
            assert b"seal=0 plan=0" in observed.stdout
        else:
            assert diagnostic.encode() in checked.stderr
        target = WORK / f"{name}.c"
        sentinel = b"P1579 pre-existing C17 target\x00\n"
        target.write_bytes(sentinel)
        before = target.stat()
        refused = run([CLI, "--emit-c17-to", target, source(name)], success=False)
        after = target.stat()
        assert case.get("emit", diagnostic).encode() in refused.stderr and not refused.stdout
        assert target.read_bytes() == sentinel
        assert (before.st_ino, before.st_mtime_ns) == (after.st_ino, after.st_mtime_ns)
        assert not list(WORK.glob(target.name + ".tmp*"))
        print(name, "named no-clobber PASS")


OOM_SOURCE = {
    "nested-result-oom": "ledger-shared",
    "outer-nodes-oom": "parcels-extracted",
    "outer-world-oom": "ledger-shared",
    "staging-oom": "ledger-shared",
}
if MODE == "positive":
    positive()
elif MODE == "negative":
    negative()
elif MODE == "identity":
    observed = run([PROBE, MODE, source("ledger-shared")])
    assert not observed.stderr
    assert b"later repeated nested actual/result, foreign receipt/RETURN rejected; restoration PASS" in observed.stdout
    print(observed.stdout.decode(), end="")
elif MODE in OOM_SOURCE:
    name = OOM_SOURCE[MODE]
    observed = run([PROBE, MODE, source(name)], oom=True)
    assert f"OOM {MODE} hit=1 no-clobber=PASS retry=PASS".encode() in observed.stdout
    assert (not observed.stderr if MODE == "staging-oom" else b"P3-OUT-OF-MEMORY" in observed.stderr)
    observations(name, observed.stdout)
    print(observed.stdout.decode(), end="")
else:
    raise AssertionError(MODE)
