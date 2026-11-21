"""R1617 bounded child capture and best-effort persistent finalization.

This module launches only the explicitly supplied child. It contains no source
checker, oracle, shell, retry, recovered-code execution or semantic admission.
"""
import hashlib
import json
import os
from pathlib import Path
import resource
import selectors
import signal
import subprocess
import time

CLEANUP_SECONDS = 2.0
MAX_ERRORS = 64
MAX_JOURNAL_BYTES = 65536

def sha256(data):
    return hashlib.sha256(data).hexdigest()

def json_bytes(value):
    return (json.dumps(value, sort_keys=True, indent=2) + "\n").encode("utf-8")

def fallback_notice(value):
    """At most 4096 bytes to the enclosing runner log; failure is not success."""
    try:
        data = (json.dumps(value, sort_keys=True) + "\n").encode("utf-8")
        os.write(2, data[:4096])
        return len(data) <= 4096
    except BaseException:
        return False

def write_new(path, data):
    """Never replaces a previous observation; caller must retain any exception."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("xb") as out:
        written = out.write(data)
        if written != len(data):
            raise OSError("short persistent write")
        out.flush()
        os.fsync(out.fileno())

def capture_process(argv, cwd, prefix, seconds, outer_deadline, stream_limit,
                    file_limit, cpu_seconds):
    """Return process facts and actual captured prefixes even on cleanup failure.

    Clock checks are periodic. Nonblocking pipe reads and bounded wait/probe
    loops do not make filesystem writes or every syscall continuously timed.
    The enclosing CTest/workflow bound is a separate outer limit.
    """
    prefix = Path(prefix)
    argv = [str(x) for x in argv]
    begun = time.monotonic()
    deadline = min(begun + seconds, outer_deadline)
    paths = {key: prefix.with_name(prefix.name + suffix) for key, suffix in (
        ("argv", ".argv.json"), ("stdout", ".stdout.bin"),
        ("stderr", ".stderr.bin"), ("journal", ".events.jsonl"),
        ("status", ".status.json"), ("incomplete", ".incomplete.json"))}
    rec = {
        "schema": "r1617-child-v2", "argv": argv, "cwd": str(cwd),
        "deadline_seconds": seconds, "deadline_kind": "periodically_checked",
        "cleanup_grace_seconds": CLEANUP_SECONDS, "spawn_attempted": False,
        "spawn_succeeded": False, "pid": None, "pgid": None,
        "returncode": None, "signal": None, "parent_reaped": False,
        "timed_out": False, "interrupted": False, "unreaped": False,
        "stdout_truncated": False, "stderr_truncated": False,
        "stdout_extra_bytes_observed": 0, "stderr_extra_bytes_observed": 0,
        "pipe_read_error": False, "stdout_eof": False, "stderr_eof": False,
        "group_state": "NOT_STARTED", "group_absence_observed": False,
        "residual_group_observed": False,
        "group_scope_limit": "launched PGID only; escaped descendants are not excluded",
        "errors": [], "error_records_incomplete": False, "first_problem": None,
        "argv_persisted": False, "journal_persisted": False,
        "stdout_primary_persisted": False, "stderr_primary_persisted": False,
        "stdout_fallback_persisted": False, "stderr_fallback_persisted": False,
        "status_write_completed": None, "persistence_complete": None,
        "status_self_confirmation": "UNCONFIRMED_INSIDE_STATUS_FILE; returned/enclosing record confirms completed write",
    }
    buffers = {"stdout": bytearray(), "stderr": bytearray()}
    outputs = {}
    stream_write_failed = {"stdout": False, "stderr": False}
    journal = None
    journal_failed = False
    journal_bytes = 0
    proc = None
    sel = None
    stop_requested = False
    body_error = False

    def error(action, exc, category="cleanup"):
        nonlocal stop_requested
        item = {"category": category, "action": action,
                "type": type(exc).__name__, "message": str(exc)[:1024]}
        if rec["first_problem"] is None:
            rec["first_problem"] = item
        if len(rec["errors"]) < MAX_ERRORS:
            rec["errors"].append(item)
        else:
            rec["error_records_incomplete"] = True
        stop_requested = True

    def event(kind, **values):
        nonlocal journal_bytes, journal_failed
        if journal is None or journal_failed:
            return
        try:
            data = (json.dumps({"event": kind, **values}, sort_keys=True)
                    + "\n").encode("utf-8")
            if journal_bytes + len(data) > MAX_JOURNAL_BYTES:
                raise OSError("journal byte limit")
            if journal.write(data) != len(data):
                raise OSError("short journal write")
            journal.flush()
            journal_bytes += len(data)
        except BaseException as exc:
            journal_failed = True
            error("journal:" + kind, exc, "persistence")

    def persist(path, data, action):
        try:
            write_new(path, data)
            return True
        except BaseException as exc:
            error(action, exc, "persistence")
            return False

    def poll_parent():
        if proc is None:
            return None
        try:
            code = proc.poll()
            if code is not None:
                rec["returncode"] = code
                rec["parent_reaped"] = True
            return code
        except BaseException as exc:
            error("poll", exc)
            return None

    def kill_group(reason):
        if proc is None or not rec["spawn_succeeded"]:
            return
        event("kill_request", reason=reason, pgid=proc.pid)
        try:
            os.killpg(proc.pid, signal.SIGKILL)
            event("kill_return", result="signal_sent")
        except ProcessLookupError:
            event("kill_return", result="group_already_absent")
        except BaseException as exc:
            error("killpg:" + reason, exc)
            event("kill_error", action=reason, error_type=type(exc).__name__)

    def group_probe():
        if proc is None or not rec["spawn_succeeded"]:
            return "NOT_STARTED"
        try:
            os.killpg(proc.pid, 0)
            rec["group_state"] = "PRESENT"
        except ProcessLookupError:
            rec["group_state"] = "ABSENT"
            rec["group_absence_observed"] = True
        except BaseException as exc:
            rec["group_state"] = "UNVERIFIABLE"
            error("group_probe", exc)
        return rec["group_state"]

    def close_pipe(stream):
        if sel is not None:
            try:
                sel.unregister(stream)
            except KeyError:
                pass
            except BaseException as exc:
                error("selector_unregister", exc)
        try:
            stream.close()
        except BaseException as exc:
            error("pipe_close", exc)

    def selector_has_entries():
        if sel is None:
            return False
        try:
            return bool(sel.get_map())
        except BaseException as exc:
            error("selector_inventory", exc)
            return False

    def drain_once(wait):
        if sel is None:
            return
        try:
            ready = sel.select(timeout=max(0.0, min(wait, 0.05)))
        except BaseException as exc:
            error("selector_select", exc)
            return
        for key, _ in ready:
            label = key.data
            try:
                data = os.read(key.fileobj.fileno(), 8192)
            except BaseException as exc:
                rec["pipe_read_error"] = True
                error("pipe_read:" + label, exc, "capture")
                close_pipe(key.fileobj)
                continue
            if not data:
                rec[label + "_eof"] = True
                close_pipe(key.fileobj)
                continue
            remaining = max(0, stream_limit - len(buffers[label]))
            prefix_bytes = data[:remaining]
            buffers[label].extend(prefix_bytes)
            if prefix_bytes and not stream_write_failed[label]:
                try:
                    if outputs[label].write(prefix_bytes) != len(prefix_bytes):
                        raise OSError("short captured-prefix write")
                    outputs[label].flush()
                except BaseException as exc:
                    stream_write_failed[label] = True
                    error("captured_prefix:" + label, exc, "persistence")
            if len(data) > remaining:
                if not rec[label + "_truncated"]:
                    event("output_limit", stream=label)
                rec[label + "_truncated"] = True
                rec[label + "_extra_bytes_observed"] += len(data) - remaining
                if rec["first_problem"] is None:
                    rec["first_problem"] = {"category": "resource",
                                            "action": label + "_limit"}

    def child_limits():
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
        resource.setrlimit(resource.RLIMIT_FSIZE, (file_limit, file_limit))
        resource.setrlimit(resource.RLIMIT_CPU, (cpu_seconds, cpu_seconds))

    try:
        rec["argv_persisted"] = persist(
            paths["argv"], json_bytes({"argv": argv, "cwd": str(cwd),
            "deadline_seconds": seconds, "status": "PREPARED_NOT_YET_SPAWNED"}),
            "initial_argv")
        for label in ("stdout", "stderr"):
            try:
                outputs[label] = paths[label].open("xb")
            except BaseException as exc:
                stream_write_failed[label] = True
                error("open_capture:" + label, exc, "persistence")
        try:
            journal = paths["journal"].open("xb")
        except BaseException as exc:
            journal_failed = True
            error("open_journal", exc, "persistence")
        if not stop_requested and time.monotonic() < deadline:
            sel = selectors.DefaultSelector()
            event("spawn_attempt", argv=argv)
            if stop_requested:
                raise OSError("spawn intent could not be persisted")
            rec["spawn_attempted"] = True
            proc = subprocess.Popen(
                argv, cwd=str(cwd), stdin=subprocess.DEVNULL,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                start_new_session=True, preexec_fn=child_limits,
                env=os.environ.copy(),
            )
            rec["spawn_succeeded"] = True
            rec["pid"] = proc.pid
            rec["pgid"] = proc.pid
            event("spawned", pid=proc.pid, pgid=proc.pid)
            for label, stream in (("stdout", proc.stdout), ("stderr", proc.stderr)):
                os.set_blocking(stream.fileno(), False)
                sel.register(stream, selectors.EVENT_READ, label)
            while True:
                if time.monotonic() >= deadline:
                    rec["timed_out"] = True
                    if rec["first_problem"] is None:
                        rec["first_problem"] = {"category": "resource",
                                                "action": "deadline"}
                    event("deadline_reached")
                    break
                code = poll_parent()
                if stop_requested or rec["stdout_truncated"] or rec["stderr_truncated"]:
                    break
                if code is not None and not selector_has_entries():
                    break
                drain_once(0.05)
        elif not stop_requested:
            rec["timed_out"] = True
            rec["first_problem"] = {"category": "resource",
                                    "action": "deadline_before_spawn"}
    except BaseException as exc:
        body_error = True
        rec["interrupted"] = isinstance(exc, (KeyboardInterrupt, SystemExit))
        error("capture_body", exc, "capture")
        event("capture_exception", error_type=type(exc).__name__)
    finally:
        # Every finalizer action records its own error; none replaces an earlier
        # timeout/truncation/returncode or discards the in-memory actual prefix.
        cleanup_end = time.monotonic() + CLEANUP_SECONDS
        try:
            if proc is not None:
                code = poll_parent()
                must_kill = (code is None or stop_requested or body_error or
                             rec["timed_out"] or rec["stdout_truncated"] or
                             rec["stderr_truncated"])
                if must_kill:
                    kill_group("bounded_finalization")
                while selector_has_entries() and time.monotonic() < cleanup_end:
                    drain_once(min(0.05, cleanup_end - time.monotonic()))
                    if stop_requested and group_probe() == "UNVERIFIABLE":
                        break
                if not rec["parent_reaped"]:
                    try:
                        remaining = max(0.0, min(0.5, cleanup_end - time.monotonic()))
                        if remaining <= 0.0:
                            raise subprocess.TimeoutExpired(argv, remaining)
                        rec["returncode"] = proc.wait(timeout=remaining)
                        rec["parent_reaped"] = True
                        event("parent_reaped", returncode=rec["returncode"])
                    except BaseException as exc:
                        error("bounded_wait", exc)
                if rec["parent_reaped"]:
                    state = group_probe()
                    if state == "PRESENT":
                        rec["residual_group_observed"] = True
                        if rec["first_problem"] is None:
                            rec["first_problem"] = {"category": "group",
                                                    "action": "residual_launched_group"}
                        event("residual_group_after_parent_reap")
                        kill_group("residual_launched_group")
                        while time.monotonic() < cleanup_end:
                            if group_probe() != "PRESENT":
                                break
                            time.sleep(min(0.05, max(0.0, cleanup_end - time.monotonic())))
                    event("group_final", state=rec["group_state"],
                          residual_observed=rec["residual_group_observed"])
                else:
                    rec["unreaped"] = True
                    group_probe()
                    kill_group("unreaped_parent_final_request")
                if rec["returncode"] is not None and rec["returncode"] < 0:
                    rec["signal"] = -rec["returncode"]
        except BaseException as exc:
            error("group_finalization", exc)
            rec["group_state"] = "UNVERIFIABLE"
            rec["unreaped"] = bool(proc is not None and not rec["parent_reaped"])
        if sel is not None:
            try:
                remaining_streams = [key.fileobj for key in sel.get_map().values()]
            except BaseException as exc:
                remaining_streams = []
                error("selector_inventory_for_close", exc)
            for stream in remaining_streams:
                close_pipe(stream)
            try:
                sel.close()
            except BaseException as exc:
                error("selector_close", exc)
        # Popen may have produced a pipe before selector setup failed.
        if proc is not None:
            for stream in (proc.stdout, proc.stderr):
                if stream is not None and not stream.closed:
                    try:
                        stream.close()
                    except BaseException as exc:
                        error("unregistered_pipe_close", exc)
        for label in ("stdout", "stderr"):
            stream = outputs.get(label)
            complete = stream is not None and not stream_write_failed[label]
            if stream is not None:
                try:
                    stream.flush()
                    os.fsync(stream.fileno())
                except BaseException as exc:
                    complete = False
                    error("capture_flush:" + label, exc, "persistence")
                try:
                    stream.close()
                except BaseException as exc:
                    complete = False
                    error("capture_close:" + label, exc, "persistence")
            rec[label + "_primary_persisted"] = complete
            if not complete:
                fallback = prefix.with_name(prefix.name + "." + label + ".captured-prefix.bin")
                rec[label + "_fallback_persisted"] = persist(
                    fallback, bytes(buffers[label]), "fallback_prefix:" + label)
                rec[label + "_fallback_path"] = str(fallback)
        event("finalization", returncode=rec["returncode"],
              parent_reaped=rec["parent_reaped"], group=rec["group_state"],
              errors=len(rec["errors"]))
        if journal is not None:
            complete = not journal_failed
            try:
                journal.flush()
                os.fsync(journal.fileno())
            except BaseException as exc:
                complete = False
                error("journal_flush", exc, "persistence")
            try:
                journal.close()
            except BaseException as exc:
                complete = False
                error("journal_close", exc, "persistence")
            rec["journal_persisted"] = complete

    out, err = bytes(buffers["stdout"]), bytes(buffers["stderr"])
    rec.update({
        "wall_seconds": time.monotonic() - begun,
        "stdout_bytes": len(out), "stdout_sha256": sha256(out),
        "stderr_bytes": len(err), "stderr_sha256": sha256(err),
        "captured_prefix_hashes_are_not_claims_of_untruncated_output": True,
    })
    rec["capture_complete"] = (
        rec["stdout_eof"] and rec["stderr_eof"] and not rec["pipe_read_error"]
        and not rec["stdout_truncated"] and not rec["stderr_truncated"]
    )
    rec["process_completion"] = (
        "NOT_SPAWNED" if not rec["spawn_succeeded"] else
        "UNREAPED" if not rec["parent_reaped"] else
        "SIGNALED" if rec["signal"] is not None else "EXITED"
    )
    rec["process_healthy"] = (
        rec["spawn_succeeded"] and rec["parent_reaped"]
        and rec["returncode"] is not None and rec["returncode"] >= 0
        and rec["group_absence_observed"] and rec["group_state"] == "ABSENT"
        and not rec["residual_group_observed"] and not rec["timed_out"]
        and not rec["interrupted"] and not rec["errors"]
        and not rec["error_records_incomplete"] and rec["capture_complete"]
    )
    pre_status_persistence = (
        rec["argv_persisted"] and rec["journal_persisted"]
        and (rec["stdout_primary_persisted"] or rec["stdout_fallback_persisted"])
        and (rec["stderr_primary_persisted"] or rec["stderr_fallback_persisted"])
    )
    rec["persistence_pre_status_complete"] = pre_status_persistence
    # The status file does not certify its own successful write. The returned
    # record and enclosing terminal manifest record that later observation.
    status_ok = persist(paths["status"], json_bytes(rec), "final_status")
    rec["status_write_completed"] = status_ok
    rec["persistence_complete"] = pre_status_persistence and status_ok
    if not rec["persistence_complete"]:
        rec["process_healthy"] = False
        fallback_ok = persist(paths["incomplete"], json_bytes(rec),
                              "explicit_incomplete_outcome")
        rec["incomplete_outcome_persisted"] = fallback_ok
        rec["fallback_runner_notice"] = fallback_notice({
            "r1617": "EVIDENCE_INCOMPLETE_STOP", "argv": argv,
            "prefix": str(prefix), "returncode": rec["returncode"],
            "first_problem": rec["first_problem"],
            "incomplete_file_written": fallback_ok,
        })
    return {"record": rec, "stdout": out, "stderr": err}
