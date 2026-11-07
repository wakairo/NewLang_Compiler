#!/usr/bin/env python3
"""Independent R1213 V1 CLI attack; no producer fixtures or privileged C API."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from datetime import datetime, timezone


HERE = Path(__file__).resolve().parent
COHORT = json.loads((HERE / "runner_cohort.json").read_text(encoding="utf-8"))
EXPECTED_BASE = "491f54adfa64ae7308344923ed14a9fcad08a8e9"


def utc():
    return datetime.now(timezone.utc).isoformat()


def attempt(argv, directory, seconds):
    entry = {"argv": list(map(str, argv)), "start_utc": utc()}
    try:
        result = subprocess.run(
            list(map(str, argv)), cwd=directory, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, text=True, errors="replace",
            timeout=seconds, check=False,
        )
        entry.update(
            returncode=result.returncode,
            stdout=result.stdout,
            stderr=result.stderr,
            timed_out=False,
        )
    except subprocess.TimeoutExpired as exc:
        entry.update(
            returncode=None, stdout=repr(exc.stdout), stderr=repr(exc.stderr),
            timed_out=True,
        )
    entry["end_utc"] = utc()
    return entry


def classify(case, compiler, cc, sanitizer, work, record):
    record.update({
        "case_id": case["id"], "source_class": case["class"],
        "predicted_oracle_not_measured": case.get("predicted"),
        "negative_intent_not_proven": case.get("intent"),
        "authority": COHORT["canonical"], "fixed_parent": EXPECTED_BASE,
        "start_utc": utc(), "stages": [],
        "public_owned_proof": "NOT_TESTED",
        "original_T_root_or_field_provenance": "NOT_TESTED",
        "numeric_old_Copy_and_sibling_runtime_oracle": "NOT_TESTED",
    })
    def fatal(message):
        record["classification"] = "STOP"
        record["stop_reason"] = message
        raise RuntimeError(message)

    source = HERE / "v1" / case["name"]
    raw = source.read_bytes()
    actual = hashlib.sha256(raw).hexdigest()
    record["source"] = str(source)
    record["actual_source_sha256"] = actual
    if (len(raw) != case["size"] or actual != case["sha256"]
            or not raw.endswith(b"\n") or raw.endswith(b"\n\n")):
        fatal("FROZEN_V1_SOURCE_ORACLE_IDENTITY_DRIFT")

    check = attempt([compiler, "--check", source], work, 60)
    record["stages"].append({"stage": "production_cli_source_and_check", **check})
    if check["timed_out"]:
        fatal("CHECK_TIMEOUT_OR_HANG")
    if check["stdout"]:
        fatal("CHECK_UNEXPECTED_STDOUT")

    if case["class"] == "negative":
        if check["returncode"] == 0:
            fatal("UNEXPECTED_FALSE_ACCEPTANCE_NEGATIVE: " + case["intent"])
        if check["returncode"] != 3:
            fatal("NEGATIVE_INFRA_OR_SIGNAL: returncode=" + str(check["returncode"]))
        record["classification"] = "NEGATIVE_REJECTED_FIRST_LAYER_UNDETERMINED"
        record["intended_negative_semantic_layer"] = "NOT_CERTIFIED"
        record["c17_native"] = "NOT_REACHED"
        return record

    if check["returncode"] == 3:
        record["classification"] = "VALID_SOURCE_PREDICTED_BUT_COMPILER_UNSUPPORTED"
        record["actual_first_layer"] = "PARSER_OR_CHECK_DIAGNOSTIC_UNCLASSIFIED"
        record["c17_native"] = "NOT_REACHED"
        return record
    if check["returncode"] != 0 or check["stderr"]:
        fatal("POSITIVE_CHECK_INFRA_OR_DIAGNOSTIC_ANOMALY")
    record["check"] = "ACCEPTED_NOT_PUBLIC_OWNED_PROOF"

    cfile = work / "generated.c"
    emission = attempt([compiler, "--emit-c17-to", cfile, source], work, 60)
    record["stages"].append({"stage": "production_cli_strict_c17_emit", **emission})
    if emission["timed_out"]:
        fatal("EMIT_TIMEOUT")
    if emission["returncode"] == 3:
        if cfile.exists():
            fatal("EMIT_REFUSAL_WITH_PARTIAL_C17_FILE")
        record["classification"] = "CHECK_ACCEPTED_EMITTER_REFUSAL"
        record["emitter_diagnostic"] = "UNSUPPORTED_OR_UNCLASSIFIED"
        record["c17_native"] = "NOT_REACHED"
        return record
    if emission["returncode"] != 0 or emission["stderr"] or emission["stdout"]:
        fatal("EMIT_FAILURE_OR_UNEXPECTED_DIAGNOSTIC")
    if not cfile.is_file() or cfile.stat().st_size == 0:
        fatal("EMITTER_ACCEPTED_BUT_NO_C17")

    record["c17_sha256"] = hashlib.sha256(cfile.read_bytes()).hexdigest()
    record["c17_byte_length"] = cfile.stat().st_size
    exe = work / "guest-executable"
    cmd = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O0"]
    if sanitizer == "address":
        cmd += ["-fsanitize=address", "-fno-omit-frame-pointer"]
    elif sanitizer == "undefined":
        cmd += ["-fsanitize=undefined", "-fno-sanitize-recover=all"]
    elif sanitizer not in ("none", ""):
        fatal("UNKNOWN_SANITIZER_SETTING:" + sanitizer)
    cmd += [str(cfile), "-o", str(exe)]
    native_build = attempt(cmd, work, 90)
    record["stages"].append({"stage": "host_strict_c17_build", **native_build})
    if native_build["timed_out"] or native_build["returncode"] != 0:
        fatal("GENERATED_C17_HOST_COMPILATION_FAILED")
    if not exe.is_file():
        fatal("HOST_BUILD_SUCCESS_WITHOUT_EXECUTABLE")

    guest = attempt([exe], work, 25)
    record["stages"].append({"stage": "native_guest_execution", **guest})
    if guest["timed_out"] or guest["returncode"] != 0:
        fatal("NATIVE_NONZERO_OR_TIMEOUT")
    if guest["stderr"] or guest["stdout"]:
        fatal("NATIVE_OUTPUT_OR_SANITIZER_ANOMALY")
    record["classification"] = "CHECK_EMIT_C17_NATIVE_EXIT0_ONLY"
    record["numeric_old_Copy_and_sibling_runtime_oracle"] = "NOT_OBSERVABLE_IN_V1_MAIN_UNIT"
    record["public_owned_proof"] = "NOT_TESTED"
    record["original_T_root_or_field_provenance"] = "NOT_TESTED"
    return record


def main():
    if len(sys.argv) != 6:
        sys.exit("usage: run_case.py NEWLANGC CC SANITIZER CASE WORK_ROOT")
    compiler, cc, sanitizer, case_id, work_root = sys.argv[1:]
    if COHORT["fixed_compiler_parent"] != EXPECTED_BASE:
        sys.exit("STOP fixed base drift in cohort manifest")
    candidates = [case for case in COHORT["cases"] if case["id"] == case_id]
    if len(candidates) != 1:
        sys.exit("STOP case identity mismatch")
    Path(work_root).mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="r1213-" + case_id + "-",
                                     dir=work_root) as temporary:
        work = Path(temporary)
        report = {"case_id": case_id, "start_utc": utc(), "stages": []}
        try:
            classify(candidates[0], compiler, cc, sanitizer, work, report)
            failure = False
        except Exception as exc:
            report.update({
                "classification": "STOP",
                "error_type": type(exc).__name__,
                "error": str(exc),
            })
            failure = True
        # All observed stages and stdout/stderr are printed for CTest failure
        # logs. CTest normally hides successful test stdout; no claim of an
        # archived success artifact without separate GitHub evidence.
        print("R1213_C272_RESULT_JSON=" + json.dumps(report, ensure_ascii=True, sort_keys=True),
              flush=True)
        if failure:
            sys.exit(1)


if __name__ == "__main__":
    main()
