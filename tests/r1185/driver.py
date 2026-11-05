"""R1185 Actions-only measurement driver; no local execution is authorized.

The observer receives actual C/AST only. Frozen oracle comparison is a separate
later command. Preserve every first result and never repair guest input here.
"""
from __future__ import annotations

import datetime
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time
import traceback


ORACLE_SHA256 = "29410e20b599aa8ddb243c1dd20895692e2635b32366ebdd6343a3bd45633801"
MAX_NATIVE_RUNS = 84


def digest(data):
    return hashlib.sha256(data).hexdigest()


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds")


def save_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + "\n")


class MaterialStop(RuntimeError):
    pass


class EquipmentError(RuntimeError):
    pass


class Commands:
    def __init__(self, root):
        self.root = root
        self.records = []
        self.native_runs = 0

    def run(self, name, argv, cwd, *, timeout=60, native=False):
        if native:
            if self.native_runs >= MAX_NATIVE_RUNS:
                raise MaterialStop("native execution count would exceed per-job budget")
            self.native_runs += 1
        directory = self.root / "commands" / name
        directory.mkdir(parents=True, exist_ok=False)
        argv = [str(x) for x in argv]
        record = {"name": name, "argv": argv, "cwd": str(cwd),
                  "started_utc": utc(), "native": native,
                  "environment": {k: os.environ.get(k) for k in (
                      "ASAN_OPTIONS", "UBSAN_OPTIONS", "CC", "CFLAGS",
                      "GITHUB_SHA", "GITHUB_HEAD_REF", "GITHUB_RUN_ID",
                      "GITHUB_RUN_ATTEMPT", "GITHUB_EVENT_NAME")},
                  "timeout_seconds": timeout}
        save_json(directory / "argv.json", record)
        start = time.monotonic()
        try:
            result = subprocess.run(argv, cwd=cwd, capture_output=True,
                                    timeout=timeout, check=False)
            stdout, stderr, rc = result.stdout, result.stderr, result.returncode
            timed_out = False
        except subprocess.TimeoutExpired as error:
            stdout, stderr = error.stdout or b"", error.stderr or b""
            rc, timed_out = None, True
        except OSError as error:
            stdout, stderr = b"", str(error).encode()
            rc, timed_out = None, False
        (directory / "stdout.bin").write_bytes(stdout)
        (directory / "stderr.bin").write_bytes(stderr)
        record.update({"completed_utc": utc(), "seconds": time.monotonic() - start,
                       "rc": rc, "timed_out": timed_out,
                       "stdout_bytes": len(stdout), "stderr_bytes": len(stderr),
                       "stdout_sha256": digest(stdout), "stderr_sha256": digest(stderr)})
        save_json(directory / "result.json", record)
        self.records.append(record)
        return {"record": record, "rc": rc, "stdout": stdout,
                "stderr": stderr, "directory": directory}


def verify_frozen(cohort):
    oracle_bytes = (cohort / "oracle.json").read_bytes()
    if digest(oracle_bytes) != ORACLE_SHA256:
        raise MaterialStop("frozen V2 oracle bytes changed")
    listed = set()
    for line in (cohort / "MANIFEST.sha256").read_text().splitlines():
        expected, count, name = line.split("\t", 2)
        if name in listed:
            raise MaterialStop("duplicate frozen manifest entry")
        listed.add(name)
        data = (cohort / name).read_bytes()
        if len(data) != int(count) or digest(data) != expected:
            raise MaterialStop("frozen V2 manifest mismatch: " + name)
    actual = {str(p.relative_to(cohort)) for p in cohort.rglob("*") if p.is_file()}
    if listed != actual - {"MANIFEST.sha256"}:
        raise MaterialStop("frozen cohort member inventory changed")
    oracle = json.loads(oracle_bytes)
    if len(oracle["cases"]) != 15:
        raise MaterialStop("frozen cohort count changed")
    return oracle


def first_public_barrier(raw):
    stages = []
    for line in raw.decode("utf-8", errors="replace").splitlines():
        try:
            item = json.loads(line)
        except ValueError:
            continue
        if item.get("record") == "stage":
            stages.append(item)
    return next((x for x in stages if x["status"] != 0), None)


def executable_receipt(path):
    data = Path(path).read_bytes()
    return {"path": str(path), "bytes": len(data), "sha256": digest(data)}


def reject_abnormal_host(result, label):
    diagnostics = result["stdout"] + result["stderr"]
    sanitizer = re.search(rb"ERROR: AddressSanitizer|SUMMARY: AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:", diagnostics)
    if (result["rc"] is not None and result["rc"] < 0) or sanitizer:
        raise MaterialStop("abnormal host/sanitizer result is not a source rejection: " + label)


def main():
    if len(sys.argv) != 8:
        raise SystemExit("driver.py COMPILER PUBLIC_PROBE HOST_CC HOST_SAN OWN OUT FIXED_RECEIPT")
    compiler, public, host_cc = map(Path, sys.argv[1:4])
    host_sanitizer = sys.argv[4]
    own, out, fixed_receipt = map(Path, sys.argv[5:8])
    own, out = own.resolve(), out.resolve()
    source_root = own.parents[1]
    if os.environ.get("GITHUB_ACTIONS") != "true":
        raise SystemExit("R1185 requires authorized GitHub Actions; local execution forbidden")
    binding = json.loads((own / "target.json").read_text())
    if not binding.get("execution_released"):
        raise SystemExit("C execution/publication STOP; no released target")
    out.mkdir(parents=True, exist_ok=False)
    commands = Commands(out)
    summary = {"schema": "R1185_ACTUAL_MEASUREMENT_V1", "started_utc": utc(),
               "target_binding": binding, "host_compiler": str(host_cc),
               "host_sanitizer": host_sanitizer,
               "independence": "pre-execution frozen independent V1 / scope-calibrated V2; C-supplied finite-profile boundaries",
               "whole_loop_validator": "NOT_RUN/NO_APPLICABLE_PUBLIC_API",
               "cases": {}, "equipment_errors": [], "status": "RUNNING",
               "machine_CALL": "NOT_MEASURED; semantic and C call flow are separate",
               "machine_STORE": "NOT_MEASURED; passive markers identify original-C store boundaries, not machine instruction survival",
               "binary_retention": "Native binaries hashed before execution; transient binaries excluded from text/log bundle"}
    result_code = 1
    try:
        oracle = verify_frozen(own / "cohort")
        shutil.copytree(own / "cohort", out / "frozen-V2")
        shutil.copyfile(fixed_receipt, out / "fixed-context.json")
        (out / "ordinary-main-entry.txt").write_text("main()")
        save_json(out / "executables.json", {
            "compiler": executable_receipt(compiler),
            "public_probe": executable_receipt(public)})
        metadata = {}
        for name, arguments in (
                ("head", ["rev-parse", "HEAD"]),
                ("tree", ["rev-parse", "HEAD^{tree}"]),
                ("parents", ["show", "--no-patch", "--format=%P", "HEAD"]),
                ("index", ["ls-files", "--stage", "-z"]),
                ("status", ["status", "--porcelain=v1", "--untracked-files=all"])):
            observed = commands.run("git-" + name, ["git", "-C", source_root, *arguments], source_root)
            if observed["rc"] != 0:
                raise EquipmentError("actual checkout metadata unavailable: " + name)
            metadata[name] = observed["stdout"].decode("utf-8", errors="replace")
        save_json(out / "actual-checkout.json", metadata)
        help_result = commands.run("compiler-help", [compiler, "--help"], source_root)
        help_text = (help_result["stdout"] + help_result["stderr"]).decode("utf-8", errors="replace")
        emit_option = None
        for candidate in ("--emit-c17", "--emit-c"):
            if re.search(re.escape(candidate) + r"(?=\s|[=,\]\)]|$)", help_text):
                emit_option = candidate
                break
        if emit_option is None:
            raise EquipmentError("ordinary CLI C17 option not discoverable from actual --help")
        summary["ordinary_c17_option"] = emit_option
        cases = oracle["cases"]
        invalid = {c["id"] for c in cases if c["canonical_legality"] == "INVALID"}
        positives = [c for c in cases if c["kind"] in ("positive", "ordinary_control")]
        for case in cases:
            ident = case["id"]
            source = own / "cohort" / case["source"]
            check = commands.run(ident + "-check", [compiler, "--check", source], source_root)
            reject_abnormal_host(check, ident + " CHECK")
            summary["cases"][ident] = {"kind": case["kind"],
                "canonical_legality": case["canonical_legality"],
                "source_sha256": digest(source.read_bytes()),
                "check": check["record"], "public_owned": "NOT_RUN",
                "C17": "NOT_RUN", "native": [], "comparisons": [],
                "predicted_first_layer": case["predicted_first_layer"],
                "target_reach": "UNDETERMINED; early rejection never proves deeper target"}
            if ident in invalid and check["rc"] == 0:
                raise MaterialStop("invalid source accepted by ordinary CHECK: " + ident)
            if case in positives and check["rc"] != 0:
                raise MaterialStop("frozen positive/control CHECK did not accept: " + ident)
            if check["rc"] is None:
                raise EquipmentError("CHECK process failed before a language result: " + ident)
        for case in cases:
            ident = case["id"]
            state = summary["cases"][ident]
            source = own / "cohort" / case["source"]
            owned = commands.run(ident + "-public", [public, source], source_root)
            reject_abnormal_host(owned, ident + " public probe")
            state["public_owned"] = owned["record"]
            state["public_first_barrier"] = first_public_barrier(owned["stdout"])
            if ident in invalid and owned["rc"] == 0:
                raise MaterialStop("invalid source acquired ordinary source-issued owned main: " + ident)
            if case in positives and owned["rc"] != 0:
                raise MaterialStop("positive/control public source-issued main path failed: " + ident)
            if owned["rc"] in (64, 70, 74, None):
                raise EquipmentError("public probe equipment/IO failure: " + ident)
            if owned["rc"] not in (0, 20, 21, 22):
                raise MaterialStop("unknown public probe exit is not a source rejection: " + ident)
        gcc = shutil.which("gcc-13") or shutil.which("gcc")
        clang = shutil.which("clang-23") or shutil.which("clang")
        if not gcc or not clang:
            raise EquipmentError("required installed GCC and Clang guest tools unavailable")
        for label, executable in (("gcc", gcc), ("clang", clang)):
            version = commands.run(label + "-version", [executable, "--version"], source_root)
            if version["rc"] != 0:
                raise EquipmentError("guest compiler version command failed")
        modes = [
            ("gcc-O0", gcc, ["-O0"]), ("gcc-O2", gcc, ["-O2"]),
            ("clang-O0", clang, ["-O0"]), ("clang-O2", clang, ["-O2"]),
            ("guest-ASan", clang, ["-O1", "-g", "-fsanitize=address", "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]),
            ("guest-UBSan", clang, ["-O1", "-g", "-fsanitize=undefined", "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"])]
        strict = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
        summary["guest_modes"] = [{"name": n, "compiler": c, "flags": strict + f} for n, c, f in modes]
        with tempfile.TemporaryDirectory(prefix="r1185-native-", dir=out.parent) as temporary:
            binary_root = Path(temporary)
            for case in cases:
                ident = case["id"]
                state = summary["cases"][ident]
                if ident in invalid or state["check"]["rc"] != 0 or state["public_owned"]["rc"] != 0:
                    continue
                source = own / "cohort" / case["source"]
                emitted = commands.run(ident + "-emit", [compiler, emit_option, source], source_root)
                state["C17"] = emitted["record"]
                if case not in positives:
                    state["coverage_probe"] = "legal accepted/refused at actual layer; not invalid-source safety result"
                    continue
                if emitted["rc"] != 0 or not emitted["stdout"]:
                    raise MaterialStop("positive/control ordinary C17 emission failed: " + ident)
                case_root = out / "cases" / ident
                case_root.mkdir(parents=True)
                original_c = case_root / "original.c"
                original_c.write_bytes(emitted["stdout"])
                ast_result = commands.run(ident + "-actual-ast", [clang, *strict,
                    "-Xclang", "-ast-dump=json", "-fsyntax-only", original_c], source_root)
                if ast_result["rc"] != 0:
                    raise MaterialStop("ordinary emitted C rejected by strict Clang AST frontend: " + ident)
                actual_ast = case_root / "actual-ast.json"
                actual_ast.write_bytes(ast_result["stdout"])
                passive_c = case_root / "passive.c"
                observation_map = case_root / "observer-map.json"
                observed = commands.run(ident + "-observer", [sys.executable, own / "observe.py",
                    original_c, actual_ast, passive_c, observation_map], source_root)
                passive_ready = observed["rc"] == 0 and passive_c.is_file() and observation_map.is_file()
                if not passive_ready:
                    summary["equipment_errors"].append(ident + ": observer did not produce a passive artifact")
                else:
                    difference = "".join(difflib.unified_diff(original_c.read_text().splitlines(keepends=True),
                        passive_c.read_text().splitlines(keepends=True), fromfile="original.c", tofile="passive.c"))
                    (case_root / "passive.diff").write_text(difference)
                for mode, guest_cc, flags in modes:
                    for variant, cfile in (("plain", original_c), ("passive", passive_c)):
                        if variant == "passive" and not passive_ready:
                            continue
                        label = ident + "-" + mode + "-" + variant
                        binary = binary_root / label
                        built = commands.run(label + "-build", [guest_cc, *strict, *flags, cfile, "-o", binary], source_root)
                        if built["rc"] != 0:
                            if variant == "plain":
                                raise MaterialStop("unmodified guest strict native build failed: " + label)
                            summary["equipment_errors"].append(label + ": passive build failed")
                            continue
                        binary_before = executable_receipt(binary)
                        native = commands.run(label + "-run", [binary], source_root, native=True)
                        binary_after = executable_receipt(binary)
                        if binary_before != binary_after:
                            raise MaterialStop("native executable bytes changed during execution: " + label)
                        native["record"]["binary_before"] = binary_before
                        native["record"]["binary_after"] = binary_after
                        save_json(native["directory"] / "result.json", native["record"])
                        state["native"].append({"mode": mode, "variant": variant, **native["record"]})
                        if native["rc"] != 0:
                            raise MaterialStop("guest native/sanitizer nonzero result; preserve raw: " + label)
                        if variant == "plain":
                            if native["stdout"] or native["stderr"]:
                                raise MaterialStop("unmodified unit guest produced unexpected output: " + label)
                        else:
                            comparison = commands.run(label + "-compare", [sys.executable, own / "compare.py",
                                own / "cohort" / "oracle.json", ident, observation_map,
                                native["directory"] / "stderr.bin", case_root / (mode + "-comparison.json")], source_root)
                            state["comparisons"].append(comparison["record"])
                            if comparison["rc"] == 42:
                                raise MaterialStop("frozen expected-value/identity mismatch: " + label)
                            if comparison["rc"] != 0:
                                summary["equipment_errors"].append(label + ": raw observation comparison incomplete")
        if commands.native_runs > MAX_NATIVE_RUNS:
            raise MaterialStop("native actual count exceeded budget")
        if summary["equipment_errors"]:
            raise EquipmentError("one or more R observation/recognizer/build paths incomplete")
        summary["status"] = "FINITE_MEASUREMENTS_COMPLETE"
        result_code = 0
    except MaterialStop as error:
        summary["status"] = "AFFECTED_STOP_C"
        summary["error"] = str(error)
        result_code = 42
    except Exception as error:
        summary["status"] = "R_EQUIPMENT_FIRST_FAILURE"
        summary["error"] = str(error)
        (out / "equipment-traceback.txt").write_text(traceback.format_exc())
        result_code = 1
    finally:
        summary["completed_utc"] = utc()
        summary["native_actual_runs"] = commands.native_runs
        summary["command_count"] = len(commands.records)
        save_json(out / "commands.json", commands.records)
        save_json(out / "summary.json", summary)
        print("R1185 " + summary["status"] + " native_actual_runs=" + str(commands.native_runs), flush=True)
        if summary.get("error"):
            print(summary["error"], flush=True)
    return result_code


if __name__ == "__main__":
    raise SystemExit(main())
