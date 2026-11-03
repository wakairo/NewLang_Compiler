#!/usr/bin/env python3
"""R1120 C219 V3 independent calibrated recorder; Actions execution only."""
import argparse
import datetime
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys


def sha(data):
    return hashlib.sha256(data).hexdigest()


class Recorder:
    def __init__(self, output):
        self.output = output
        self.count = 0

    def run(self, label, argv, timeout=20):
        self.count += 1
        out_path = self.output / (label + ".stdout")
        err_path = self.output / (label + ".stderr")
        timed_out = False
        with out_path.open("wb") as out, err_path.open("wb") as err:
            try:
                status = subprocess.run(argv, cwd=self.output, stdout=out, stderr=err,
                                        timeout=timeout, check=False).returncode
            except subprocess.TimeoutExpired:
                status = None
                timed_out = True
        out_bytes, err_bytes = out_path.read_bytes(), err_path.read_bytes()
        row = {"sequence": self.count, "label": label, "argv": argv,
               "returncode": status, "timed_out": timed_out, "timeout_seconds": timeout,
               "stdout_bytes": len(out_bytes), "stdout_sha256": sha(out_bytes),
               "stderr_bytes": len(err_bytes), "stderr_sha256": sha(err_bytes),
               "diagnostics": re.findall(r"\b[A-Z][A-Z0-9]+(?:-[A-Z0-9]+)+\b",
                                         err_bytes.decode("utf-8", errors="replace"))}
        with (self.output / "commands.jsonl").open("a") as log:
            log.write(json.dumps(row, sort_keys=True) + "\n")
        print("R1120_C219_COMMAND " + json.dumps(row, sort_keys=True), flush=True)
        if timed_out:
            raise RuntimeError("STOP: command timeout; partial raw retained: " + label)
        return subprocess.CompletedProcess(argv, status, out_bytes, err_bytes), row


def require(condition, reason):
    if not condition:
        raise RuntimeError("STOP: " + reason)


def no_clobber(recorder, compiler, source, label, expected_rc):
    dest = recorder.output / (label + ".sentinel")
    original = ("R1120_C219_UNCHANGED_" + label + "\n").encode()
    dest.write_bytes(original)
    result, row = recorder.run(label + "_no_clobber",
                              [compiler, "--emit-c17-to", str(dest), str(source)])
    require(result.returncode == expected_rc and not result.stdout and
            dest.read_bytes() == original,
            "named output refusal/no-clobber mismatch " + label)
    return row


def compiler_flags(args):
    flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    # Preserve the already-reviewed job CFLAGS, including its known tool warning.
    flags += shlex.split(os.environ.get("CFLAGS", ""))
    if args.sanitizer != "none":
        require(args.sanitizer in ("address", "undefined"), "unknown sanitizer mode")
        flags += ["-fsanitize=" + args.sanitizer, "-fno-omit-frame-pointer",
                  "-fno-sanitize-recover=all"]
    return flags


def instrument(raw):
    original = raw.decode("ascii")
    edits = {"formals": 0, "results": 0, "bools": 0, "main": 0}
    signature = re.compile(
        r"(static uint8_t (nl_fn_\d+)\(size_t (nl_local_\d+)\)\n\{\n)")
    def formal(match):
        edits["formals"] += 1
        return match.group(1) + (
            '    printf("R1120_C219_ARG ' + match.group(2) + ' %zu\\n", ' +
            match.group(3) + ");\n")
    observed = signature.sub(formal, original)
    call = re.compile(
        r"(^[ \t]*const uint8_t (nl_call_\d+) = (nl_fn_\d+)\([^;\n]*\);\n)", re.M)
    def returned(match):
        edits["results"] += 1
        return match.group(1) + (
            '    printf("R1120_C219_RESULT ' + match.group(3) + " " +
            match.group(2) + ' %u\\n", (unsigned)' + match.group(2) + ");\n")
    observed = call.sub(returned, observed)
    boolean = re.compile(r"(^[ \t]*const _Bool (nl_local_\d+) = [^;\n]*;\n)", re.M)
    def bool_local(match):
        edits["bools"] += 1
        return match.group(1) + (
            '    printf("R1120_C219_BOOL ' + match.group(2) + ' %u\\n", (unsigned)' +
            match.group(2) + ");\n")
    observed = boolean.sub(bool_local, observed)
    main_header = "int main(void)\n{\n"
    edits["main"] = observed.count(main_header)
    require(edits["main"] == 1, "observer needs one actual emitted main")
    observed = observed.replace(main_header, main_header +
        '    printf("R1120_C219_WIDTH %zu %zu\\n", sizeof(size_t) * CHAR_BIT, (size_t)SIZE_MAX);\n')
    # Pure text instrumentation of lexical branch entry, not its condition.
    # Both supported brace layouts are recognized; no guest expression is copied.
    lines, branch_lines = observed.splitlines(True), []
    current_function, waiting_arm = None, None
    arm_sites = {}
    for line in lines:
        function = re.match(r"static uint8_t (nl_fn_\d+)\(size_t ", line)
        if function:
            current_function = function.group(1)
        branch = None
        if current_function and re.match(r"^\s*if\s*\(", line):
            branch = 0
        elif current_function and re.match(r"^\s*}\s*else\s*{?\s*$", line):
            branch = 1
        elif current_function and re.match(r"^\s*else\s*{?\s*$", line):
            branch = 1
        if branch is not None:
            waiting_arm = branch
        branch_lines.append(line)
        if waiting_arm is not None and "{" in line:
            indentation = re.match(r"^\s*", line).group(0) + "    "
            branch_lines.append(indentation +
                'printf("R1120_C219_ARM ' + current_function + " " +
                str(waiting_arm) + '\\n");\n')
            key = current_function + ":" + str(waiting_arm)
            arm_sites[key] = arm_sites.get(key, 0) + 1
            waiting_arm = None
    require(waiting_arm is None, "unrecognized emitted arm brace layout")
    require(all(value == 1 for value in arm_sites.values()),
            "ambiguous repeated emitted arm site")
    require(len(arm_sites) == 2 * edits["formals"],
            "one genuine if/else per frozen native function not observed")
    edits["arm_sites"] = arm_sites
    observed = "".join(branch_lines)
    observed = "#include <stdio.h>\n#include <limits.h>\n" + observed
    return observed.encode("ascii"), edits


def native_case(recorder, args, case, raw, bin_dir, flags, target):
    name = Path(case["file"]).stem
    original_path = recorder.output / (name + "_original.c")
    observed_path = recorder.output / (name + "_observed.c")
    original_path.write_bytes(raw)
    observed, edits = instrument(raw)
    require(edits["results"] == len(case["calls"]), "actual emitted call count " + name)
    require(edits["formals"] == len({x["entry"] for x in case["calls"]}),
            "actual emitted formal count " + name)
    guard = "_Static_assert(SIZE_MAX == UINTMAX_C(" + str(target["max"]) + ")"
    require(guard.encode() in raw, "source/C target guard absent or mismatched " + name)
    observed_path.write_bytes(observed)
    (recorder.output / (name + "_observer.diff")).write_text("".join(
        difflib.unified_diff(raw.decode().splitlines(True), observed.decode().splitlines(True),
                             fromfile="original.c", tofile="observed.c")))
    runs = []
    for optimization in ("-O0", "-O2"):
        for mode, c_path in (("original", original_path), ("observed", observed_path)):
            label = name + "_" + optimization[1:] + "_" + mode
            exe = bin_dir / label
            compiled, compile_row = recorder.run(
                label + "_compile", [args.cc, *flags, optimization, str(c_path), "-o", str(exe)])
            require(compiled.returncode == 0, "strict native compilation " + label)
            executed, execute_row = recorder.run(label + "_run", [str(exe)])
            require(executed.returncode == 0 and not executed.stderr,
                    "native/sanitizer execution " + label)
            row = {"optimization": optimization, "mode": mode, "compile": compile_row,
                   "run": execute_row, "binary_sha256": sha(exe.read_bytes()),
                   "binary_bytes": exe.stat().st_size}
            if mode == "original":
                require(not executed.stdout, "unexpected original guest stdout " + label)
            else:
                lines = executed.stdout.decode("ascii").splitlines()
                widths = [line.split() for line in lines if line.startswith("R1120_C219_WIDTH ")]
                arguments = [line.split() for line in lines if line.startswith("R1120_C219_ARG ")]
                returned = [line.split() for line in lines if line.startswith("R1120_C219_RESULT ")]
                require(len(widths) == 1 and int(widths[0][1]) == target["bits"] and
                        int(widths[0][2]) == target["max"], "observed C target mismatch " + label)
                require(len(arguments) == len(returned) == len(case["calls"]),
                        "actual runtime call trace count " + label)
                measured = []
                for index, (actual_arg, actual_result, expected) in enumerate(
                        zip(arguments, returned, case["calls"])):
                    require(actual_arg[1] == actual_result[1],
                            "actual function argument/result pairing " + label)
                    got_input, got_result = int(actual_arg[2]), int(actual_result[3])
                    require(got_input == int(expected["input"]) and got_result == expected["expected"],
                            "REAL NATIVE ORACLE MISMATCH " + label + " row " + str(index))
                    measured.append({"ordinal": index, "source_entry": expected["entry"],
                                     "physical_function": actual_arg[1], "input": got_input,
                                     "result": got_result, "result_carrier": actual_result[2]})
                row["observed_calls"] = measured
                bool_lines = [line for line in lines if line.startswith("R1120_C219_BOOL ")]
                bool_actual = [int(line.split()[2]) for line in bool_lines]
                require(bool_actual == case["expected_bool_values"],
                        "REAL BOOL LOCAL ORACLE MISMATCH " + label)
                arms = [line.split() for line in lines
                        if line.startswith("R1120_C219_ARM ")]
                require(len(arms) == len(case["calls"]),
                        "selected/unselected arm execution count " + label)
                for actual_arm, actual_arg, expected in zip(arms, arguments, case["calls"]):
                    require(actual_arm[1] == actual_arg[1] and
                            int(actual_arm[2]) == expected["expected_arm"],
                            "REAL SELECTED ARM ORACLE MISMATCH " + label)
                row["bool_local_observations"] = bool_lines
                row["bool_local_values_verified"] = bool_actual
                row["selected_arm_trace"] = arms
            runs.append(row)
    require(original_path.read_bytes() == raw, "original emitted C mutated")
    return {"original_c_sha256": sha(raw), "observed_c_sha256": sha(observed),
            "observer_edits": edits, "runs": runs}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--source-dir", required=True)
    parser.add_argument("--output-dir", required=True)
    parser.add_argument("--cc", required=True)
    parser.add_argument("--sanitizer", required=True)
    parser.add_argument("--phase", choices=["v3"], required=True)
    args = parser.parse_args()
    require(os.environ.get("GITHUB_ACTIONS") == "true", "execution is Actions-only")
    source, output = Path(args.source_dir).resolve(), Path(args.output_dir).resolve()
    output.mkdir(parents=True, exist_ok=False)
    bin_dir = output.parent / "r1120-c219-executables"
    bin_dir.mkdir(exist_ok=False)
    recorder = Recorder(output)
    summary = {"phase": "C219_v3", "independence": "NON_BLIND_CALIBRATED",
               "production_head": "a1091b2672b0a4b1d37e2571ef97bf100882b60b",
               "canonical": "Draft17.34", "cases": [],
               "definition_time_unknown_observed": False,
               "public_pure_predicate_validator_used": False,
               "original_v1": "BLIND_INCONCLUSIVE; old FIRST and refs unchanged",
               "original_v2": "UNEXECUTED; old source/oracle/commit unchanged",
               "limitations": ["No 32-bit execution unless actually measured",
                   "No general LOOP Continue",
                   "Arm nonexecution observed only in separate passive C copy",
                   "No foreign public graph/owner mutation test",
                   "No Product GO or audit-locality value measurement"]}
    for key in ("GITHUB_SHA", "GITHUB_REF", "GITHUB_EVENT_NAME", "GITHUB_RUN_ID",
                "GITHUB_RUN_ATTEMPT", "GITHUB_JOB", "GITHUB_HEAD_REF"):
        summary[key] = os.environ.get(key)
    summary["checked_git_head"] = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], text=True).strip()
    summary["checked_git_tree"] = subprocess.check_output(
        ["git", "rev-parse", "HEAD^{tree}"], text=True).strip()
    summary["compiler_binary_sha256"] = sha(Path(args.compiler).read_bytes())
    summary["cc"], summary["sanitizer"] = args.cc, args.sanitizer
    summary["started_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    (output / "provenance.json").write_text(json.dumps(summary, indent=2) + "\n")
    status = 1
    try:
        receipt = json.loads((source / "EXECUTION_RECEIPT.json").read_text())
        for item in receipt["files"]:
            material = (source / item["path"]).read_bytes()
            require(len(material) == item["bytes"] and sha(material) == item["sha256"],
                    "R execution material integrity " + item["path"])
        summary["receipt_sha256"] = sha((source / "EXECUTION_RECEIPT.json").read_bytes())
        flags = compiler_flags(args)
        version, version_row = recorder.run("native_compiler_version", [args.cc, "--version"])
        require(version.returncode == 0, "native compiler version unavailable")
        summary["native_compiler"] = version.stdout.decode(errors="replace").splitlines()[0]
        summary["native_flags"] = flags
        target_source = output / "target.c"
        target_source.write_text(
            "#include <stdint.h>\n#include <stddef.h>\n#include <limits.h>\n"
            "#include <stdio.h>\nint main(void) {\n"
            '    printf("%zu %zu\\n", sizeof(size_t) * CHAR_BIT, (size_t)SIZE_MAX);\n'
            "    return 0;\n}\n")
        target_exe = bin_dir / "target"
        built, _ = recorder.run("target_compile",
            [args.cc, *flags, "-O0", str(target_source), "-o", str(target_exe)])
        require(built.returncode == 0, "target measurement compilation")
        target_run, _ = recorder.run("target_run", [str(target_exe)])
        require(target_run.returncode == 0 and not target_run.stderr, "target measurement")
        bits, maximum = map(int, target_run.stdout.split())
        require(bits in (32, 64) and maximum == (1 << bits) - 1, "unsupported target model")
        target = {"bits": bits, "max": maximum}
        summary["native_target"] = target
        oracle_bytes = (source / "v3/R1120_C219_V3_EXPECTED.json").read_bytes()
        require(sha(oracle_bytes) == "77219031e4a02354079d300a60507df4a2a1245bdddc314ec845a6bf155be791",
                "prospective C219 V3 oracle differs")
        oracle = json.loads(oracle_bytes)
        summary["v3_oracle_sha256"] = sha(oracle_bytes)
        # Supplementary immutable views, not a new predicate proof issuer.
        public_c = output / "public_observer.c"
        public_c.write_bytes((source / "public_observer.c.txt").read_bytes())
        public_exe = bin_dir / "public_observer"
        repo = source.parents[2]
        libraries = [str(output.parent / ("libnewlang_" + name + ".a"))
                     for name in ("semantic", "checked", "parser", "syntax",
                                  "lexer", "source", "diagnostic")]
        public_build, public_build_row = recorder.run("public_observer_compile",
            [args.cc, *flags, "-O0", "-I", str(repo / "include"), str(public_c),
             "-Wl,--start-group", *libraries, "-Wl,--end-group", "-o", str(public_exe)])
        require(public_build.returncode == 0, "R public observer compilation")
        summary["public_observer"] = {
            "source_sha256": sha(public_c.read_bytes()),
            "compile": public_build_row,
            "binary_sha256": sha(public_exe.read_bytes()),
            "binary_bytes": public_exe.stat().st_size,
            "scope": "public CHECK and retained immutable views; no new public predicate validator"}
        ordered_cases = sorted(oracle["cases"], key=lambda item: (
            0 if item["kind"] == "NEGATIVE_ENTRY_ADAPTER" else
            2 if item["kind"] == "ORIGINAL_PREFIX_ENTRY_ADAPTER" else 1, item["file"]))
        for case in ordered_cases:
            path = source / "v3" / case["file"]
            content = path.read_bytes()
            require(sha(content) == case["sha256"] and len(content) == case["bytes"],
                    "V3 source SHA/size " + case["file"])
            result = {"file": case["file"], "source_sha256": sha(content), "kind": case["kind"]}
            summary["cases"].append(result)
            if case["target"] == "native_64_only" and bits != 64:
                result["disposition"] = "NOT_TESTED_TARGET_CONDITIONAL"
                continue
            label = Path(case["file"]).stem
            checked, row = recorder.run(label + "_check", [args.compiler, "--check", str(path)])
            result["check"] = row
            if case["kind"] == "NEGATIVE_ENTRY_ADAPTER":
                require(checked.returncode == 3 and not checked.stdout and row["diagnostics"],
                        "negative incorrectly accepted or lacks rejection " + label)
                emitted, emit_row = recorder.run(label + "_emit", [args.compiler, str(path)])
                require(emitted.returncode == 3 and not emitted.stdout and emit_row["diagnostics"],
                        "negative emitted C or lacks rejection " + label)
                result["emit"] = emit_row
                result["no_clobber"] = no_clobber(recorder, args.compiler, path, label, 3)
                result["diagnostic_matches_prediction"] = (
                    case["expected_diagnostic"] in row["diagnostics"] and
                    case["expected_diagnostic"] in emit_row["diagnostics"])
                result["disposition"] = ("FRONTEND_REJECT_OBSERVED_NATIVE_NOT_REACHED"
                    if result["diagnostic_matches_prediction"] else
                    "FRONTEND_REJECT_DIFFERENT_LAYER_NATIVE_NOT_REACHED")
                continue
            require(checked.returncode == case["expected_check_rc"] and
                    not checked.stdout and not checked.stderr,
                    "unexpected actual semantic CHECK " + label)
            observed_public, public_row = recorder.run(label + "_public_observer",
                [str(public_exe), str(path)])
            require(observed_public.returncode == 0 and not observed_public.stderr,
                    "R public observation failed " + label)
            public_rows = [json.loads(line) for line in observed_public.stdout.splitlines()]
            public_summary = [item for item in public_rows if item.get("event") == "summary"]
            require(len(public_summary) == 1 and public_summary[0]["complete_observation"],
                    "public view trace incomplete " + label)
            result["public_observation"] = {"command": public_row,
                                            "summary": public_summary[0]}
            emitted, row = recorder.run(label + "_emit", [args.compiler, str(path)])
            result["emit"] = row
            permitted_profile_refusal = ("backend_fallback_policy" in case and
                emitted.returncode == 4 and "V1-BACKEND-UNSUPPORTED" in row["diagnostics"])
            require(emitted.returncode == case["expected_emitter_rc"] or permitted_profile_refusal,
                    "unexpected emitter result/profile " + label)
            if emitted.returncode:
                require(not emitted.stdout and case.get("expected_emitter_diagnostic",
                    "V1-BACKEND-UNSUPPORTED") in row["diagnostics"],
                        "coverage refusal diagnostic " + label)
                result["no_clobber"] = no_clobber(recorder, args.compiler, path, label, 4)
                result["disposition"] = "CHECK_PASS_NATIVE_COVERAGE_REFUSAL"
                result["forecast_emitter_rc"] = case["expected_emitter_rc"]
                continue
            require(bool(emitted.stdout) and not emitted.stderr, "invalid successful C17 " + label)
            named = output / (label + "_named.c")
            named.write_bytes(b"R1120_C219_EXISTING_OUTPUT\n")
            installed, installed_row = recorder.run(label + "_named",
                [args.compiler, "--emit-c17-to", str(named), str(path)])
            require(installed.returncode == 0 and not installed.stdout and not installed.stderr and
                    named.read_bytes() == emitted.stdout, "named/stdout C bytes differ " + label)
            result["named"] = installed_row
            result["native"] = native_case(recorder, args, case, emitted.stdout,
                                            bin_dir, flags, target)
            result["disposition"] = "FINITE_NATIVE_OBSERVED_PASS"
        summary["verdict"] = "C219_V3_NON_BLIND_FINITE_RESULTS_WITH_EXPLICIT_COVERAGE_LIMITS"
        status = 0
    except Exception as exc:
        summary["verdict"] = "STOP"
        summary["reason"] = str(exc)
    summary["command_count"] = recorder.count
    summary["finished_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    (output / "summary.json").write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n")
    print("R1120_C219_SUMMARY " + json.dumps(summary, sort_keys=True), flush=True)
    return status


if __name__ == "__main__":
    sys.exit(main())
