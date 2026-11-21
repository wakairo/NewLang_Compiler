"""R1589 inherited ORIGINAL-FIRST measurement in GitHub Actions.

Original inputs stay immutable. A lawful-source refusal records a capability
boundary, not source invalidity or unsoundness. Suppressed CTest success output
is not considered retrieved raw merely because this driver saved a report.
"""
from __future__ import annotations

import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys

HERE = Path(__file__).resolve().parent
TARGET = "fd42817e053e773d7b7bcbbadb9ac4fb6a161f23"
CANONICAL = "3d2c0e05a206c905d5f5cfab12659d1b90b63456"


class MeasurementStop(Exception):
    pass


class CoverageStop(MeasurementStop):
    pass


class ApparatusStop(MeasurementStop):
    pass


def digest(data):
    return hashlib.sha256(data).hexdigest()


def verify_originals():
    manifest = json.loads((HERE / "manifest.json").read_text())
    entries = manifest["artifacts"]
    if len(entries) != 7:
        raise ApparatusStop("manifest must have exactly seven entries")
    total = 0
    for item in entries:
        data = (HERE / "original" / item["name"]).read_bytes()
        if len(data) != item["bytes"] or digest(data) != item["sha256"]:
            raise MeasurementStop("ORIGINAL_EVIDENCE_BYTE_HASH_BREACH")
        if b"\r" in data or not data.endswith(b"\n") or data.endswith(b"\n\n"):
            raise MeasurementStop("ORIGINAL_EVIDENCE_LF_BREACH")
        if any(value >= 128 for value in data):
            raise MeasurementStop("ORIGINAL_EVIDENCE_ASCII_BREACH")
        total += len(data)
    if total != 25104:
        raise MeasurementStop("ORIGINAL_EVIDENCE_TOTAL_BREACH")
    return manifest


def git_text(*args):
    result = subprocess.run(
        ["git", *args], cwd=HERE, capture_output=True, check=False,
        text=True, timeout=10,
    )
    if result.returncode:
        raise ApparatusStop("checkout metadata unavailable: " + result.stderr)
    return result.stdout.strip()


def checkout():
    return {
        "head": git_text("rev-parse", "HEAD"),
        "tree": git_text("rev-parse", "HEAD^{tree}"),
        "ordered_commit_and_parents": git_text(
            "rev-list", "--parents", "-n", "1", "HEAD"
        ).split(),
        "github_sha": os.environ.get("GITHUB_SHA"),
        "head_ref": os.environ.get("GITHUB_HEAD_REF"),
        "run_id": os.environ.get("GITHUB_RUN_ID"),
        "attempt": os.environ.get("GITHUB_RUN_ATTEMPT"),
    }


def inventory(ctest, build):
    manifest = verify_originals()
    print("R1589_CHECKOUT " + json.dumps(checkout(), sort_keys=True), flush=True)
    result = subprocess.run(
        [ctest, "--test-dir", build, "-N"],
        capture_output=True, text=True, check=False, timeout=30,
    )
    print(result.stdout, end="", flush=True)
    print(result.stderr, end="", file=sys.stderr, flush=True)
    names = re.findall(r"(?m)^\s*Test\s+#\d+:\s*(\S+)\s*$", result.stdout)
    total = re.search(r"(?m)^Total Tests:\s*(\d+)\s*$", result.stdout)
    expected = {
        "r1589.original_" + Path(item["name"]).stem
        for item in manifest["artifacts"] if item["name"].endswith(".nl")
    }
    if (
        result.returncode or total is None
        or int(total.group(1)) != len(names)
        or len(set(names)) != len(names)
        or not expected.issubset(set(names))
    ):
        raise ApparatusStop("R1589 actual CTest-N inventory is incomplete")
    print("R1589_INVENTORY " + json.dumps({
        "total": len(names), "unique": len(set(names)),
        "own": sorted(expected), "original_bytes": 25104,
        "tests_executed": 0, "semantic_native_verdict": "NOT_TESTED",
    }, sort_keys=True), flush=True)


def expected_main(oracle, source):
    for case in oracle["positives"]:
        if case["source"] == source:
            return {
                place.removeprefix("main."): number
                for invocation in case["invocations"]
                for place, number in invocation["executed_flow"]
                if place.startswith("main.")
            }
    if source == "lawful_no_if_control.nl":
        values = next(
            case["expected_values"] for case in oracle["controls"]
            if case["source"] == source
        )
        return {
            "realzero": values["realzero"],
            "quotient": values["quotient=floor(0/7)"],
            "remainder": values["remainder=29%6"],
        }
    return {}


def execute(record, directory, stage, command, timeout=30):
    command = [str(part) for part in command]
    try:
        completed = subprocess.run(
            command, stdin=subprocess.DEVNULL, capture_output=True,
            check=False, timeout=timeout,
        )
        code, out, err = (
            completed.returncode, completed.stdout, completed.stderr
        )
    except subprocess.TimeoutExpired as failure:
        code = "TIMEOUT"
        out, err = failure.stdout or b"", failure.stderr or b""
    (directory / (stage + ".stdout")).write_bytes(out)
    (directory / (stage + ".stderr")).write_bytes(err)
    item = {
        "stage": stage, "command": command, "returncode": code,
        "stdout_sha256": digest(out), "stderr_sha256": digest(err),
        "stdout": out.decode("utf-8", errors="replace"),
        "stderr": err.decode("utf-8", errors="replace"),
    }
    record["commands"].append(item)
    print("R1589_COMMAND " + json.dumps(item, sort_keys=True), flush=True)
    if code == "TIMEOUT":
        raise ApparatusStop("process timeout at " + stage)
    if code < 0 or any(marker in item["stderr"] for marker in (
        "ERROR: AddressSanitizer", "runtime error:", "UndefinedBehaviorSanitizer"
    )):
        raise MeasurementStop("PROCESS_OR_SANITIZER_ANOMALY at " + stage)
    return item


def public_observations(record, output, expected, minimum):
    lines = [line.split("\t") for line in output.splitlines()]
    kinds = [line for line in lines if line[:2] == ["TYPE_KIND", "USIZE"]]
    if len(kinds) != 1:
        raise ApparatusStop("missing own probe type-kind metadata")
    usize_kind = int(kinds[0][2])
    main = {}
    for fields in lines:
        if fields[0] == "BINDING" and fields[3] == "1":
            if fields[5] == "UNAVAILABLE":
                continue
            name = bytes.fromhex(fields[5]).decode("ascii")
            main.setdefault(name, []).append({
                "fragment": int(fields[1]), "node": int(fields[2]),
                "initializer": int(fields[4]), "result_count": int(fields[6]),
                "visible": fields[7] == "1", "known": fields[8] == "1",
                "value": int(fields[9]), "type_kind": int(fields[10]),
            })
    comparisons = []
    for name, prediction in expected.items():
        rows = main.get(name, [])
        if len(rows) != 1 or not rows[0]["visible"]:
            comparisons.append({
                "binding": name, "status": "NOT_OBSERVABLE",
                "expected": prediction,
            })
            continue
        row = rows[0]
        if row["type_kind"] != usize_kind:
            raise MeasurementStop("OBSERVED_MAIN_QUANTITY_TYPE_MISMATCH")
        if not row["known"]:
            comparisons.append({
                "binding": name, "status": "UNKNOWN_NOT_KNOWN_ZERO",
                "expected": prediction, "observation": row,
            })
            continue
        comparisons.append({
            "binding": name, "status": (
                "KNOWN_STATIC_NUMBER_MATCH" if row["value"] == prediction
                else "KNOWN_STATIC_NUMBER_MISMATCH"
            ),
            "expected": prediction, "observation": row,
        })
        if row["value"] != prediction:
            record["main_comparisons"] = comparisons
            raise MeasurementStop("OBSERVED_KNOWN_MAIN_NUMBER_MISMATCH")
    arithmetic = [line for line in lines if line[0] in ("DIV", "REM")]
    maxima = sorted({int(line[8]) for line in arithmetic})
    record["main_comparisons"] = comparisons
    record["public_summary"] = {
        "local_arithmetic_events": len(arithmetic),
        "known_local_events": sum(
            line[3] == "1" and line[4] == "1" for line in arithmetic
        ),
        "unknown_local_events": sum(
            line[3] != "1" or line[4] != "1" for line in arithmetic
        ),
        "observed_target_maxima": maxima,
        "minimum_target_maximum": minimum,
        "target_qualification": "OBSERVED" if maxima else "UNRESOLVED",
        "hypothetical_arm_fragments": sum(
            line[0] == "FRAGMENT" and line[5] == "1" for line in lines
        ),
        "whole_call_result_world_custody": "NOT_OBSERVABLE",
        "selected_runtime_arm": "NOT_OBSERVABLE",
        "native_internal_quantity": "NOT_OBSERVABLE",
        "binding_name_role": "source locator, not an identity certificate",
    }
    if len(maxima) > 1:
        raise MeasurementStop("INCONSISTENT_OBSERVED_TARGET_MAXIMA")
    if maxima and maxima[0] < minimum:
        raise CoverageStop("TARGET_INELIGIBLE_OR_SOURCE_TARGET_CONTRADICTION")


def publish(record, directory):
    payload = json.dumps(record, sort_keys=True, indent=2) + "\n"
    (directory / "report.json").write_text(payload, encoding="utf-8")
    print("R1589_RESULT " + payload, flush=True)
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        fence = chr(96) * 3
        block = (
            "\n<details><summary>R1589 " + record["case"] + ": "
            + record["outcome"] + "</summary>\n\n" + fence + "json\n"
            + payload + fence + "\n</details>\n"
        )
        with open(summary, "a", encoding="utf-8") as stream:
            fcntl.flock(stream.fileno(), fcntl.LOCK_EX)
            stream.write(block)
            stream.flush()
            fcntl.flock(stream.fileno(), fcntl.LOCK_UN)


def measure(name, compiler, probe, cc, sanitizer, build_type, output_root):
    manifest = verify_originals()
    entries = {item["name"]: item for item in manifest["artifacts"]}
    source_name = name + ".nl"
    entry = entries[source_name]
    source = HERE / "original" / source_name
    oracle = json.loads((HERE / "original" / "oracle.json").read_text())
    expected = expected_main(oracle, source_name)
    invalid = entry["classification"] == "true-invalid"
    minimum = oracle["target_policy"]["per_source_minimum_U"][source_name]
    directory = Path(output_root) / name
    directory.mkdir(parents=True, exist_ok=True)
    record = {
        "case": source_name, "classification": entry["classification"],
        "original": entry, "canonical": CANONICAL, "target": TARGET,
        "checkout": checkout(), "sanitizer": sanitizer,
        "build_type": build_type, "commands": [], "outcome": "IN_PROGRESS",
        "original_first": 7055394999,
        "independence": "inherited original FIRST, not new blind selection",
        "native_internal_quantity": "NOT_OBSERVABLE",
        "runtime_selected_arm": "NOT_OBSERVABLE",
        "whole_call_result_world_custody": "NOT_OBSERVABLE",
        "successful_child_console_transport": "may be suppressed by CTest",
    }
    result = 1
    try:
        check = execute(record, directory, "cli_check",
                        [compiler, "--check", source])
        if invalid and check["returncode"] == 0:
            raise MeasurementStop("TRUE_INVALID_ACCEPTED_BY_ORDINARY_CHECK")
        observed = execute(
            record, directory, "public_probe",
            [probe, source, "invalid" if invalid else "lawful"],
        )
        public_code = observed["returncode"]
        record["public_stage_records"] = [
            line for line in observed["stdout"].splitlines()
            if line.startswith(("STAGE\t", "STOP\t", "LIMIT\t"))
        ]
        input_lengths = [
            line.split("\t")[1] for line in observed["stdout"].splitlines()
            if line.startswith("INPUT_LENGTH\t")
        ]
        if public_code < 70 and input_lengths != [str(entry["bytes"])]:
            raise MeasurementStop("PUBLIC_INPUT_LENGTH_MISMATCH")
        if public_code >= 70:
            if "R_APPARATUS_" in observed["stdout"] or public_code == 70:
                raise ApparatusStop("public probe host/apparatus failure")
            raise MeasurementStop("PUBLIC_OR_SEMANTIC_ANOMALY")
        if invalid:
            if public_code not in (10, 20):
                raise MeasurementStop("INVALID_NOT_REJECTED_BY_PUBLIC_PATH")
            record["outcome"] = "TRUE_INVALID_REJECTED_BOUNDARY_RECORDED"
            record["intended_specific_check"] = (
                "NOT_TESTED: parse refusal" if public_code == 20
                else "semantic refusal; reason requires diagnostic inspection"
            )
            record["target_qualification"] = (
                "UNRESOLVED in this rejected path; do not infer target width"
            )
            record["native"] = "NOT_RUN_INVALID_SOURCE"
            result = 0
        else:
            if public_code == 0:
                public_observations(
                    record, observed["stdout"], expected, minimum
                )
            elif public_code in (30, 31):
                record["public_summary"] = {
                    "adapter": "NOT_OBSERVABLE", "quantities": "NOT_VERIFIED",
                }
            elif public_code in (10, 20):
                record["public_summary"] = {
                    "first_refusal": public_code,
                    "deeper_public_layers": "NOT_TESTED",
                }
            else:
                raise ApparatusStop("unrecognized own probe result")
            if check["returncode"] != 0:
                record["native"] = "NOT_RUN_AFTER_ORDINARY_CHECK_REFUSAL"
                raise CoverageStop("LAWFUL_ORDINARY_CHECK_COVERAGE_REFUSAL")
            if public_code in (10, 20):
                raise MeasurementStop("CLI_ACCEPTED_PUBLIC_UNIT_REFUSED")
            generated = directory / "pristine.c"
            emission = execute(
                record, directory, "emit_c17",
                [compiler, "--emit-c17-to", generated, source],
            )
            if emission["returncode"] != 0 or not generated.is_file():
                raise MeasurementStop("C17_EMISSION_AFTER_CHECK_FAILED")
            pristine = generated.read_bytes()
            record["pristine_c_sha256"] = digest(pristine)
            record["pristine_c_bytes"] = len(pristine)
            version = execute(record, directory, "native_compiler_version",
                              [cc, "--version"])
            if version["returncode"] != 0:
                raise ApparatusStop("native compiler version unavailable")
            binary = directory / "pristine"
            flags = shlex.split(os.environ.get("CFLAGS", ""))
            flags += ["-std=c17", "-pedantic-errors", "-Wall", "-Wextra",
                      "-Werror", "-O2" if build_type == "Release" else "-O0"]
            if sanitizer != "none":
                if sanitizer not in ("address", "undefined"):
                    raise ApparatusStop("unsupported sanitizer metadata")
                flags += [
                    "-fsanitize=" + sanitizer, "-fno-omit-frame-pointer",
                    "-fno-sanitize-recover=all",
                ]
            compiled = execute(
                record, directory, "native_compile",
                [cc, *flags, generated, "-o", binary], timeout=45,
            )
            if compiled["returncode"] != 0:
                raise MeasurementStop("PRISTINE_C17_COMPILE_FAILED")
            native = execute(record, directory, "native_run", [binary])
            if native["returncode"] != 0:
                raise MeasurementStop("PRISTINE_NATIVE_NONZERO")
            if generated.read_bytes() != pristine:
                raise MeasurementStop("PRISTINE_C_WAS_CHANGED")
            record["native"] = "PRISTINE_BUILD_AND_RUN_EXIT0"
            record["native_limit"] = (
                "unit exit0 does not observe any internal quantity or arm"
            )
            record["outcome"] = "BOUNDED_EXECUTION_NO_OBSERVED_CONTRADICTION"
            result = 0
        verify_originals()
    except CoverageStop as failure:
        record["outcome"] = "LAWFUL_COVERAGE_NOT_REACHED"
        record["stop"] = str(failure)
        record["classification_limit"] = (
            "capability outcome, not canonical invalidity or unsoundness"
        )
    except ApparatusStop as failure:
        record["outcome"] = "R_APPARATUS_OR_RESOURCE_STOP"
        record["stop"] = str(failure)
        result = 2
    except MeasurementStop as failure:
        record["outcome"] = "STOP_TO_COORDINATION"
        record["stop"] = str(failure)
    finally:
        publish(record, directory)
    return result


def main():
    if len(sys.argv) == 4 and sys.argv[1] == "inventory":
        inventory(sys.argv[2], sys.argv[3])
        return 0
    if len(sys.argv) != 9 or sys.argv[1] != "measure":
        raise ApparatusStop("invalid R1589 driver invocation")
    return measure(*sys.argv[2:])


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, KeyError, MeasurementStop) as failure:
        print("R1589_APPARATUS_ERROR " + str(failure), file=sys.stderr,
              flush=True)
        sys.exit(2)
