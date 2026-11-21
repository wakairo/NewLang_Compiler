"""P1561 ordinary quantity composition, identity, C17 and rollback tests."""

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
for filename, expected in FREEZE["files"].items():
    path = ROOT / filename if filename == "oracle.json" else ROOT / "source" / filename
    data = path.read_bytes()
    assert len(data) == expected["bytes"], filename
    assert hashlib.sha256(data).hexdigest() == expected["sha256"], filename
assert os.sysconf("SC_LONG_BIT") == ORACLE["target_bits"]

ENV = {
    **os.environ,
    "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
    "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1",
}
LIMITS = {"selected": 300, "native": 24, "oom_cases": 16}
sequence = 0


def charge(kind, command, amount=1):
    filename = os.environ.get("P1561_LEDGER")
    if not filename:
        return
    ledger = Path(filename)
    with ledger.with_suffix(".lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        record = json.loads(ledger.read_bytes())
        total = record["counts"][kind] + amount
        assert total <= LIMITS[kind], (kind, total)
        record["counts"][kind] = total
        record["events"].append(
            dict(kind=kind, mode=MODE, command=[str(part) for part in command],
                 charged=amount, raw=str(WORK))
        )
        ledger.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")


def run(command, success=True, guest=False):
    global sequence
    command = [str(part) for part in command]
    charge("selected", command)
    if guest:
        charge("native", command)
    sequence += 1
    stem = WORK / f"{sequence:03}"
    actual = subprocess.run(command, capture_output=True, timeout=90, env=ENV)
    stem.with_suffix(".stdout").write_bytes(actual.stdout)
    stem.with_suffix(".stderr").write_bytes(actual.stderr)
    stem.with_suffix(".json").write_text(
        json.dumps({"command": command, "returncode": actual.returncode}, indent=2) + "\n",
        encoding="utf-8",
    )
    assert (actual.returncode == 0) == success, (
        command, actual.returncode, actual.stdout.decode(errors="replace"),
        actual.stderr.decode(errors="replace"), str(stem),
    )
    return actual


def source(filename):
    return ROOT / "source" / filename


def trace_observations(case, report):
    lines = report.decode().splitlines()
    assert "SOURCE PASS" in lines and "CHECK PASS" in lines, lines
    assert any(re.fullmatch(r"DEFINITION unknown=[1-9]\d*", line) for line in lines)
    old_division = int(case["source"] == "same-leaf.nl")
    assert f"OUTER result=1 world=1 division={old_division} remainder=0" in lines
    calls, copies, bindings = [], [], {}
    for line in lines:
        if line.startswith("CALL "):
            (
                _, destination, callee, op, argument, actual, divisor, result,
                edge, function, actual_id, formal, event, value, input_id,
                operand, seal,
            ) = line.split()
            assert seal == "seal=1"
            calls.append((
                [destination, callee, op, None if argument == "-" else argument,
                 int(actual), int(divisor), int(result), edge],
                list(map(int, (function, actual_id, formal, event, value, input_id, operand))),
            ))
        elif line.startswith("BIND "):
            _, name, symbol, number, value = line.split()
            assert name not in bindings
            bindings[name] = tuple(map(int, (symbol, number, value)))
        elif line.startswith("COPY "):
            _, dest, origin, old, new = line.split()
            copies.append((dest, origin, int(old), int(new)))
    assert [observed for observed, _ in calls] == case["calls"], calls
    assert {name: value[1] for name, value in bindings.items()} == case["caller"]
    assert len({v[0] for v in bindings.values()}) == len(bindings)
    definitions = {}
    actual_ids, formals, event_ids, result_ids = set(), set(), set(), set()
    for observed, ids in calls:
        destination, callee, op, argument, actual, divisor, _, _ = observed
        function, actual_id, formal, event, result, input_id, operand = ids
        assert 0 < actual_id < operand < event <= result
        assert actual_id not in actual_ids and formal not in formals
        assert event not in event_ids and result not in result_ids
        actual_ids.add(actual_id)
        formals.add(formal)
        event_ids.add(event)
        result_ids.add(result)
        assert bindings[destination][2] == result
        if argument:
            assert bindings[argument][1:] == (actual, input_id)
            assert 0 < input_id < actual_id
        else:
            assert input_id == 0
        signature = (function, op, divisor)
        assert definitions.setdefault(callee, signature) == signature
    assert len({spec[0] for spec in definitions.values()}) == len(definitions)
    for dest, origin, old, new in copies:
        assert bindings[origin][2] == old < new == bindings[dest][2]
        assert bindings[origin][1] == bindings[dest][1]
    return calls, bindings, definitions


def check_generated_c(program, calls, bindings, definitions):
    assert "NewLang usize target mismatch" in program
    functions = re.findall(r"^static size_t nl_qty_fn_(\d+)\(size_t nl_local_\d+\)\n\{\n(.*?)^\}",
                           program, re.M | re.S)
    assert [int(number) for number, _ in functions] == [spec[0] for spec in definitions.values()]
    for (_, body), (_, op, divisor) in zip(functions, definitions.values(), strict=True):
        assert re.findall(r" ([/%]) ", body) == [op]
        assert f" {op} ((size_t)UINTMAX_C({divisor}))" in body
        assert f'_Static_assert(UINTMAX_C({divisor}) != 0, "nonzero divisor");' in body
    caller = re.search(r"^static void nl_qty_fn_\d+\(void\)\n\{\n(.*?)^\}",
                       program, re.M | re.S)
    assert caller
    emitted_calls = re.findall(r"^    size_t nl_local_(\d+) = nl_qty_fn_(\d+)\((.*)\);$",
                               caller[1], re.M)
    expected_calls = []
    for observed, ids in calls:
        destination, _, _, formal_source, actual, _, _, _ = observed
        argument = (f"nl_local_{bindings[formal_source][0]}"
                    if formal_source else f"((size_t)UINTMAX_C({actual}))")
        expected_calls.append((str(bindings[destination][0]), str(ids[0]), argument))
    assert emitted_calls == expected_calls


def positive():
    gcc = shutil.which("gcc")
    clang = shutil.which("clang-23") or shutil.which("clang")
    assert gcc and clang, "P1561 requires GCC and Clang"
    strict = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    builds = [
        ("gcc", gcc, ["-O2"]),
        ("clang", clang, ["-O2"]),
        ("asan", gcc, ["-O1", "-fsanitize=address", "-fno-omit-frame-pointer", "-no-pie"]),
        ("ubsan", gcc, ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"]),
    ]
    for case in ORACLE["positive"]:
        filename = case["source"]
        path = source(filename)
        checked = run([CLI, "--check", path])
        assert not checked.stdout and not checked.stderr
        report = run([PROBE, "inspect", path])
        assert not report.stderr
        calls, bindings, definitions = trace_observations(case, report.stdout)
        original = WORK / (filename + ".unmodified.c")
        emitted = run([CLI, "--emit-c17-to", original, path])
        assert not emitted.stdout and not emitted.stderr
        original_bytes = original.read_bytes()
        program = original_bytes.decode("utf-8")
        check_generated_c(program, calls, bindings, definitions)
        run([gcc, *strict, "-fsyntax-only", original])

        marker = "    return;\n}\n\nint main(void)\n"
        assert program.count(marker) == 1
        observations = "".join(
            f'    printf("{name}=%zu\\n", nl_local_{bindings[name][0]});\n'
            for name in case["caller"]
        )
        instrumented = WORK / (filename + ".observed.c")
        inserted = "#include <stdio.h>\n" + program.replace(marker, observations + marker)
        assert inserted.removeprefix("#include <stdio.h>\n").replace(observations + marker, marker) == program
        assert original.read_bytes() == original_bytes
        instrumented.write_text(inserted, encoding="utf-8")
        wanted = "".join(f"{name}={value}\n" for name, value in case["caller"].items())
        for tag, compiler, flags in builds:
            executable = WORK / (filename + "." + tag)
            run([compiler, *strict, *flags, instrumented, "-o", executable])
            result = run([executable], guest=True)
            assert not result.stderr and result.stdout.decode() == wanted
            print(f"{filename} {tag} {len(definitions)} definitions, actual values PASS")


def negative():
    for case in ORACLE["negative"]:
        name, stage, diagnostic = case["source"], case["stage"], case["diagnostic"].encode()
        checked = run([CLI, "--check", source(name)], success=stage == "C17")
        assert not checked.stdout
        assert (not checked.stderr if stage == "C17" else diagnostic in checked.stderr)
        dest = WORK / (name + ".c")
        # P1579 selects this formerly excluded legal composition. The old
        # source and oracle remain frozen with their original rejection.
        if name == "nested-helper.nl":
            assert stage == "C17" and diagnostic == b"V1-BACKEND-UNSUPPORTED"
            emitted = run([CLI, "--emit-c17-to", dest, source(name)])
            assert not emitted.stdout and not emitted.stderr
            c17 = dest.read_text()
            assert " / ((size_t)UINTMAX_C(8))" in c17 and " % ((size_t)UINTMAX_C(7))" in c17
            print(f"{name} C1298 lawful helper composition C17 PASS")
            continue
        sentinel = b"P1561 pre-existing named output\n"
        dest.write_bytes(sentinel)
        before = dest.stat()
        emitted = run([CLI, "--emit-c17-to", dest, source(name)], success=False)
        after = dest.stat()
        assert not emitted.stdout and diagnostic in emitted.stderr
        assert dest.read_bytes() == sentinel
        assert (before.st_ino, before.st_mtime_ns) == (after.st_ino, after.st_mtime_ns)
        assert not list(WORK.glob(dest.name + ".tmp*"))
        print(f"{name} {stage} {case['diagnostic']} named no-clobber PASS")


MIXED = ORACLE["positive"][2]
if MODE == "positive":
    positive()
elif MODE == "negative":
    negative()
elif MODE == "authenticity":
    report = run([PROBE, MODE, source(MIXED["source"])])
    assert not report.stderr
    assert b"callee/operator/result/source rejected with equal values; restoration PASS" in report.stdout
    print(report.stdout.decode(), end="")
elif MODE in {"second-leaf-oom", "outer-nodes-oom", "outer-world-oom", "staging-oom"}:
    command = [PROBE, MODE, source(MIXED["source"])]
    charge("oom_cases", command, amount=2)  # Injected failure and pristine retry.
    report = run(command)
    assert f"OOM {MODE} hit=1 no-clobber=PASS retry=PASS".encode() in report.stdout
    assert (not report.stderr if MODE == "staging-oom" else b"P3-OUT-OF-MEMORY" in report.stderr)
    trace_observations(MIXED, report.stdout)
    print(report.stdout.decode(), end="")
else:
    raise AssertionError(MODE)
