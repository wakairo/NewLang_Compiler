"""R1197 GitHub-only runner TEXT DRAFT; never executed during preparation.

HOLD is checked before every subprocess pathway. Actual semantic role mapping
is a required unresolved interface, not a guessed emitter convention.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
import traceback

from compare import InterfaceUnresolved, ObservationMismatch
from compare import compare, parse_trace, require_actual_binding


MANIFEST = "ea4c4ad9d855bfcc6f98f41885dfc7c722c7c4b4e759896304b4c0623bd9a005"
STRICT = ["-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror"]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")


class Stop(RuntimeError):
    pass


class Run:
    def __init__(self, root, phase):
        self.root = root
        self.phase = phase
        self.count = 0
        self.native_attempts = 0
        self.native_launched = 0

    def command(self, label, argv, guest=False, timeout=60):
        if guest:
            if self.phase != "COMPLETION":
                raise InterfaceUnresolved("CAPTURE_ONLY release phase blocks every native launch")
            if self.native_attempts >= 60:
                raise Stop("planned native budget exhausted")
            self.native_attempts += 1
        self.count += 1
        stem = self.root / "commands" / f"{self.count:04d}_{label}"
        stem.parent.mkdir(parents=True, exist_ok=True)
        env = os.environ.copy()
        env.update(LC_ALL="C",
                   ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:abort_on_error=1",
                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        begin = time.monotonic()
        stdout = stderr = b""
        code, error, timeout_hit, launched = None, None, False, False
        try:
            process = subprocess.Popen([str(x) for x in argv], stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE, env=env)
            launched = True
            if guest:
                self.native_launched += 1
            try:
                stdout, stderr = process.communicate(timeout=timeout)
            except subprocess.TimeoutExpired:
                timeout_hit = True
                process.kill()
                stdout, stderr = process.communicate()
            code = process.returncode
        except OSError as exc:
            error = repr(exc)
        stem.with_suffix(".stdout").write_bytes(stdout)
        stem.with_suffix(".stderr").write_bytes(stderr)
        save(stem.with_suffix(".json"), {
            "label": label, "argv": [str(x) for x in argv], "returncode": code,
            "guest": guest, "launched": launched, "timeout": timeout_hit,
            "launch_error": error, "wall_seconds": time.monotonic() - begin,
            "stdout_sha256": digest(stdout), "stderr_sha256": digest(stderr),
            "ASAN_OPTIONS": env["ASAN_OPTIONS"], "UBSAN_OPTIONS": env["UBSAN_OPTIONS"],
        })
        save(self.root / "budget.json", {
            "native_attempts": self.native_attempts, "native_launched": self.native_launched,
            "planned_limit": 60, "issue_limit": 72, "manual_rerun_dispatch": 0,
        })
        if not launched or timeout_hit:
            raise Stop("command launch/timeout: " + label)
        if code is not None and code < 0:
            raise Stop("process terminated by signal: " + label)
        diagnostics = stdout + stderr
        if any(marker in diagnostics for marker in
               (b"AddressSanitizer", b"UndefinedBehaviorSanitizer", b"LeakSanitizer",
                b"runtime error:", b"Assertion failed", b"Assertion `")):
            raise Stop("sanitizer/assertion diagnostic: " + label)
        return code, stdout, stderr

    def successful(self, label, argv, timeout=60):
        code, stdout, stderr = self.command(label, argv, timeout=timeout)
        if code != 0:
            raise Stop("command failed; preserve raw layer: " + label)
        return stdout, stderr


def identity(run, args, binding):
    event = json.loads(Path(os.environ["GITHUB_EVENT_PATH"]).read_text())
    pr = event["pull_request"]
    if (not pr.get("draft") or pr["base"]["sha"] != binding["compiler_commit"]
            or pr["base"]["ref"] != binding["pr_base_ref"]
            or pr["head"]["ref"] != binding["r_head_ref"]
            or pr["head"]["repo"]["full_name"] != "wakairo/NewLang_Compiler"):
        raise Stop("PR identity/draft mismatch")
    run.successful("exact_merge_history", ["git", "fetch", "--no-tags", "--depth=3",
                                           "origin", os.environ["GITHUB_SHA"]])
    raw, _ = run.successful("checkout", ["git", "rev-parse", "HEAD", "HEAD^1", "HEAD^2"])
    expected = [os.environ["GITHUB_SHA"], binding["compiler_commit"], pr["head"]["sha"]]
    if raw.decode().splitlines() != expected:
        raise Stop("actual merge checkout/parents mismatch")
    run.successful("ancestor", ["git", "merge-base", "--is-ancestor",
                               binding["compiler_commit"], pr["head"]["sha"]])
    paths, _ = run.successful("changed_paths", ["git", "diff", "--name-only",
                                                binding["compiler_commit"], "HEAD"])
    suffixes = {
        "CMakeLists.txt": "CMakeLists.append.cmake",
        ".github/workflows/compiler-ci.yml": "compiler-ci.append.yml",
        ".github/workflows/compiler-frontend-ci.yml": "compiler-frontend-ci.append.yml",
    }
    if any(path not in suffixes and not path.startswith("tests/r1197/")
           for path in paths.decode().splitlines()):
        raise Stop("non-R Production path changed")
    for index, (path, suffix) in enumerate(suffixes.items()):
        original, _ = run.successful(f"original_interface_{index}",
                                    ["git", "show", binding["compiler_commit"] + ":" + path])
        addition = (args.cohort_root / "apparatus" / suffix).read_bytes()
        if Path(path).read_bytes() != original + addition:
            raise Stop("original interface bytes were changed: " + path)
    save(run.root / "identity.json", {
        "checkout_base_rhead": expected, "event": event,
        "run_id": os.environ.get("GITHUB_RUN_ID"), "attempt": os.environ.get("GITHUB_RUN_ATTEMPT"),
        "workflow": os.environ.get("GITHUB_WORKFLOW"), "job_key": os.environ.get("GITHUB_JOB"),
        "host_compiler": args.host_compiler, "host_build_type": args.host_build_type,
        "host_sanitizer": args.host_sanitizer,
        "compiler_sha256": digest(args.compiler_cli.read_bytes()),
        "probe_sha256": digest(args.probe.read_bytes()),
    })


def cohort(run, args, oracle, gcc, clang):
    prepared = []
    apparatus = args.cohort_root / "apparatus"
    roles = json.loads((apparatus / "role_bindings.pending.json").read_text())
    # Source checks and all invalid-source acceptance gates precede any guest.
    for name, case in oracle["cases"].items():
        source = args.cohort_root / "V1" / case["source"]
        folder = run.root / "cases" / name
        folder.mkdir(parents=True)
        (folder / "source.nl").write_bytes(source.read_bytes())
        code, _, _ = run.command(name + "_check", [args.compiler_cli, "--check", source])
        save(folder / "check_result.json", {"returncode": code, "role": case["role"],
                                          "nonzero_disposition": "raw diagnostic retained; refusal unclassified" if code else None,
                                          "source_sha256": digest(source.read_bytes())})
        if case["role"] == "invalid_negative":
            if code == 0:
                raise Stop("INVALID SOURCE ACCEPTED: " + name)
            continue
        if code != 0:
            if case["role"] == "legal_boundary":
                continue
            raise Stop("positive CHECK nonzero; classify preserved diagnostic: " + name)
        prepared.append((name, case, source, folder))
    # No native is released by emission alone or by a synthetic host world.
    candidates = []
    for name, case, source, folder in prepared:
        receipt = b""
        if case["role"] != "legal_boundary":
            code, receipt, _ = run.command(name + "_public", [args.probe, source])
            (folder / "public_receipt.jsonl").write_bytes(receipt)
            rows = [json.loads(line) for line in receipt.splitlines() if line.startswith(b"{")]
            issued = [r for r in rows if r.get("record") == "source_issued_main_call"]
            complete = [r for r in rows if r.get("record") == "probe_complete"]
            if (code != 0 or len(issued) != 1 or issued[0]["status"] != 0
                    or len(complete) != 1 or complete[0]["status"] != 0):
                raise Stop("genuine public source-issued layer not reached: " + name)
        raw = folder / "raw.c"
        code, _, _ = run.command(name + "_emit",
                                 [args.compiler_cli, "--emit-c17-to", raw, source])
        if case["role"] == "legal_boundary":
            if code == 0:
                raise Stop("outside declared admission emitted; no native: " + name)
            continue
        if code != 0 or not raw.is_file() or not raw.stat().st_size:
            raise Stop("positive C17 not reached; preserve raw layer: " + name)
        candidates.append((name, source, folder, receipt, raw))
    unresolved = []
    linked = []
    for name, source, folder, receipt, raw in candidates:
        ast, passive, mapping, linked_map = (folder / filename for filename in
                                            ("actual_ast.json", "passive.c", "map.json", "linked_map.json"))
        captured, _ = run.successful(name + "_ast",
                                     [clang, *STRICT, "-Xclang", "-ast-dump=json", "-fsyntax-only", raw])
        ast.write_bytes(captured)
        run.successful(name + "_observe", [sys.executable, apparatus / "observe_r1197.py",
                                           raw, ast, passive, mapping])
        run.successful(name + "_link", [sys.executable, apparatus / "neutral" / "link_actual_ast.py",
                                        mapping, ast, linked_map])
        model = json.loads(linked_map.read_text())
        inputs = {"source": source.read_bytes(), "public_receipt": receipt,
                  "raw_c": raw.read_bytes(), "actual_ast": captured,
                  "linked_map": linked_map.read_bytes()}
        try:
            bound = require_actual_binding(name, roles, inputs, model)
        except InterfaceUnresolved as exc:
            unresolved.append({"case": name, "reason": str(exc)})
            continue
        linked.append((name, folder, raw, passive, model, bound))
    if unresolved:
        save(run.root / "remaining_interfaces.json", unresolved)
        raise InterfaceUnresolved("actual-output semantic roles unbound; zero guest execution")
    executables = run.root.parent / "executables"
    executables.mkdir()
    for name, folder, raw, passive, model, bound in linked:
        variants = [(cc, opt, mode, []) for cc in (gcc, clang)
                    for opt in ("-O0", "-O2") for mode in ("raw", "passive")]
        if name in ("P02_then_return", "P04_both_return", "P06_eq_copy_both_return"):
            variants += [(clang, "-O1", mode, ["-fsanitize=" + san,
                                              "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"])
                         for san in ("address", "undefined") for mode in ("raw", "passive")]
        for index, (cc, opt, mode, flags) in enumerate(variants):
            c = raw if mode == "raw" else passive
            executable = executables / f"{name}_{index}"
            run.successful(f"{name}_{index}_compile", [cc, *STRICT, opt, *flags, c, "-o", executable])
            save(folder / f"guest_{index}_identity.json", {
                "compiler": cc, "mode": mode, "optimization": opt, "sanitizers": flags,
                "c_sha256": digest(c.read_bytes()), "guest_sha256": digest(executable.read_bytes()),
            })
            code, stdout, stderr = run.command(f"{name}_{index}_guest", [executable], guest=True, timeout=5)
            if code != 0 or stdout:
                raise Stop("NATIVE/SANITIZER divergence: " + name)
            if mode == "raw":
                if stderr:
                    raise Stop("unexpected raw stderr/sanitizer: " + name)
            else:
                observations = parse_trace(stderr, model)
                save(folder / f"guest_{index}_observations.json", observations)
                save(folder / f"guest_{index}_comparison.json", compare(observations, bound, oracle, name))


def main():
    parser = argparse.ArgumentParser()
    for name in ("compiler-cli", "probe", "cohort-root", "evidence-dir"):
        parser.add_argument("--" + name, type=Path, required=True)
    for name in ("host-compiler", "host-build-type", "host-sanitizer"):
        parser.add_argument("--" + name, required=True)
    args = parser.parse_args()
    binding = json.loads((args.cohort_root / "apparatus" / "target_binding.json").read_text())
    if (not binding.get("execution_released") or binding.get("status") != "RELEASED"
            or not binding.get("coordination_release_url")
            or os.environ.get("GITHUB_ACTIONS") != "true"
            or os.environ.get("GITHUB_EVENT_NAME") != "pull_request"
            or os.environ.get("GITHUB_RUN_ATTEMPT") != "1"
            or os.environ.get("GITHUB_REPOSITORY") != "wakairo/NewLang_Compiler"):
        raise SystemExit("HOLD: no separately released natural GitHub PR execution")
    args.evidence_dir.mkdir(parents=True, exist_ok=False)
    first = args.evidence_dir / "FIRST"
    first.mkdir()
    run = Run(first, binding.get("execution_phase"))
    status, detail, result = "NOT_COMPLETED", None, 1
    try:
        manifest = (args.cohort_root / "V1" / "SHA256SUMS").read_bytes()
        if digest(manifest) != MANIFEST:
            raise Stop("V1 manifest changed")
        for line in manifest.decode().splitlines():
            expected, path = line.split("  ", 1)
            if digest((args.cohort_root / "V1" / path).read_bytes()) != expected:
                raise Stop("frozen bytes changed: " + path)
        shutil.copytree(args.cohort_root / "V1", first / "V1")
        shutil.copytree(args.cohort_root / "apparatus", first / "apparatus")
        identity(run, args, binding)
        gcc = shutil.which("gcc-13") or shutil.which("gcc")
        clang = "/usr/bin/clang" if Path("/usr/bin/clang").is_file() else (
            shutil.which("clang-23") or shutil.which("clang"))
        if not gcc or not clang:
            raise Stop("required GCC/Clang tool missing")
        for name, cc in (("gcc", gcc), ("clang", clang), ("newlangc", args.compiler_cli)):
            run.successful(name + "_version", [cc, "--version"])
        oracle = json.loads((args.cohort_root / "V1" / "oracle.json").read_text())
        cohort(run, args, oracle, gcc, clang)
        status, result = "FINITE_REQUIRED_OBSERVATIONS_MATCH", 0
    except InterfaceUnresolved as exc:
        status, detail = "APPARATUS_INTERFACE_UNRESOLVED", str(exc)
    except ObservationMismatch as exc:
        status, detail = "NATIVE_OBSERVATION_MISMATCH", str(exc)
    except Stop as exc:
        status, detail = "STOP_LAYER_RECORDED", str(exc)
    except Exception as exc:
        status, detail = "APPARATUS_EXCEPTION", repr(exc)
        (first / "exception.txt").write_text(traceback.format_exc())
    finally:
        save(first / "summary.json", {
            "status": status, "detail": detail, "native_attempts": run.native_attempts,
            "native_launched": run.native_launched,
            "limits": ["no public whole-loop validator invented", "no general soundness/Product claim",
                       "original FIRST never replaced by repair", "API run/job metadata must be bound after the terminal jobs"],
        })
        save(first / "artifact_hashes.json", {
            str(p.relative_to(first)): digest(p.read_bytes())
            for p in sorted(first.rglob("*")) if p.is_file()
        })
        print("R1197 " + json.dumps({"status": status, "detail": detail,
                                     "native": run.native_launched}))
    return result


if __name__ == "__main__":
    raise SystemExit(main())
