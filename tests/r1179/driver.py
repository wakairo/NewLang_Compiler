"""R1179 independent V2 Actions-only driver; no producer oracle/fixture inputs."""
import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
import traceback

BUNDLE_SHA = "2dff96cd7271a2b5637c46e7a45ab798fbbf84249b447fdf82657b484a69d680"
ORACLE_SHA = "4fe82f7bec6315d85a1c4eb4b41634d0135e2f363c1e38640ae1fa7f9c511cff"
PIN = "bb8b33cefc030ed99941c9c93fef9c7ef89b0ac0"
TREE = "d8fd0b0a95184d3449d7e1525ba56574429dbf8f"


class Stop(RuntimeError):
    def __init__(self, layer, message):
        super().__init__(message)
        self.layer = layer


def digest(data):
    return hashlib.sha256(data).hexdigest()


def require(condition, layer, message):
    if not condition:
        raise Stop(layer, message)


def write_json(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")


class Recorder:
    def __init__(self, root):
        self.root = root
        self.raw = root / "raw"
        self.raw.mkdir(parents=True)
        self.commands = []

    def run(self, label, argv, *, cwd=None, env_delta=None, timeout=90):
        serial = len(self.commands) + 1
        stem = f"{serial:04d}-{label}"
        env = os.environ.copy()
        env.update(env_delta or {})
        before = datetime.datetime.now(datetime.timezone.utc).isoformat()
        tick = time.monotonic()
        try:
            result = subprocess.run([str(x) for x in argv], cwd=cwd, env=env,
                                    capture_output=True, timeout=timeout, check=False)
            code, stdout, stderr = result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired as error:
            code, stdout, stderr = "TIMEOUT", error.stdout or b"", error.stderr or b""
        stdout_path, stderr_path = self.raw / (stem + ".stdout"), self.raw / (stem + ".stderr")
        stdout_path.write_bytes(stdout)
        stderr_path.write_bytes(stderr)
        receipt = {
            "serial": serial, "label": label, "argv": [str(x) for x in argv],
            "cwd": str(Path(cwd).resolve() if cwd else Path.cwd()),
            "env_delta": env_delta or {}, "returncode": code,
            "started_utc": before, "elapsed_seconds": time.monotonic() - tick,
            "stdout": {"file": str(stdout_path.relative_to(self.root)),
                       "bytes": len(stdout), "sha256": digest(stdout)},
            "stderr": {"file": str(stderr_path.relative_to(self.root)),
                       "bytes": len(stderr), "sha256": digest(stderr)},
        }
        self.commands.append(receipt)
        write_json(self.raw / (stem + ".receipt.json"), receipt)
        write_json(self.root / "commands.json", self.commands)
        return code, stdout, stderr, receipt


def check_passive(data, case, mapping):
    lines = data.decode("utf-8").splitlines()
    require(lines and all(x.startswith("R1179 ") for x in lines),
            "PASSIVE_OUTPUT", "unrecognized native output")
    trace = [json.loads(x[6:]) for x in lines]
    states = [x for x in trace if "root" in x]
    require(states, "NATIVE_ADDRESS", "no actual original address observations")
    baselines = [x for x in trace if x["e"] == "B0"]
    require(len(baselines) == 1, "NATIVE_ADDRESS", "one source original B0 required")
    base = baselines[0]
    require([base["f0"], base["f1"]] == case["initial_original"],
            "NATIVE_VALUE", "original construction differs from frozen oracle")
    require(base["c0"] != base["c1"], "NATIVE_ADDRESS", "children alias")
    for event in states:
        require([event[x] for x in ("root", "c0", "c1")] ==
                [base[x] for x in ("root", "c0", "c1")],
                "NATIVE_ADDRESS", "original/root child address changed")
    selected = case["selected_declaration_index"]
    require(mapping["selected_declaration_index"] == selected,
            "NATIVE_ROLE_MAP", "actual C selected member differs from frozen field index")
    headers = [i for i, x in enumerate(trace) if x["e"] == "header"]
    require([trace[i]["phase"] for i in headers] == case["header_phases"],
            "NATIVE_PHASE", "actual incoming header phases differ")
    require(len(headers) == len(case["rows"]) + 1,
            "NATIVE_PHASE", "header count does not include real final Break header")
    role_map = mapping["roles"]
    require(mapping["incoming"]["kind"] ==
            ("literal" if case["category"] == "LITERAL_CONTROL" else "phase_conditional"),
            "NATIVE_ROLE_MAP", "actual incoming family differs from frozen source category")

    def binding(segment, role, expected):
        events = [x for x in segment if x["e"] == role["event"] and x.get("id") == role["id"]]
        require(len(events) == 1 and events[0]["value"] == expected,
                "NATIVE_BINDING", f"actual binding {role} does not equal frozen {expected}")
        return next(i for i, x in enumerate(segment) if x is events[0])

    for index, row in enumerate(case["rows"]):
        segment = trace[headers[index]:headers[index + 1]]
        first = segment[0]
        require(first["phase"] == row["carry_in"] and
                first["f" + str(selected)] == row["old"] and
                first["f" + str(1 - selected)] == row["sibling_before"],
                "NATIVE_HEADER", "actual header state mismatch")
        before = [i for i, x in enumerate(segment) if x["e"] == "store_before"]
        after = [i for i, x in enumerate(segment) if x["e"] == "store_after"]
        outgoing = [i for i, x in enumerate(segment) if x["e"] == "continue"]
        require(len(before) == len(after) == len(outgoing) == 1,
                "NATIVE_CONTROL", "one actual store and Continue per reached row")
        wi, ai, ci = before[0], after[0], outgoing[0]
        w, a, c = segment[wi], segment[ai], segment[ci]
        require(w["incoming"] == row["incoming"] and w["phase"] == row["phase"] and
                w["f" + str(selected)] == row["old"] and
                w["f" + str(1 - selected)] == row["sibling_before"],
                "NATIVE_VALUE", "actual incoming/old/sibling before store mismatch")
        require(w["destination"] == base["c" + str(selected)],
                "NATIVE_ADDRESS", "actual writer destination is not selected original child")
        require(a["f" + str(selected)] == row["current"] and
                a["f" + str(1 - selected)] == row["sibling"],
                "NATIVE_VALUE", "actual original after store mismatch")
        require(c["phase"] == row["carry_out"] and
                c["f" + str(selected)] == row["current"] and
                c["f" + str(1 - selected)] == row["sibling"],
                "NATIVE_CARRY", "actual outgoing carry/state mismatch")
        phase_i = binding(segment, role_map["runtime_phase"], row["phase"])
        incoming_i = [binding(segment, role, row["incoming"])
                      for role in mapping["incoming"]["bindings"]]
        pi = binding(segment, role_map["prewrite"], row["prewrite"])
        sbi = binding(segment, role_map["sibling_before"], row["sibling_before"])
        oi = binding(segment, role_map["old_load"], row["old"])
        current_i = binding(segment, role_map["current"], row["current"])
        sibling_i = binding(segment, role_map["sibling"], row["sibling"])
        copies = [binding(segment, role, row["old_copy"])
                  for role in mapping["old_result_and_copy"]]
        require(phase_i < pi < oi < wi < ai < min(copies) and sbi < wi and
                all(pi < ii < oi for ii in incoming_i) and
                max(copies) < current_i < ci and sibling_i < ci,
                "NATIVE_ORDER", "real load/store/result/Copy/current/Continue order mismatch")
    final = trace[headers[-1]:]
    require(not any(x["e"] in ("store_before", "store_after", "continue") for x in final),
            "NATIVE_BREAK", "final false arm performed a writer or Continue")
    breaks = [x for x in final if x["e"] == "break"]
    exits = [x for x in final if x["e"] == "loop_exit"]
    post_loads = [x for x in final if x["e"] == "post_loads"]
    require(len(breaks) == len(exits) == len(post_loads) == 1,
            "NATIVE_BREAK", "actual Break, loop exit and post-load checkpoint required")
    for event in [breaks[0], exits[0], post_loads[0]]:
        require(event["phase"] == case["break"]["phase"] and
                event["finished"] == case["outer_loop_result_usize"] and
                [event["f0"], event["f1"]] == case["break"]["original"],
                "NATIVE_BREAK", "actual Break/post-Break phase or original mismatch")
    binding(final, role_map["runtime_phase"], case["break"]["phase"])
    binding(final, {"id": mapping["break_result_id"], "event": "assign"},
            case["break"]["result"])
    for role in mapping["finished"]:
        binding(final, role, case["outer_loop_result_usize"])
    binding(final, role_map["post_current"], case["break"]["selected_current"])
    binding(final, role_map["post_sibling"], case["break"]["sibling_current"])
    return {"events": len(trace), "headers": len(headers), "replace_count": len(case["rows"]),
            "original_addresses": {x: base[x] for x in ("root", "c0", "c1")},
            "actual_break": breaks[0], "all_frozen_values_compared": True}


def public_observations(raw, case):
    records = [json.loads(x) for x in raw.decode("utf-8").splitlines()]
    contracts = [r for r in records if r.get("record") == "contract"]
    require(len(contracts) == 1, "OWNED_OBSERVATION", "missing public contract receipt")
    for stage in ("parse", "register", "ordinary_call"):
        states = [r for r in records if r.get("record") == stage]
        require(len(states) == 1 and states[0]["status"] == 0,
                "OWNED_OBSERVATION", "ordinary source-issued stage failed: " + stage)
    fragments = [r for r in records if r.get("record") == "fragment"]
    require(any(r["edge"] == "call_body" for r in fragments),
            "OWNED_OBSERVATION", "no genuine ordinary source-backed main body")
    require(any(r["edge"] == "abstract_loop_body" for r in fragments),
            "OWNED_OBSERVATION", "no publicly borrowed abstract loop body")
    nodes = [r for r in records if r.get("record") == "node"]
    loops = [r for r in nodes if r["kind"] == "LOOP"]
    require(loops and all(r["header_inductive"] for r in loops),
            "OWNED_OBSERVATION", "source loop did not publish header_inductive")
    changes = [r for r in nodes if r["kind"] == "REPLACE" and r["field"]["present"]]
    require(all(r["field"]["index"] == case["selected_declaration_index"]
                for r in changes),
            "OWNED_OBSERVATION", "source-issued selected field observation mismatch")
    return {
        "mode": "read-only owned observations, no whole-loop validation",
        "whole_loop_validator": "NOT_RUN/NO_APPLICABLE_PUBLIC_API",
        "records": len(records), "fragments": fragments,
        "worlds": sorted({r["world"] for r in fragments}), "loops": loops,
        "field_changes": changes,
        "replace_field_observability": "OBSERVED" if changes else "NO_PUBLIC_FIELD_RECEIPT_ON_REPLACE",
        "usize_bits": contracts[0]["usize_bits"],
        "boundary": "IDs are world-qualified historical observations, not C addresses or present authority",
    }


def main():
    compiler, probe, host_cc, host_san, source_dir, output, fixed_root = sys.argv[1:8]
    own, out, fixed = Path(source_dir).resolve(), Path(output).resolve(), Path(fixed_root).resolve()
    require(os.environ.get("GITHUB_ACTIONS") == "true", "ENVIRONMENT",
            "compiler/native execution is authorized only in GitHub Actions")
    require(not out.exists(), "EVIDENCE", "never replace or reuse evidence from another run")
    out.mkdir(parents=True)
    record = Recorder(out)
    report = {
        "schema": "R1179_C244A_EXECUTION_V1", "state": "RUNNING",
        "compiler_commit": PIN, "compiler_tree": TREE,
        "source_bundle_sha256": BUNDLE_SHA, "oracle_sha256": ORACLE_SHA,
        "independence": "pre-execution frozen independent cohort / scope-calibrated V2; post-freeze incidental P CI-summary exposure",
        "whole_loop_validator": "NOT_RUN/NO_APPLICABLE_PUBLIC_API",
        "host_compiler": host_cc, "host_sanitizer": host_san,
        "github": {x: os.environ.get(x) for x in (
            "GITHUB_SHA", "GITHUB_REF", "GITHUB_HEAD_REF", "GITHUB_BASE_REF",
            "GITHUB_RUN_ID", "GITHUB_RUN_ATTEMPT", "GITHUB_JOB")},
        "positives": [], "negatives": [], "guest_runs": [],
        "holds": "audit locality UNMEASURED; GO624 PAUSED; H1 HOLD; P1036-PR1050 STOP",
    }
    executables = out.parent / ("r1179-guest-executables-" + out.name)
    executables.mkdir()
    try:
        bundle_bytes = (own / "c244_r1179_freeze_v2.json").read_bytes()
        require(digest(bundle_bytes) == BUNDLE_SHA, "SOURCE_IDENTITY", "V2 bundle changed")
        bundle = json.loads(bundle_bytes)
        for entry in bundle["files"]:
            content = (own / entry["filename"]).read_bytes()
            require(len(content) == entry["byte_count"] and digest(content) == entry["sha256"] and
                    content == entry["content"].encode("utf-8"),
                    "SOURCE_IDENTITY", "frozen payload differs: " + entry["filename"])
        oracle_bytes = (own / "c244_r1179_oracle_v2.json").read_bytes()
        require(digest(oracle_bytes) == ORACLE_SHA, "SOURCE_IDENTITY", "oracle changed")
        oracle = json.loads(oracle_bytes)
        for key in ("fixed-context.json",):
            path = fixed.parent / ("r1179-" + key)
            require(path.is_file(), "FIXED_CONTEXT", "missing fixed Git receipt")
            shutil.copyfile(path, out / key)
        for flag, expected in [("HEAD", PIN), ("HEAD^{tree}", TREE)]:
            rc, so, se, _ = record.run("fixed-identity", ["git", "-C", fixed, "rev-parse", flag])
            require(rc == 0 and not se and so.decode().strip() == expected,
                    "FIXED_CONTEXT", "fixed Git identity mismatch")
        rc, so, se, _ = record.run("fixed-clean", ["git", "-C", fixed, "status", "--porcelain=v1"])
        require(rc == 0 and not so and not se, "FIXED_CONTEXT", "fixed context not clean")
        for binary, name in [(compiler, "production-compiler"), (probe, "public-probe")]:
            data = Path(binary).read_bytes()
            write_json(out / (name + ".binary.json"), {"path": binary, "bytes": len(data),
                                                       "sha256": digest(data)})

        positive_work = []
        usize_bits = None
        # Frontend/public checks precede every guest native execution.
        for case in oracle["positive_and_control"]:
            sid = case["id"]
            folder = out / sid
            folder.mkdir()
            source = own / case["source_file"]
            rc, so, se, receipt = record.run(sid + "-check", [compiler, "--check", source])
            require(rc == 0 and not so and not se, "POSITIVE_CHECK",
                    sid + " did not pass ordinary CLI CHECK")
            rc, po, pe, _ = record.run(sid + "-owned", [probe, source])
            (folder / "public-owned.jsonl").write_bytes(po)
            require(rc == 0 and not pe, "OWNED_OBSERVATION", sid + " public probe failed")
            observed = public_observations(po, case)
            write_json(folder / "public-summary.json", observed)
            usize_bits = observed["usize_bits"] if usize_bits is None else usize_bits
            require(usize_bits == observed["usize_bits"], "TARGET", "usize width differs by source")
            rc, c, ce, _ = record.run(sid + "-emit", [compiler, source])
            (folder / "original.c").write_bytes(c)
            require(rc == 0 and c and not ce, "POSITIVE_EMISSION",
                    sid + " ordinary emitter did not produce C")
            positive_work.append((case, folder))
            report["positives"].append({"id": sid, "check": "PASS", "owned": "OBSERVED",
                                        "emission": "PASS", "source_file": case["source_file"]})
            write_json(out / "report.json", report)

        require(usize_bits in (32, 64), "TARGET",
                "N05 width applicability requires C adjudication beyond 64-bit usize")
        for case in oracle["negatives"]:
            sid = case["id"]
            source = own / case["source_file"]
            rc, so, se, receipt = record.run(sid + "-check", [compiler, "--check", source])
            legal_scope = case["class"].startswith("SCOPE_NEGATIVE")
            item = {"id": sid, "frozen_class": case["class"], "actual_check_rc": rc,
                    "actual_check_stderr": se.decode("utf-8", errors="replace"),
                    "intended_layer": case["predicted_first_relevant_layer"],
                    "native": "NOT_RUN_SCOPE_NEGATIVE" if legal_scope else "NOT_RUN_INVALID_SOURCE"}
            report["negatives"].append(item)
            write_json(out / "report.json", report)
            require(not so and rc in (0, 3), "NEGATIVE_FRONTEND",
                    sid + " unexpected CLI status or stdout")
            if not legal_scope:
                require(rc == 3, "CANONICAL_INVALID_ACCEPTED",
                        sid + " canonically invalid CHECK acceptance; affected guest native STOP")
            erc, eo, ee, _ = record.run(sid + "-emit-classification", [compiler, source])
            item.update(actual_emit_rc=erc, actual_emit_stdout_bytes=len(eo),
                        actual_emit_stderr=ee.decode("utf-8", errors="replace"))
            if not legal_scope:
                require(erc == 3 and not eo, "CANONICAL_INVALID_EMITTED",
                        sid + " invalid source emitted code; guest native STOP")
            else:
                require(isinstance(erc, int) and erc >= 0, "SCOPE_NEGATIVE",
                        sid + " emitter crash or timeout")
                require((rc == 3 and erc == 3 and not eo) or
                        (rc == 0 and ((erc == 0 and eo and not ee) or
                                      (erc != 0 and not eo and b"V1-BACKEND-UNSUPPORTED" in ee))),
                        "SCOPE_NEGATIVE", sid + " unexpected CHECK/emission relationship")
                item["classification"] = (
                    "CHECK_ACCEPTED_SCOPE_OUTSIDE" if rc == 0 else "EARLIER_CHECK_REJECTION_SCOPE_OUTSIDE")
                item["emission"] = "EMITTED_BUT_NOT_EXECUTED" if erc == 0 else "REFUSED"
                if eo:
                    (out / (sid + "-scope-only.c")).write_bytes(eo)
            write_json(out / "report.json", report)

        gcc = shutil.which("gcc-13") or shutil.which("gcc")
        clang = shutil.which("clang-23") or shutil.which("clang")
        require(gcc and clang, "NATIVE_TOOLING", "strict GCC and Clang are required")
        for name, binary in [("gcc", gcc), ("clang", clang)]:
            rc, so, se, _ = record.run(name + "-version", [binary, "--version"])
            require(rc == 0, "NATIVE_TOOLING", name + " version unavailable")
        strict = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]

        for case, folder in positive_work:
            sid = case["id"]
            c_path = folder / "original.c"
            passive = folder / "passive.c"
            mapping_path = folder / "role-map.json"
            rc, ast, ae, _ = record.run(sid + "-ast", [
                clang, *strict, "-Xclang", "-ast-dump=json", "-fsyntax-only", c_path])
            (folder / "original.ast.json").write_bytes(ast)
            require(rc == 0 and not ae, "OBSERVER_AST", sid + " unmodified C AST failed")
            rc, oo, oe, _ = record.run(sid + "-observer", [
                sys.executable, own / "observer.py", c_path,
                folder / "original.ast.json", passive, mapping_path])
            require(rc == 0 and not oe, "MECHANISM_RECOGNIZER",
                    sid + " C correspondence unrecognized; original C/AST preserved")
            mapping = json.loads(mapping_path.read_text())
            modes = [(name + "-" + opt, binary, ["-" + opt], {})
                     for name, binary in [("gcc", gcc), ("clang", clang)]
                     for opt in ("O0", "O2")]
            modes += [
                ("guest-ASan", clang, ["-O1", "-g", "-fsanitize=address",
                                     "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"],
                 {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1"}),
                ("guest-UBSan", clang, ["-O1", "-g", "-fsanitize=undefined",
                                      "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"],
                 {"UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}),
            ]
            for mode, cc, flags, env_delta in modes:
                for kind, path in [("unmodified", c_path), ("passive", passive)]:
                    label = sid + "-" + mode + "-" + kind
                    binary = executables / (label + ".exe")
                    rc, co, ce, compile_receipt = record.run(
                        label + "-compile", [cc, *strict, *flags, path, "-o", binary])
                    require(rc == 0 and not co and not ce, "NATIVE_COMPILATION",
                            label + " strict compilation failed")
                    binary_data = binary.read_bytes()
                    rc, no, ne, native_receipt = record.run(
                        label + "-execute", [binary], env_delta=env_delta, timeout=15)
                    item = {"id": sid, "mode": mode, "kind": kind,
                            "compiler": cc, "flags": strict + flags,
                            "compile_receipt": compile_receipt["serial"],
                            "native_receipt": native_receipt["serial"],
                            "binary_sha256": digest(binary_data), "binary_bytes": len(binary_data),
                            "returncode": rc, "stderr_bytes": len(ne)}
                    report["guest_runs"].append(item)
                    write_json(out / "report.json", report)
                    require(rc == 0 and not ne, "NATIVE_OR_SANITIZER",
                            label + " real native/sanitizer failure; affected further execution STOP")
                    if kind == "passive":
                        item["trace"] = check_passive(no, case, mapping)
                    else:
                        require(not no, "UNMODIFIED_NATIVE", label + " unexpected guest stdout")
                    item["result"] = "PASS"
                    write_json(out / "report.json", report)
        rc, so, se, _ = record.run("fixed-clean-final", [
            "git", "-C", fixed, "status", "--porcelain=v1", "--untracked-files=all"])
        require(rc == 0 and not so and not se, "FIXED_CONTEXT", "fixed context changed during CTest")
        report["state"] = "PASS_FINITE_RUNTIME_AND_PUBLIC_OBSERVATIONS"
        report["counts"] = {
            "positive_control": len(report["positives"]),
            "negative_classifications": len(report["negatives"]),
            "guest_executions": len(report["guest_runs"]),
            "guest_asan": sum(x["mode"] == "guest-ASan" for x in report["guest_runs"]),
            "guest_ubsan": sum(x["mode"] == "guest-UBSan" for x in report["guest_runs"]),
            "commands": len(record.commands),
        }
        print("R1179 FINITE PASS " + json.dumps(report["counts"], sort_keys=True))
        return 0
    except BaseException as error:
        report["state"] = "STOP"
        report["stop_layer"] = getattr(error, "layer", "MECHANISM_UNEXPECTED")
        report["stop_reason"] = str(error)
        (out / "exception.txt").write_text(traceback.format_exc(), encoding="utf-8")
        print("R1179 STOP " + report["stop_layer"] + ": " + str(error), file=sys.stderr)
        return 1
    finally:
        write_json(out / "report.json", report)
        files = []
        for path in sorted(out.rglob("*")):
            if path.is_file() and path.name != "evidence-index.json":
                data = path.read_bytes()
                files.append({"path": str(path.relative_to(out)), "bytes": len(data),
                              "sha256": digest(data)})
        write_json(out / "evidence-index.json", {"files": files, "count": len(files)})


if __name__ == "__main__":
    raise SystemExit(main())
