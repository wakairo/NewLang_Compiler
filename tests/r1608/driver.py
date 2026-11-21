"""R1620 prospective retest v1. TEXT ONLY until a separate exact C release."""
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
import stat
import subprocess
import sys
import time

from rawio import (CASES, FIRST, FIRST_MANIFEST_SHA, PHASES, ROOTS,
                   STREAM_LIMIT, TOTAL_PUBLISH_BYTES, allowlist, canonical,
                   open_directory, read_regular_at, sha, write_new)

APPARATUS_ID = "R1620_RETEST_TEXT_V1"
ORIGINAL_HEAD = "947d1cdb6e26c4de23947939c66ccccc50273168"
ORIGINAL_TARGET = "3e979df92a8ff37d456896cbb761185f1725f871"
MAX_CHILDREN = 21
SUITE_SECONDS = 540
FINALIZATION_RESERVE = 12
CHILD_SECONDS = {"observer": 20, "check": 20, "emit": 20, "cc": 45, "native": 10}
WORK_NAME = "r1620-work-v1"
HARD_DEADLINE = None
CHECK = {0: "OK", 1: "SEMANTIC_ERROR", 2: "SEMANTIC_UNSUPPORTED",
         3: "ANALYSIS_PRECISION_LIMIT", 4: "OUT_OF_MEMORY",
         5: "RESOURCE_LIMIT", 6: "INTERNAL_ERROR"}
PARSE = {0: "OK", 1: "LEXICALLY_UNSUPPORTED", 2: "SYNTAX_UNSUPPORTED",
         3: "SYNTAX_ERROR", 4: "OUT_OF_MEMORY",
         5: "RESOURCE_LIMIT", 6: "INTERNAL_ERROR"}

# These 17 payloads must retain their original R1608 HEAD bytes.
IMMUTABLE_SHA = {
    "entry_adapter.nl": "a8b6c5ba40d39d59b1f8f55b823765e05fb67439986c02df6bcaee2310ddf791",
    "first/SHA256SUMS": "91f79b36bc923673a2291a23195b285b19e6857950629d36d0120b771d836032",
    "first/r1608_c1_selected_div_zero.nl": "9a668f0b475544e4e64262519916eb01115ae6d333c970c4ffc91beec6870d0b",
    "first/r1608_c2_selected_rem_zero.nl": "3edb789d72b295117f0e0500121a5e61cc79a10181c8b725fa13ce407e759d5a",
    "first/r1608_c3_unselected_unit_arm.nl": "8a35c6fac4b353c49bfd7590784fa48d56868480fe2b13392a6be8e1923a1670",
    "first/r1608_c4_hoisted_alternative.nl": "f0e172aec2fd66566d5c14ceeb3d59e755823f3d81fd54d3f4e428ce1f33d1f2",
    "first/r1608_oracle.md": "8e75b130bd39e4fcbcec78920fdf8437eb292a9f4c2b26c2a87d4e812efed60c",
    "first/r1608_p1_main_zero_then.nl": "01980812c162c599bbb1d95a148f9e5c6f3de02798550156e545f5138a61e948",
    "first/r1608_p2_helper_else.nl": "4123b20b911149ffaaaf21bbd9184fba333e806cd61e0db61769132cc461e548",
    "full_publisher.yml.txt": "b1427c009bda1d56c1c854e9c9e912645c249fe189cfd2ad88d0d7b8789b8e72",
    "observe.c": "18fb916e18408bed7c4dfc2158a914617089fc951fd9ef293bbb1663d2903586",
    "partial_publisher.yml.txt": "b79a072201620dda28e6d47b226f38a0b0bc4df5eed6d32b21d9c534cb8cf23c",
    "publish.py": "747234373ce58e430163b4d0468391fc9e8be65311a3eb551047585f97ba7529",
    "rawio.py": "f2d530446dfc065a59808d23e367b8f9b7d02420ecc26c2ed7e7583322ea2bb9",
    "recover.py": "64b15923c5839f848c08254fead0f7fc02e5903dff5d33ea1c0bd4cf585db31e",
    "registration.cmake": "604c585a7b9fbc289586a536cb8362b23d9717d561f2e13667a94cd1869364f9",
    "root_cmake_include.txt": "fc0e73f3140b770aeb5bf4e7eeacdd9f044e426696a638bbf2bfa532561bb3f4"
}
OWN_CAPS = {
    "driver.py": 131072, "PLAN.md": 65536, "READ_LEDGER.json": 65536,
    "CMakeLists.txt": 16384, "observe.c": 65536, "rawio.py": 16384,
    "publish.py": 16384, "recover.py": 32768, "registration.cmake": 8192,
    "entry_adapter.nl": 64, "root_cmake_include.txt": 1024,
    "full_publisher.yml.txt": 4096, "partial_publisher.yml.txt": 4096,
    "first/SHA256SUMS": 2048,
}
OWN_CAPS.update({"first/" + name: 32768 for name in FIRST})

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
    # Preserve sanitizer address spaces: no RLIMIT_AS.

def hard_alarm(_signum, _frame):
    # No catch, cleanup, print or buffered flush after the terminal deadline.
    os._exit(124)

def arm_hard_deadline():
    remaining = HARD_DEADLINE - time.monotonic()
    if remaining <= 0:
        hard_alarm(None, None)
    signal.signal(signal.SIGALRM, hard_alarm)
    signal.setitimer(signal.ITIMER_REAL, remaining)

def work_alarm(_signum, _frame):
    arm_hard_deadline()
    raise Stop("cohort_work_deadline_reserved_finalization")

def directory_key(st):
    if not stat.S_ISDIR(st.st_mode):
        raise Stop("directory_is_not_directory")
    return (st.st_dev, st.st_ino, st.st_mode, st.st_uid, st.st_gid)

def same_directory(parent_fd, name, fd, expected):
    opened = directory_key(os.fstat(fd))
    named = directory_key(os.stat(name, dir_fd=parent_fd, follow_symlinks=False))
    if opened != expected or named != expected:
        raise Stop("directory_identity_changed:" + name)

def names_at(fd, maximum):
    names = set()
    with os.scandir(fd) as entries:
        for entry in entries:
            names.add(entry.name)
            if len(names) > maximum:
                raise Stop("directory_entry_count_limit")
    return names

def require_absent(fd, name):
    try:
        os.stat(name, dir_fd=fd, follow_symlinks=False)
    except FileNotFoundError:
        return
    raise Stop("path_not_absent_without_following:" + name)

def group_absent(pgid):
    try:
        os.killpg(pgid, 0)
    except ProcessLookupError:
        return True
    return False

class Run:
    def __init__(self, args, workspace, workspace_fd, build_fd, root, raw_fd,
                 started):
        self.args, self.workspace = args, workspace
        self.workspace_fd, self.build_fd = workspace_fd, build_fd
        self.root, self.fd = root, raw_fd
        self.deadline = started + SUITE_SECONDS
        self.work_deadline = self.deadline - FINALIZATION_RESERVE
        self.finalizing = False
        self.children, self.raw_budget_used = 0, 0
        self.charged, self.sealed = {}, {}
        self.allowed = allowlist()
        self.active_pgid = None
        self.work_parent_fd, self.own_fd, self.first_fd = None, None, None
        self.work_cases, self.resets = {}, []
        self.work, self.current = None, None
        self.payloads = {}
        self.workspace_key = directory_key(os.fstat(workspace_fd))
        self.build_key = directory_key(os.fstat(build_fd))
        self.raw_key = directory_key(os.fstat(raw_fd))
        self.own_root = Path(__file__).absolute().parent

    def check_time(self):
        limit = self.deadline if self.finalizing else self.work_deadline
        if time.monotonic() >= limit:
            raise Stop("cohort_wall_budget_during_io_or_reset")

    def charge(self, name, amount):
        self.check_time()
        if name not in self.allowed or amount < 0:
            raise Stop("raw_charge_name_or_size")
        previous = self.charged.get(name, 0)
        if previous + amount > self.allowed[name]:
            raise Stop("raw_per_file_byte_limit:" + name)
        if self.raw_budget_used + amount > TOTAL_PUBLISH_BYTES:
            raise Stop("shared_raw_byte_budget")
        self.charged[name] = previous + amount
        self.raw_budget_used += amount

    def save(self, name, value):
        self.check_time()
        data = canonical(value)
        if name in self.sealed:
            raise Stop("attempt_to_rewrite_evidence:" + name)
        require_absent(self.fd, name)
        self.charge(name, len(data))
        write_new(self.fd, name, data)
        os.fsync(self.fd)
        actual, identity = read_regular_at(self.fd, name, self.allowed[name])
        if actual != data:
            raise Stop("saved_record_bytes_changed:" + name)
        self.sealed[name] = (len(actual), sha(actual), identity)

    def copy_new(self, name, data):
        require_absent(self.fd, name)
        self.charge(name, len(data))
        write_new(self.fd, name, data)
        os.fsync(self.fd)
        actual, identity = read_regular_at(self.fd, name, self.allowed[name])
        if actual != data:
            raise Stop("copied_input_bytes_changed:" + name)
        self.sealed[name] = (len(actual), sha(actual), identity)

    def read(self, name, cap=STREAM_LIMIT):
        self.check_time()
        return read_regular_at(self.fd, name, cap)[0]

    def prepare_payload_identity(self):
        self.check_time()
        self.own_fd = open_directory(self.own_root)
        self.own_key = directory_key(os.fstat(self.own_fd))
        self.first_fd = os.open("first", os.O_RDONLY | os.O_DIRECTORY |
                                os.O_NOFOLLOW | os.O_NONBLOCK,
                                dir_fd=self.own_fd)
        self.first_key = directory_key(os.fstat(self.first_fd))
        for name, cap in sorted(OWN_CAPS.items()):
            self.check_time()
            fd, leaf = ((self.first_fd, name[6:]) if name.startswith("first/")
                        else (self.own_fd, name))
            data, identity = read_regular_at(fd, leaf, cap)
            digest = sha(data)
            if name in IMMUTABLE_SHA and digest != IMMUTABLE_SHA[name]:
                raise Stop("immutable_original_payload_changed:" + name)
            self.payloads[name] = (len(data), digest, identity)
        self.payload_manifest = "".join(
            self.payloads[name][1] + "  tests/r1608/" + name + "\n"
            for name in sorted(self.payloads)).encode("ascii")
        self.payload_manifest_sha = sha(self.payload_manifest)

    def check_payloads(self):
        if not self.payloads:
            raise Stop("payload_identity_not_initialized")
        named = directory_key(os.stat(self.own_root, follow_symlinks=False))
        if named != self.own_key or directory_key(os.fstat(self.own_fd)) != self.own_key:
            raise Stop("own_payload_directory_changed")
        same_directory(self.own_fd, "first", self.first_fd, self.first_key)
        for name, expected in self.payloads.items():
            self.check_time()
            fd, leaf = ((self.first_fd, name[6:]) if name.startswith("first/")
                        else (self.own_fd, name))
            data, identity = read_regular_at(fd, leaf, OWN_CAPS[name])
            if (len(data), sha(data), identity) != expected:
                raise Stop("frozen_payload_changed:" + name)

    def audit_raw(self, new_names=()):
        self.check_time()
        present = names_at(self.fd, len(self.allowed))
        existing = set(self.sealed)
        if not existing <= present or present - existing - set(new_names):
            raise Stop("raw_evidence_missing_or_unexpected_entry")
        if not present <= set(self.allowed):
            raise Stop("raw_name_not_allowlisted")
        for name in sorted(present):
            self.check_time()
            data, identity = read_regular_at(self.fd, name, self.allowed[name])
            value = (len(data), sha(data), identity)
            if name in self.sealed:
                if value != self.sealed[name]:
                    raise Stop("earlier_raw_evidence_changed:" + name)
            else:
                already = self.charged.get(name, 0)
                if already > len(data):
                    raise Stop("captured_raw_file_shrank:" + name)
                self.charge(name, len(data) - already)
                self.sealed[name] = value

    def audit_work(self, new_binary_case=None):
        if self.work_parent_fd is None:
            return
        same_directory(self.build_fd, WORK_NAME, self.work_parent_fd,
                       self.work_parent_key)
        if names_at(self.work_parent_fd, len(CASES)) != set(self.work_cases):
            raise Stop("work_root_unexpected_entry")
        for cid, entry in self.work_cases.items():
            self.check_time()
            same_directory(self.work_parent_fd, cid, entry["fd"], entry["key"])
            present = names_at(entry["fd"], 1)
            permitted = set(entry["files"])
            binary_name = cid + ".native"
            if cid == new_binary_case:
                permitted.add(binary_name)
            if not set(entry["files"]) <= present or not present <= permitted:
                raise Stop("case_work_staging_or_changed_entry:" + cid)
            for name in present:
                data, identity = read_regular_at(entry["fd"], name, 32 * 1024 * 1024)
                value = (len(data), sha(data), identity)
                if name in entry["files"] and value != entry["files"][name]:
                    raise Stop("earlier_native_evidence_changed:" + cid)
                entry["files"][name] = value

    def audit(self, new_raw=(), new_binary_case=None):
        self.check_time()
        if directory_key(os.stat(self.workspace, follow_symlinks=False)) != self.workspace_key:
            raise Stop("workspace_directory_changed")
        if directory_key(os.fstat(self.workspace_fd)) != self.workspace_key:
            raise Stop("workspace_descriptor_changed")
        same_directory(self.workspace_fd, ROOTS[self.args.mode][0],
                       self.build_fd, self.build_key)
        same_directory(self.build_fd, ROOTS[self.args.mode][1], self.fd, self.raw_key)
        self.check_payloads()
        self.audit_raw(new_raw)
        self.audit_work(new_binary_case)
        self.check_time()

    def raw_receipt(self):
        rows = {name: {"bytes": value[0], "sha256": value[1]}
                for name, value in sorted(self.sealed.items())}
        return {"sealed_files": len(rows),
                "sealed_bytes": sum(row["bytes"] for row in rows.values()),
                "sealed_map_sha256": sha(canonical(rows)),
                "shared_bytes_charged": self.raw_budget_used}

    def prepare_work_root(self):
        self.check_time()
        os.mkdir(WORK_NAME, 0o700, dir_fd=self.build_fd)
        os.fsync(self.build_fd)
        self.work_parent_fd = os.open(
            WORK_NAME, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW | os.O_NONBLOCK,
            dir_fd=self.build_fd)
        self.work_parent_key = directory_key(os.fstat(self.work_parent_fd))
        if names_at(self.work_parent_fd, 0):
            raise Stop("new_work_root_not_empty")

    def enter_case(self, cid, predecessor):
        self.audit()
        if self.active_pgid is not None or cid in self.work_cases:
            raise Stop("case_reset_active_group_or_reuse")
        prior = self.raw_receipt()
        if predecessor is not None and predecessor + ".decision.json" not in self.sealed:
            raise Stop("previous_case_terminal_not_durable")
        os.mkdir(cid, 0o700, dir_fd=self.work_parent_fd)
        os.fsync(self.work_parent_fd)
        fd = os.open(cid, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW | os.O_NONBLOCK,
                     dir_fd=self.work_parent_fd)
        entry = {"fd": fd, "key": directory_key(os.fstat(fd)), "files": {}}
        self.work_cases[cid] = entry
        if names_at(fd, 0):
            raise Stop("new_case_work_not_empty")
        self.work = self.root.parent / WORK_NAME / cid
        self.current = cid
        self.audit()
        reset = {"case": cid, "cwd": str(self.work), "created_exclusively": True,
                 "empty_before_first_child": True, "predecessor": predecessor,
                 "predecessor_decision_sha256":
                     self.sealed[predecessor + ".decision.json"][1]
                     if predecessor is not None else None,
                 "shared_children_before": self.children,
                 "prior_evidence": prior, "same_shared_deadline": True,
                 "reset_completed_utc": utc()}
        self.resets.append(reset)
        return reset

    def child(self, cid, phase, argv):
        self.audit()
        if (self.finalizing or self.current != cid or self.work is None or
                self.active_pgid is not None):
            raise Stop("child_without_fresh_case_or_after_hard_stop")
        if self.children >= MAX_CHILDREN:
            raise Stop("cohort_child_budget")
        stem = cid + "." + phase
        for stream in ("stdout", "stderr"):
            require_absent(self.fd, stem + "." + stream + ".bin")
        self.children += 1
        started = time.monotonic()
        deadline = min(self.work_deadline, started + CHILD_SECONDS[phase])
        self.save(stem + ".start.json", {
            "attempted": True, "argv": argv, "cwd": str(self.work),
            "utc": utc(), "child_number": self.children,
            "timeout_seconds": CHILD_SECONDS[phase], "stdin": "DEVNULL", "shell": False,
            "apparatus_id": APPARATUS_ID, "case_reset": self.resets[-1],
            "environment_observed": {
                name: os.environ.get(name) for name in
                ("GITHUB_RUN_ID", "GITHUB_RUN_ATTEMPT", "GITHUB_SHA",
                 "GITHUB_JOB", "GITHUB_WORKFLOW", "CC", "CFLAGS",
                 "ASAN_OPTIONS", "UBSAN_OPTIONS",
                 "PYTHONDONTWRITEBYTECODE", "PYTHONHASHSEED", "LC_ALL")
            },
        })
        output, process = {}, None
        reason, returncode = None, None
        launched, reaped, pg_absent, cleanup_attempted = False, False, None, False
        sizes, seen = {"stdout": 0, "stderr": 0}, {"stdout": 0, "stderr": 0}
        selector = selectors.DefaultSelector()
        cleanup_deadline = None

        def fail(value):
            nonlocal reason
            reason = value if reason is None else reason + ":" + value

        def kill_group():
            nonlocal cleanup_attempted, cleanup_deadline
            cleanup_attempted = True
            if cleanup_deadline is None:
                cleanup_deadline = min(self.deadline, time.monotonic() + 2)
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            except OSError as error:
                fail("group_kill_error:" + type(error).__name__)

        try:
            for stream in ("stdout", "stderr"):
                name = stem + "." + stream + ".bin"
                output[stream] = os.fdopen(os.open(
                    name, os.O_WRONLY | os.O_CREAT | os.O_EXCL |
                    os.O_NOFOLLOW | os.O_NONBLOCK, 0o600, dir_fd=self.fd), "wb")
            try:
                process = subprocess.Popen(
                    argv, cwd=self.work, stdin=subprocess.DEVNULL,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    shell=False, start_new_session=True, preexec_fn=limits)
                launched = True
                self.active_pgid = process.pid
            except OSError as error:
                fail("launch_error:" + type(error).__name__ + ":" + str(error))
            if process is not None:
                for stream, pipe in (("stdout", process.stdout),
                                     ("stderr", process.stderr)):
                    os.set_blocking(pipe.fileno(), False)
                    selector.register(pipe, selectors.EVENT_READ, stream)
                while selector.get_map() or process.poll() is None:
                    now = time.monotonic()
                    if reason is None and now >= deadline:
                        fail("timeout")
                    if reason is not None and cleanup_deadline is None:
                        kill_group()
                    if cleanup_deadline is not None and now >= cleanup_deadline:
                        fail("drain_incomplete")
                        break
                    for key, _mask in selector.select(0.05):
                        data = os.read(key.fileobj.fileno(), 8192)
                        if not data:
                            selector.unregister(key.fileobj)
                            key.fileobj.close()
                            continue
                        stream = key.data
                        seen[stream] += len(data)
                        kept = data[:max(0, STREAM_LIMIT - sizes[stream])]
                        self.charge(stem + "." + stream + ".bin", len(kept))
                        output[stream].write(kept)
                        output[stream].flush()
                        sizes[stream] += len(kept)
                        if len(kept) != len(data) and reason is None:
                            fail(stream + "_limit")
        except BaseException as error:
            fail("capture_or_budget_error:" + type(error).__name__ + ":" + str(error))
            if process is not None:
                kill_group()
        finally:
            if process is not None:
                if process.poll() is None:
                    kill_group()
                wait_until = (cleanup_deadline if cleanup_deadline is not None
                              else min(self.deadline, time.monotonic() + 2))
                try:
                    returncode = process.wait(timeout=max(0, wait_until - time.monotonic()))
                    reaped = True
                except subprocess.TimeoutExpired:
                    fail("wait_timeout_unreaped")
                    kill_group()
                except OSError as error:
                    fail("wait_error:" + type(error).__name__)
                try:
                    pg_absent = group_absent(process.pid)
                    if not pg_absent:
                        fail("surviving_process_group")
                        kill_group()
                        while time.monotonic() < cleanup_deadline:
                            if group_absent(process.pid):
                                pg_absent = True
                                break
                            time.sleep(0.01)
                except OSError as error:
                    fail("group_probe_error:" + type(error).__name__)
                if pg_absent:
                    self.active_pgid = None
            for key in list(selector.get_map().values()):
                try:
                    selector.unregister(key.fileobj)
                    key.fileobj.close()
                except BaseException as error:
                    fail("pipe_close_error:" + type(error).__name__)
            selector.close()
            for file in output.values():
                try:
                    file.flush()
                    os.fsync(file.fileno())
                except BaseException as error:
                    fail("stream_finalize_error:" + type(error).__name__)
                finally:
                    try:
                        file.close()
                    except BaseException as error:
                        fail("stream_close_error:" + type(error).__name__)
        stdout = self.read(stem + ".stdout.bin")
        stderr = self.read(stem + ".stderr.bin")
        complete = (reason is None and launched and reaped and pg_absent is True and
                    returncode is not None and
                    len(stdout) == seen["stdout"] and len(stderr) == seen["stderr"])
        result = {
            "attempted": True, "launched": launched, "reaped": reaped,
            "returncode": returncode,
            "signal": -returncode if returncode is not None and returncode < 0 else None,
            "reason": reason, "complete": complete,
            "process_group_absent": pg_absent, "cleanup_attempted": cleanup_attempted,
            "utc": utc(), "elapsed_seconds": time.monotonic() - started,
            "streams": {
                "stdout": {"bytes": len(stdout), "sha256": sha(stdout),
                           "seen_bytes": seen["stdout"]},
                "stderr": {"bytes": len(stderr), "sha256": sha(stderr),
                           "seen_bytes": seen["stderr"]},
            },
        }
        self.save(stem + ".result.json", result)
        new_raw = [stem + ".stdout.bin", stem + ".stderr.bin"]
        if phase == "observer":
            new_raw.append(cid + ".observer.rendered")
        if phase == "emit":
            new_raw.append(cid + ".emitted.c")
        self.audit(new_raw, cid if phase == "cc" else None)
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
    return (result["complete"] and result["launched"] and result["reaped"] and
            result["process_group_absent"] is True and
            result["reason"] is None and result["signal"] is None and
            not result["cleanup_attempted"] and
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
    if invalid and cli[0]["returncode"] == 0:
        raise Stop("true_invalid_cli_check_accepted_unconditional_stop")
    if run.read(cid + ".source.nl", 4096) != before:
        raise Stop("source_changed_during_public_or_cli_check")
    decision = {
        "original_parse": PARSE[ps], "original_register": CHECK.get(cs, "NOT_RUN"),
        "adapter_parse": PARSE.get(status.get("adapter_parse"), "NOT_RUN"),
        "adapter_check": CHECK.get(status.get("adapter_check"), "NOT_RUN"),
        "runtime_internal_quantities_order": "NOT_OBSERVED",
        "whole_call_result_world_theorem": "NOT_OBSERVED",
        "native": "NOT_RUN", "source_legal_oracle_unchanged": True,
        "case_state": "OBSERVATION_INCOMPLETE", "capability": "NONPASS",
        "continuation": "NONE", "apparatus_id": APPARATUS_ID,
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
        if invalid and ps == 0 and cs == 1:
            decision.update(case_state="CONTROL_REJECTION_OBSERVED",
                            capability="NOT_INFERRED",
                            continuation="NORMAL_NEXT_DISTINCT_CASE_AFTER_RESET")
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
    run.audit()
    require_absent(run.fd, emitted.name)
    emit_result, emit_out, emit_err = run.child(cid, "emit", [
        args.newlangc, "--emit-c17-to", str(emitted), str(source)])
    if not healthy_child(emit_result):
        raise Stop("emission_resource_signal_timeout_or_sanitizer")
    if emit_result["returncode"] != 0:
        decision["classification"] = "CHECK_OK_EMISSION_NONZERO_UNCLASSIFIED"
        decision["emission_status"] = emit_result["returncode"]
        expected_stderr = (
            b"error(cli)[V1-BACKEND-UNSUPPORTED]: accepted program uses a construct "
            b"outside the V0/V1 Checked-C spine\nnote: " +
            os.fsencode(str(source)) + b"\n")
        # The only new case-terminal return. No exception is caught here.
        if (cid not in ("p1", "p2", "c4") or invalid or
                emit_result["returncode"] != 4 or emit_out or
                emit_err != expected_stderr):
            raise Stop("emission_nonzero_not_a_proved_healthy_refusal", decision)
        require_absent(run.fd, emitted.name)
        run.audit()
        decision.update(
            case_state="STOPPED_UNCLASSIFIED", capability="NONPASS",
            native="NOT_RUN",
            stop="Stop:emission_nonzero_not_a_proved_healthy_refusal",
            backend_health="UNPROVED",
            continuation="NEXT_DISTINCT_CASE_AFTER_RESET",
            continuation_state="AWAITING_DURABLE_TERMINAL_AND_FRESH_RESET",
            continuation_basis={
                "entire_stderr_exact": True, "stderr_bytes": len(emit_err),
                "stderr_sha256": sha(emit_err), "source_absolute_path": str(source),
                "exit4_no_signal_or_capture_event": True,
                "child_reaped": True, "process_group_absent": True,
                "emitted_path_absent_before_and_after_without_following": True,
                "no_staging_or_unexpected_raw_or_work_entry": True,
                "frozen_payload_manifest_sha256": run.payload_manifest_sha,
                "earlier_evidence_before_terminal": run.raw_receipt(),
                "health_contract": "NOT_ESTABLISHED_BY_PUBLIC_EXIT4_OR_TEXT",
            })
        return decision
    if emit_out or emit_err:
        raise Stop("successful_emission_unexpected_stdout_or_stderr")
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
    cc_result, cc_out, cc_err = run.child(cid, "cc", [
        args.cc, *flags, str(emitted), *ldflags, "-o", str(binary)])
    if not healthy_child(cc_result) or cc_result["returncode"] != 0:
        raise Stop("pristine_c17_host_compile_failed")
    if cc_out or cc_err:
        raise Stop("successful_host_compile_unexpected_stdout_or_stderr")
    work_fd = open_directory(run.work)
    try:
        binary_bytes, identity = read_regular_at(work_fd, binary.name, 32 * 1024 * 1024)
    finally:
        os.close(work_fd)
    if not binary_bytes or not identity[2] & 0o111:
        raise Stop("native_binary_empty_or_not_executable")
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
    decision.update(case_state="OBSERVATION_COMPLETE", capability="NOT_INFERRED",
                    continuation="NORMAL_NEXT_DISTINCT_CASE_AFTER_RESET")
    return decision

def decision_summary(decision, saved, digest):
    return {
        "classification": decision.get("classification"),
        "case_state": decision.get("case_state"),
        "capability": decision.get("capability", "NONPASS"),
        "native": decision.get("native"),
        "continuation": decision.get("continuation", "NONE"),
        "continuation_state": decision.get("continuation_state"),
        "decision_saved": saved, "decision_sha256": digest,
        "source_legal_oracle_unchanged": True,
    }

def main():
    global HARD_DEADLINE
    parser = argparse.ArgumentParser()
    for name in ("observer", "newlangc", "cc", "sanitizer", "mode", "target",
                 "cflags", "config-cflags", "ldflags", "config-ldflags"):
        parser.add_argument("--" + name, required=True)
    args = parser.parse_args()
    # A target-shaped string is not release authority; C's remote binding is required.
    if os.environ.get("GITHUB_ACTIONS") != "true":
        raise SystemExit("STOP: Actions-only environment required")
    if not re.fullmatch(r"[0-9a-f]{40}", args.target):
        raise SystemExit("STOP: future execution target NOT_RELEASED")
    if args.mode not in ROOTS or args.sanitizer not in ("none", "address", "undefined"):
        raise SystemExit("STOP: invalid mode or sanitizer")
    workspace_text = os.environ.get("GITHUB_WORKSPACE", "")
    if not os.path.isabs(workspace_text) or any(c in workspace_text for c in "\n\r\0"):
        raise SystemExit("STOP: invalid immutable absolute workspace path")
    if not all(os.path.isabs(p) for p in (args.observer, args.newlangc, args.cc)):
        raise SystemExit("STOP: tool paths must be absolute before fresh-case cwd")
    started = time.monotonic()
    HARD_DEADLINE = started + SUITE_SECONDS
    signal.signal(signal.SIGALRM, work_alarm)
    signal.setitimer(signal.ITIMER_REAL, SUITE_SECONDS - FINALIZATION_RESERVE)
    workspace = Path(workspace_text)
    workspace_fd = build_fd = raw_fd = None
    run = None
    decisions, saved_decisions, attempted = {}, set(), set()
    first_nonpass_case = None
    hard_stop = "INCOMPLETE:intended_completion_not_reached"
    current, predecessor = None, None
    procedure_completed = False
    coverage_status = "NOT_STARTED"
    finalization_errors = []
    try:
        workspace_fd = open_directory(workspace)
        build_fd = os.open(ROOTS[args.mode][0],
                           os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW | os.O_NONBLOCK,
                           dir_fd=workspace_fd)
        raw_name = ROOTS[args.mode][1]
        # Existing roots are never opened for writing, deleted or reused.
        os.mkdir(raw_name, 0o700, dir_fd=build_fd)
        os.fsync(build_fd)
        raw_fd = os.open(raw_name,
                         os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW | os.O_NONBLOCK,
                         dir_fd=build_fd)
        root = workspace.joinpath(*ROOTS[args.mode])
        run = Run(args, workspace, workspace_fd, build_fd, root, raw_fd, started)
        run.prepare_payload_identity()
        run.save("suite.start.json", {
            "utc": utc(), "apparatus_id": APPARATUS_ID,
            "apparatus_manifest_sha256": run.payload_manifest_sha,
            "apparatus_files": {name: {"bytes": value[0], "sha256": value[1]}
                                for name, value in sorted(run.payloads.items())},
            "target": args.target, "original_target": ORIGINAL_TARGET,
            "original_head": ORIGINAL_HEAD, "not_a_new_blind_first": True,
            "interface_base": "088ccdef7d18620f1d5dd698e46c1aae0613bf52",
            "source_law_base": "6287fcb4cb94df3da1c67f57d7d717b29c25ecf9",
            "canonical_main": "b217d83c0b4253abf4832d36916d2107f8ab205b",
            "first_manifest_sha256": FIRST_MANIFEST_SHA,
            "mode": args.mode, "max_children": MAX_CHILDREN,
            "suite_seconds": SUITE_SECONDS,
            "reserved_cleanup_finalization_seconds": FINALIZATION_RESERVE,
            "case_order": [x[0] for x in CASES], "work_root_name": WORK_NAME,
            "shared_raw_byte_limit": TOTAL_PUBLISH_BYTES,
            "actions": {key: os.environ.get(key) for key in
                        ("GITHUB_RUN_ID", "GITHUB_RUN_ATTEMPT", "GITHUB_SHA",
                         "GITHUB_JOB", "GITHUB_EVENT_NAME", "GITHUB_WORKFLOW")},
        })
        manifest, _identity = read_regular_at(run.first_fd, "SHA256SUMS", 2048)
        if sha(manifest) != FIRST_MANIFEST_SHA:
            raise Stop("FIRST_manifest_changed")
        run.copy_new("first.manifest", manifest)
        for name, digest in FIRST.items():
            run.check_time()
            data, _identity = read_regular_at(run.first_fd, name, 32768)
            if sha(data) != digest:
                raise Stop("FIRST_payload_changed:" + name)
            for cid, filename, _invalid, _number in CASES:
                if name == filename:
                    run.copy_new(cid + ".source.nl", data)
        adapter, _identity = read_regular_at(run.own_fd, "entry_adapter.nl", 64)
        if adapter != b"main()\n":
            raise Stop("entry_adapter_changed")
        run.copy_new("adapter.nl", adapter)
        run.prepare_work_root()
        for position, (cid, filename, invalid, numerator) in enumerate(CASES):
            current = cid
            reset = run.enter_case(cid, predecessor)
            attempted.add(cid)
            coverage_status = "PREFIX_IN_PROGRESS"
            decision = case(run, cid, filename, invalid, numerator, args)
            if decision.get("capability") == "NONPASS" and first_nonpass_case is None:
                first_nonpass_case = cid
            next_case = CASES[position + 1][0] if position + 1 < len(CASES) else None
            decision["next_case"] = next_case
            decision["reset_receipt_sha256"] = sha(canonical(reset))
            if decision.get("continuation") == "NEXT_DISTINCT_CASE_AFTER_RESET":
                decision["continuation_state"] = (
                    "AUTHORIZED_PENDING_NEXT_RESET" if next_case else "NO_NEXT_CASE")
            elif decision.get("continuation") == "NORMAL_NEXT_DISTINCT_CASE_AFTER_RESET":
                decision["continuation_state"] = (
                    "NORMAL_PENDING_NEXT_RESET" if next_case else "NO_NEXT_CASE")
            else:
                decision["continuation_state"] = "NOT_AUTHORIZED"
            decision["first_nonpass_case_latched"] = first_nonpass_case
            run.audit()
            decisions[cid] = decision
            run.save(cid + ".decision.json", decision)
            saved_decisions.add(cid)
            run.audit()
            # No exception handler surrounds this iteration or restarts a case.
            if decision.get("continuation") == "NEXT_DISTINCT_CASE_AFTER_RESET":
                if (decision.get("classification") != "CHECK_OK_EMISSION_NONZERO_UNCLASSIFIED" or
                        decision.get("case_state") != "STOPPED_UNCLASSIFIED" or
                        decision.get("capability") != "NONPASS" or
                        decision.get("native") != "NOT_RUN" or
                        cid not in ("p1", "p2", "c4")):
                    raise Stop("invalid_explicit_continuation_terminal")
            elif (decision.get("continuation") != "NORMAL_NEXT_DISTINCT_CASE_AFTER_RESET" or
                  decision.get("case_state") not in
                  ("OBSERVATION_COMPLETE", "CONTROL_REJECTION_OBSERVED")):
                raise Stop("recorded_boundary_does_not_authorize_next_case")
            predecessor = cid
        run.audit()
        if saved_decisions != {x[0] for x in CASES}:
            raise Stop("intended_case_terminal_set_incomplete")
        procedure_completed = True
        coverage_status = ("ALL_SIX_TERMINAL_WITH_NONPASS" if first_nonpass_case is not None
                           else "ALL_SIX_TERMINAL_NO_LATCHED_NONPASS")
        hard_stop = None
    except BaseException as error:
        hard_stop = type(error).__name__ + ":" + str(error)
        procedure_completed = False
        coverage_status = "PREFIX_ONLY_OR_SETUP_FAILED"
        if first_nonpass_case is None:
            first_nonpass_case = current
        if current is not None and current not in saved_decisions:
            detail = (error.detail if isinstance(error, Stop) and
                      error.detail is not None else decisions.get(current))
            if detail is not None:
                decisions[current] = dict(detail, hard_stop=hard_stop,
                                          capability="NONPASS", continuation="NONE",
                                          continuation_state="NOT_AUTHORIZED")
    finally:
        arm_hard_deadline()
        if run is not None:
            run.finalizing = True
            # Finalization attempts record the prefix; they never resume the case loop.
            for cid, _filename, _invalid, _number in CASES:
                if cid not in decisions:
                    decisions[cid] = {
                        "classification": "STARTED_INCOMPLETE_OR_STOP"
                                          if cid in attempted else "NOT_RUN",
                        "case_state": "INCOMPLETE" if cid in attempted else "NOT_RUN",
                        "capability": "NONPASS" if cid in attempted or cid == current
                                      else "NOT_ASSESSED",
                        "reason": hard_stop, "is_current_case": cid == current,
                        "native": "NOT_RUN_OR_SEE_PRESERVED_CHILD_START",
                        "source_legal_oracle_unchanged": True,
                        "continuation": "NONE", "continuation_state": "NOT_AUTHORIZED",
                    }
                if cid not in saved_decisions:
                    try:
                        run.save(cid + ".decision.json", decisions[cid])
                        saved_decisions.add(cid)
                    except BaseException as error:
                        finalization_errors.append(
                            cid + ".decision:" + type(error).__name__ + ":" + str(error))
            if run.active_pgid is not None:
                try:
                    os.killpg(run.active_pgid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                except OSError as error:
                    finalization_errors.append("active_group_cleanup:" + type(error).__name__)
                finalization_errors.append("group_not_proved_absent_at_suite_finalization")
            if finalization_errors:
                procedure_completed = False
                coverage_status = "PREFIX_ONLY_FINALIZATION_FAILED"
                hard_stop = hard_stop or "Stop:evidence_finalization_failed"
            result_nonpass = (hard_stop is not None or not procedure_completed or
                              first_nonpass_case is not None or bool(finalization_errors))
            summaries = {
                cid: decision_summary(
                    decision, cid in saved_decisions,
                    run.sealed[cid + ".decision.json"][1]
                    if cid in saved_decisions else None)
                for cid, decision in decisions.items()}
            try:
                run.save("suite.result.json", {
                    "utc": utc(), "apparatus_id": APPARATUS_ID,
                    "apparatus_manifest_sha256": getattr(run, "payload_manifest_sha", None),
                    "procedure_completed": procedure_completed,
                    "coverage_status": coverage_status,
                    "first_nonpass_case": first_nonpass_case,
                    "hard_stop": hard_stop,
                    "nonpass_latched": result_nonpass,
                    "capability": "NONPASS" if result_nonpass else "NOT_INFERRED",
                    "expected_driver_exit_if_finalization_completes": 1 if result_nonpass else 0,
                    "terminal_exit_authority": "ACTUAL_DRIVER_AND_CTEST_EXIT_REQUIRED",
                    "children_attempted": run.children, "decisions": summaries,
                    "case_resets": run.resets,
                    "raw_budget_charged_before_final_record": run.raw_budget_used,
                    "raw_evidence_before_final_record": run.raw_receipt(),
                    "finalization_errors": finalization_errors,
                    "whole_call_and_native_internal_values": "NOT_OBSERVED",
                    "capability_pass": "not_inferred",
                })
            except BaseException as error:
                finalization_errors.append(
                    "suite.result:" + type(error).__name__ + ":" + str(error))
                procedure_completed = False
                hard_stop = hard_stop or "Stop:suite_result_not_durable"
            close_fds = [entry["fd"] for entry in run.work_cases.values()]
            close_fds += [run.work_parent_fd, run.first_fd, run.own_fd]
        else:
            close_fds = []
        close_fds += [raw_fd, build_fd, workspace_fd]
        for fd in dict.fromkeys(x for x in close_fds if x is not None):
            try:
                os.close(fd)
            except OSError as error:
                finalization_errors.append("descriptor_close:" + type(error).__name__)
                procedure_completed = False
                hard_stop = hard_stop or "Stop:descriptor_finalization_failed"
    nonpass = (not procedure_completed or hard_stop is not None or
               first_nonpass_case is not None or bool(finalization_errors))
    if nonpass:
        print("R1620 NONPASS; preserved original case grade; first_nonpass=" +
              str(first_nonpass_case) + "; hard_stop=" + str(hard_stop) +
              "; finalization_errors=" + str(finalization_errors), flush=True)
        return 1
    print("R1620 finite observation procedure complete. Actual case scopes remain "
          "limited; CTest exit 0 is not compiler capability or Product PASS.", flush=True)
    # The terminal alarm stays armed through output and interpreter finalization.
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
