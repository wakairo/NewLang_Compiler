#!/usr/bin/env python3
"""R1595 V2 apparatus draft: text only, never executed during preparation."""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import signal
import subprocess
import sys
import time

FROZEN = {
    "c1306_p01_nested_arithmetic.nl": [354, "2493c6ad048a0f6113c24331c567da26d6c016eebde11f5fbe2854c6dee57331"],
    "c1306_p02_nested_branch_result.nl": [330, "a013ce04b600d52b9bc0ae12f36a0332606d7cc43e3566e66e9ed9a2ea8d2009"],
    "c1306_c01_explicit_intermediates.nl": [524, "9c3674fd577a7ced2a6193a90b7a8fb947e9f9cee39c0edaee104016b92ebe24"],
    "c1306_c02_unit_actual_invalid.nl": [188, "dfcdff735d33d5c1dd49fc86ece097bfa63b40f39bb8eef8f95643d3622fad41"],
    "c1306_c03_zero_divisor_invalid.nl": [198, "75aab90e0c43c2e86281f9ebe9b7971b8ee86ebbd65125969a0bfbe7d1f972a4"],
    "c1306_oracle_v1.md": [21612, "1bfa41e14f268e8166d3740c7ffbdb61ff9e365798736ef9124407329d66fec5"],
    "c1306_manifest_v1.txt": [829, "5caee8b4e889390eaa8615aff2e0c62aaf0b94be9d312b4bf8740f625a5815e2"],
}
CASES = [
    ("c02", "c1306_c02_unit_actual_invalid.nl", "invalid", {}, []),
    ("c03", "c1306_c03_zero_divisor_invalid.nl", "invalid", {}, []),
    ("c01", "c1306_c01_explicit_intermediates.nl", "lawful_control",
     {"first_argument": 36, "first": 3, "second_argument": 3, "second": 0,
      "repeat_argument": 36, "repeat": 3, "copied": 3,
      "copied_argument": 3, "copied_round": 0}, []),
    ("p01", "c1306_p01_nested_arithmetic.nl", "positive",
     {"first": 3, "second": 0, "repeat": 3, "copied": 3, "copied_round": 0},
     ["first", "second", "repeat", "copied_round"]),
    ("p02", "c1306_p02_nested_branch_result.nl", "positive",
     {"low": 2, "high": 1, "boundary": 1, "zero": 0},
     ["low", "high", "boundary", "zero"]),
]
ENTRY = b"main()\n"

class Stop(Exception):
    pass

def need(condition, message):
    if not condition:
        raise Stop(message)

def digest(data):
    return hashlib.sha256(data).hexdigest()

def write_json(path, value):
    path.write_text(json.dumps(value, ensure_ascii=True, indent=2) + "\n",
                    encoding="utf-8")

def verify_originals(root):
    for name, (size, sha) in FROZEN.items():
        data = (root / "original" / name).read_bytes()
        need(len(data) == size and digest(data) == sha,
             "ORIGINAL_INTEGRITY_STOP: " + name)
        need(b"\r" not in data and data.endswith(b"\n"),
             "ORIGINAL_LINE_ENDING_STOP: " + name)
        data.decode("utf-8")
    need((root / "entry_expression.nl").read_bytes() == ENTRY,
         "APPARATUS_ENTRY_INTEGRITY_STOP")

def authorize(root):
    release = json.loads((root / "release.json").read_text(encoding="utf-8"))
    need(release.get("execution_enabled") is True,
         "NOT_RELEASED: draft apparatus has execution budget zero")
    need(release.get("mode") == "github-dedicated-branch-actions-only",
         "EXECUTION_MODE_STOP")
    need(os.environ.get("GITHUB_ACTIONS") == "true",
         "LOCAL_EXECUTION_FORBIDDEN")
    target = release.get("target_sha")
    need(isinstance(target, str) and re.fullmatch(r"[0-9a-f]{40}", target),
         "IMMUTABLE_TARGET_NOT_RELEASED")
    branch = release.get("branch")
    need(isinstance(branch, str) and branch.startswith("r-fast/r1595-"),
         "OWN_BRANCH_NOT_RELEASED")
    actual = os.environ.get("GITHUB_HEAD_REF") or os.environ.get("GITHUB_REF", "")
    if actual.startswith("refs/heads/"):
        actual = actual[len("refs/heads/"):]
    need(actual == branch, "OWN_BRANCH_MISMATCH")
    need(release.get("cases") == [item[0] for item in CASES],
         "FINITE_CASE_SCOPE_NOT_RELEASED")
    need(release.get("max_children") == 19 and
         release.get("per_child_timeout_seconds") == 30,
         "CHILD_BUDGET_NOT_RELEASED")
    need(release.get("raw_log_route") == "actions-job-log-v2",
         "RAW_LOG_ROUTE_NOT_RELEASED")
    need(bool(release.get("logging_workflow_delta_release")) and
         bool(release.get("successful_raw_log_retrieval_release")),
         "WORKFLOW_DELTA_OR_SUCCESS_LOG_RETRIEVAL_NOT_RELEASED")
    # These fields record a separate C release, not self-issued permission.
    # C independently binds the R head/CI run to the released compiler target.
    return release

def failure_markers(stdout, stderr):
    # Detection can only force STOP. Absence never proves semantic rejection.
    data = stdout.lower() + b"\n" + stderr.lower()
    families = {
        "SANITIZER": (b"addresssanitizer", b"undefinedbehaviorsanitizer",
                      b"leaksanitizer", b"memorysanitizer", b"threadsanitizer",
                      b"runtime error:", b"asan:", b"ubsan:"),
        "RESOURCE": (b"out of memory", b"out_of_memory", b"out-of-memory",
                     b"resource limit", b"resource-limit",
                     b"resource_limit", b"allocation failed",
                     b"allocation failure", b"failed to allocate",
                     b"cannot allocate memory", b"memory exhausted",
                     b"resource exhaustion", b"std::bad_alloc"),
        "INTERNAL": (b"internal compiler error", b"internal error",
                     b"internal_error", b"assertion failed",
                     b"assertion failure"),
        "ABNORMAL": (b"segmentation fault", b"bus error", b"core dumped",
                     b"illegal instruction", b"stack overflow", b"fatal signal"),
    }
    return [name for name, markers in families.items()
            if any(marker in data for marker in markers)]

def child(command, folder, label, records):
    need(len(records) < 19, "CHILD_BUDGET_EXHAUSTED")
    stdout_path = folder / (label + ".stdout.bin")
    stderr_path = folder / (label + ".stderr.bin")
    status_path = folder / (label + ".status.json")
    started = time.monotonic()
    timeout = False
    process = None
    launch_error = None
    with stdout_path.open("wb") as stdout, stderr_path.open("wb") as stderr:
        try:
            process = subprocess.Popen(
                command, stdin=subprocess.DEVNULL, stdout=stdout, stderr=stderr,
                start_new_session=True)
            try:
                process.wait(timeout=30)
            except subprocess.TimeoutExpired:
                timeout = True
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
        except OSError as error:
            launch_error = str(error)
    out = stdout_path.read_bytes()
    err = stderr_path.read_bytes()
    code = None if process is None else process.returncode
    detected = failure_markers(out, err)
    termination = (
        "LAUNCH_ERROR" if launch_error is not None else
        "TIMEOUT" if timeout else
        "SIGNAL" if code is not None and code < 0 else
        "EXIT_ZERO" if code == 0 else "EXIT_NONZERO_UNCLASSIFIED"
    )
    record = {
        "schema": "r1595.child.v2", "case": folder.name,
        "label": label, "argv": command, "returncode": code,
        "termination": termination,
        "signal_number": -code if code is not None and code < 0 else None,
        "timed_out": timeout, "launch_error": launch_error,
        "failure_markers": detected,
        "elapsed_seconds": time.monotonic() - started,
        "stdout_path": str(stdout_path), "stdout_bytes": len(out),
        "stdout_sha256": digest(out),
        "stderr_path": str(stderr_path), "stderr_bytes": len(err),
        "stderr_sha256": digest(err),
    }
    write_json(status_path, record)
    records.append(record)
    # Raw files and actual status are preserved before any STOP/verdict.
    print(json.dumps({
        "event": "preserved_child", **record,
        "stdout_base64": base64.b64encode(out).decode("ascii"),
        "stderr_base64": base64.b64encode(err).decode("ascii")
    }), flush=True)
    need(not timeout and launch_error is None and code is not None,
         "APPARATUS_CHILD_FAILURE: " + label)
    need(code >= 0, "ABNORMAL_SIGNAL_STOP: " + label)
    need(code < 128, "ABNORMAL_OR_UNEXPLAINED_HIGH_EXIT_STOP: " + label)
    need(not detected, "SANITIZER_RESOURCE_INTERNAL_STOP: " + label +
         ":" + ",".join(detected))
    return code, out, err

def public_records(data):
    try:
        return [json.loads(line) for line in data.decode("utf-8").splitlines()
                if line]
    except (UnicodeError, json.JSONDecodeError) as error:
        raise Stop("PUBLIC_OBSERVATION_SERIALIZATION_STOP") from error

def status_map(records):
    statuses = {}
    for record in records:
        if record.get("event") != "status":
            continue
        stage = record.get("stage")
        need(isinstance(stage, str) and stage not in statuses,
             "AMBIGUOUS_PUBLIC_STATUS_STOP")
        statuses[stage] = record.get("status", record.get("number"))
    return statuses

def name_of(node):
    value = node.get("name_hex")
    if not isinstance(value, str):
        return None
    try:
        return bytes.fromhex(value).decode("ascii")
    except (ValueError, UnicodeError):
        return None

def assess_public(records, original, expected, nested_names):
    complete = [r for r in records if r.get("event") == "observation_complete"]
    need(len(complete) == 1 and complete[0].get("failed") is False,
         "PUBLIC_COLLECTION_INCOMPLETE")
    fragments = {r["id"]: r for r in records if r.get("event") == "fragment"}
    nodes = {(r["fragment"], r["id"]): r
             for r in records if r.get("event") == "node"}
    sources = {r["id"]: r for r in records if r.get("event") == "source"}
    root_fid = complete[0]["root_fragment"]
    root_fragment = fragments.get(root_fid)
    need(root_fragment is not None, "APPARATUS_ENTRY_FRAGMENT_ABSENT")
    root_node = nodes.get((root_fid, root_fragment["root"]))
    need(root_node is not None and root_node.get("kind") == "CALL" and
         name_of(root_node) == "main" and root_node.get("argument_count") == 0,
         "APPARATUS_ENTRY_RELATION_ABSENT")
    entries = [r for r in records if r.get("event") == "edge" and
               r.get("relation") == "call_body" and
               r.get("parent_fragment") == root_fid and
               r.get("parent_node") == root_fragment["root"]]
    need(len(entries) == 1 and entries[0]["child_fragment"] != 0,
         "MAIN_BODY_NOT_OBSERVABLE")
    main_fid = entries[0]["child_fragment"]
    main_fragment = fragments.get(main_fid)
    need(main_fragment is not None, "MAIN_BODY_FRAGMENT_ABSENT")
    retained = sources.get(main_fragment["source"], {}).get("bytes_hex")
    main_decls = [r for r in records if r.get("event") == "declaration" and
                  name_of(r) == "main"]
    correlated = False
    if isinstance(retained, str) and len(main_decls) == 1:
        start, end = main_decls[0]["body_span"]
        need(0 <= start <= end <= len(original), "DECLARATION_SPAN_INVALID")
        retained_bytes = bytes.fromhex(retained)
        correlated = retained_bytes in (original, original[start:end])
    observations = []
    for name, mathematical in expected.items():
        bindings = [r for r in nodes.values() if r["fragment"] == main_fid and
                    r.get("kind") == "BINDING" and name_of(r) == name]
        record = {"binding": name, "expected": mathematical,
                  "source_byte_correlated": correlated,
                  "quantity_public": "NOT_OBSERVED",
                  "nested_actual_relation": "NOT_OBSERVED"}
        if correlated and len(bindings) == 1:
            binding = bindings[0]
            initial = nodes.get((main_fid, binding["initializer"]))
            if initial is not None:
                values = initial.get("results", [])
                if len(values) == 1 and values[0].get("type_view_present") is True:
                    need(values[0].get("type_is_usize") is True and
                         values[0].get("type_copy") is True and
                         values[0].get("type_discardable") is True,
                         "PUBLIC_QUANTITY_TYPE_MISMATCH: " + name)
                if len(values) == 1 and values[0].get("view_present") is True:
                    need(values[0].get("types_match") is True,
                         "PUBLIC_RESULT_VALUE_TYPE_MISMATCH: " + name)
                if (len(values) == 1 and values[0].get("view_present") is True
                        and values[0].get("known") is True):
                    record["quantity_public"] = "OBSERVED_KNOWN"
                    record["observed"] = values[0]["number"]
                    need(values[0]["number"] == mathematical,
                         "KNOWN_SOURCE_QUANTITY_MISMATCH: " + name)
                if name in nested_names and initial.get("kind") == "CALL":
                    inner = nodes.get((main_fid, initial["first_argument"]))
                    if (initial.get("argument_count") == 1 and inner and
                            inner.get("kind") == "CALL"):
                        record["nested_actual_relation"] = "PUBLISHED_CHILD"
                        record["outer_node"] = initial["id"]
                        record["inner_node"] = inner["id"]
                        record["context"] = main_fragment["context"]
        observations.append(record)
    return {
        "public_scope": "PARTIAL",
        "main_source_byte_correlation": correlated,
        "bindings": observations,
        "arithmetic_events": [r for r in records
                              if r.get("event") == "arithmetic"],
        "complete_call_result_auth": "NOT_OBSERVABLE_IN_PERMITTED_HEADERS",
        "quantity_runtime": "NOT_OBSERVED",
        "order_runtime": "NOT_OBSERVED",
        "note": "Equal IDs/numbers and byte equality alone do not prove identity."
    }

def collect_public(args, root, source, original, folder, case,
                   classification, result, report):
    code, stdout, stderr = child(
        [args.observer, str(source), str(root / "entry_expression.nl")],
        folder, "public-observer", report["children"])
    need(source.read_bytes() == original, "ORIGINAL_SOURCE_CHANGED: " + case)
    records = public_records(stdout)
    write_json(folder / "public-records.json", records)
    statuses = status_map(records)
    result["public_statuses"] = statuses
    # Registration commits only after every original declaration is checked.
    # This check precedes exit/adapter/observation-complete classification.
    if classification == "invalid" and statuses.get("unit_register") == "OK":
        result["unexpected_original_admission"] = True
        result["native"] = "NOT_RUN_INVALID"
        write_json(folder / "case-report.json", result)
        raise Stop("TRUE_INVALID_UNIT_REGISTERED: " + case)
    need(code == 0, "PUBLIC_APPARATUS_OR_CONTRACT_STOP: " + case)
    result["public_diagnostic_stderr"] = {
        "bytes": len(stderr), "sha256": digest(stderr)}
    return records, statuses, stderr

def invalid_cli_outcome(code, stdout, stderr, public_diagnostic,
                        case, result, folder):
    result["cli_check_status"] = code
    result["native"] = "NOT_RUN_INVALID"
    if code == 0:
        result["cli_rejection"] = "UNEXPECTED_ACCEPTANCE"
        write_json(folder / "case-report.json", result)
        raise Stop("TRUE_INVALID_ACCEPTED_BY_CLI: " + case)
    # README.md 391-402 at fixed main: source I/O=1, options=2,
    # parser/semantic refusal=3 retaining diagnostics. This is not a contract
    # that every nonzero code is a source rejection, nor a named-law proof.
    if code != 3:
        result["cli_rejection"] = {
            1: "SOURCE_IO_FAILURE",
            2: "OPTIONS_OR_ARGUMENT_FAILURE",
        }.get(code, "UNCLASSIFIED_EXIT_STATUS")
        write_json(folder / "case-report.json", result)
        raise Stop("CLI_NON_SEMANTIC_OR_UNCLASSIFIED_STOP: " + case)
    # R's conservative consistency gate is stricter than README formatting:
    # require exactly the public renderer's same-original registration error,
    # with no extra stdout. Difference or absence is INCONCLUSIVE, never a
    # new source-law prohibition or a guessed private diagnostic spelling.
    if not public_diagnostic or stdout or stderr != public_diagnostic:
        result["cli_rejection"] = "INCONCLUSIVE_DIAGNOSTIC_ABSENT_OR_DIFFERENT"
        write_json(folder / "case-report.json", result)
        raise Stop("INCONCLUSIVE_CLI_DIAGNOSTIC_CONSISTENCY: " + case)
    result["cli_rejection"] = (
        "EXIT3_WITH_MATCHING_PUBLIC_REGISTRATION_DIAGNOSTIC")
    result["diagnostic_consistency"] = {
        "stderr_bytes": len(stderr), "stderr_sha256": digest(stderr),
        "stdout_bytes": len(stdout), "comparison": "EXACT_BYTES"}
    result["specific_source_law_proof_from_exit_status"] = False
    write_json(folder / "case-report.json", result)

def run(args, output, report):
    root = Path(args.root).resolve()
    verify_originals(root)
    release = authorize(root)
    report["release"] = release
    report["github_metadata"] = {
        name: os.environ.get(name) for name in
        ("GITHUB_REPOSITORY", "GITHUB_SHA", "GITHUB_REF", "GITHUB_HEAD_REF",
         "GITHUB_RUN_ID", "GITHUB_RUN_ATTEMPT", "GITHUB_JOB")
    }
    need(args.sanitizer in ("none", "address", "undefined"),
         "UNKNOWN_SANITIZER_MODE")
    for case, filename, classification, expected, nested_names in CASES:
        folder = output / case
        folder.mkdir()
        source = root / "original" / filename
        original = source.read_bytes()
        result = {"case": case, "classification": classification}
        report["cases"].append(result)
        if classification == "invalid":
            # Observe original registration before the separate CLI outcome;
            # no later adapter refusal can excuse unexpected unit admission.
            records, statuses, public_diagnostic = collect_public(
                args, root, source, original, folder, case,
                classification, result, report)
            need(not any(r.get("event") == "observation_complete"
                         for r in records),
                 "TRUE_INVALID_CHECKED_PUBLICLY: " + case)
            need(statuses.get("unit_parse") == "OK" and
                 statuses.get("semantic_create") == "OK" and
                 statuses.get("unit_register") == "SEMANTIC_ERROR",
                 "INCONCLUSIVE_PUBLIC_REJECTION_BOUNDARY: " + case)
            result["public_source_rejection"] = "SEMANTIC_ERROR_OBSERVED"
            result["native"] = "NOT_RUN_INVALID"
            write_json(folder / "case-report.json", result)
            code, stdout, stderr = child(
                [args.newlangc, "--check", str(source)], folder, "cli-check",
                report["children"])
            need(source.read_bytes() == original,
                 "ORIGINAL_SOURCE_CHANGED: " + case)
            invalid_cli_outcome(code, stdout, stderr, public_diagnostic,
                                case, result, folder)
            continue
        code, stdout, stderr = child(
            [args.newlangc, "--check", str(source)], folder, "cli-check",
            report["children"])
        result["cli_check_status"] = code
        need(source.read_bytes() == original, "ORIGINAL_SOURCE_CHANGED: " + case)
        need(code == 0, "LEGAL_SOURCE_CHECK_BOUNDARY_STOP: " + case)
        need(not stdout and not stderr,
             "CLI_SUCCESS_OUTPUT_CONTRACT_STOP: " + case)
        records, statuses, _ = collect_public(
            args, root, source, original, folder, case,
            classification, result, report)
        need(statuses.get("unit_register") == "OK" and
             statuses.get("apparatus_entry_check") == "OK",
             "PUBLIC_REGISTER_OR_ENTRY_BOUNDARY_STOP: " + case)
        result["public"] = assess_public(records, original, expected, nested_names)
        emitted = folder / "original-emitted.c"
        code, _, _ = child(
            [args.newlangc, "--emit-c17-to", str(emitted), str(source)],
            folder, "emit-c17", report["children"])
        need(code == 0 and emitted.is_file(), "C17_EMISSION_STOP: " + case)
        need(source.read_bytes() == original, "ORIGINAL_SOURCE_CHANGED: " + case)
        pristine = emitted.read_bytes()
        result["emitted_c17"] = {"bytes": len(pristine),
                                "sha256": digest(pristine)}
        executable = folder / "original-native"
        flags = shlex.split(args.host_cflags)
        flags += ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                  "-O2" if args.build_type == "Release" else "-O0"]
        if args.sanitizer != "none":
            flags += ["-fsanitize=" + args.sanitizer, "-fno-omit-frame-pointer",
                      "-fno-sanitize-recover=all"]
        code, _, _ = child(
            [args.cc, *flags, str(emitted), "-o", str(executable)],
            folder, "pristine-c17-compile", report["children"])
        need(emitted.read_bytes() == pristine, "GENERATED_C_CHANGED: " + case)
        need(code == 0, "STRICT_C17_COMPILE_STOP: " + case)
        code, _, _ = child([str(executable)], folder, "pristine-native",
                            report["children"])
        need(emitted.read_bytes() == pristine, "GENERATED_C_CHANGED: " + case)
        need(code == 0, "PRISTINE_NATIVE_EXIT_STOP: " + case)
        need(source.read_bytes() == original, "ORIGINAL_SOURCE_CHANGED: " + case)
        result["native"] = "PRISTINE_UNIT_EXIT_ZERO"
        result["quantity_runtime"] = "NOT_OBSERVED"
        write_json(folder / "case-report.json", result)
    verify_originals(root)
    report["verdict"] = "BOUNDED_COLLECTION_COMPLETE_WITH_EXPLICIT_LIMITS"
    report["quantity_runtime"] = "NOT_OBSERVED"
    report["complete_call_result_auth"] = "NOT_OBSERVABLE_IN_PERMITTED_HEADERS"
    report["product_go"] = "PAUSED"
    report["raw_remote_logs"] = "RETRIEVAL_MUST_BE_VERIFIED_SEPARATELY"

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--newlangc", required=True)
    parser.add_argument("--observer", required=True)
    parser.add_argument("--cc", required=True)
    parser.add_argument("--host-cflags", default="")
    parser.add_argument("--build-type", default="")
    parser.add_argument("--sanitizer", required=True)
    parser.add_argument("--root", required=True)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()
    output = Path(args.out).resolve()
    report = {"phase": "R1595_V2", "children": [], "cases": [],
              "verdict": "NOT_STARTED"}
    exit_status = 3
    output_created = False
    try:
        output.mkdir(parents=True, exist_ok=False)
        output_created = True
        run(args, output, report)
        exit_status = 0
    except Stop as error:
        report["verdict"] = "STOP_C"
        report["reason"] = str(error)
        exit_status = 2
    except Exception as error:
        report["verdict"] = "APPARATUS_FAILURE_STOP"
        report["reason"] = repr(error)
    finally:
        report["exit_status"] = exit_status
        if output_created:
            write_json(output / "pipeline-report.json", report)
        print(json.dumps({"event": "parent_verdict", **report}), flush=True)
    return exit_status

if __name__ == "__main__":
    sys.exit(main())
