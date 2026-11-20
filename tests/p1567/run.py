"""Check the frozen pre-operation Copy family at the public and native boundaries."""

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
    filename = os.environ.get("P1567_LEDGER")
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


def evidence(case, text):
    lines = text.decode().splitlines()
    assert {"SOURCE PASS", "CHECK PASS", "OUTER result=1 world=1 plan=1 machine=1"} <= set(lines), lines
    leaf_definitions = {call["function"] for call in case["calls"]}
    assert f"DEFINITION unknown={len(leaf_definitions)}" in lines, lines
    bindings, calls, copies = {}, [], {}
    for line in lines:
        fields = line.split()
        if fields[0] == "BIND":
            _, name, symbol, number, value = fields
            assert name not in bindings
            bindings[name] = tuple(map(int, (symbol, number, value)))
        elif fields[0] == "COPY":
            _, destination, phase, name, src, dst, value = fields
            copies.setdefault(destination, []).append((phase, name, int(src), int(dst), int(value)))
        elif fields[0] == "CALL":
            assert fields[-3:] == ["seal=1", "plan=1", "event=1"]
            _, dest, function, op, actual, rhs, result, before, after, edge, *ids = fields[:-3]
            details = dict(function=function, actual=int(actual), op=op, rhs=int(rhs),
                           result=int(result), copies_before=int(before),
                           copies_after=int(after), body_exit=edge)
            calls.append((details, dest, tuple(map(int, ids))))
    assert [details for details, _, _ in calls] == case["calls"], calls
    assert {name: value[1] for name, value in bindings.items()} == case["bindings"]
    assert len({value[0] for value in bindings.values()}) == len(bindings)
    caller_symbols = {symbol: (number, value) for symbol, number, value in bindings.values()}

    definitions, actual_ids, formal_symbols, event_ids, result_ids = {}, set(), set(), set(), set()
    for spec, dest, ids in calls:
        fn, actual, formal, lhs, operand, event, result, argument = ids
        assert 0 < actual < operand < event <= result
        assert actual not in actual_ids and formal not in formal_symbols
        assert event not in event_ids and result not in result_ids
        actual_ids.add(actual)
        formal_symbols.add(formal)
        event_ids.add(event)
        result_ids.add(result)
        assert formal not in caller_symbols and bindings[dest][2] == result
        if argument:
            earlier = caller_symbols[argument]
            assert earlier[0] == spec["actual"] and 0 < earlier[1] < actual
        prefix = {formal: actual}
        pre_count = post_count = 0
        for phase, _, src, dst, value in copies.get(dest, ()):
            if phase == "pre":
                assert not post_count and src in prefix and dst not in prefix
                assert prefix[src] < value < operand
                prefix[dst] = value
                pre_count += 1
            else:
                assert phase == "post" and src not in prefix and dst not in prefix
                assert event < value < result
                post_count += 1
        assert lhs in prefix
        assert (pre_count, post_count) == (spec["copies_before"], spec["copies_after"])
        if spec["body_exit"] == "return":
            assert event < result
        representative = (fn, formal, lhs, spec["op"], spec["rhs"])
        saved = definitions.setdefault(spec["function"], representative)
        assert (saved[0], saved[3:]) == (fn, representative[3:])
    return bindings, calls, definitions


def inspect_c(program, report):
    bindings, calls, definitions = report
    assert "NewLang usize target mismatch" in program
    helpers = re.findall(r"^static size_t nl_qty_fn_(\d+)\(size_t nl_local_(\d+)\)\n\{\n(.*?)^\}",
                         program, re.M | re.S)
    assert len(helpers) == len(definitions)
    for (fn, formal, body), (want_fn, want_formal, lhs, op, rhs) in zip(
        helpers, definitions.values(), strict=True
    ):
        assert (int(fn), int(formal)) == (want_fn, want_formal)
        assert re.findall(r" ([/%]) ", body) == [op]
        assert f"nl_local_{lhs} {op} ((size_t)UINTMAX_C({rhs}))" in body
        assert f'_Static_assert(UINTMAX_C({rhs}) != 0, "nonzero divisor");' in body
    caller = re.search(r"^static void nl_qty_fn_\d+\(void\)\n\{\n(.*?)^\}", program, re.M | re.S)
    assert caller
    actual = re.findall(r"^    size_t nl_local_(\d+) = nl_qty_fn_(\d+)\((.*)\);$",
                        caller[1], re.M)
    expected = []
    for spec, dest, ids in calls:
        fn, argument = ids[0], ids[-1]
        expression = f"nl_local_{argument}" if argument else f"((size_t)UINTMAX_C({spec['actual']}))"
        expected.append((str(bindings[dest][0]), str(fn), expression))
    assert actual == expected


def positive():
    gcc = shutil.which("gcc")
    clang = shutil.which("clang-23") or shutil.which("clang")
    assert gcc and clang
    strict = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    for name in ("shelves-direct", "shelves-aliases", "mixed-lots"):
        checked = run([CLI, "--check", source(name)])
        assert not checked.stdout and not checked.stderr
        observed = run([PROBE, "inspect", source(name)])
        assert not observed.stderr
        case = ORACLE[name]
        report = evidence(case, observed.stdout)
        original = WORK / f"{name}.c"
        emitted = run([CLI, "--emit-c17-to", original, source(name)])
        assert not emitted.stdout and not emitted.stderr
        raw = original.read_bytes()
        program = raw.decode("utf-8")
        inspect_c(program, report)

        bindings = report[0]
        marker = "    return;\n}\n\nint main(void)\n"
        assert program.count(marker) == 1
        print_calls = "".join(
            f'    printf("{key}=%zu\\n", nl_local_{bindings[key][0]});\n'
            for key in case["bindings"]
        )
        measured = "#include <stdio.h>\n" + program.replace(marker, print_calls + marker)
        assert measured.removeprefix("#include <stdio.h>\n").replace(print_calls + marker, marker) == program
        observed_c = WORK / f"{name}.observed.c"
        observed_c.write_text(measured, encoding="utf-8")
        expected = "".join(f"{key}={value}\n" for key, value in case["bindings"].items())
        hosts = [("gcc", gcc, ["-O2"], observed_c, expected)]
        if name != "shelves-direct":
            hosts.append(("clang", clang, ["-O2"], observed_c, expected))
        if name == "mixed-lots":
            hosts.extend([
                ("exact-c17", gcc, ["-O2"], original, ""),
                ("asan", gcc, ["-O1", "-fsanitize=address", "-fno-omit-frame-pointer", "-no-pie"],
                 observed_c, expected),
                ("ubsan", gcc, ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"],
                 observed_c, expected),
            ])
        for tag, compiler, flags, input_c, stdout in hosts:
            exe = WORK / f"{name}.{tag}"
            run([compiler, *strict, *flags, input_c, "-o", exe], native=True)
            native = run([exe], native=True)
            assert native.stdout.decode() == stdout and not native.stderr
            print(name, tag, "PASS")
        assert original.read_bytes() == raw


def negative():
    for name in ("equal-literal", "return-prefix", "zero", "wrong-type"):
        case = ORACLE[name]
        checked = run([CLI, "--check", source(name)], success=case["check"] == "PASS")
        assert not checked.stdout
        if "computed" in case:
            assert not checked.stderr
            observation = run([PROBE, "excluded", source(name)])
            assert not observation.stderr
            assert f"EXCLUDED CHECK value={case['computed']} selected seal=0 no-clobber=PASS" in observation.stdout.decode()
        else:
            assert case["check"].encode() in checked.stderr
        dest = WORK / f"{name}.c"
        sentinel = b"P1567 existing named target\x00\n"
        dest.write_bytes(sentinel)
        before = dest.stat()
        refused = run([CLI, "--emit-c17-to", dest, source(name)], success=False)
        after = dest.stat()
        assert case.get("emit", case["check"]).encode() in refused.stderr and not refused.stdout
        assert dest.read_bytes() == sentinel
        assert (before.st_ino, before.st_mtime_ns) == (after.st_ino, after.st_mtime_ns)
        assert not list(WORK.glob(dest.name + ".tmp*"))
        print(name, "named no-clobber PASS")


MIXED = "mixed-lots"
if MODE == "positive":
    positive()
elif MODE == "negative":
    negative()
elif MODE == "identity":
    report = run([PROBE, MODE, source(MIXED)])
    assert not report.stderr
    assert b"equal-valued operand/result and foreign receipt rejected; restoration PASS" in report.stdout
    print(report.stdout.decode(), end="")
elif MODE in {"second-leaf-oom", "outer-nodes-oom", "outer-world-oom", "staging-oom"}:
    report = run([PROBE, MODE, source(MIXED)], oom=True)
    assert f"OOM {MODE} hit=1 no-clobber=PASS retry=PASS".encode() in report.stdout
    assert (not report.stderr if MODE == "staging-oom" else b"P3-OUT-OF-MEMORY" in report.stderr)
    evidence(ORACLE[MIXED], report.stdout)
    print(report.stdout.decode(), end="")
else:
    raise AssertionError(MODE)
