"""R1185 E6 Actions-only recovery: existing raw + 36 positive passive runs/job.

No NewLang compiler/public-probe invocation, C17 emission, AST generation,
plain guest run, or control guest run occurs here. The six original NORMAL2
archives are frozen inputs; each job reuses its own actual host/tool version.
Execution remains disabled until an explicit C recovery release is installed.
"""
from __future__ import annotations

import base64
import io
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import sys
import tarfile
import tempfile
import traceback

from driver import (Commands, EquipmentError, MaterialStop, digest,
                    executable_receipt, save_json, utc, verify_frozen)


REUSE_RECEIPT_SHA256 = "ea2c7197d7e5168db06d2b363c9a66dd25deebd3023c59b6fd1c352d4c6dfd90"
REPLAY_RECEIPT_SHA256 = "e37adc0529a005853557e71c866ee1456534eaac95c727263e4c31649685e00d"
FIXED_P = "faad092b5b0deb608a048ff60bee0056082f9a59"
FIXED_TREE = "01388ae82b1749453546933cb4ca6b311a25df77"
POSITIVES = ("P01", "P02", "P03", "P04", "P05", "P06")
MODES = ("gcc-O0", "gcc-O2", "clang-O0", "clang-O2", "guest-ASan", "guest-UBSan")
NATIVE_LIMIT = 36


class RecoveryCommands(Commands):
    def run(self, name, argv, cwd, *, timeout=60, native=False):
        if native and self.native_runs >= NATIVE_LIMIT:
            raise MaterialStop("positive-passive recovery would exceed 36 native runs/job")
        return super().run(name, argv, cwd, timeout=timeout, native=native)


def select_host(host_cc, sanitizer, build_type):
    job = os.environ.get("GITHUB_JOB")
    compiler = host_cc.name
    if job == "frontend-c17-partial":
        if sanitizer != "none" or build_type != "Debug" or compiler != "gcc":
            raise MaterialStop("PARTIAL host identity differs from frozen NORMAL2")
        return "partial"
    if job != "validate":
        raise MaterialStop("recovery is limited to the existing FULL/PARTIAL workflow jobs")
    if compiler == "gcc-13" and sanitizer == "none":
        if build_type == "Debug":
            return "gcc"
        if build_type == "Release":
            return "gcc_release"
    if compiler == "clang-23" and build_type == "Debug":
        known = {"none": "clang", "address": "asan", "undefined": "ubsan"}
        if sanitizer in known:
            return known[sanitizer]
    raise MaterialStop("FULL host identity differs from frozen NORMAL2")


def reuse_archive(own, out, host):
    receipt_bytes = (own / "reuse/reuse-receipt.json").read_bytes()
    replay_bytes = (own / "reuse/replay-receipt.json").read_bytes()
    if digest(receipt_bytes) != REUSE_RECEIPT_SHA256 or digest(replay_bytes) != REPLAY_RECEIPT_SHA256:
        raise MaterialStop("frozen original reuse/replay receipt changed")
    receipt = json.loads(receipt_bytes)
    replay = json.loads(replay_bytes)["hosts"][host]
    selected = receipt["hosts"][host]
    encoded = (own / "reuse" / (host + ".tar.gz.b64")).read_bytes()
    archive = base64.b64decode(encoded, validate=True)
    custody = selected["raw_custody"]
    if len(archive) != custody["gzip_bytes"] or digest(archive) != custody["gzip_sha256"]:
        raise MaterialStop("original NORMAL2 archive bytes/hash changed")
    reused = out / "reused-normal2"
    reused.mkdir()
    (reused / "original-export.tar.gz").write_bytes(archive)
    (reused / "reuse-receipt.json").write_bytes(receipt_bytes)
    (reused / "replay-receipt.json").write_bytes(replay_bytes)
    root = reused / "evidence"
    root.mkdir()
    members = {}
    with tarfile.open(fileobj=io.BytesIO(archive), mode="r:gz") as tar:
        for item in tar.getmembers():
            name = PurePosixPath(item.name)
            if (not item.isfile() or name.is_absolute() or ".." in name.parts
                    or str(name) != item.name or item.name in members):
                raise MaterialStop("noncanonical/duplicate original evidence member")
            stream = tar.extractfile(item)
            if stream is None:
                raise MaterialStop("original evidence member unavailable")
            data = stream.read()
            if len(data) != item.size:
                raise MaterialStop("original evidence member size differs")
            target = root / item.name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            members[item.name] = {"bytes": len(data), "sha256": digest(data)}
    index_bytes = (root / "MEMBER-INDEX.json").read_bytes()
    if digest(index_bytes) != custody["member_index_sha256"]:
        raise MaterialStop("original member-index hash changed")
    listed = json.loads(index_bytes)
    expected = {x["path"]: {"bytes": x["bytes"], "sha256": x["sha256"]} for x in listed}
    actual = {k: v for k, v in members.items() if k != "MEMBER-INDEX.json"}
    if len(expected) != len(listed) or expected != actual or len(members) != 697:
        raise MaterialStop("original full member inventory/hash mismatch")
    old_summary = json.loads((root / "summary.json").read_bytes())
    fixed = json.loads((root / "fixed-context.json").read_bytes())
    if fixed["commit"] != FIXED_P or fixed["tree"] != FIXED_TREE:
        raise MaterialStop("original evidence belongs to another P target")
    if (old_summary["native_actual_runs"] != 48 or selected["native_counts"] !=
            {"plain": 42, "control_passive": 6, "positive_passive": 0}):
        raise MaterialStop("original successful native partition differs")
    for native in selected["native"]:
        command = root / "commands" / native["run_name"]
        data = (command / "result.json").read_bytes()
        result = json.loads(data)
        if (digest(data) != native["run_result_sha256"] or result["rc"] != 0 or result["timed_out"]
                or result["binary_before"] != result["binary_after"]):
            raise MaterialStop("original successful native result/custody changed")
    for ident in POSITIVES + ("C01",):
        old_source = root / "frozen-V2/source" / (ident + ".nl")
        if old_source.read_bytes() != (own / "cohort/source" / (ident + ".nl")).read_bytes():
            raise MaterialStop("current V2 source differs from the captured emission input")
        for filename, key in (("original.c", "original_c_sha256"), ("actual-ast.json", "actual_ast_sha256")):
            if digest((root / "cases" / ident / filename).read_bytes()) != selected["cases"][ident][key]:
                raise MaterialStop("captured C/AST identity differs from original receipt")
    return root, selected, replay


def main():
    if len(sys.argv) != 9:
        raise SystemExit("recovery_driver.py UNUSED_COMPILER UNUSED_PROBE HOST_CC HOST_SAN OWN OUT FIXED_RECEIPT BUILD_TYPE")
    # The first two target paths remain registered for ordinary host build
    # compatibility; they are never invoked by this recovery driver.
    host_cc = Path(sys.argv[3])
    sanitizer = sys.argv[4]
    own, out, fixed_receipt = map(Path, sys.argv[5:8])
    build_type = sys.argv[8]
    own, out = own.resolve(), out.resolve()
    source_root = own.parents[1]
    if (os.environ.get("GITHUB_ACTIONS") != "true"
            or os.environ.get("GITHUB_EVENT_NAME") != "pull_request"
            or os.environ.get("GITHUB_HEAD_REF") != "r-fast/r1185-helper-result-current"
            or os.environ.get("GITHUB_RUN_ATTEMPT") != "1"):
        raise SystemExit("R1185 recovery requires the released natural Actions PR attempt1; local/manual forbidden")
    release = json.loads((own / "recovery-release.json").read_bytes())
    if (not release.get("execution_released") or release.get("fixed_P") != FIXED_P
            or release.get("fixed_tree") != FIXED_TREE
            or release.get("native_per_job") != NATIVE_LIMIT
            or release.get("additional_normal_jobs") != 6):
        raise SystemExit("C additional-execution STOP: exact recovery release absent")
    out.mkdir(parents=True, exist_ok=False)
    commands = RecoveryCommands(out)
    summary = {"schema": "R1185_E6_POSITIVE_PASSIVE_RECOVERY", "started_utc": utc(),
               "release": release, "status": "RUNNING", "cases": {}, "equipment_errors": [],
               "new_NewLang_CLI": 0, "new_public_probe": 0, "new_C17": 0, "new_AST": 0,
               "new_plain_native": 0, "new_control_native": 0,
               "whole_loop_validator": "NOT_RUN/NO_APPLICABLE_PUBLIC_API",
               "machine_CALL_STORE_survival": "NOT_MEASURED"}
    result_code = 1
    try:
        verify_frozen(own / "cohort")
        shutil.copytree(own / "cohort", out / "frozen-V2")
        shutil.copyfile(fixed_receipt, out / "fixed-context.json")
        current_fixed = json.loads(fixed_receipt.read_bytes())
        if current_fixed["commit"] != FIXED_P or current_fixed["tree"] != FIXED_TREE:
            raise MaterialStop("current genuine fixed-P context changed")
        host = select_host(host_cc, sanitizer, build_type)
        summary["host"] = host
        original, selected, replay = reuse_archive(own, out, host)
        summary["reused_native"] = {"plain": 42, "control_passive": 6, "new_execution": 0,
                                    "old_run_id": selected["run_id"], "old_job_id": selected["job_id"]}
        metadata = {}
        for name, args in (("head", ["rev-parse", "HEAD"]), ("tree", ["rev-parse", "HEAD^{tree}"]),
                           ("parents", ["show", "--no-patch", "--format=%P", "HEAD"]),
                           ("index", ["ls-files", "--stage", "-z"]),
                           ("status", ["status", "--porcelain=v1", "--untracked-files=all"])):
            observed = commands.run("git-" + name, ["git", "-C", source_root, *args], source_root)
            if observed["rc"] != 0:
                raise EquipmentError("actual recovery checkout metadata unavailable")
            metadata[name] = observed["stdout"].decode("utf-8")
        save_json(out / "actual-checkout.json", metadata)
        if metadata["head"].strip() != current_fixed["source_head"] or metadata["tree"].strip() != current_fixed["source_tree"]:
            raise MaterialStop("actual recovery Git context differs from fixed snapshot receipt")
        tools = {"gcc": shutil.which("gcc-13"), "clang": shutil.which("clang-23") or shutil.which("clang")}
        for label, compiler in tools.items():
            if compiler is None or compiler != selected["versions"][label]["argv"][0]:
                raise MaterialStop("installed guest executable differs from paired NORMAL2 host")
            version = commands.run(label + "-version", [compiler, "--version"], source_root)
            if (version["rc"] != 0 or not version["stdout"]
                    or version["stdout"].decode().splitlines()[0] != selected["versions"][label]["first_line"]):
                raise MaterialStop("guest version differs from paired NORMAL2 host")
        summary["guest_modes"] = selected["guest_modes"]
        if tuple(m["name"] for m in summary["guest_modes"]) != MODES:
            raise MaterialStop("frozen guest mode inventory/order changed")
        control_out = out / "control-redecode"
        control_out.mkdir()
        linked_control = control_out / "original-linked-map.json"
        linked = commands.run("C01-original-link", [sys.executable, own / "link_actual_ast.py",
            original / "cases/C01/observer-map.json", original / "cases/C01/actual-ast.json", linked_control], source_root)
        if linked["rc"] != 0:
            raise EquipmentError("same-captured-AST control identity linkage incomplete")
        for mode in summary["guest_modes"]:
            name = mode["name"]
            comparison = commands.run("C01-" + name + "-redecode", [sys.executable, own / "compare.py",
                own / "cohort/oracle.json", "C01", linked_control,
                original / "commands" / ("C01-" + name + "-passive-run") / "stderr.bin",
                control_out / (name + ".json")], source_root)
            if comparison["rc"] == 42:
                raise MaterialStop("existing control raw has a finite value/identity mismatch")
            if comparison["rc"] != 0:
                raise EquipmentError("existing control raw redecode incomplete")
        with tempfile.TemporaryDirectory(prefix="r1185-recovery-native-", dir=out.parent) as temporary:
            for ident in POSITIVES:
                case_out = out / "cases" / ident
                case_out.mkdir(parents=True)
                for filename in ("original.c", "actual-ast.json"):
                    shutil.copyfile(original / "cases" / ident / filename, case_out / filename)
                passive, model, linked_map = (case_out / n for n in ("passive.c", "observer-map.json", "linked-map.json"))
                state = {"source": "unchanged frozen V2", "original_C_AST": "REUSED_EXACT_BYTES", "native": [], "comparisons": []}
                summary["cases"][ident] = state
                observed = commands.run(ident + "-observer", [sys.executable, own / "observe.py",
                    case_out / "original.c", case_out / "actual-ast.json", passive, model], source_root)
                if observed["rc"] != 0 or not passive.is_file() or not model.is_file():
                    summary["equipment_errors"].append(ident + ": same-C/AST observer incomplete")
                    continue
                frozen = replay["observer"][ident]
                if digest(passive.read_bytes()) != frozen["passive_c_sha256"] or digest(model.read_bytes()) != frozen["observer_map_sha256"]:
                    raise MaterialStop("recovery observer output differs from pre-release same-C/AST replay freeze")
                linked = commands.run(ident + "-link", [sys.executable, own / "link_actual_ast.py",
                    model, case_out / "actual-ast.json", linked_map], source_root)
                if linked["rc"] != 0:
                    summary["equipment_errors"].append(ident + ": same-AST callee linkage incomplete")
                    continue
                if digest(linked_map.read_bytes()) != frozen["linked_map_sha256"]:
                    raise MaterialStop("same-AST linked map differs from pre-release replay freeze")
                for mode in summary["guest_modes"]:
                    label = ident + "-" + mode["name"] + "-passive"
                    binary = Path(temporary) / label
                    built = commands.run(label + "-build", [mode["compiler"], *mode["flags"], passive, "-o", binary], source_root)
                    if built["rc"] != 0:
                        summary["equipment_errors"].append(label + ": passive build failed")
                        continue
                    before = executable_receipt(binary)
                    native = commands.run(label + "-run", [binary], source_root, native=True)
                    after = executable_receipt(binary)
                    native["record"].update({"binary_before": before, "binary_after": after})
                    save_json(native["directory"] / "result.json", native["record"])
                    state["native"].append({"mode": mode["name"], "variant": "passive", **native["record"]})
                    if before != after or native["rc"] != 0 or native["stdout"]:
                        raise MaterialStop("positive passive native/identity result abnormal: " + label)
                    compared = commands.run(label + "-compare", [sys.executable, own / "compare.py",
                        own / "cohort/oracle.json", ident, linked_map, native["directory"] / "stderr.bin",
                        case_out / (mode["name"] + "-comparison.json")], source_root)
                    state["comparisons"].append(compared["record"])
                    if compared["rc"] == 42:
                        raise MaterialStop("positive frozen expected-value/identity mismatch: " + label)
                    if compared["rc"] != 0:
                        summary["equipment_errors"].append(label + ": actual raw comparison incomplete")
        if summary["equipment_errors"] or commands.native_runs != NATIVE_LIMIT:
            raise EquipmentError("positive-passive recovery has missing build/observation/decoding evidence")
        summary["status"], result_code = "FINITE_RECOVERY_COMPLETE_WITH_PRIOR_RAW_REUSE", 0
    except MaterialStop as error:
        summary["status"], summary["error"], result_code = "AFFECTED_STOP_C", str(error), 42
    except Exception as error:
        summary["status"], summary["error"] = "R_EQUIPMENT_FIRST_FAILURE", str(error)
        (out / "equipment-traceback.txt").write_text(traceback.format_exc())
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
