"""One future bounded R1608 six-source observation pass. No retries."""
import argparse
import datetime
import json
import os
from pathlib import Path
import re
import resource
import selectors
import shlex
import signal
import subprocess
import sys
import time

from rawio import (CASES, FIRST, FIRST_MANIFEST_SHA, PHASES, ROOTS,
                   STREAM_LIMIT, canonical, open_directory,
                   read_regular_at, sha, write_new)

MAX_CHILDREN = 21
SUITE_SECONDS = 540
CHILD_SECONDS = {"observer": 20, "check": 20, "emit": 20, "cc": 45, "native": 10}
CHECK = {0: "OK", 1: "SEMANTIC_ERROR", 2: "SEMANTIC_UNSUPPORTED",
         3: "ANALYSIS_PRECISION_LIMIT", 4: "OUT_OF_MEMORY",
         5: "RESOURCE_LIMIT", 6: "INTERNAL_ERROR"}
PARSE = {0: "OK", 1: "LEXICALLY_UNSUPPORTED", 2: "SYNTAX_UNSUPPORTED",
         3: "SYNTAX_ERROR", 4: "OUT_OF_MEMORY",
         5: "RESOURCE_LIMIT", 6: "INTERNAL_ERROR"}

class Stop(Exception):
    def __init__(self, reason, detail=None):
        super().__init__(reason)
        self.detail = detail

def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()

def limits():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    resource.setrlimit(resource.RLIMIT_FSIZE, (32 * 1024 * 1024,) * 2)
    resource.setrlimit(resource.RLIMIT_CPU, (50, 50))
    resource.setrlimit(resource.RLIMIT_NOFILE, (128, 128))
    # Do not set RLIMIT_AS: that would interfere with sanitizer address spaces.

class Run:
    def __init__(self, args, root, work, raw_fd):
        self.args, self.root, self.work, self.fd = args, root, work, raw_fd
        self.deadline = time.monotonic() + SUITE_SECONDS
        self.children = 0

    def save(self, name, value):
        write_new(self.fd, name, canonical(value))

    def read(self, name, cap=STREAM_LIMIT):
        return read_regular_at(self.fd, name, cap)[0]

    def child(self, cid, phase, argv):
        if self.children >= MAX_CHILDREN or time.monotonic() >= self.deadline:
            raise Stop("cohort_child_or_wall_budget")
        stem = cid + "." + phase
        self.children += 1
        started = time.monotonic()
        self.save(stem + ".start.json", {
            "attempted": True, "argv": argv, "cwd": str(self.work),
            "utc": utc(), "child_number": self.children,
            "timeout_seconds": CHILD_SECONDS[phase],
            "stdin": "DEVNULL", "shell": False,
            "environment_observed": {
                name: os.environ.get(name) for name in
                ("GITHUB_RUN_ID", "GITHUB_RUN_ATTEMPT", "GITHUB_SHA",
                 "GITHUB_JOB", "GITHUB_WORKFLOW", "CC", "CFLAGS",
                 "ASAN_OPTIONS", "UBSAN_OPTIONS")
            },
        })
        output = {}
        for stream in ("stdout", "stderr"):
            name = stem + "." + stream + ".bin"
            output[stream] = os.fdopen(os.open(
                name, os.O_WRONLY | os.O_CREAT | os.O_EXCL |
                os.O_NOFOLLOW | os.O_NONBLOCK, 0o600, dir_fd=self.fd), "wb")
        process = None
        reason, launched, returncode = None, False, None
        sizes = {"stdout": 0, "stderr": 0}
        seen = {"stdout": 0, "stderr": 0}
        selector = selectors.DefaultSelector()
        deadline = min(self.deadline, started + CHILD_SECONDS[phase])
        try:
            try:
                process = subprocess.Popen(
                    argv, cwd=self.work, stdin=subprocess.DEVNULL,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    shell=False, start_new_session=True, preexec_fn=limits)
                launched = True
            except OSError as error:
                reason = "launch_error:" + type(error).__name__ + ":" + str(error)
            if process is not None:
                for stream, pipe in (("stdout", process.stdout),
                                     ("stderr", process.stderr)):
                    os.set_blocking(pipe.fileno(), False)
                    selector.register(pipe, selectors.EVENT_READ, stream)
                killed_at = None
                while selector.get_map() or process.poll() is None:
                    now = time.monotonic()
                    if reason is None and now >= deadline:
                        reason = "timeout"
                    if reason is not None and killed_at is None:
                        try:
                            os.killpg(process.pid, signal.SIGKILL)
                        except ProcessLookupError:
                            pass
                        killed_at = now
                    if killed_at is not None and now - killed_at > 2:
                        reason += ":drain_incomplete"
                        break
                    for key, _mask in selector.select(0.05):
                        data = os.read(key.fileobj.fileno(), 8192)
                        if not data:
                            selector.unregister(key.fileobj)
                            key.fileobj.close()
                            continue
                        stream = key.data
                        seen[stream] += len(data)
                        room = STREAM_LIMIT - sizes[stream]
                        kept = data[:max(0, room)]
                        output[stream].write(kept)
                        output[stream].flush()
                        sizes[stream] += len(kept)
                        if len(kept) != len(data) and reason is None:
                            reason = stream + "_limit"
                try:
                    returncode = process.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    reason = (reason or "wait_timeout") + ":unreaped"
        except BaseException as error:
            reason = (reason or "capture_error") + ":" + type(error).__name__
            if process is not None:
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                try:
                    returncode = process.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    pass
        finally:
            for key in list(selector.get_map().values()):
                selector.unregister(key.fileobj)
                key.fileobj.close()
            selector.close()
            for file in output.values():
                file.flush()
                os.fsync(file.fileno())
                file.close()
        stdout = self.read(stem + ".stdout.bin")
        stderr = self.read(stem + ".stderr.bin")
        result = {
            "attempted": True, "launched": launched, "returncode": returncode,
            "signal": -returncode if returncode is not None and returncode < 0 else None,
            "reason": reason, "complete": reason is None and returncode is not None,
            "utc": utc(), "elapsed_seconds": time.monotonic() - started,
            "streams": {
                "stdout": {"bytes": len(stdout), "sha256": sha(stdout),
                           "seen_bytes": seen["stdout"]},
                "stderr": {"bytes": len(stderr), "sha256": sha(stderr),
                           "seen_bytes": seen["stderr"]},
            },
        }
        self.save(stem + ".result.json", result)
        return result, stdout, stderr

def records_prefix(data):
    records, complete = [], True
    for line in data.splitlines(keepends=True):
        if not line.endswith(b"\n"):
            complete = False
            break
        try:
            item = json.loads(line)
            if not isinstance(item, dict):
                raise ValueError("record must be object")
            records.append(item)
        except (ValueError, UnicodeDecodeError):
            complete = False
            break
    return records, complete

def healthy_child(result):
    return (result["complete"] and result["launched"] and
            result["returncode"] is not None and result["returncode"] >= 0)

def original_status(records):
    status = {}
    for item in records:
        if item.get("record") == "status":
            stage = item.get("stage")
            if stage in status:
                raise Stop("duplicate_observer_status")
            status[stage] = item.get("value")
    return status

def verify_public(records, numerator):
    """Only bounded structural/isolate checks; no whole-call or runtime claim."""
    fragments = {r["fragment"]: r for r in records if r.get("record") == "fragment"}
    sources = {r["source"]: bytes.fromhex(r["bytes_hex"])
               for r in records if r.get("record") == "source"}
    nodes = {(r["fragment"], r["node"]): r for r in records
             if r.get("record") == "node"}
    arithmetic = [r for r in records if r.get("record") in ("division", "remainder")]
    if not arithmetic or not any(r["record"] == "division" for r in arithmetic) or \
            not any(r["record"] == "remainder" for r in arithmetic):
        return False, "arithmetic_coverage_not_observed"
    for r in arithmetic:
        if not r.get("view"):
            return False, "public_arithmetic_view_unavailable"
        if not all(r.get(key) is True for key in ("origin_same", "event_valid", "copy_valid")):
            raise Stop("authentic_arithmetic_validation_failed")
        if r.get("wrong_zero_accepted") or r.get("foreign_origin_accepted") or \
                r.get("foreign_source_accepted"):
            raise Stop("read_only_candidate_negative_accepted")
        if r["record"] == "remainder" and not r.get("source_same"):
            raise Stop("remainder_retained_source_mismatch")
        node = nodes.get((r["fragment"], r["node"]))
        fragment = fragments.get(r["fragment"])
        if node is None or fragment is None or fragment["source"] not in sources:
            return False, "missing_qualified_structure"
        source = sources[fragment["source"]]
        start, end = r["operator_span"]
        operator = b"/" if r["record"] == "division" else b"%"
        if not (0 <= start <= end <= len(source)) or source[start:end] != operator:
            raise Stop("operator_source_span_mismatch")
        if r["span"] != node["span"] or r["target_max"] < max(7, numerator):
            raise Stop("span_or_target_literal_range_mismatch")
        operands = r["operand_spans"]
        actual_tokens = []
        for a, b in operands:
            if not 0 <= a <= b <= len(source):
                raise Stop("operand_source_span_invalid")
            actual_tokens.append(source[a:b].strip(b" \t\r\n"))
        if actual_tokens != [b"copied", b"usize(7)"]:
            raise Stop("frozen_arithmetic_source_mismatch")
        if r["numbers"][1] != 7:
            raise Stop("observed_divisor_mismatch")
        if r["numerator_known"]:
            if r["numbers"][0] != numerator:
                raise Stop("known_numerator_contradicts_frozen_actual")
        elif r["numbers"][0] != 0:
            raise Stop("unknown_numerator_placeholder_not_canonical")
        expected = numerator // 7 if operator == b"/" else numerator % 7
        if r["result_known"]:
            if r["quantity"] != expected:
                raise Stop("known_arithmetic_result_contradicts_oracle")
        elif r["quantity"] != 0:
            raise Stop("unknown_result_placeholder_not_canonical")
    edges = [r.get("edge") for r in fragments.values()]
    if "then_hypothetical" not in edges or "else_hypothetical" not in edges:
        return False, "both_hypothetical_arms_not_observed"
    return True, "isolated_arithmetic_and_borrowed_structure_only"

def diagnose_original(run, cid, records, cli):
    result, out, err = cli
    diag = [r for r in records if r.get("record") == "diagnostic" and
            r.get("stage") in ("original_parse", "original_register")]
    rendered = run.read(cid + ".observer.rendered")
    if (not healthy_child(result) or result["returncode"] != 3 or out or
            not rendered or err != rendered or len(diag) != 1 or
            not diag[0].get("span_valid") or not diag[0].get("rendered") or
            diag[0].get("input_index") != 0 or diag[0].get("severity") != 2 or
            not diag[0].get("message_hex")):
        raise Stop("original_public_diagnostic_cli_correspondence_unestablished")
    return {"diagnostic": diag[0], "renderer_cli_bytes_equal": True,
            "intended_law_reason": "not_inferred_from_untyped_diagnostic"}

def case(run, cid, filename, invalid, numerator, args):
    source = run.root / (cid + ".source.nl")
    before = run.read(cid + ".source.nl", 4096)
    if sha(before) != FIRST[filename]:
        raise Stop("frozen_source_mismatch_before_child")
    observed = run.child(cid, "observer", [
        args.observer, str(source), str(run.root / "adapter.nl"),
        str(run.root / (cid + ".observer.rendered")),
        "invalid" if invalid else "lawful"])
    obs_result, obs_out, obs_err = observed
    records, complete_records = records_prefix(obs_out)
    status = original_status(records)
    # This precedes child/adapter/CLI rescue and even end-record requirements.
    if invalid and status.get("original_register") == 0:
        raise Stop("true_invalid_original_admitted_unconditional_stop")
    if not healthy_child(obs_result) or obs_result["returncode"] != 0 or obs_err:
        raise Stop("observer_signal_resource_sanitizer_or_apparatus_failure")
    if not complete_records or not records or records[-1].get("record") != "end":
        raise Stop("incomplete_observer_records")
    if records[-1].get("fatal"):
        raise Stop("observer_authenticity_fatal")
    loaded_originals = [bytes.fromhex(r["bytes_hex"]) for r in records
                        if r.get("record") == "source" and r.get("original_owner")]
    if loaded_originals != [before]:
        raise Stop("original_loaded_source_bytes_not_frozen_input")
    loaded_adapters = [bytes.fromhex(r["bytes_hex"]) for r in records
                       if r.get("record") == "source" and r.get("adapter_owner")]
    if loaded_adapters and loaded_adapters != [b"main()\n"]:
        raise Stop("adapter_loaded_bytes_mismatch")
    ps, cs = status.get("original_parse"), status.get("original_register")
    if ps not in PARSE or ps in (4, 5, 6) or (ps == 0 and cs not in CHECK):
        raise Stop("original_status_missing_resource_or_internal")
    if ps == 0 and cs in (4, 5, 6):
        raise Stop("original_check_resource_or_internal")
    cli = run.child(cid, "check", [args.newlangc, "--check", str(source)])
    if run.read(cid + ".source.nl", 4096) != before:
        raise Stop("source_changed_during_public_or_cli_check")
    decision = {
        "original_parse": PARSE[ps], "original_register": CHECK.get(cs, "NOT_RUN"),
        "adapter_parse": PARSE.get(status.get("adapter_parse"), "NOT_RUN"),
        "adapter_check": CHECK.get(status.get("adapter_check"), "NOT_RUN"),
        "runtime_internal_quantities_order": "NOT_OBSERVED",
        "whole_call_result_world_theorem": "NOT_OBSERVED",
        "native": "NOT_RUN", "source_legal_oracle_unchanged": True,
    }
    if ps != 0 or cs != 0:
        decision.update(diagnose_original(run, cid, records, cli))
        if not invalid and (ps == 3 or cs == 1):
            raise Stop("lawful_source_semantic_or_syntax_error_stop_c", decision)
        if ps != 0:
            decision["classification"] = "ORIGINAL_PARSE_BOUNDARY_NOT_INTENDED_LAW"
        elif cs == 1:
            decision["classification"] = "ORIGINAL_INPUT_SEMANTIC_REJECTION_ONLY"
        else:
            decision["classification"] = "ORIGINAL_UNSUPPORTED_OR_PRECISION_BOUNDARY"
        return decision
    if invalid:
        raise Stop("invalid_original_admission_guard")
    cli_result, cli_out, cli_err = cli
    if status.get("adapter_parse") != 0 or status.get("adapter_check") != 0:
        decision["classification"] = "ORIGINAL_ADMITTED_ADAPTER_BOUNDARY"
        decision["cli_diagnostic_correspondence"] = "NOT_ESTABLISHED"
        raise Stop("original_admitted_adapter_boundary_not_source_rejection", decision)
    if (not healthy_child(cli_result) or cli_result["returncode"] != 0 or
            cli_out or cli_err):
        raise Stop("successful_public_check_cli_contract_divergence")
    if records[-1].get("limited"):
        decision["classification"] = "ORIGINAL_AND_ENTRY_CHECK_OK_PUBLIC_OBSERVATION_LIMIT"
        return decision
    observed_ok, evidence = verify_public(records, numerator)
    decision["public_evidence"] = evidence
    if not observed_ok:
        decision["classification"] = "PUBLIC_OBSERVATION_LIMIT"
        return decision
    if run.read(cid + ".source.nl", 4096) != before:
        raise Stop("source_changed_after_check")
    emitted = run.root / (cid + ".emitted.c")
    emit_result, _emit_out, _emit_err = run.child(cid, "emit", [
        args.newlangc, "--emit-c17-to", str(emitted), str(source)])
    if not healthy_child(emit_result):
        raise Stop("emission_resource_signal_timeout_or_sanitizer")
    if emit_result["returncode"] != 0:
        decision["classification"] = "CHECK_OK_EMISSION_NONZERO_UNCLASSIFIED"
        decision["emission_status"] = emit_result["returncode"]
        raise Stop("emission_nonzero_not_a_proved_healthy_refusal", decision)
    cbytes = run.read(cid + ".emitted.c", 262144)
    if not cbytes:
        raise Stop("empty_successful_c17_output")
    flags = shlex.split(args.cflags) + shlex.split(args.config_cflags)
    flags += ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    ldflags = shlex.split(args.ldflags) + shlex.split(args.config_ldflags)
    if args.sanitizer != "none":
        flags += ["-fsanitize=" + args.sanitizer, "-fno-omit-frame-pointer",
                  "-fno-sanitize-recover=all"]
        ldflags += ["-fsanitize=" + args.sanitizer, "-fno-sanitize-recover=all"]
    binary = run.work / (cid + ".native")
    cc_result, _cc_out, _cc_err = run.child(cid, "cc", [
        args.cc, *flags, str(emitted), *ldflags, "-o", str(binary)])
    if not healthy_child(cc_result) or cc_result["returncode"] != 0:
        raise Stop("pristine_c17_host_compile_failed")
    work_fd = open_directory(run.work)
    try:
        binary_bytes, identity = read_regular_at(work_fd, binary.name, 32 * 1024 * 1024)
    finally:
        os.close(work_fd)
    run.save(cid + ".native.info.json", {
        "bytes": len(binary_bytes), "sha256": sha(binary_bytes),
        "identity": identity, "c17_bytes": len(cbytes), "c17_sha256": sha(cbytes),
        "binary_transport": "excluded; saved in separate work root",
    })
    if run.read(cid + ".emitted.c", 262144) != cbytes:
        raise Stop("emitted_c_changed_before_native")
    native, native_out, native_err = run.child(cid, "native", [str(binary)])
    if (not healthy_child(native) or native["returncode"] != 0 or
            native_out or native_err):
        raise Stop("native_signal_timeout_sanitizer_or_nonzero")
    if run.read(cid + ".emitted.c", 262144) != cbytes or \
            run.read(cid + ".source.nl", 4096) != before:
        raise Stop("pristine_source_or_c_changed")
    decision["classification"] = "CHECK_PUBLIC_ISOLATE_AND_NATIVE_PROCESS_COMPLETED"
    decision["native"] = "EXIT_0_PROCESS_COMPLETION_ONLY"
    return decision

def main():
    parser = argparse.ArgumentParser()
    for name in ("observer", "newlangc", "cc", "sanitizer", "mode", "target",
                 "cflags", "config-cflags", "ldflags", "config-ldflags"):
        parser.add_argument("--" + name, required=True)
    args = parser.parse_args()
    workspace = Path(os.environ.get("GITHUB_WORKSPACE", "")).absolute()
    if args.mode not in ROOTS:
        raise SystemExit("STOP: invalid mode")
    root = workspace.joinpath(*ROOTS[args.mode])
    work = root.parent / "r1608-work-v1"
    # Creation must fail on a prior run, preserving all first artifacts.
    os.mkdir(root, 0o700)
    fd = open_directory(root)
    run = Run(args, root, work, fd)
    decisions, attempted = {}, set()
    stopping = "INCOMPLETE:intended_completion_not_reached"
    current = None
    try:
        run.save("suite.start.json", {
            "utc": utc(), "target": args.target, "interface_base":
            "088ccdef7d18620f1d5dd698e46c1aae0613bf52",
            "source_law_base": "6287fcb4cb94df3da1c67f57d7d717b29c25ecf9",
            "first_manifest_sha256": FIRST_MANIFEST_SHA,
            "mode": args.mode, "max_children": MAX_CHILDREN,
            "suite_seconds": SUITE_SECONDS, "case_order": [x[0] for x in CASES],
            "actions": {key: os.environ.get(key) for key in
                        ("GITHUB_RUN_ID", "GITHUB_RUN_ATTEMPT", "GITHUB_SHA",
                         "GITHUB_JOB", "GITHUB_EVENT_NAME", "GITHUB_WORKFLOW")},
        })
        if os.environ.get("GITHUB_ACTIONS") != "true":
            raise Stop("actions_only_environment_required")
        if not re.fullmatch(r"[0-9a-f]{40}", args.target):
            raise Stop("execution_target_not_released")
        if args.sanitizer not in ("none", "address", "undefined"):
            raise Stop("unknown_sanitizer")
        os.mkdir(work, 0o700)
        first_fd = open_directory(Path(__file__).absolute().parent / "first")
        try:
            manifest, _identity = read_regular_at(first_fd, "SHA256SUMS", 2048)
            write_new(fd, "first.manifest", manifest)
            if sha(manifest) != FIRST_MANIFEST_SHA:
                raise Stop("FIRST_manifest_changed")
            for name, digest in FIRST.items():
                data, _identity = read_regular_at(first_fd, name, 32768)
                if sha(data) != digest:
                    raise Stop("FIRST_payload_changed:" + name)
                for cid, filename, _invalid, _number in CASES:
                    if name == filename:
                        write_new(fd, cid + ".source.nl", data)
        finally:
            os.close(first_fd)
        own_fd = open_directory(Path(__file__).absolute().parent)
        try:
            adapter, _identity = read_regular_at(own_fd, "entry_adapter.nl", 64)
        finally:
            os.close(own_fd)
        if adapter != b"main()\n":
            raise Stop("entry_adapter_changed")
        write_new(fd, "adapter.nl", adapter)
        for cid, filename, invalid, numerator in CASES:
            current = cid
            attempted.add(cid)
            decision = case(run, cid, filename, invalid, numerator, args)
            decisions[cid] = decision
            run.save(cid + ".decision.json", decision)
        # Only this intended path may clear the default incomplete state.
        stopping = None
    except BaseException as error:
        stopping = type(error).__name__ + ":" + str(error)
        if isinstance(error, Stop) and error.detail is not None and current is not None:
            decisions[current] = dict(error.detail, stop=stopping)
            run.save(current + ".decision.json", decisions[current])
    finally:
        for cid, _filename, _invalid, _number in CASES:
            if cid not in decisions:
                decision = {"classification":
                            "STARTED_INCOMPLETE_OR_STOP" if cid in attempted else "NOT_RUN",
                            "reason": stopping, "is_current_case": cid == current,
                            "native": "NOT_RUN_OR_SEE_PRESERVED_CHILD_START",
                            "source_legal_oracle_unchanged": True}
                decisions[cid] = decision
                try:
                    run.save(cid + ".decision.json", decision)
                except OSError:
                    pass
        try:
            run.save("suite.result.json", {
                "utc": utc(), "stop": stopping, "children_attempted": run.children,
                "decisions": decisions,
                "capability_pass": "not_inferred",
                "whole_call_and_native_internal_values": "NOT_OBSERVED",
            })
        finally:
            os.close(fd)
    if stopping is not None:
        print("R1608 STOP; saved raw evidence: " + stopping, flush=True)
        return 1
    print("R1608 finite observation pass complete; inspect per-case scope. "
          "CTest exit 0 is not a compiler-capability or Product PASS.", flush=True)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
