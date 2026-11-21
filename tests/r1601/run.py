"""R1601 independent finite runner V2. TEXT DRAFT; execution needs C release.

This runner may execute only in GitHub Actions. It records complete successful
child streams too. Ordinary source refusal is evidence, never a blanket PASS.
No assertion statement is used for error handling.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import selectors
import signal
import subprocess
import sys
import time

FIRST = (
    ("p1", "r1601_p1_direct_add.nl", False,
     "37a8fffa5b012976280b9ac8e3d1d0a6c5fa36f4b1c4a4693c8d631ec7cc11a1",
     {"result": 9}),
    ("p2", "r1601_p2_acyclic_product.nl", False,
     "d46b7b8547f8c09cf0ba2fc72ecd2fd9051b663e91c3c5c275b2a6763e30830e",
     {"result": 0}),
    ("c1", "r1601_c1_zero_divisor.nl", True,
     "f12cb4a25aefa5013bd712e46924dee080b7e596e360afe30238dc0300f79370",
     {}),
    ("c2", "r1601_c2_underflow.nl", True,
     "8ad0be326d05c40e2a2776a38677e673914ccd82113b06c3bb8595276148c5c2",
     {}),
    ("c3", "r1601_c3_named_copy.nl", False,
     "a625bb17bf899cdf8f0d38fdcff318bb646e9ba0381fb87dbe0afd44aa8c1a27",
     {"quantity": 37, "first": 9, "second": 9}),
    ("c4", "r1601_c4_symbolic_identity.nl", False,
     "7b10e78e2fc655655e8999898d9e71906199c966263e29be8e78038c0f7ac3d0",
     {"zero_result": 0, "nonzero_result": 7}),
)
FIRST_MANIFEST_SHA = (
    "e74cb47ae78d19b1ea30849293d1c79c78326e711228e5d9518a44050644b97d"
)
ENV_KEYS = (
    "GITHUB_REPOSITORY", "GITHUB_RUN_ID", "GITHUB_RUN_ATTEMPT",
    "GITHUB_JOB", "GITHUB_SHA", "GITHUB_EVENT_NAME", "GITHUB_REF",
)
MAX_COMMANDS = 25
TOTAL_CAPTURE = 512 * 1024
TOTAL_SECONDS = 330
SANITIZER_WORDS = (
    b"AddressSanitizer", b"UndefinedBehaviorSanitizer",
    b"LeakSanitizer", b"runtime error:",
)


class Stop(Exception):
    pass


def digest(data):
    return hashlib.sha256(data).hexdigest()


def dump(path, value):
    data = (json.dumps(value, sort_keys=True, ensure_ascii=True,
                       separators=(",", ":")) + "\n").encode("ascii")
    with path.open("xb") as stream:
        stream.write(data)
        stream.flush()
        os.fsync(stream.fileno())


def regular(path):
    if path.is_symlink() or not path.is_file():
        raise Stop("nonregular or symlink data path: " + path.name)
    return path.read_bytes()


class Session:
    def __init__(self, raw, work):
        self.raw = raw
        self.work = work
        self.started = time.monotonic()
        self.captured = 0
        self.commands = []
        self.results = []

    def command(self, label, argv, seconds, stdout_limit=32768,
                stderr_limit=32768):
        if len(self.commands) >= MAX_COMMANDS:
            raise Stop("R_COMMAND_BUDGET")
        remaining_time = TOTAL_SECONDS - (time.monotonic() - self.started)
        if remaining_time < 1:
            raise Stop("R_TIME_BUDGET")
        seconds = min(seconds, remaining_time)
        index = len(self.commands)
        prefix = f"{index:02d}-{label}"
        if not re.fullmatch(r"[0-9]{2}-[a-z0-9-]+", prefix):
            raise Stop("unsafe command label")
        argv = [str(part) for part in argv]
        start = {
            "index": index, "label": label, "argv": argv, "shell": False,
            "cwd": str(self.work), "timeout_seconds": seconds,
            "stdout_limit": stdout_limit, "stderr_limit": stderr_limit,
        }
        dump(self.raw / (prefix + ".start.json"), start)
        out_path = self.raw / (prefix + ".stdout.bin")
        err_path = self.raw / (prefix + ".stderr.bin")
        captured = {"stdout": bytearray(), "stderr": bytearray()}
        limits = {"stdout": stdout_limit, "stderr": stderr_limit}
        truncated = {"stdout": False, "stderr": False}
        discarded = {"stdout": 0, "stderr": 0}
        eof = {"stdout": False, "stderr": False}
        reason = None
        launch_error = None
        returncode = None
        process = None
        selector = selectors.DefaultSelector()
        began = time.monotonic()
        killed_at = None
        output_streams = {
            "stdout": out_path.open("xb"), "stderr": err_path.open("xb"),
        }

        def kill_group():
            nonlocal killed_at
            if process is not None and killed_at is None:
                killed_at = time.monotonic()
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass

        try:
            process = subprocess.Popen(
                argv, cwd=self.work, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, start_new_session=True,
            )
            selector.register(process.stdout, selectors.EVENT_READ, "stdout")
            selector.register(process.stderr, selectors.EVENT_READ, "stderr")
            while selector.get_map():
                now = time.monotonic()
                if killed_at is None and now - began > seconds:
                    reason = "TIMEOUT"
                    kill_group()
                if killed_at is not None and now - killed_at > 2:
                    reason = reason or "DRAIN_TIMEOUT"
                    break
                for key, unused_events in selector.select(timeout=0.1):
                    del unused_events
                    name = key.data
                    chunk = os.read(key.fileobj.fileno(), 4096)
                    if not chunk:
                        eof[name] = True
                        selector.unregister(key.fileobj)
                        key.fileobj.close()
                        continue
                    allowance = min(limits[name] - len(captured[name]),
                                    TOTAL_CAPTURE - self.captured)
                    allowance = max(0, allowance)
                    kept = chunk[:allowance]
                    captured[name].extend(kept)
                    output_streams[name].write(kept)
                    output_streams[name].flush()
                    self.captured += len(kept)
                    if len(kept) != len(chunk):
                        truncated[name] = True
                        discarded[name] += len(chunk) - len(kept)
                        reason = reason or "OUTPUT_LIMIT"
                        kill_group()
            if process.poll() is None:
                if killed_at is None:
                    remaining = max(0.1, seconds - (time.monotonic() - began))
                    try:
                        returncode = process.wait(timeout=remaining)
                    except subprocess.TimeoutExpired:
                        reason = reason or "TIMEOUT"
                        kill_group()
                if process.poll() is None:
                    try:
                        returncode = process.wait(timeout=2)
                    except subprocess.TimeoutExpired:
                        reason = reason or "KILL_WAIT_TIMEOUT"
                else:
                    returncode = process.returncode
            else:
                returncode = process.returncode
        except OSError as error:
            launch_error = type(error).__name__ + ": " + str(error)
            reason = reason or "HOST_IO_OR_LAUNCH"
            kill_group()
            if process is not None:
                try:
                    returncode = process.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    reason = "KILL_WAIT_TIMEOUT"
        finally:
            for key in list(selector.get_map().values()):
                key.fileobj.close()
            selector.close()
            for stream in output_streams.values():
                stream.flush()
                os.fsync(stream.fileno())
                stream.close()

        stdout = bytes(captured["stdout"])
        stderr = bytes(captured["stderr"])
        result = {
            **start, "returncode": returncode, "reason": reason,
            "launch_error": launch_error,
            "elapsed_seconds": round(time.monotonic() - began, 6),
            "sanitizer_markers": [
                word.decode("ascii") for word in SANITIZER_WORDS
                if word in stderr or
                (label.endswith(("-check", "-observe")) and word in stdout)
            ],
            "streams": {
                name: {
                    "file": prefix + "." + name + ".bin",
                    "bytes": len(captured[name]),
                    "sha256": digest(captured[name]),
                    "eof": eof[name],
                    "truncated": truncated[name],
                    "discarded_observed_bytes": discarded[name],
                    "complete": eof[name] and not truncated[name],
                }
                for name in ("stdout", "stderr")
            },
        }
        # Status and both raw streams are durable before any classification.
        dump(self.raw / (prefix + ".result.json"), result)
        self.commands.append(result)
        if reason or launch_error or returncode is None:
            raise Stop("HOST_OR_R_RESOURCE: " + prefix)
        if result["sanitizer_markers"]:
            raise Stop("SANITIZER_EVIDENCE: " + prefix)
        if returncode < 0:
            raise Stop("SIGNAL: " + prefix + ": " + str(-returncode))
        if not all(result["streams"][name]["complete"]
                   for name in ("stdout", "stderr")):
            raise Stop("INCOMPLETE_CAPTURE: " + prefix)
        return returncode, stdout, stderr, out_path, result


def decode_name(row):
    value = row.get("name_hex")
    if value is None:
        return None
    try:
        return bytes.fromhex(value).decode("ascii")
    except (TypeError, ValueError, UnicodeError):
        raise Stop("observer emitted invalid name encoding")


def observer_rows(data, returncode):
    try:
        rows = [json.loads(line) for line in data.decode("ascii").splitlines()]
    except (UnicodeError, ValueError) as error:
        raise Stop("R_OBSERVER_PROTOCOL: " + str(error))
    if (not data.endswith(b"\n") or not rows or
            any(not isinstance(row, dict) for row in rows) or
            rows[-1] != {"record": "observer_end", "exit": returncode} or
            sum(row.get("record") == "observer_end" for row in rows) != 1):
        raise Stop("OBSERVER_INCOMPLETE_PROTOCOL_OR_TARGET_EXIT: no completion")
    return rows


PARSE_REFUSALS = {"LEXICALLY_UNSUPPORTED", "SYNTAX_UNSUPPORTED", "SYNTAX_ERROR"}
CHECK_REFUSALS = {
    "SEMANTIC_ERROR", "SEMANTIC_UNSUPPORTED", "ANALYSIS_PRECISION_LIMIT",
}
PHASE_ORDER = (
    "original_source_load", "unit_parser_create", "original_unit_parse",
    "context_create", "original_unit_register", "synthetic_entry_create",
    "entry_parser_create", "synthetic_entry_parse", "synthetic_entry_check",
)


def public_protocol(rows, returncode, original, invalid):
    configs = [row for row in rows if row.get("record") == "observer_config"]
    if configs != [{
        "record": "observer_config", "revision": "V2",
        "source_law": "INVALID" if invalid else "LAWFUL",
    }]:
        raise Stop("R_OBSERVER_CONFIG_MISMATCH")
    originals = [row for row in rows if row.get("record") == "source" and
                 row.get("role") == "original_unit"]
    if len(originals) != 1:
        raise Stop("R_OBSERVER_ORIGINAL_SOURCE_MISSING")
    source = originals[0]
    if (source.get("frame") != 0 or source.get("available") is not True or
            source.get("length") != len(original) or
            hex_field(source, "hex", False) != original):
        raise Stop("ORIGINAL_SOURCE_BYTES_CHANGED_OR_UNAVAILABLE")
    phase_rows = [row for row in rows if row.get("record") == "phase"]
    names = [row.get("phase") for row in phase_rows]
    if len(set(names)) != len(names) or tuple(names) != PHASE_ORDER[:len(names)]:
        raise Stop("R_OBSERVER_PHASE_ORDER_OR_DUPLICATE")
    phases = {row["phase"]: row for row in phase_rows}
    for row in phase_rows:
        if row.get("status") in {
            "OUT_OF_MEMORY", "RESOURCE_LIMIT", "INTERNAL_ERROR", "UNCLASSIFIED",
        }:
            raise Stop("PUBLIC_RESOURCE_OR_INTERNAL: " + row["phase"])
        if "ok" in row and row["ok"] is not True:
            raise Stop("PUBLIC_SOURCE_OR_APPARATUS_FAILURE: " + row["phase"])
    expected_lengths = {0: 9, 10: 3, 11: 5, 12: 9}
    if (returncode not in expected_lengths or
            len(names) != expected_lengths[returncode]):
        raise Stop("R_OBSERVER_EXIT_PHASE_MISMATCH")
    for index, row in enumerate(phase_rows):
        name = row["phase"]
        if name in {"original_source_load", "synthetic_entry_create"}:
            if row.get("ok") is not True:
                raise Stop("PUBLIC_SOURCE_PHASE_NOT_OK: " + name)
            continue
        is_refusal = returncode != 0 and index == len(phase_rows) - 1
        allowed = (PARSE_REFUSALS if returncode == 10 else CHECK_REFUSALS)
        if not is_refusal:
            allowed = {"OK"}
        if row.get("status") not in allowed:
            raise Stop("PUBLIC_PHASE_STATUS_NOT_CLASSIFIED: " + name)
    diagnostic_rows = [row for row in rows if row.get("record") in {
        "diagnostic", "diagnostic_render",
    }]
    ends = [row for row in rows if row.get("record") == "observation_end"]
    if returncode == 0:
        if diagnostic_rows or len(ends) != 1:
            raise Stop("PUBLIC_SUCCESS_COMPLETION_OR_DIAGNOSTIC_MISMATCH")
    elif ends:
        raise Stop("PUBLIC_REFUSAL_WITH_UNEXPECTED_COMPLETED_OBSERVATION")
    return phases


def hex_field(row, field, nullable):
    if field not in row:
        raise Stop("R_DIAGNOSTIC_FIELD_MISSING: " + field)
    value = row[field]
    if value is None and nullable:
        return None
    if (not isinstance(value, str) or len(value) % 2 or
            not re.fullmatch(r"[0-9a-f]*", value)):
        raise Stop("R_DIAGNOSTIC_HEX_INVALID: " + field)
    return bytes.fromhex(value)


def source_position(source, offset):
    # FIRST is exact LF-only ASCII. Offsets are validated before this call.
    prefix = source[:offset]
    return [prefix.count(b"\n") + 1, offset - prefix.rfind(b"\n")]


def diagnostic_range_fields(value):
    if value is None:
        return
    if not isinstance(value, dict):
        raise Stop("R_DIAGNOSTIC_RANGE_INVALID")
    hex_field(value, "path_hex", True)
    coords = [value.get(key) for key in (
        "start_line", "start_column", "end_line", "end_column",
    )]
    if any(type(item) is not int or item < 1 for item in coords):
        raise Stop("R_DIAGNOSTIC_RANGE_COORDINATES_INVALID")
    if tuple(coords[2:]) < tuple(coords[:2]):
        raise Stop("R_DIAGNOSTIC_RANGE_REVERSED")


def bind_original_diagnostic(rows, phases, original, source_path,
                             observer_code, observer_stderr,
                             cli_code, cli_stdout, cli_stderr):
    if observer_code not in {10, 11}:
        raise Stop("INCONCLUSIVE_NO_ORIGINAL_PUBLIC_REFUSAL")
    phase = ("original_unit_parse" if observer_code == 10
             else "original_unit_register")
    diagnostics = [row for row in rows if row.get("record") == "diagnostic"]
    renders = [row for row in rows if row.get("record") == "diagnostic_render"]
    if len(diagnostics) != 1 or len(renders) != 1:
        raise Stop("INCONCLUSIVE_DIAGNOSTIC_COUNT_OR_RENDER_RECEIPT")
    diagnostic, render = diagnostics[0], renders[0]
    for row in (diagnostic, render):
        if (row.get("phase") != phase or
                row.get("source_role") != "original_unit" or
                type(row.get("input_index")) is not int or
                row["input_index"] != 0):
            raise Stop("INCONCLUSIVE_DIAGNOSTIC_NOT_SAME_ORIGINAL_PHASE")
    if observer_code == 11 and phases[phase].get("input_index") != 0:
        raise Stop("INCONCLUSIVE_DIAGNOSTIC_INPUT_INDEX")
    if (diagnostic.get("complete") is not True or
            diagnostic.get("severity") != "ERROR" or
            render.get("attempted") is not True or render.get("ok") is not True):
        raise Stop("INCONCLUSIVE_DIAGNOSTIC_OR_RENDER_UNHEALTHY")
    source_name = os.fsencode(str(source_path))
    if hex_field(diagnostic, "source_name_hex", False) != source_name:
        raise Stop("INCONCLUSIVE_DIAGNOSTIC_SOURCE_NAME_MISMATCH")
    # Optional actual category/code are preserved; no expected spelling or
    # law-specific enum is introduced by the apparatus.
    hex_field(diagnostic, "category_hex", True)
    hex_field(diagnostic, "code_hex", True)
    if not hex_field(diagnostic, "message_hex", False):
        raise Stop("INCONCLUSIVE_DIAGNOSTIC_MESSAGE_ABSENT")
    span = diagnostic.get("span")
    if (not isinstance(span, list) or len(span) != 2 or
            any(type(item) is not int for item in span) or
            not 0 <= span[0] <= span[1] <= len(original) or
            diagnostic.get("span_valid") is not True):
        raise Stop("INCONCLUSIVE_DIAGNOSTIC_SPAN_INVALID")
    if hex_field(diagnostic, "span_hex", False) != original[span[0]:span[1]]:
        raise Stop("INCONCLUSIVE_DIAGNOSTIC_SPAN_BYTES_MISMATCH")
    if "range" not in diagnostic:
        raise Stop("INCONCLUSIVE_DIAGNOSTIC_RANGE_FIELD_MISSING")
    primary_range = diagnostic["range"]
    diagnostic_range_fields(primary_range)
    if primary_range is not None:
        actual_coords = [primary_range[key] for key in (
            "start_line", "start_column", "end_line", "end_column",
        )]
        expected_coords = (source_position(original, span[0]) +
                           source_position(original, span[1]))
        if (hex_field(primary_range, "path_hex", False) != source_name or
                actual_coords != expected_coords):
            raise Stop("INCONCLUSIVE_DIAGNOSTIC_RANGE_NOT_ORIGINAL_SPAN")
    notes = diagnostic.get("notes")
    count = diagnostic.get("note_count")
    if (type(count) is not int or not 0 <= count <= 16 or
            not isinstance(notes, list) or len(notes) != count or
            type(diagnostic.get("notes_present")) is not bool or
            (count != 0 and diagnostic["notes_present"] is not True)):
        raise Stop("INCONCLUSIVE_DIAGNOSTIC_NOTES_INCOMPLETE")
    for note in notes:
        if not isinstance(note, dict) or "range" not in note:
            raise Stop("INCONCLUSIVE_DIAGNOSTIC_NOTE_INVALID")
        hex_field(note, "message_hex", False)
        diagnostic_range_fields(note["range"])
    if cli_code != 3 or not observer_stderr:
        raise Stop("INCONCLUSIVE_CLI_OR_PUBLIC_DIAGNOSTIC_MISSING")
    if not cli_stdout and cli_stderr == observer_stderr:
        channel = "stderr"
    elif not cli_stderr and cli_stdout == observer_stderr:
        channel = "stdout"
    else:
        raise Stop("INCONCLUSIVE_CLI_PUBLIC_DIAGNOSTIC_BYTES_MISMATCH")
    # Exact one-stream equality leaves no extra/generated CLI bytes. This is
    # comparison to the renderer's actual output, not to producer fixtures.
    return {
        "status": "EXACT_ORIGINAL_PUBLIC_RENDERER_CLI_MATCH",
        "original_phase": phase, "public_status": phases[phase]["status"],
        "source_sha256": digest(original), "diagnostic": diagnostic,
        "render_receipt": render, "cli_diagnostic_channel": channel,
        "rendered_bytes": len(observer_stderr),
        "rendered_sha256": digest(observer_stderr),
        "other_cli_stream_bytes": 0, "extra_generated_cli_bytes": 0,
        "law_specific_reason": "UNESTABLISHED",
        "limit": "same original-input refusal; no arithmetic algorithm proof",
    }


def observation(rows, original, wanted):
    phases = {
        row["phase"]: row for row in rows if row.get("record") == "phase"
    }
    original_records = [
        row for row in rows
        if row.get("record") == "source" and row.get("role") == "original_unit"
    ]
    if len(original_records) != 1:
        raise Stop("R_OBSERVER_PROTOCOL: original source record missing")
    try:
        observed_original = bytes.fromhex(original_records[0]["hex"])
    except (KeyError, TypeError, ValueError):
        raise Stop("R_OBSERVER_PROTOCOL: original bytes unavailable")
    if observed_original != original:
        raise Stop("ORIGINAL_SOURCE_BYTES_CHANGED")
    for phase in phases.values():
        status = phase.get("status")
        if status in {"OUT_OF_MEMORY", "RESOURCE_LIMIT", "INTERNAL_ERROR",
                      "UNCLASSIFIED"}:
            raise Stop("PUBLIC_RESOURCE_OR_INTERNAL: " + phase["phase"])
    nodes = {
        (row["frame"], row["node"]): row
        for row in rows if row.get("record") == "node"
    }
    entry = next((row for row in rows
                  if row.get("record") == "entry_artifact"), None)
    main_frame = None
    if entry:
        root = nodes.get((0, entry["root"]))
        if root and root["kind"] == "CALL" and decode_name(root) == "main":
            bodies = [
                row for row in rows
                if row.get("record") == "body" and row.get("available") and
                row["frame"] == 0 and row["call"] == entry["root"]
            ]
            if len(bodies) == 1:
                main_frame = bodies[0]["child_frame"]
    matches = {}
    missing = []
    for name, expected in wanted.items():
        binding_rows = [
            row for row in nodes.values()
            if row["frame"] == main_frame and row["kind"] == "BINDING" and
            decode_name(row) == name
        ]
        if len(binding_rows) != 1:
            missing.append(name + ": unique original-main binding unavailable")
            continue
        binding = binding_rows[0]
        initial = nodes.get((main_frame, binding["initializer"]))
        candidates = [] if initial is None else initial.get("results", [])
        if len(candidates) != 1 or not candidates[0].get("usize"):
            missing.append(name + ": unique usize initializer result unavailable")
            continue
        value = candidates[0]
        if not value["known"]:
            missing.append(name + ": Unknown (not Known zero)")
            continue
        if value["number"] != expected:
            raise Stop("FROZEN_NUMERIC_ORACLE_MISMATCH: " + name)
        matches[name] = {
            "expected": expected, "observed": value["number"],
            "frame": main_frame, "binding_node": binding["node"],
            "initializer_node": binding["initializer"],
            "meaning": "observed relation only; not whole-call custody",
        }
    arithmetic_rows = [row for row in rows
                       if row.get("record") == "arithmetic"]
    for row in arithmetic_rows:
        if not row.get("present"):
            missing.append(row["op"] + ": public relation unavailable")
            continue
        if not row["actual_valid"] or not row["copy_valid"]:
            raise Stop("AUTHENTIC_ARITHMETIC_VALIDATION_FAILED")
        if not row["origin_is_fragment"] or row["event_node"] != row["node"]:
            raise Stop("ARITHMETIC_ORIGIN_CONTRACT_MISMATCH")
        if row["target_max"] < 56:
            raise Stop("FROZEN_TARGET_PROFILE_NOT_SATISFIED")
        divisor = row["divisor"]
        if divisor == 0 or divisor > row["target_max"]:
            raise Stop("PUBLIC_NONZERO_OR_RANGE_CONTRADICTION")
        if row["numerator_known"]:
            numerator = row["raw_numerator"]
            if numerator > row["target_max"]:
                raise Stop("PUBLIC_NUMERATOR_RANGE_CONTRADICTION")
            expected = (numerator // divisor if row["op"] == "division"
                        else numerator % divisor)
            if row["result_known"] and row["raw_result"] != expected:
                raise Stop("PUBLIC_ARITHMETIC_EQUATION_CONTRADICTION")
        if not row["result_known"]:
            missing.append(row["op"] + ": result Unknown, raw placeholder ignored")
    for row in rows:
        if (row.get("record") == "source" and
                row.get("role") != "original_unit" and
                not row.get("available")):
            missing.append("retained_source_bytes_NOT_OBSERVABLE")
        if row.get("record") == "observation_end":
            if row["contract_failure"]:
                raise Stop("PUBLIC_GRAPH_CONTRACT_STOP")
            if row["bounded_stop"]:
                raise Stop("R_TRAVERSAL_BUDGET_STOP")
        if row.get("record") == "limit":
            missing.append(row["relation"])
    return {
        "phases": phases, "main_body_frame": main_frame,
        "matched_binding_values": matches, "missing": missing,
        "arithmetic_events_observed": len(arithmetic_rows),
        "whole_call_custody": "NOT_OBSERVABLE",
        "formal_definition_unknown_values": "NOT_OBSERVABLE",
        "runtime_quantity": "NOT_OBSERVABLE",
        "runtime_order": "NOT_OBSERVABLE",
    }


def cli_check_class(code, out, err):
    if code == 0:
        if out or err:
            raise Stop("CHECK_SUCCESS_OUTPUT_CONTRACT")
        return "CHECK_ACCEPTED"
    if code == 3:
        # Provisional only. Neither nonempty output nor code3 earns a refusal
        # grade until the genuine original public status/diagnostic is bound.
        return "CHECK_CODE3_PENDING_PUBLIC_DIAGNOSTIC_BINDING"
    if code == 1:
        raise Stop("CHECK_SOURCE_IO_FAILURE")
    if code == 2:
        raise Stop("CHECK_USAGE_FAILURE")
    raise Stop("CHECK_UNCLASSIFIED_EXIT_" + str(code))


def run_case(session, args, row, source_dir):
    key, filename, invalid, source_hash, wanted = row
    source_path = source_dir / filename
    original = regular(source_path)
    if digest(original) != source_hash:
        raise Stop("FIRST_SOURCE_HASH_MISMATCH: " + key)
    result = {
        "case": key, "source": filename, "source_sha256": source_hash,
        "source_law": "INVALID" if invalid else "LAWFUL_UNDER_U_GE_56",
        "check": "NOT_RUN", "public": "NOT_RUN", "emit": "NOT_RUN",
        "native_compile": "NOT_RUN", "native_run": "NOT_RUN",
        "classification": "INCONCLUSIVE_UNFINISHED",
        "law_specific_reason": "UNESTABLISHED",
    }
    session.results.append(result)
    result["check"] = "ATTEMPTING_OR_INCOMPLETE"
    code, out, err, unused_path, unused_record = session.command(
        key + "-check", [args.newlangc, "--check", source_path], 10)
    del unused_path, unused_record
    result["check"] = cli_check_class(code, out, err)
    result["check_raw"] = {
        "returncode": code, "stdout_bytes": len(out), "stderr_bytes": len(err),
        "stdout_sha256": digest(out), "stderr_sha256": digest(err),
    }
    if invalid and code == 0:
        result["classification"] = "TRUE_INVALID_CLI_ACCEPTED_STOP"
        raise Stop("TRUE_INVALID_CLI_ACCEPTED: " + key)
    result["public"] = "ATTEMPTING_OR_INCOMPLETE"
    obs_code, obs_out, obs_err, unused_path, unused_record = session.command(
        key + "-observe",
        [args.observer, source_path, "invalid" if invalid else "lawful"], 10)
    del unused_path, unused_record
    rows = observer_rows(obs_out, obs_code)
    phase_rows = [record for record in rows if record.get("record") == "phase"]
    result["public"] = {
        "observer_exit": obs_code, "phase_records": phase_rows,
        "diagnostic_records": [record for record in rows
                               if record.get("record") in {
                                   "diagnostic", "diagnostic_render",
                               }],
        "stdout_bytes": len(obs_out), "stdout_sha256": digest(obs_out),
        "stderr_bytes": len(obs_err), "stderr_sha256": digest(obs_err),
        "admission_stop_records": [record for record in rows
                                   if record.get("record") == "admission_stop"],
    }
    # Inspect original admission before exit/adapter/refusal classification.
    # A later exit12 or any other adapter result cannot turn this into a PASS.
    if invalid and any(
            record.get("phase") == "original_unit_register" and
            record.get("status") == "OK" for record in phase_rows):
        result["classification"] = "ORIGINAL_INVALID_UNIT_ADMITTED_STOP"
        raise Stop("TRUE_INVALID_ORIGINAL_UNIT_REGISTERED: " + key)
    if obs_code == 20:
        raise Stop("PUBLIC_RESOURCE_INTERNAL_OR_APPARATUS_FAILURE: " + key)
    if obs_code == 21:
        raise Stop("PUBLIC_GRAPH_OR_ARITHMETIC_CONTRACT_FAILURE: " + key)
    if obs_code == 22:
        raise Stop("R_OBSERVER_TRAVERSAL_BUDGET: " + key)
    if obs_code not in {0, 10, 11, 12}:
        raise Stop("OBSERVER_UNCLASSIFIED_EXIT: " + key + ": " + str(obs_code))
    phases = public_protocol(rows, obs_code, original, invalid)
    result["public"]["phases"] = phases
    result["public"]["original_parse"] = phases["original_unit_parse"]["status"]
    result["public"]["original_registration"] = phases.get(
        "original_unit_register", {"status": "NOT_STARTED"})["status"]
    result["public"]["synthetic_entry"] = phases.get(
        "synthetic_entry_check", {"status": "NOT_STARTED"})["status"]

    if code == 3:
        binding = bind_original_diagnostic(
            rows, phases, original, source_path, obs_code, obs_err,
            code, out, err)
        result["diagnostic_binding"] = binding
        result["check"] = "CHECK_CODE3_BOUND_TO_ORIGINAL_PUBLIC_REFUSAL"
        if invalid:
            if (obs_code != 11 or phases["original_unit_parse"]["status"] != "OK" or
                    phases["original_unit_register"]["status"] != "SEMANTIC_ERROR"):
                result["classification"] = "INCONCLUSIVE_ORIGINAL_BOUNDARY_STOP"
                raise Stop("TRUE_INVALID_INTENDED_SEMANTIC_GRADE_NOT_REACHED: " + key)
            result["classification"] = "ORIGINAL_INVALID_SOURCE_REJECTED"
            result["evidence_limit"] = (
                "healthy rejection of frozen original invalid input; "
                "law-specific algorithm/diagnostic path remains unestablished"
            )
            return
        result["classification"] = "LAWFUL_SOURCE_REFUSED_AT_ORIGINAL_BOUNDARY"
        result["evidence_limit"] = (
            "actual parser/check status and matched diagnostic retained; "
            "no numeric/backend capability or source-invalidity PASS"
        )
        return
    if obs_code != 0:
        raise Stop("INCONCLUSIVE_CLI_ACCEPTED_PUBLIC_REFUSED: " + key)
    if obs_err:
        raise Stop("PUBLIC_SUCCESS_STDERR_HEALTH_UNCONFIRMED: " + key)
    result["public"]["observation"] = observation(rows, original, wanted)

    result["emit"] = "ATTEMPTING_OR_INCOMPLETE"
    emit_code, c_bytes, emit_err, c_path, unused_record = session.command(
        key + "-emit", [args.newlangc, source_path], 10,
        stdout_limit=65536)
    del unused_record
    result["emit"] = {
        "returncode": emit_code, "bytes": len(c_bytes),
        "sha256": digest(c_bytes),
        "documented_backend_diagnostic":
            b"V1-BACKEND-UNSUPPORTED" in emit_err,
    }
    if emit_code != 0:
        result["emit"]["numeric_exit_contract"] = "UNSPECIFIED"
        if not c_bytes and b"V1-BACKEND-UNSUPPORTED" in emit_err:
            result["classification"] = (
                "DOCUMENTED_BACKEND_DIAGNOSTIC; numeric exit unspecified; "
                "C17/native NOT_REACHED"
            )
            return
        raise Stop("EMIT_UNCLASSIFIED_REFUSAL: " + key)
    if not c_bytes or emit_err:
        raise Stop("EMIT_OUTPUT_HEALTH_UNCONFIRMED: " + key)
    result["emit"]["numeric_exit_contract"] = "normal process completion observed"
    program = session.work / (key + "-program")
    flags = ["-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror"]
    if args.compiler_id == "Clang":
        # The same specific warning exception as the existing full workflow.
        flags.append("-Wno-error=gcc-install-dir-libstdcxx")
    flags += (["-O2", "-DNDEBUG"] if args.build_type == "Release"
              else ["-O0", "-g"])
    if args.sanitizer != "none":
        flags += [
            "-fsanitize=" + args.sanitizer, "-fno-omit-frame-pointer",
            "-fno-sanitize-recover=all",
        ]
    before = digest(regular(c_path))
    result["native_compile"] = "ATTEMPTING_OR_INCOMPLETE"
    cc_code, cc_out, cc_err, unused_path, unused_record = session.command(
        key + "-cc", [args.cc, *flags, "-x", "c", c_path, "-o", program], 30)
    del cc_out, cc_err, unused_path, unused_record
    result["native_compile"] = {"returncode": cc_code, "argv_flags": flags}
    if digest(regular(c_path)) != before or before != digest(c_bytes):
        raise Stop("PRISTINE_C_BYTES_CHANGED: " + key)
    if cc_code != 0 or not program.is_file() or program.is_symlink():
        raise Stop("PRISTINE_C17_COMPILATION_FAILED: " + key)
    result["native_run"] = "ATTEMPTING_OR_INCOMPLETE"
    native_code, native_out, native_err, unused_path, unused_record = (
        session.command(key + "-native", [program], 5)
    )
    del unused_path, unused_record
    result["native_run"] = {
        "returncode": native_code,
        "stdout_bytes": len(native_out), "stderr_bytes": len(native_err),
        "quantity": "NOT_OBSERVABLE", "order": "NOT_OBSERVABLE",
        "whole_call_custody": "NOT_OBSERVABLE",
    }
    if native_code != 0 or native_out or native_err:
        raise Stop("NATIVE_PROCESS_OR_OUTPUT_HEALTH_FAILURE: " + key)
    result["classification"] = (
        "PRISTINE_NATIVE_PROCESS_COMPLETED; internal quantities/order/custody "
        "not inferred from exit0"
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--newlangc", required=True)
    parser.add_argument("--observer", required=True)
    parser.add_argument("--cc", required=True)
    parser.add_argument("--compiler-id", choices=["GNU", "Clang"], required=True)
    parser.add_argument("--sanitizer",
                        choices=["none", "address", "undefined"], required=True)
    parser.add_argument("--build-type", required=True)
    parser.add_argument("--raw", required=True)
    parser.add_argument("--work", required=True)
    args = parser.parse_args()
    if os.environ.get("GITHUB_ACTIONS") != "true":
        print("R1601 MODE_BLOCKED: GitHub Actions only", file=sys.stderr)
        return 20
    if os.environ.get("GITHUB_REPOSITORY") != "wakairo/NewLang_Compiler":
        print("R1601 REPOSITORY_BLOCKED", file=sys.stderr)
        return 20

    project = Path(__file__).resolve().parents[2]
    own = project / "tests" / "r1601"
    raw, work = Path(args.raw), Path(args.work)
    allowed_raw = {project / "build" / "r1601-raw",
                   project / "build-frontend" / "r1601-raw"}
    allowed_work = {project / "build" / "r1601-work",
                    project / "build-frontend" / "r1601-work"}
    if raw not in allowed_raw or work not in allowed_work:
        print("R1601 fixed output path mismatch", file=sys.stderr)
        return 20
    if raw.exists() or raw.is_symlink() or work.exists() or work.is_symlink():
        print("R1601 refuses to overwrite an earlier run", file=sys.stderr)
        return 20
    raw.mkdir()
    work.mkdir()
    session = Session(raw, work)
    final = {
        "completed": False, "source_oracle_revision": "FIRST V1",
        "apparatus_revision": "V2",
        "cases": session.results, "status": "NOT_STARTED",
        "read_base": "6287fcb4cb94df3da1c67f57d7d717b29c25ecf9",
        "product_go": "PAUSED", "audit_reaudit_gain": "UNMEASURED",
    }
    exit_code = 20
    try:
        first_manifest = regular(own / "FIRST.sha256")
        if digest(first_manifest) != FIRST_MANIFEST_SHA:
            raise Stop("FIRST_MANIFEST_HASH_MISMATCH")
        source_hashes = {}
        for key, filename, unused_invalid, expected, unused_wanted in FIRST:
            del unused_invalid, unused_wanted
            data = regular(own / "sources" / filename)
            if digest(data) != expected:
                raise Stop("FIRST_SOURCE_HASH_MISMATCH: " + key)
            source_hashes[filename] = expected
        context = {
            "environment": {key: os.environ.get(key) for key in ENV_KEYS},
            "source_sha256": source_hashes,
            "first_manifest_sha256": FIRST_MANIFEST_SHA,
            "apparatus_revision": "V2",
            "plan_manifest_sha256": digest(regular(own / "PLAN_V2.sha256")),
            "apparatus_sha256": {
                name: digest(regular(own / name))
                for name in ("observe.c", "run.py", "publish.py", "CMakeLists.txt")
            },
            "budgets": {
                "max_commands": MAX_COMMANDS, "total_seconds": TOTAL_SECONDS,
                "captured_stream_bytes": TOTAL_CAPTURE,
            },
        }
        dump(raw / "context.json", context)
        head_code, head_out, head_err, unused_path, unused_record = (
            session.command("git-head", ["git", "-C", project,
                                         "rev-parse", "HEAD"], 5,
                            stdout_limit=4096, stderr_limit=4096)
        )
        del unused_path, unused_record
        if head_code != 0 or head_err or not re.fullmatch(
                rb"[0-9a-f]{40}\n", head_out):
            raise Stop("CHECKOUT_SHA_NOT_OBSERVED")
        final["checkout_sha"] = head_out.decode("ascii").strip()
        if final["checkout_sha"] != os.environ.get("GITHUB_SHA"):
            raise Stop("CHECKOUT_EVENT_SHA_MISMATCH")
        for row in FIRST:
            run_case(session, args, row, own / "sources")
        final["completed"] = True
        final["status"] = "FINITE_OBSERVATIONS_COMPLETE_WITH_EXPLICIT_LIMITS"
        exit_code = 0
    except Stop as error:
        final["status"] = "STOP_C"
        final["reason"] = str(error)
        exit_code = 1
    except Exception as error:
        final["status"] = "APPARATUS_EXCEPTION"
        final["reason"] = type(error).__name__ + ": " + str(error)
        exit_code = 20
    finally:
        visited = {row["case"] for row in session.results}
        final["not_started_cases"] = [row[0] for row in FIRST
                                      if row[0] not in visited]
        final["command_count"] = len(session.commands)
        final["captured_stream_bytes"] = session.captured
        final["elapsed_seconds"] = round(time.monotonic() - session.started, 6)
        final["exit"] = exit_code
        dump(raw / "summary.json", final)
        lines = []
        for path in sorted(raw.iterdir()):
            if path.is_symlink() or not path.is_file():
                raise Stop("nonregular raw artifact")
            data = path.read_bytes()
            lines.append(digest(data) + "  " + path.name + "\n")
        with (raw / "SHA256SUMS").open("xb") as stream:
            stream.write("".join(lines).encode("ascii"))
            stream.flush()
            os.fsync(stream.fileno())
    print(json.dumps({
        "R1601": final["status"], "exit": exit_code,
        "commands": final["command_count"],
        "raw_dir": str(raw),
        "note": "Always-step publishes success and failure bytes separately",
    }, sort_keys=True), flush=True)
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
