#!/usr/bin/env python3
"""Five bounded checkout/registration commands; CTest --show-only executes no test."""
import argparse
import json
import os
from pathlib import Path
import stat
import sys
import time

# Own module import must not create an untracked __pycache__ in the checkout.
sys.dont_write_bytecode = True
from capture import capture_process, fallback_notice, json_bytes, sha256, write_new

LIMIT = 8 * 1024 * 1024
WALL_SECONDS = 45.0
ENV = [
    "PYTHONDONTWRITEBYTECODE=1", "PYTHONHASHSEED=0", "LC_ALL=C",
    "ASAN_OPTIONS=detect_leaks=1:halt_on_error=1",
    "UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1",
]
CLEAN_POLICY = {
    "policy": "git status --porcelain must be exactly empty",
    "verified_empty_porcelain": True,
    "allowed_reported_tracked_or_untracked_paths": [],
    "scope": "this checkout at metadata time; Git-ignored outputs are outside porcelain",
}

class MetadataStop(Exception):
    pass

def unique_object(items):
    result = {}
    for key, value in items:
        if key in result:
            raise MetadataStop("duplicate JSON key")
        result[key] = value
    return result

def parsed(data):
    return json.loads(data, object_pairs_hook=unique_object)

def bounded_read(path, limit=LIMIT):
    st = path.lstat()
    if not stat.S_ISREG(st.st_mode) or st.st_size >= limit:
        raise MetadataStop("nonregular/oversized metadata input: " + str(path))
    with path.open("rb") as stream:
        value = stream.read(limit)
    if len(value) != st.st_size:
        raise MetadataStop("metadata input changed during read")
    return value

def exact_sha(value):
    return isinstance(value, str) and len(value) == 40 \
        and all(c in "0123456789abcdef" for c in value)

def identity_lines(value, count):
    if not value.endswith(b"\n") or len(value) > 8192:
        raise MetadataStop("identity output shape/size")
    lines = value.decode("ascii").splitlines()
    if len(lines) != count:
        raise MetadataStop("identity output line count")
    return lines

class Metadata:
    def __init__(self, args):
        self.args = args
        self.root = Path(args.root)
        self.build = Path(args.build)
        self.dest = Path(args.destination).absolute()
        self.dest_created = False
        self.begun = time.monotonic()
        self.deadline = self.begun + WALL_SECONDS
        self.attempts = []
        self.data = {}
        self.commands = []
        self.first_failure = None
        self.evidence_incomplete = False
        self.persistence_errors = []
        self.active = "configuration"
        self.clean_verified = False

    def clock_gate(self):
        if time.monotonic() >= self.deadline:
            raise MetadataStop("periodic 45s metadata clock exhausted")

    def prepare(self):
        self.root = self.root.resolve(strict=True)
        self.build = self.build.resolve(strict=True)
        if os.environ.get("GITHUB_ACTIONS") != "true":
            raise MetadataStop("metadata is released only for GitHub Actions")
        expected_dest = Path(os.environ["RUNNER_TEMP"]).absolute() / "r1617-metadata"
        if self.dest != expected_dest or self.dest == self.root or self.root in self.dest.parents:
            raise MetadataStop("metadata destination must be the own RUNNER_TEMP directory")
        if self.dest.exists() or self.dest.is_symlink():
            raise MetadataStop("metadata destination exists; no clobber")
        self.dest.mkdir(parents=True, exist_ok=False)
        self.dest_created = True
        self.commands = [
            ("head", ["git", "rev-parse", "HEAD"]),
            ("tree-parents", ["git", "show", "-s", "--format=%T%n%P", "HEAD"]),
            ("candidate-parent", ["git", "show", "-s", "--format=%H%n%T%n%P", "HEAD^1"]),
            ("worktree", ["git", "status", "--porcelain"]),
            ("ctest-registration", ["ctest", "--test-dir", str(self.build),
                                    "--show-only=json-v1"]),
        ]

    def run(self):
        for name, argv in self.commands:
            self.active = name
            self.clock_gate()
            result = capture_process(
                argv, self.root, self.dest / name, 8.0, self.deadline, LIMIT, LIMIT, 10,
            )
            rec, stdout, stderr = result["record"], result["stdout"], result["stderr"]
            rec.update({"name": name, "ordinal": len(self.attempts) + 1, "not_retried": True})
            self.attempts.append(rec)
            if not rec["persistence_complete"]:
                self.evidence_incomplete = True
            if not rec["process_healthy"] or not rec["persistence_complete"] \
                    or rec["returncode"] != 0 or stderr \
                    or len(stdout) >= LIMIT or len(stderr) >= LIMIT:
                raise MetadataStop("command process/stream/group/persistence failure: " + name)
            self.data[name] = stdout
            # Validate each identity before another command can be launched.
            if name == "head":
                lines = identity_lines(stdout, 1)
                if not exact_sha(lines[0]):
                    raise MetadataStop("malformed exact HEAD")
            elif name == "tree-parents":
                lines = identity_lines(stdout, 2)
                if not exact_sha(lines[0]) or any(not exact_sha(x) for x in lines[1].split()):
                    raise MetadataStop("malformed checkout tree/parents")
            elif name == "candidate-parent":
                lines = identity_lines(stdout, 3)
                if any(not exact_sha(x) for x in lines[:2]) \
                        or any(not exact_sha(x) for x in lines[2].split()):
                    raise MetadataStop("malformed candidate commit/tree/parents")
            elif name == "worktree":
                if stdout != b"":
                    raise MetadataStop("dirty checkout: no reported tracked/untracked path is allowed")
                self.clean_verified = True
            self.clock_gate()
        if len(self.attempts) != 5 or not self.clean_verified:
            raise MetadataStop("incomplete five-command metadata/clean-worktree gate")
        self.active = "metadata-validation"
        inventory = parsed(self.data["ctest-registration"].decode("utf-8"))
        found = [test for test in inventory["tests"]
                 if test.get("name") == "r1617_multiarg.integration"]
        if len(found) != 1:
            raise MetadataStop("own CTest is not present exactly once")
        test = found[0]
        properties = test.get("properties", [])
        props = {prop["name"]: prop["value"] for prop in properties}
        if len(props) != len(properties):
            raise MetadataStop("duplicate CTest properties")
        expected_bytes = bounded_read(self.build / "r1617-expected-command.txt")
        expected = expected_bytes.decode("utf-8").splitlines()
        if test.get("command") != expected or props.get("TIMEOUT") != 420.0 \
                or props.get("RUN_SERIAL") is not True \
                or sorted(props.get("ENVIRONMENT", [])) != sorted(ENV):
            raise MetadataStop("generated own timeout/ENV/argv mismatch")
        write_new(self.dest / "expected-command.txt", expected_bytes)
        head = identity_lines(self.data["head"], 1)[0]
        tree_lines = identity_lines(self.data["tree-parents"], 2)
        candidate_lines = identity_lines(self.data["candidate-parent"], 3)
        selected_env = {name: os.environ.get(name) for name in (
            "GITHUB_REPOSITORY", "GITHUB_SHA", "GITHUB_REF", "GITHUB_HEAD_REF",
            "GITHUB_BASE_REF", "GITHUB_EVENT_NAME", "GITHUB_RUN_ID",
            "GITHUB_RUN_ATTEMPT", "GITHUB_JOB", "RUNNER_OS", "RUNNER_ARCH",
        )}
        if selected_env["GITHUB_REPOSITORY"] != "wakairo/NewLang_Compiler" \
                or selected_env["GITHUB_SHA"] != head:
            raise MetadataStop("repository/event checkout identity mismatch")
        event_bytes = bounded_read(Path(os.environ["GITHUB_EVENT_PATH"]))
        event = parsed(event_bytes.decode("utf-8"))
        pull = event.get("pull_request", {})
        pr_event = {"number": event.get("number"),
                    "head_sha": pull.get("head", {}).get("sha"),
                    "base_sha": pull.get("base", {}).get("sha")}
        self.clock_gate()
        result = {
            "schema": "r1617-checkout-registration-v2",
            "procedure_state": "COMPLETED_METADATA_ONLY",
            "label": self.args.label, "environment": selected_env,
            "checkout_commit": head, "checkout_tree": tree_lines[0],
            "checkout_parents": tree_lines[1].split(), "pr_event": pr_event,
            "candidate_parent": {"commit": candidate_lines[0],
                                 "tree": candidate_lines[1],
                                 "parents": candidate_lines[2].split()},
            "worktree_porcelain_hex": self.data["worktree"].hex(),
            "clean_worktree": CLEAN_POLICY,
            "registered_own_test": test, "registered_own_test_count": 1,
            "expected_command_sha256": sha256(expected_bytes),
            "ctest_semantic_native_executions": 0,
            "command_attempts": len(self.attempts),
            "spawned_direct_children": sum(int(r["spawn_succeeded"]) for r in self.attempts),
            "maximum_direct_children": 5, "all_commands_healthy": True,
            "commands": self.attempts,
            "periodic_clock_limit_seconds": WALL_SECONDS,
            "clock_scope": "cooperative polls/gates, not continuously timed hashing/traversal/writes",
            "outer_bounds": "FAST metadata step60s; FULL/PARTIAL existing build-step/job bounds",
            "group_scope_limit": "launched PGID only; escaped descendants are not excluded",
            "capability_pass": False,
            "numeric_job_id_and_full_steps":
                "REQUIRED_FROM_LATER_APPROVED_ATTEMPT_JOBS_API_ALL_PAGES",
            "metadata_write_self_confirmation": "runner completion receipt required",
        }
        write_new(self.dest / "metadata.json", json_bytes(result))
        self.clock_gate()
        print(json.dumps({
            "r1617_registration": "CONFIRMED_ONCE", "metadata_write_completed": True,
            "checkout": head, "checkout_tree": tree_lines[0],
            "checkout_parents": tree_lines[1].split(),
            "clean_worktree": True, "own_test": test, "timeout": 420,
            "semantic_native_tests_executed_by_this_step": 0,
            "capability_pass": False,
        }), flush=True)

    def stop(self, exc):
        self.first_failure = self.first_failure or {
            "command_or_phase": self.active, "type": type(exc).__name__,
            "message": str(exc)[:2048],
        }
        attempted_names = {record["name"] for record in self.attempts}
        outcome = {
            "schema": "r1617-metadata-stop-v2", "procedure_state": "STOP",
            "first_failure": self.first_failure, "commands": self.attempts,
            "not_run": [name for name, _ in self.commands if name not in attempted_names],
            "clean_worktree_verified": self.clean_verified,
            "evidence_incomplete": self.evidence_incomplete,
            "persistence_errors": self.persistence_errors,
            "capability_pass": False, "ctest_semantic_native_executions": 0,
            "group_scope_limit": "launched PGID only; escaped descendants not excluded",
            "clock_limit_scope": "45s periodic; external workflow/step bound is separate",
        }
        persisted = False
        try:
            if not self.dest_created:
                raise OSError("fresh metadata destination unavailable; no overwrite")
            write_new(self.dest / "metadata-stop.json", json_bytes(outcome))
            persisted = True
        except BaseException as write_exc:
            self.evidence_incomplete = True
            self.persistence_errors.append({
                "action": "metadata_stop_persistence", "type": type(write_exc).__name__,
                "message": str(write_exc)[:1024],
            })
            outcome["evidence_incomplete"] = True
            try:
                if self.dest_created:
                    write_new(self.dest / "metadata-incomplete.json", json_bytes(outcome))
            except BaseException as fallback_exc:
                self.persistence_errors.append({
                    "action": "metadata_incomplete_persistence",
                    "type": type(fallback_exc).__name__, "message": str(fallback_exc)[:1024],
                })
        fallback_notice({
            "r1617_metadata": "STOP", "first_failure": self.first_failure,
            "stop_record_persisted": persisted,
            "evidence_incomplete": self.evidence_incomplete,
            "persistence_errors": self.persistence_errors,
            "destination": str(self.dest),
        })

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True)
    ap.add_argument("--build", required=True)
    ap.add_argument("--destination", required=True)
    ap.add_argument("--label", required=True)
    args = ap.parse_args()
    metadata = Metadata(args)
    try:
        metadata.prepare()
        metadata.run()
    except BaseException as exc:
        metadata.stop(exc)
        return 1
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
