#!/usr/bin/env python3
"""R1129 C221c: Actions-only finite source/owned/C17/native evidence driver."""
import argparse
import datetime
import difflib
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
import traceback

from assess import MaterialFinding, native, owned
from instrument import ObservationLimit, instrument


def digest(data):
    return hashlib.sha256(data).hexdigest()


def stamp():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


class MechanicalFailure(RuntimeError):
    pass


class Driver:
    def __init__(self, args):
        self.args = args
        self.root = Path(args.root).resolve()
        self.out = Path(args.output).resolve()
        self.out.mkdir(parents=True, exist_ok=False)
        self.started = time.monotonic()
        self.deadline = self.started + 240
        self.sequence = 0
        self.summary = {
            "schema": "r1129-c221c-execution-receipt-v1",
            "provenance": "PROFILE-CALIBRATED / NON-BLIND to declared structural scope; implementation and fixtures unseen",
            "authority_comment": 6614995406,
            "target_p_head": "dc21cf33105a7dba832e197f24e153ad87862a0f",
            "target_p_tree": "34c85ebdebb0fe601d20856ad3f5f3a5446362e0",
            "canonical_main": "73ef10a97f2fab7b70d094b2114359e961acefb5",
            "started": stamp(), "status": "STARTED", "cases": {},
            "manual_runs": 0, "local_executor_runs": 0,
            "value": "UNMEASURED / GO624 PAUSED / H1 HOLD",
            "limitations": [
                "Finite selected source/control observations do not prove all-H, general termination or Product PASS.",
                "No universal public current-valid or source-to-C storage-map API is available.",
                "Owned public check and CLI emission are separate source-issued checks of exactly the same bytes.",
                "Unmodified C execution and passive live-storage instrumentation are distinct evidence.",
                "No P OOM/heap/CFG campaign is reproduced.",
            ],
        }
        for name in ("normal", "immediate_break", "equal_bits", "loan_escape",
                     "continue_type", "continue_arity", "continue_target", "v1_stutter"):
            self.summary["cases"][name] = {"status": "NOT_RUN"}

    def write(self, relative, data):
        path = self.out / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        if path.exists():
            raise MechanicalFailure("refusing to overwrite R raw: " + relative)
        path.write_bytes(data)
        return path

    def json(self, relative, value):
        return self.write(relative, (json.dumps(value, indent=2, sort_keys=True) + "\n").encode())

    def command(self, label, argv, timeout=30):
        self.sequence += 1
        remaining = self.deadline - time.monotonic()
        if remaining <= 0:
            raise MechanicalFailure("R driver 240-second total budget reached")
        limit = min(timeout, remaining)
        prefix = f"commands/{self.sequence:03d}-{label}"
        record = {
            "sequence": self.sequence, "label": label,
            "argv": [str(a) for a in argv], "cwd": str(self.out),
            "started": stamp(), "timeout_seconds": limit,
        }
        self.json(prefix + ".start.json", record)
        begin = time.monotonic()
        try:
            result = subprocess.run(record["argv"], cwd=self.out,
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                    timeout=limit, check=False)
            out, err, rc = result.stdout, result.stderr, result.returncode
            timed_out = False
        except subprocess.TimeoutExpired as exc:
            out, err, rc = exc.stdout or b"", exc.stderr or b"", None
            timed_out = True
        except OSError as exc:
            out, err, rc = b"", str(exc).encode(), None
            timed_out = False
            record["launch_error"] = type(exc).__name__
        self.write(prefix + ".stdout", out)
        self.write(prefix + ".stderr", err)
        record.update({
            "exit": rc, "timed_out": timed_out, "seconds": time.monotonic() - begin,
            "stdout_bytes": len(out), "stdout_sha256": digest(out),
            "stderr_bytes": len(err), "stderr_sha256": digest(err),
        })
        self.json(prefix + ".result.json", record)
        print("R1129_COMMAND " + json.dumps(record, sort_keys=True), flush=True)
        return record, out, err

    def success(self, result, purpose, material=True):
        record = result[0]
        if record["exit"] != 0 or record["timed_out"] or record.get("launch_error"):
            error = MaterialFinding if material else MechanicalFailure
            raise error(purpose + ": " + json.dumps(record, sort_keys=True))
        return result[1]

    def verify(self):
        if os.environ.get("GITHUB_ACTIONS") != "true":
            raise MechanicalFailure("R execution is authorized only in GitHub Actions")
        if os.environ.get("GITHUB_EVENT_NAME") != "pull_request":
            raise MechanicalFailure("semantic/native execution requires natural R pull_request CI")
        if os.environ.get("GITHUB_HEAD_REF") != "r-fast/r1129-c221-stutter":
            raise MechanicalFailure("unexpected R head ref")
        if os.environ.get("GITHUB_BASE_REF") != "p1123-c219-stuttering-loop-continue":
            raise MechanicalFailure("unexpected R base ref")
        manifest_path = self.root / "evidence/r1129/harness/manifest.json"
        manifest_bytes = manifest_path.read_bytes()
        manifest = json.loads(manifest_bytes)
        self.summary["harness_manifest_sha256"] = digest(manifest_bytes)
        verified = []
        for item in manifest["files"]:
            data = (self.root / item["path"]).read_bytes()
            if len(data) != item["bytes"] or digest(data) != item["sha256"]:
                raise MaterialFinding("immutable runtime byte/hash mismatch: " + item["path"])
            verified.append(item)
        self.json("identity/verified-files.json", verified)
        environment = {key: os.environ.get(key) for key in (
            "GITHUB_ACTIONS", "GITHUB_SHA", "GITHUB_REF", "GITHUB_HEAD_REF",
            "GITHUB_BASE_REF", "GITHUB_EVENT_NAME", "GITHUB_RUN_ID", "GITHUB_RUN_ATTEMPT",
            "GITHUB_JOB", "GITHUB_REPOSITORY", "CFLAGS", "ASAN_OPTIONS", "UBSAN_OPTIONS",
        )}
        self.summary["environment"] = environment
        self.summary["host"] = {
            "compiler": self.args.compiler, "compiler_id": self.args.compiler_id,
            "cmake_c_flags": self.args.host_cflags, "build_type": self.args.build_type,
            "newlang_sanitizer": self.args.sanitizer,
            "frontend_only": self.args.frontend_only,
            "python": sys.version, "executable": sys.executable,
        }
        for name, expression in (("head", "HEAD"), ("tree", "HEAD^{tree}")):
            result = self.command("git-" + name, ["git", "-C", self.root, "rev-parse", expression])
            value = self.success(result, "checkout metadata", False).decode().strip()
            self.summary["host"][name] = value
        if self.summary["host"]["head"] != environment["GITHUB_SHA"]:
            raise MaterialFinding("runtime checkout HEAD differs from Actions SHA")
        commit_object = self.success(self.command(
            "git-commit-object", ["git", "-C", self.root, "cat-file", "-p", "HEAD"]),
            "runtime commit object metadata", False).decode("utf-8")
        self.summary["host"]["parents"] = [line.split(" ", 1)[1]
            for line in commit_object.splitlines() if line.startswith("parent ")]
        tracked_status = self.success(self.command(
            "git-worktree", ["git", "-C", self.root, "status",
                            "--porcelain", "--untracked-files=no"]),
            "tracked tree metadata", False)
        if tracked_status.strip():
            raise MaterialFinding("tracked checkout changed before R execution")
        compiler_version = self.success(
            self.command("cc-version", [self.args.compiler, "--version"]), "C compiler version", False
        ).decode(errors="replace")
        self.summary["host"]["compiler_version"] = compiler_version
        self.success(self.command("newlang-version", [self.args.cli, "--version"]),
                     "public CLI version", False)
        for name in ("cli", "owned"):
            path = Path(getattr(self.args, name)).resolve()
            data = path.read_bytes()
            self.summary["host"][name] = {"path": str(path), "bytes": len(data),
                                         "sha256": digest(data)}
        guest = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
        if self.args.compiler_id == "Clang" and "23.1.3" in compiler_version:
            guest.append("-Wno-error=gcc-install-dir-libstdcxx")
        if self.args.sanitizer != "none":
            if self.args.sanitizer not in ("address", "undefined"):
                raise MechanicalFailure("unexpected sanitizer selection")
            guest += ["-fsanitize=" + self.args.sanitizer, "-fno-omit-frame-pointer",
                      "-fno-sanitize-recover=all"]
        self.guest_flags = guest
        self.summary["guest_flags"] = guest
        self.json("identity/runtime.json", self.summary.copy())

    def checked_records(self, result, name):
        data = self.success(result, "genuine public owned check: " + name)
        try:
            return [json.loads(line) for line in data.decode("utf-8").splitlines() if line]
        except (UnicodeError, ValueError) as exc:
            raise MechanicalFailure("public observer JSON protocol: " + str(exc)) from exc

    def compile_and_run(self, case, source, variant):
        results = []
        for optimization in ("O0", "O2"):
            executable = self.out / case / (variant + "." + optimization + ".exe")
            compiled = self.command(case + "-" + variant + "-" + optimization + "-compile",
                                    [self.args.compiler, *self.guest_flags, "-" + optimization,
                                     source, "-o", executable])
            self.success(compiled, "generated C compile: " + case + "/" + variant,
                         material=variant == "unmodified")
            data = executable.read_bytes()
            self.json(case + "/" + variant + "." + optimization + ".binary.json",
                      {"bytes": len(data), "sha256": digest(data),
                       "variant": variant, "optimization": optimization,
                       "sanitizer": self.args.sanitizer, "compiler": self.args.compiler,
                       "flags": self.guest_flags + ["-" + optimization]})
            ran = self.command(case + "-" + variant + "-" + optimization + "-run",
                               [executable], timeout=2)
            self.success(ran, "native execution: " + case + "/" + variant + "/" + optimization)
            results.append((optimization, ran))
        return results

    def primary(self, name):
        case = self.summary["cases"][name]
        source = self.root / ("evidence/r1129/v2/" + name + ".nl")
        case["status"] = "CHECK_STARTED"
        checked = self.command(name + "-check", [self.args.cli, "--check", source])
        self.success(checked, "primary source CHECK: " + name)
        case["check"] = "ACCEPTED"
        records = self.checked_records(self.command(name + "-owned", [self.args.owned, source]), name)
        assessment = owned(records, name)
        self.json(name + "/owned-assessment.json", assessment)
        case["owned"] = "SOURCE_ISSUED_PUBLIC_OBSERVATIONS"
        generated = self.out / name / "unmodified.c"
        generated.parent.mkdir(parents=True, exist_ok=True)
        emitted = self.command(name + "-emit", [self.args.cli, "--emit-c17-to", generated, source])
        self.success(emitted, "primary C17 emission: " + name)
        if not generated.is_file() or not generated.read_bytes():
            raise MaterialFinding("successful named C17 emission produced no bytes: " + name)
        original_bytes = generated.read_bytes()
        self.json(name + "/c17-identity.json", {
            "bytes": len(original_bytes), "sha256": digest(original_bytes),
            "source_sha256": digest(source.read_bytes()),
            "generated_file": str(generated),
            "same_owned_object_as_public_observer": False,
            "bridge": "same immutable complete source, independently genuine public/CLI checks",
        })
        self.compile_and_run(name, generated, "unmodified")
        case["unmodified_native"] = ["O0", "O2"]
        clang = shutil.which("clang-23") or shutil.which("clang")
        if not clang:
            raise ObservationLimit("Clang JSON AST interface unavailable; no guessed C storage map")
        clang_version = self.success(self.command(name + "-ast-version", [clang, "--version"]),
                                     "AST tool version", False).decode(errors="replace")
        ast_result = self.command(name + "-ast", [
            clang, "-std=c17", "-Xclang", "-ast-dump=json", "-fsyntax-only", generated,
        ])
        ast_bytes = self.success(ast_result, "passive C AST preparation", False)
        try:
            ast = json.loads(ast_bytes)
            code = original_bytes.decode("ascii")
            observed, mapping = instrument(code, ast, name, str(generated))
        except (UnicodeError, ValueError, KeyError, IndexError) as exc:
            raise ObservationLimit("passive AST/mapping interface: " + str(exc)) from exc
        mapping["ast_compiler"] = clang
        mapping["ast_compiler_version"] = clang_version
        mapping["original_sha256"] = digest(original_bytes)
        mapping["observed_sha256"] = digest(observed.encode("ascii"))
        observed_path = self.write(name + "/observed.c", observed.encode("ascii"))
        self.json(name + "/storage-map.json", mapping)
        difference = "".join(difflib.unified_diff(
            code.splitlines(True), observed.splitlines(True),
            fromfile="unmodified.c", tofile="observed.c",
        ))
        self.write(name + "/observer.diff", difference.encode())
        for optimization, ran in self.compile_and_run(name, observed_path, "observed"):
            trace = []
            try:
                for line in ran[1].decode("utf-8").splitlines():
                    if line.startswith("R1129_TRACE "):
                        trace.append(json.loads(line[len("R1129_TRACE "):]))
            except (UnicodeError, ValueError) as exc:
                raise MechanicalFailure("native observation JSON protocol: " + str(exc)) from exc
            result = native(trace, name, mapping)
            result["optimization"] = optimization
            result["sanitizer"] = self.args.sanitizer
            self.json(name + "/native-assessment-" + optimization + ".json", result)
        if generated.read_bytes() != original_bytes:
            raise MaterialFinding("unmodified C bytes changed during observation")
        case.update({"status": "FINITE_PRIMARY_OBSERVATIONS_MATCH",
                     "observed_native": ["O0", "O2"],
                     "public_limits": assessment["limits"],
                     "storage_map_limits": mapping["limits"]})

    def negative(self, name):
        case = self.summary["cases"][name]
        source = self.root / ("evidence/r1129/v2/" + name + ".nl")
        case["status"] = "CHECK_STARTED"
        result = self.command(name + "-check", [self.args.cli, "--check", source])
        if result[0]["exit"] == 0:
            if name == "equal_bits":
                raise MaterialFinding("equal-bits canonical-legal source accepted beyond frozen pure-C profile expectation; C review required, not automatically unsafe")
            raise MaterialFinding("illegal source accepted by ordinary CHECK: " + name)
        if result[0]["exit"] != 3 or result[0]["timed_out"]:
            raise MaterialFinding("focused probe failed outside ordinary source diagnostic status: " + name)
        public = self.command(name + "-owned", [self.args.owned, source])
        if public[0]["exit"] != 3 or public[0]["timed_out"]:
            raise MaterialFinding("public/CLI rejection disagreement or host failure: " + name)
        try:
            records = [json.loads(line) for line in public[1].decode().splitlines() if line]
        except (UnicodeError, ValueError) as exc:
            raise MechanicalFailure("negative public diagnostic protocol: " + str(exc)) from exc
        refusals = [r for r in records if r["event"] == "status" and r["status"] != 0]
        if not refusals:
            raise MechanicalFailure("negative public refusal lacks actual stage record")
        first = refusals[0]
        sentinel = ("R1129-V2:r1129/v2/" + name + ".nl:KEEP\n").encode()
        target = self.write(name + "/named-output.c", sentinel)
        emitted = self.command(name + "-emit-refusal",
                               [self.args.cli, "--emit-c17-to", target, source])
        if emitted[0]["exit"] != 3 or emitted[0]["timed_out"]:
            raise MaterialFinding("refused source named emission differs from CHECK: " + name)
        if target.read_bytes() != sentinel:
            raise MaterialFinding("named-output sentinel clobbered on source refusal: " + name)
        report = {
            "status": "REJECTED_AT_RECORDED_LAYER",
            "first_public_refusal": first,
            "cli_exit": result[0]["exit"], "owned_exit": public[0]["exit"],
            "named_emit_exit": emitted[0]["exit"],
            "sentinel_bytes": len(sentinel), "sentinel_sha256": digest(sentinel),
            "named_no_clobber": "EXACT_BYTES_PRESERVED",
            "targeted_negative_pass": False,
            "target_boundary": "UNCLASSIFIED_PENDING_RAW_REVIEW",
            "grading": "A profile/parser/registration refusal is not an unvisited loan/type/arity/target boundary PASS.",
            "canonical": "legal but non-stuttering" if name == "equal_bits" else "illegal",
        }
        self.json(name + "/diagnostic-assessment.json", report)
        case.update(report)

    def wider_v1(self):
        source = self.root / "evidence/r1129/v1/stutter.nl"
        result = self.command("v1-stutter-check", [self.args.cli, "--check", source])
        if result[0]["exit"] not in (0, 3) or result[0]["timed_out"]:
            raise MaterialFinding("wider V1 CHECK has non-diagnostic host failure")
        self.summary["cases"]["v1_stutter"] = {
            "status": "CHECK_ONLY", "exit": result[0]["exit"],
            "coverage": "Separate wider two-Continue/nested-IF question, outside declared C1+B1 profile.",
            "owned_c17_native": "NOT_RUN",
            "grading": "Scope refusal is not a primary V2 failure; acceptance is not native PASS.",
        }

    def finish(self, status, error=None):
        self.summary["status"] = status
        self.summary["finished"] = stamp()
        self.summary["seconds"] = time.monotonic() - self.started
        if error:
            self.summary["first_stop"] = {"type": type(error).__name__, "message": str(error)}
            self.json("FIRST.json", self.summary["first_stop"])
        self.json("summary.json", self.summary)
        print("R1129_FINAL " + json.dumps(self.summary, sort_keys=True), flush=True)


def main():
    parser = argparse.ArgumentParser()
    for name in ("root", "output", "cli", "owned", "compiler", "compiler-id",
                 "sanitizer", "build-type", "frontend-only"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--host-cflags", default="")
    args = parser.parse_args()
    driver = Driver(args)
    try:
        driver.verify()
        driver.primary("normal")
        driver.primary("immediate_break")
        for name in ("equal_bits", "loan_escape", "continue_type",
                     "continue_arity", "continue_target"):
            driver.negative(name)
        driver.wider_v1()
    except MaterialFinding as exc:
        driver.finish("MATERIAL_STOP_REVIEW_REQUIRED", exc)
        return 1
    except (ObservationLimit, MechanicalFailure) as exc:
        driver.finish("MECHANICAL_OR_OBSERVATION_LIMIT", exc)
        return 1
    except Exception as exc:
        driver.write("unexpected-driver-exception.txt", traceback.format_exc().encode())
        driver.finish("UNEXPECTED_DRIVER_FAILURE_REVIEW_REQUIRED", exc)
        return 1
    driver.finish("FINITE_R_OBSERVATIONS_COMPLETE")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
