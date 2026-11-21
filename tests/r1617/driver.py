#!/usr/bin/env python3
"""R1617 finite evidence procedure; capability PASS is never implied by exit0."""
import argparse
import json
import os
from pathlib import Path
import shlex
import stat
import sys
import time

# Own module import must not create an untracked __pycache__ in the checkout.
sys.dont_write_bytecode = True
from capture import capture_process, fallback_notice, json_bytes, sha256, write_new

TEST_NAME = "r1617_multiarg.integration"
ORDER = [
    "05_missing_actual", "06_unsafe_unused_actual",
    "S1_copy_branch_calls", "S2_unused_actual_pipeline",
    "01_ratio_forward", "02_ratio_reversed",
    "03_copy_two_calls", "04_unused_actual_order",
]
STAGES = ("check", "observer", "emit", "cc", "native")
DEADLINES = {"check": 8.0, "observer": 8.0, "emit": 8.0,
             "cc": 20.0, "native": 4.0}
WALL_SECONDS = 360.0
MAX_CHILDREN = 34
MAX_STREAM_BYTES = 262144
MAX_FILE_BYTES = 8 * 1024 * 1024
MAX_RAW_BYTES = 32 * 1024 * 1024
MAX_RAW_FILES = 256
SENTINEL = b"R1617-NO-CLOBBER\n"
STANDARD_ENV = {
    "PYTHONDONTWRITEBYTECODE": "1",
    "PYTHONHASHSEED": "0",
    "LC_ALL": "C",
    "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
    "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1",
}
PARSE_REFUSALS = {"LEXICALLY_UNSUPPORTED", "SYNTAX_UNSUPPORTED", "SYNTAX_ERROR"}
CHECK_REFUSALS = {"SEMANTIC_ERROR", "SEMANTIC_UNSUPPORTED", "ANALYSIS_PRECISION_LIMIT"}
PHASES = [
    "source_load", "parser_create", "original_parse", "semantic_create",
    "original_register", "entry_source_create", "entry_parser_create",
    "entry_parse", "entry_check",
]
RECORD_KINDS = {
    "begin", "source_load", "source", "phase", "syntax", "function_syntax",
    "formal_syntax", "call_syntax", "binding_syntax", "unwalked_syntax_kind",
    "world", "admission", "entry_source_create", "diagnostic", "renderer",
    "unit_diagnostic_input", "value", "binding", "fragment_edge", "fragment",
    "checked_node", "division", "remainder", "fragment_walk_end",
    "missing_fragment", "observation_limit", "terminal",
}
CHECKED_KINDS = {
    "value", "binding", "fragment_edge", "fragment", "checked_node",
    "division", "remainder", "fragment_walk_end", "missing_fragment",
}

class Stop(Exception):
    def __init__(self, message, grade="EVIDENCE_FAILURE_STOP"):
        super().__init__(message)
        self.grade = grade

digest = sha256
encode = json_bytes
create = write_new

def unique_object(items):
    result = {}
    for key, value in items:
        if key in result:
            raise Stop("duplicate JSON key")
        result[key] = value
    return result

def parse_json(data):
    return json.loads(data, object_pairs_hook=unique_object)

def safe_file(path, limit=MAX_FILE_BYTES):
    st = path.lstat()
    if not stat.S_ISREG(st.st_mode) or st.st_size > limit:
        raise Stop("nonregular/oversized file: " + str(path))
    with path.open("rb") as stream:
        data = stream.read(limit + 1)
    if len(data) != st.st_size or len(data) > limit:
        raise Stop("file changed/oversized during read: " + str(path))
    return data

class Driver:
    def __init__(self, args):
        # Construction does not touch disk; main retains this object if prepare fails.
        self.args = args
        self.source_dir = Path(args.source_dir)
        self.raw = Path(args.raw_dir).absolute()
        self.raw_created = False
        self.start = time.monotonic()
        self.deadline = self.start + WALL_SECONDS
        self.child_count = 0
        self.spawned_count = 0
        self.reports = {}
        self.attempts = []
        self.terminal = "STARTED"
        self.stop_reason = None
        self.active_case = None
        self.active_stage = "configuration"
        self.cases = {}
        self.first_failure = {}
        self.attempted = set()
        self.validated_inputs = {}
        self.findings = []
        self.unsafe_admission = False
        self.evidence_incomplete = False
        self.persistence_errors = []

    def prepare(self):
        self.source_dir = self.source_dir.resolve(strict=True)
        if self.raw.exists() or self.raw.is_symlink():
            raise Stop("raw destination already exists; no clobber")
        self.raw.mkdir(parents=True, exist_ok=False)
        self.raw_created = True
        for name in ("first-oracle.json", "supplemental-oracle.json"):
            original = safe_file(self.source_dir / name)
            create(self.raw / "inputs" / name, original)
            oracle = parse_json(original.decode("utf-8"))
            for case in oracle["cases"]:
                if case["id"] in self.cases:
                    raise Stop("duplicate oracle case")
                self.cases[case["id"]] = case
        if set(self.cases) != set(ORDER) or len(self.cases) != 8:
            raise Stop("case identity mismatch")

    def clock_gate(self):
        if time.monotonic() >= self.deadline:
            raise Stop("periodic 360s driver clock exhausted", "HEALTH_OR_RESOURCE_STOP")

    def raw_budget(self):
        # A continuation gate, not a continuously enforced filesystem quota.
        self.clock_gate()
        count = total = 0
        for path in self.raw.rglob("*"):
            if path.is_symlink():
                raise Stop("symlink in raw evidence")
            if path.is_dir():
                continue
            data = safe_file(path)
            count += 1
            total += len(data)
        if count > MAX_RAW_FILES or total > MAX_RAW_BYTES:
            raise Stop("raw evidence budget exceeded")
        self.clock_gate()
        return {"files": count, "bytes": total}

    def record_failure(self, case_id, stage, reason):
        self.first_failure.setdefault(case_id, {"stage": stage, "reason": reason})

    def finding(self, case_id, stage, kind, detail):
        item = {"ordinal": len(self.findings) + 1, "case": case_id,
                "stage": stage, "kind": kind, "detail": detail}
        self.findings.append(item)
        if kind == "UNSAFE_ADMISSION_STOP":
            self.unsafe_admission = True
            self.reports[case_id]["grade"] = kind
        try:
            create(self.raw / "latches" / ("%02d.json" % item["ordinal"]), encode(item))
        except BaseException as exc:
            self.evidence_incomplete = True
            self.persistence_errors.append({
                "action": "finding_persistence", "finding": item["ordinal"],
                "type": type(exc).__name__, "message": str(exc)[:1024],
            })
            raise Stop("latched finding could not be completely persisted") from exc
        return item

    def prefix_rows(self, out):
        """Only complete captured JSON lines; malformed later output erases nothing."""
        rows = []
        errors = []
        for ordinal, line in enumerate(out.splitlines(keepends=True)):
            if ordinal >= 8192:
                errors.append({"line": ordinal + 1, "problem": "record_limit"})
                break
            if not line.endswith(b"\n"):
                errors.append({"line": ordinal + 1, "problem": "incomplete_line"})
                break
            try:
                row = parse_json(line.decode("utf-8"))
                if not isinstance(row, dict):
                    raise Stop("record is not an object")
                rows.append((ordinal + 1, row))
            except BaseException as exc:
                errors.append({"line": ordinal + 1, "problem": type(exc).__name__})
        return rows, errors

    def latch_original_admission_prefix(self, case_id, out, mode):
        if self.cases[case_id]["law"] != "INVALID":
            return
        rows, errors = self.prefix_rows(out)
        expected_begin = {"record": "begin", "schema": "r1617-observer-v2",
                          "mode": mode}
        if not rows or rows[0] != (1, expected_begin):
            return
        original = self.validated_inputs[case_id][1]
        original_ids = set()
        for line, row in rows[1:]:
            if row.get("record") == "source" and type(row.get("source")) is int \
                    and row["source"] > 0 and row.get("bytes") == len(original) \
                    and row.get("hex") == original.hex():
                original_ids.add(row["source"])
            admitted = (
                row.get("record") == "phase"
                and row.get("phase") == "original_register"
                and row.get("status") == "OK"
                and type(row.get("source")) is int
                and row["source"] in original_ids
            ) or (
                row.get("record") == "admission"
                and row.get("phase") == "original_register"
                and row.get("status") == "OK" and row.get("admitted") is True
                and type(row.get("source")) is int
                and row["source"] in original_ids
            )
            if admitted:
                self.record_failure(case_id, "observer", "UNSAFE_ADMISSION_STOP")
                self.finding(case_id, "observer", "UNSAFE_ADMISSION_STOP", {
                    "fact": "actual original public registration reported OK",
                    "source_sha256": digest(original), "mode": mode,
                    "line": line, "record": row,
                    "prefix_parse_errors": errors,
                    "terminal_health_and_cleanup_cannot_erase_admission": True,
                })
                return

    def run(self, case_id, stage, argv, cwd, observer_mode=None):
        self.active_case = case_id
        self.active_stage = stage
        self.clock_gate()
        if self.child_count >= MAX_CHILDREN:
            raise Stop("direct child attempt ceiling exhausted", "HEALTH_OR_RESOURCE_STOP")
        if (case_id, stage) in self.attempted:
            raise Stop("duplicate attempt; first result must not be replaced")
        self.child_count += 1
        ordinal = self.child_count
        self.attempted.add((case_id, stage))
        prefix = self.raw / "children" / (f"{ordinal:02d}-{case_id}-{stage}")
        result = capture_process(
            argv, cwd, prefix, DEADLINES[stage], self.deadline,
            MAX_STREAM_BYTES, MAX_FILE_BYTES, 24,
        )
        rec = result["record"]
        rec.update({"ordinal": ordinal, "case": case_id, "stage": stage,
                    "not_retried": True, "capture_prefix": str(prefix)})
        self.attempts.append(rec)
        self.spawned_count += int(rec["spawn_succeeded"])
        out, err = result["stdout"], result["stderr"]
        # Admission has priority over every later health/stream/persistence check.
        if stage == "check" and self.cases[case_id]["law"] == "INVALID" \
                and rec["returncode"] == 0:
            self.record_failure(case_id, stage, "UNSAFE_ADMISSION_STOP")
            self.finding(case_id, stage, "UNSAFE_ADMISSION_STOP", {
                "fact": "real CHECK of byte-identical invalid original exited0",
                "source_sha256": digest(self.validated_inputs[case_id][1]),
                "child_ordinal": ordinal,
                "health_or_capture_failure_does_not_erase_exit0": True,
            })
        if stage == "observer":
            self.latch_original_admission_prefix(case_id, out, observer_mode)
        if self.unsafe_admission:
            raise Stop(case_id + ": invalid original admitted; all later work forbidden",
                       "UNSAFE_ADMISSION_STOP")
        if stage == "emit":
            # Caller retains the sentinel/postimage facts before classifying health.
            return rec, out, err
        self.require_child_completion(case_id, stage, rec)
        return rec, out, err

    def require_child_completion(self, case_id, stage, rec):
        if not rec["process_healthy"] or not rec["persistence_complete"]:
            if not rec["persistence_complete"]:
                self.evidence_incomplete = True
            self.record_failure(case_id, stage, "CHILD_HEALTH_CAPTURE_OR_CLEANUP_STOP")
            raise Stop(case_id + "/" + stage + ": process/capture/group/persistence failure",
                       "HEALTH_OR_RESOURCE_STOP")
        self.raw_budget()
        for source_path, original_bytes in self.validated_inputs.values():
            if safe_file(source_path, 8192) != original_bytes:
                raise Stop("original input changed during child execution")

    def observer_records(self, case_id, result, mode):
        rec, out, err = result
        indexed, prefix_errors = self.prefix_rows(out)
        rows = [row for _, row in indexed]
        if prefix_errors or not rows or len(rows) > 8192 \
                or rows[0] != {"record": "begin", "schema": "r1617-observer-v2",
                              "mode": mode}:
            raise Stop(case_id + ": malformed observer begin/records")
        if any(row.get("record") not in RECORD_KINDS for row in rows):
            raise Stop(case_id + ": unclassified observer record")
        if sum(row["record"] == "begin" for row in rows) != 1:
            raise Stop(case_id + ": duplicate observer begin")
        ends = [r for r in rows if r["record"] == "terminal"]
        limits = [r for r in rows if r["record"] == "observation_limit"]
        if len(ends) != 1 or rows[-1] != ends[0] or len(limits) != 1 \
                or len(rows) < 2 or rows[-2] != limits[0]:
            raise Stop(case_id + ": missing/nonterminal observer terminal/limits")
        end = ends[0]
        code = rec["returncode"]
        if type(end.get("rc")) is not int or end["rc"] != code \
                or type(end.get("observation_gap")) is not bool \
                or end.get("evidence_error") is not False:
            raise Stop(case_id + ": observer terminal/exit/evidence contradiction")
        if limits[0] != {
                "record": "observation_limit",
                "actual_to_formal_identity": "NOT_EXPOSED_BY_SELECTED_HEADERS",
                "native_temporal_order": "NOT_OBSERVED",
                "synthetic_guest_state": False, "oracle_read": False}:
            raise Stop(case_id + ": observer limit contract mismatch")
        states = {
            (0, "registration"): "REGISTRATION_ONLY_ENTRY_NOT_RUN",
            (0, "entry"): "PUBLIC_OBSERVATION_COMPLETE",
            (10, "registration"): "ORIGINAL_PARSE_REFUSAL",
            (10, "entry"): "ORIGINAL_PARSE_REFUSAL",
            (11, "registration"): "ORIGINAL_REGISTRATION_REFUSAL",
            (11, "entry"): "ORIGINAL_REGISTRATION_REFUSAL",
            (12, "entry"): "ENTRY_REFUSAL",
        }
        if (code, mode) not in states or end.get("state") != states[(code, mode)]:
            raise Stop(case_id + ": unsupported phase/mode/terminal/exit combination")
        length = 3 if code == 10 else 5 if code == 11 or mode == "registration" else 9
        phases = [r for r in rows if r["record"] == "phase"]
        if len(phases) != length or [r.get("phase") for r in phases] != PHASES[:length]:
            raise Stop(case_id + ": phase sequence mismatch")
        sources = [r for r in rows if r["record"] == "source"]
        source_map = {}
        for row in sources:
            sid = row.get("source")
            if type(sid) is not int or sid != len(source_map) + 1:
                raise Stop(case_id + ": duplicate/nonsequential source identity")
            try:
                value = bytes.fromhex(row["hex"])
            except (KeyError, TypeError, ValueError) as exc:
                raise Stop(case_id + ": malformed source bytes") from exc
            if type(row.get("bytes")) is not int or row["bytes"] != len(value) \
                    or row["hex"] != value.hex() or len(value) > 8192:
                raise Stop(case_id + ": source byte count mismatch")
            source_map[sid] = value
        if source_map.get(1) != self.validated_inputs[case_id][1]:
            raise Stop(case_id + ": original observer bytes differ")
        if length == 9 and source_map.get(2) != b"main()\n":
            raise Stop(case_id + ": entry adapter bytes differ")
        original_source = next(r for r in sources if r["source"] == 1)
        if not rows.index(phases[0]) < rows.index(original_source) < rows.index(phases[1]):
            raise Stop(case_id + ": original source/phase order mismatch")
        if length == 9:
            entry_source = next(r for r in sources if r["source"] == 2)
            if not rows.index(phases[5]) < rows.index(entry_source) < rows.index(phases[6]):
                raise Stop(case_id + ": entry source/phase order mismatch")
        for index, row in enumerate(phases):
            if set(row) != {"record", "phase", "status", "source"}:
                raise Stop(case_id + ": phase field mismatch")
            expected_source = 0 if index in (0, 5) else 1 if index < 5 else 2
            if type(row["source"]) is not int or row["source"] != expected_source:
                raise Stop(case_id + ": phase source identity mismatch")
            if index < length - 1 or code == 0:
                if row["status"] != "OK":
                    raise Stop(case_id + ": premature non-OK phase")
            elif row["status"] not in (PARSE_REFUSALS if code == 10 else CHECK_REFUSALS):
                raise Stop(case_id + ": unclassified refusal status")
        raw_loads = [r for r in rows if r["record"] == "source_load"]
        raw_entries = [r for r in rows if r["record"] == "entry_source_create"]
        if raw_loads != [{"record": "source_load", "status": 0}] \
                or raw_entries != ([{"record": "entry_source_create", "status": 0}]
                                   if length == 9 else []):
            raise Stop(case_id + ": source creation record mismatch")
        admissions = [r for r in rows if r["record"] == "admission"]
        expected_admissions = []
        if length >= 5 and phases[4]["status"] == "OK":
            expected_admissions.append({"record": "admission",
                "phase": "original_register", "source": 1, "status": "OK", "admitted": True})
        if length == 9 and code == 0:
            expected_admissions.append({"record": "admission",
                "phase": "entry_check", "source": 2, "status": "OK", "admitted": True})
        if admissions != expected_admissions:
            raise Stop(case_id + ": admission/phase contradiction")
        for admission in admissions:
            phase = next(r for r in phases if r["phase"] == admission["phase"])
            if rows.index(admission) <= rows.index(phase):
                raise Stop(case_id + ": admission precedes actual phase")
        if any(r["record"] in CHECKED_KINDS for r in rows) \
                and not (code == 0 and mode == "entry"):
            raise Stop(case_id + ": checked observation without successful entry")
        if code == 0 and mode == "entry":
            admission_index = rows.index(admissions[-1])
            if any(index <= admission_index and row["record"] in CHECKED_KINDS
                   for index, row in enumerate(rows)):
                raise Stop(case_id + ": checked observation before entry admission")
        diagnostics = [r for r in rows if r["record"] == "diagnostic"]
        renderers = [r for r in rows if r["record"] == "renderer"]
        unit_inputs = [r for r in rows if r["record"] == "unit_diagnostic_input"]
        refusal = None
        if code == 0:
            if err or diagnostics or renderers or unit_inputs:
                raise Stop(case_id + ": success had unexpected observer stderr/diagnostics",
                           "STREAM_CONTRACT_STOP")
        else:
            phase = PHASES[length - 1]
            sid = 2 if code == 12 else 1
            if len(diagnostics) != 1 or len(renderers) != 1:
                raise Stop(case_id + ": refusal lacks unique structured/renderer evidence")
            diagnostic, renderer = diagnostics[0], renderers[0]
            span = diagnostic.get("span")
            if diagnostic.get("phase") != phase \
                    or type(diagnostic.get("severity")) is not int \
                    or diagnostic["severity"] != 2 \
                    or not isinstance(span, list) or len(span) != 2 \
                    or any(type(x) is not int for x in span) \
                    or not 0 <= span[0] <= span[1] <= len(source_map[sid]):
                raise Stop(case_id + ": malformed phase/severity/span diagnostic")
            try:
                for field in ("code_hex", "category_hex", "message_hex"):
                    bytes.fromhex(diagnostic[field])
                rendered = bytes.fromhex(renderer["hex"])
            except (KeyError, TypeError, ValueError) as exc:
                raise Stop(case_id + ": malformed renderer/diagnostic bytes") from exc
            if not diagnostic["message_hex"] or not rendered or len(rendered) > 32768 \
                    or renderer.get("phase") != phase or renderer.get("source") != sid \
                    or renderer.get("kind") != ("parse" if code == 10 else "check") \
                    or renderer.get("public_renderer_return") is not True \
                    or renderer.get("capture_ok") is not True \
                    or renderer.get("truncated") is not False \
                    or renderer.get("bytes") != len(rendered) \
                    or renderer.get("stderr_forwarded_bytes") != len(rendered) \
                    or renderer.get("stderr_write_ok") is not True or err != rendered:
                raise Stop(case_id + ": unexpected/unbound refusal stderr",
                           "STREAM_CONTRACT_STOP")
            if rows.index(diagnostic) <= rows.index(phases[-1]) \
                    or rows.index(renderer) <= rows.index(diagnostic):
                raise Stop(case_id + ": diagnostic/renderer phase order mismatch")
            if unit_inputs != ([{"record": "unit_diagnostic_input", "index": 0}]
                               if code == 11 else []):
                raise Stop(case_id + ": original unit diagnostic input mismatch")
            refusal = {
                "phase": phase, "public_status": phases[-1]["status"],
                "diagnostic": diagnostic, "renderer_sha256": digest(rendered),
                "renderer_bytes": len(rendered),
                "reason_confirmation": "UNCONFIRMED_REQUIRES_OWN_C_REVIEW",
            }
        for row in rows:
            if row["record"] in ("division", "remainder") and (
                    row.get("view_available") is not True or row.get("valid") is not True
                    or row.get("origin_matches") is not True):
                raise Stop(case_id + ": arithmetic origin/validator failure")
        return rows, refusal

    def compare_visible_scalars(self, case_id, rows):
        """Compare only public observations. Never create or repair guest state."""
        case = self.cases[case_id]
        nodes = {}
        fragments = {}
        edges = {}
        scalar_values = {}
        arithmetic = []
        for row in rows:
            kind = row.get("record")
            if kind == "checked_node":
                key = (row["fragment"], row["node"])
                if key in nodes:
                    raise Stop(case_id + ": duplicate checked node identity")
                nodes[key] = row
            elif kind == "fragment":
                fragments[row["fragment"]] = row
            elif kind == "fragment_edge":
                if row["role"] == "retained_call_body":
                    edges[(row["parent"], row["owner"])] = row["fragment"]
            elif kind == "value" and row.get("available"):
                key = (row["world"], row["value"])
                old = scalar_values.get(key)
                current = (row["scalar_known"], row["scalar"])
                if old is not None and old != current:
                    raise Stop(case_id + ": contradictory value observations")
                scalar_values[key] = current
            elif kind in ("division", "remainder"):
                arithmetic.append(row)
        visible = []
        missing = ["ordinary actual-to-formal identity seal",
                   "native temporal order", "native internal numeric values"]
        def scalar(fid, result):
            if not result or result.get("value", 0) == 0:
                return None
            world = fragments.get(fid, {}).get("world")
            found = scalar_values.get((world, result["value"]))
            return found[1] if found is not None and found[0] else None
        expected_calls = [e for e in case["events"] if "invocation" in e]
        for callee in sorted({e["callee"] for e in expected_calls}):
            expected = [e for e in expected_calls if e["callee"] == callee]
            candidates = []
            for (fid, nid), row in nodes.items():
                try:
                    name = bytes.fromhex(row["name_hex"]).decode("ascii")
                except (ValueError, UnicodeDecodeError):
                    continue
                if name == callee and row["body_backed"]:
                    candidates.append((fid, nid, row))
            candidates.sort(key=lambda x: (x[0], x[2]["span"][0], x[1]))
            if len(candidates) != len(expected):
                missing.append(callee + ": expected concrete call count not observed")
                continue
            for (fid, nid, row), prediction in zip(candidates, expected):
                actuals = []
                arg = row["first_argument"]
                visited = set()
                for position in range(row["argument_count"]):
                    if arg == 0 or arg in visited or (fid, arg) not in nodes:
                        raise Stop(case_id + ": invalid public argument chain")
                    visited.add(arg)
                    a = nodes[(fid, arg)]
                    actuals.append(scalar(fid, a["results"][0])
                                   if len(a["results"]) == 1 else None)
                    arg = a["next_argument"]
                if arg != 0 or row["argument_count"] != len(prediction["actuals"]):
                    raise Stop(case_id + ": actual arity differs from original")
                for got, wanted in zip(actuals, prediction["actuals"]):
                    if got is not None and got != wanted:
                        raise Stop(case_id + ": visible actual scalar mismatch", "SEMANTIC_MISMATCH_STOP")
                result = scalar(fid, row["results"][0]) \
                    if len(row["results"]) == 1 else None
                if result is not None and result != prediction["helper_result"]:
                    raise Stop(case_id + ": visible helper result mismatch", "SEMANTIC_MISMATCH_STOP")
                if any(x is None for x in actuals) or result is None:
                    missing.append(prediction["invocation"] + ": hidden scalar")
                visible.append({
                    "invocation_question": prediction["invocation"],
                    "fragment": fid, "call_node": nid,
                    "resolved_function_id": row["function"],
                    "retained_body_fragment": edges.get((fid, nid)),
                    "actual_scalar_observations": actuals,
                    "result_scalar_observation": result,
                    "formal_identity_claim": "UNOBSERVED",
                    "matching_numbers_do_not_prove_correspondence": True,
                })
        result = {
            "schema": "r1617-visible-comparison-v1",
            "case": case_id, "visible_calls": visible,
            "raw_arithmetic_events": arithmetic, "missing": missing,
            "grade_ceiling": "VALID_ACCEPTED_OBSERVATION_INCOMPLETE",
            "native_numeric_or_order_pass": False,
        }
        create(self.raw / "cases" / case_id / "visible-comparison.json",
               encode(result))
        return result

    def check_release_and_inputs(self):
        for name, value in STANDARD_ENV.items():
            if os.environ.get(name) != value:
                raise Stop("required standard ENV mismatch: " + name)
        release_bytes = safe_file(self.source_dir / "execution-release.json")
        create(self.raw / "inputs" / "execution-release.json", release_bytes)
        release = parse_json(release_bytes.decode("utf-8"))
        if release.get("execution_target") == "NOT_RELEASED":
            raise Stop("execution target NOT_RELEASED")
        if release.get("mode") != "GitHub branch + Actions ONLY":
            raise Stop("execution mode is not released")
        if os.environ.get("GITHUB_ACTIONS") != "true":
            raise Stop("Actions-only execution guard")
        candidate = release.get("candidate_commit", "")
        if len(candidate) != 40 or any(c not in "0123456789abcdef" for c in candidate):
            raise Stop("missing exact released candidate commit")
        if release.get("execution_target") != candidate:
            raise Stop("execution target is not the released candidate SHA")
        if release.get("canonical_commit") != \
                "b217d83c0b4253abf4832d36916d2107f8ab205b":
            raise Stop("canonical authority mismatch")
        if not release.get("coordination_release_url", "").startswith(
                "https://github.com/wakairo/NewLang_Compiler/issues/1617#issuecomment-"):
            raise Stop("missing own exact C execution release")
        metadata_path = Path(os.environ["RUNNER_TEMP"]) / "r1617-metadata" / "metadata.json"
        metadata_bytes = safe_file(metadata_path)
        create(self.raw / "inputs" / "execution-metadata.json", metadata_bytes)
        metadata = parse_json(metadata_bytes.decode("utf-8"))
        if metadata.get("schema") != "r1617-checkout-registration-v2" \
                or metadata.get("procedure_state") != "COMPLETED_METADATA_ONLY" \
                or metadata.get("all_commands_healthy") is not True \
                or metadata.get("command_attempts") != 5 \
                or metadata.get("spawned_direct_children") != 5 \
                or metadata.get("worktree_porcelain_hex") != "" \
                or metadata.get("clean_worktree") != {
                    "policy": "git status --porcelain must be exactly empty",
                    "verified_empty_porcelain": True,
                    "allowed_reported_tracked_or_untracked_paths": [],
                    "scope": "this checkout at metadata time; Git-ignored outputs are outside porcelain",
                }:
            raise Stop("missing complete clean-worktree metadata before target trust")
        event = metadata["environment"]
        pr = metadata["pr_event"]
        parent = metadata["candidate_parent"]
        if event.get("GITHUB_REPOSITORY") != "wakairo/NewLang_Compiler" \
                or event.get("GITHUB_EVENT_NAME") != "pull_request" \
                or event.get("GITHUB_RUN_ID") != os.environ.get("GITHUB_RUN_ID") \
                or event.get("GITHUB_RUN_ATTEMPT") != "1" \
                or metadata.get("checkout_commit") != os.environ.get("GITHUB_SHA") \
                or pr.get("base_sha") != candidate \
                or metadata.get("checkout_parents") != [candidate, pr.get("head_sha")] \
                or parent != {"commit": candidate, "tree": release.get("candidate_tree"),
                              "parents": release.get("candidate_parents")}:
            raise Stop("exact candidate/PR checkout/run/attempt authority mismatch")
        manifest_bytes = safe_file(self.source_dir / "inputs-manifest.json")
        create(self.raw / "inputs" / "inputs-manifest.json", manifest_bytes)
        manifest = parse_json(manifest_bytes.decode("utf-8"))
        if set(manifest["sources"]) != set(ORDER):
            raise Stop("source manifest identity mismatch")
        for case_id in ORDER:
            item = manifest["sources"][case_id]
            path = self.source_dir / item["path"]
            data = safe_file(path, 8192)
            if len(data) != item["bytes"] or digest(data) != item["sha256"]:
                raise Stop("source hash mismatch: " + case_id)
            execution_input = self.raw / "inputs" / (case_id + ".nl")
            create(execution_input, data)
            self.validated_inputs[case_id] = (execution_input, data)
        for name in ("first-oracle.json", "supplemental-oracle.json"):
            expected = manifest["oracles"][name]
            data = safe_file(self.source_dir / name)
            if len(data) != expected["bytes"] or digest(data) != expected["sha256"]:
                raise Stop("oracle hash mismatch: " + name)
        return release

    def execute_case(self, case_id):
        case = self.cases[case_id]
        source, data = self.validated_inputs[case_id]
        work = self.raw / "cases" / case_id
        work.mkdir(parents=True, exist_ok=False)
        report = {
            "case": case_id, "law": case["law"], "source_sha256": digest(data),
            "grade": "NOT_RUN", "entry_result": case.get("entry_result"),
            "capability_pass": False,
        }
        self.reports[case_id] = report
        check = self.run(case_id, "check",
                         [self.args.compiler, "--check", source], work)
        check_rec, check_out, check_err = check
        code = check_rec["returncode"]
        report["check"] = {"process_exit": code, "stdout_bytes": len(check_out),
                           "stderr_bytes": len(check_err)}
        if code == 0 and (check_out or check_err):
            raise Stop(case_id + ": CHECK0 had unexpected output", "STREAM_CONTRACT_STOP")
        if code not in (0, 3):
            self.record_failure(case_id, "check", "CLI_HEALTH_OR_CONTRACT_FAILURE")
            raise Stop(case_id + ": unclassified CLI status " + str(code))
        if code == 3:
            if check_out or not check_err:
                raise Stop(case_id + ": rejection output contract failure",
                           "STREAM_CONTRACT_STOP")
            self.record_failure(case_id, "check", "ORIGINAL_CHECK_REFUSAL")
        mode = "entry" if code == 0 and case["law"] == "LEGAL" else "registration"
        observed = self.run(case_id, "observer",
                            [self.args.observer, source, mode], work, mode)
        rows, refusal = self.observer_records(case_id, observed, mode)
        observer_code = observed[0]["returncode"]
        report["public_observer"] = {
            "mode": mode, "process_exit": observer_code,
            "semantic_phase": "entry_check" if observer_code == 0 and mode == "entry"
                else "original_register" if observer_code == 0 else refusal["phase"],
            "reason_evidence": refusal,
        }
        if code == 3:
            # INVALID original public registration OK already latched in run().
            if observer_code not in (10, 11) or refusal is None:
                self.record_failure(case_id, "observer", "CLI_PUBLIC_PATH_DISAGREEMENT")
                raise Stop(case_id + ": CLI3/public admission or entry contradiction",
                           "CLI_PUBLIC_PATH_DISAGREEMENT_STOP")
            if check_err != observed[2]:
                self.record_failure(case_id, "observer", "CLI_PUBLIC_REASON_UNCLASSIFIED")
                raise Stop(case_id + ": CLI3 stderr is not exact original public renderer evidence",
                           "CLI_PUBLIC_REASON_UNCLASSIFIED_STOP")
            kind = ("INVALID_REJECTION_REASON_UNCONFIRMED" if case["law"] == "INVALID"
                    else "LEGAL_REFUSAL_UNCLASSIFIED_NONPASS")
            report["grade"] = kind
            report["reason_confirmation"] = "UNCONFIRMED_REQUIRES_OWN_C_REVIEW"
            report["entry_adapter"] = "NOT_RUN"
            report["c17_native"] = "NOT_RUN"
            self.finding(case_id, "observer", kind, {
                "fact": "healthy CLI3 and genuine original public refusal have identical rendered stderr",
                "semantic_phase": refusal["phase"], "public_status": refusal["public_status"],
                "reason_confirmation": report["reason_confirmation"],
                "capability_pass": False,
            })
            return
        if observer_code != 0:
            self.record_failure(case_id, "observer", "CLI_PUBLIC_PATH_DISAGREEMENT")
            self.finding(case_id, "observer", "PUBLIC_REFUSAL_NONPASS", refusal)
            raise Stop(case_id + ": CLI0/public observer refusal",
                       "CLI_PUBLIC_PATH_DISAGREEMENT_STOP")
        comparison = self.compare_visible_scalars(case_id, rows)
        report["public_observation_gaps"] = comparison["missing"]
        self.finding(case_id, "observer", "OBSERVATION_GAP_NONPASS", {
            "missing": comparison["missing"], "native_numeric_or_order_pass": False,
            "actual_to_formal_identity_pass": False,
        })
        self.active_stage = "emit_prepare"
        emitted = work / "pristine.c"
        create(emitted, SENTINEL)
        before_names = set(p.name for p in work.iterdir())
        emit = self.run(case_id, "emit",
                        [self.args.compiler, "--emit-c17-to", emitted, source], work)
        emit_rec, emit_out, emit_err = emit
        after = None
        extra = None
        postimage_error = None
        try:
            after = safe_file(emitted)
            extra = sorted(set(p.name for p in work.iterdir()) - before_names)
        except BaseException as exc:
            postimage_error = {"type": type(exc).__name__, "message": str(exc)[:1024]}
        emit_facts = {
            "process_exit": emit_rec["returncode"],
            "process_completion": emit_rec["process_completion"],
            "process_healthy": emit_rec["process_healthy"],
            "capture_complete": emit_rec["capture_complete"],
            "persistence_complete": emit_rec["persistence_complete"],
            "stdout_bytes": len(emit_out), "stderr_bytes": len(emit_err),
            "stdout_sha256": digest(emit_out), "stderr_sha256": digest(emit_err),
            "sentinel_preserved": after == SENTINEL if after is not None else None,
            "postimage_bytes": len(after) if after is not None else None,
            "postimage_sha256": digest(after) if after is not None else None,
            "unexpected_side_files": extra, "postimage_error": postimage_error,
            "reason_classification": "UNCLASSIFIED",
        }
        report["emission"] = emit_facts
        if emit_rec["returncode"] != 0:
            report["grade"] = "EMISSION_UNCLASSIFIED_NONPASS"
            self.record_failure(case_id, "emit", "EMISSION_UNCLASSIFIED_NONPASS")
            self.finding(case_id, "emit", "EMISSION_UNCLASSIFIED_NONPASS", emit_facts)
            self.require_child_completion(case_id, "emit", emit_rec)
            raise Stop(case_id + ": unclassified emission NONPASS; exact reason requires C review",
                       "EMISSION_UNCLASSIFIED_NONPASS")
        self.require_child_completion(case_id, "emit", emit_rec)
        if postimage_error is not None or extra:
            raise Stop(case_id + ": named-output postimage/side-file failure")
        if emit_out or emit_err or not after or after == SENTINEL:
            raise Stop(case_id + ": invalid named-output success contract",
                       "STREAM_CONTRACT_STOP")
        report["emission"]["reason_classification"] = "NAMED_OUTPUT_SUCCESS_CONTRACT_OBSERVED"
        c_identity = {"bytes": len(after), "sha256": digest(after),
                      "observation": "pristine compiler C17; unmodified"}
        create(work / "pristine-c-identity.json", encode(c_identity))
        native = work / "pristine-native"
        if native.exists() or native.is_symlink():
            raise Stop(case_id + ": native destination preexists")
        self.active_stage = "cc_prepare"
        flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                 "-O2" if self.args.build_type == "Release" else "-O0"]
        inherited = shlex.split(os.environ.get("CFLAGS", ""))
        if inherited not in ([], ["-Wno-error=gcc-install-dir-libstdcxx"]):
            raise Stop("unexpected inherited CFLAGS; no silent weakening")
        flags += inherited
        if self.args.sanitizer != "none":
            flags += ["-fsanitize=" + self.args.sanitizer,
                      "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
        compiled = self.run(case_id, "cc",
                            [self.args.cc, *flags, emitted, "-o", native], work)
        report["host_compile"] = {
            "process_exit": compiled[0]["returncode"],
            "stdout_bytes": len(compiled[1]), "stderr_bytes": len(compiled[2]),
            "allowed_success_diagnostic_bytes": [],
            "inherited_warning_flag_does_not_authorize_ignored_output": True,
        }
        if compiled[0]["returncode"] != 0:
            self.record_failure(case_id, "cc", "PRISTINE_C17_COMPILE_FAILURE")
            raise Stop(case_id + ": pristine emitted C did not compile",
                       "PRISTINE_C17_COMPILE_NONPASS")
        if compiled[1] or compiled[2]:
            self.record_failure(case_id, "cc", "HOST_COMPILE_STREAM_NONPASS")
            self.finding(case_id, "cc", "HOST_COMPILE_STREAM_NONPASS", report["host_compile"])
            raise Stop(case_id + ": host success had unexpected streams; no warning whitelist",
                       "HOST_COMPILE_STREAM_NONPASS")
        if safe_file(emitted) != after:
            raise Stop(case_id + ": pristine C bytes changed")
        native_data = safe_file(native)
        create(work / "native-identity.json", encode({
            "bytes": len(native_data), "sha256": digest(native_data),
            "pristine_c_sha256": digest(after), "argv": compiled[0]["argv"],
        }))
        executed = self.run(case_id, "native", [native], work)
        if executed[0]["returncode"] != 0 or executed[1] or executed[2]:
            self.record_failure(case_id, "native", "NATIVE_OR_SANITIZER_STOP")
            raise Stop(case_id + ": native failed or emitted unexpected output",
                       "NATIVE_OR_SANITIZER_STOP")
        report["grade"] = "VALID_ACCEPTED_OBSERVATION_INCOMPLETE"
        report["pristine_native"] = {
            "exit": 0, "stdout_bytes": 0, "stderr_bytes": 0,
            "claim": "finite native health/unit completion only",
            "internal_numeric_or_temporal_order_proof": False,
        }
        report["passive_native_observation"] = "NOT_PLANNED"
        self.raw_budget()

    def finish(self):
        for case_id in ORDER:
            self.reports.setdefault(case_id, {
                "case": case_id, "law": self.cases.get(case_id, {}).get("law", "UNKNOWN"),
                "grade": "NOT_RUN", "reason": self.stop_reason or "not reached",
                "capability_pass": False,
            })
            for stage in STAGES:
                if (case_id, stage) not in self.attempted:
                    self.attempts.append({
                        "case": case_id, "stage": stage, "status": "NOT_RUN",
                        "reason": self.stop_reason or
                            "prior refusal or invalid-control stage prohibition",
                    })
        if self.unsafe_admission:
            self.terminal = "UNSAFE_ADMISSION_STOP"
        terminal = {
            "schema": "r1617-run-v2", "test": TEST_NAME,
            "procedure_state": self.terminal, "stop_reason": self.stop_reason,
            "procedure_exit0_means": "evidence protocol completed with explicit NONPASS/gaps",
            "capability_pass": False, "capability_status": "NONPASS_OR_UNOBSERVED",
            "unsafe_admission_latched": self.unsafe_admission,
            "evidence_incomplete": self.evidence_incomplete,
            "persistence_errors": self.persistence_errors,
            "command_attempt_slots": self.child_count, "spawned_children": self.spawned_count,
            "maximum_direct_children": MAX_CHILDREN,
            "wall_seconds": time.monotonic() - self.start,
            "periodic_clock_limit_seconds": WALL_SECONDS, "outer_ctest_timeout": 420,
            "clock_limit_scope": "cooperative gates/polls; traversal/hash/write syscalls not continuously timed",
            "group_scope_limit": "launched PGID absence only; escaped descendants not excluded",
            "cases": [self.reports[x] for x in ORDER],
            "latched_findings": self.findings,
            "first_failures": self.first_failure, "attempts": self.attempts,
            "product_go": "PAUSED", "h1": "HOLD", "audit_locality": "UNMEASURED",
            "numeric_correspondence_or_order_pass": False,
            "terminal_write_self_confirmation": "not self-certifying; enclosing runner receipt required",
            "terminal_precedes_runner_summary": True,
        }
        persisted = False
        try:
            if not self.raw_created:
                raise OSError("fresh raw root unavailable; no overwrite attempted")
            create(self.raw / "terminal.json", encode(terminal))
            persisted = True
        except BaseException as exc:
            self.evidence_incomplete = True
            self.persistence_errors.append({
                "action": "terminal_persistence", "type": type(exc).__name__,
                "message": str(exc)[:1024],
            })
            terminal["evidence_incomplete"] = True
            terminal["terminal_persistence_failed"] = True
            terminal["persistence_errors"] = self.persistence_errors
            if not self.unsafe_admission:
                self.terminal = "EVIDENCE_INCOMPLETE_STOP"
                terminal["procedure_state"] = self.terminal
            try:
                if self.raw_created:
                    create(self.raw / "terminal.incomplete.json", encode(terminal))
            except BaseException as fallback_exc:
                self.persistence_errors.append({
                    "action": "terminal_incomplete_persistence",
                    "type": type(fallback_exc).__name__,
                    "message": str(fallback_exc)[:1024],
                })
            fallback_notice({
                "r1617": self.terminal, "evidence_incomplete": True,
                "unsafe_admission_latched": self.unsafe_admission,
                "first_failures": self.first_failure,
                "persistence_errors": self.persistence_errors, "raw": str(self.raw),
            })
        try:
            print(json.dumps({
                "r1617_procedure_state": self.terminal,
                "terminal_write_completed": persisted,
                "command_attempt_slots": self.child_count,
                "spawned_children": self.spawned_count, "raw": str(self.raw),
                "unsafe_admission_latched": self.unsafe_admission,
                "capability_pass": False,
            }), flush=True)
        except BaseException as exc:
            self.evidence_incomplete = True
            marker = {"state": "RUNNER_SUMMARY_INCOMPLETE_STOP",
                      "unsafe_admission_latched": self.unsafe_admission,
                      "type": type(exc).__name__, "message": str(exc)[:1024]}
            try:
                if self.raw_created:
                    create(self.raw / "runner-summary-incomplete.json", encode(marker))
            except BaseException as marker_exc:
                self.persistence_errors.append({
                    "action": "runner_summary_marker_persistence",
                    "type": type(marker_exc).__name__, "message": str(marker_exc)[:1024],
                })
            fallback_notice({"r1617": "RUNNER_SUMMARY_INCOMPLETE_STOP",
                             "unsafe_admission_latched": self.unsafe_admission,
                             "type": type(exc).__name__,
                             "persistence_errors": self.persistence_errors})
            return False
        return persisted and not self.evidence_incomplete

def main():
    parser = argparse.ArgumentParser()
    for flag in ("compiler", "observer", "cc", "sanitizer", "build-type",
                 "source-dir", "raw-dir"):
        parser.add_argument("--" + flag, required=True)
    args = parser.parse_args()
    driver = Driver(args)
    try:
        if args.sanitizer not in ("none", "address", "undefined"):
            raise Stop("unknown sanitizer")
        if args.build_type not in ("Debug", "Release"):
            raise Stop("unknown build type")
        driver.prepare()
        driver.check_release_and_inputs()
        for case_id in ORDER:
            driver.execute_case(case_id)
        driver.terminal = "COMPLETED_WITH_NONPASS_OR_GAPS"
    except BaseException as exc:
        driver.terminal = "UNSAFE_ADMISSION_STOP" if driver.unsafe_admission else "STOP"
        driver.stop_reason = type(exc).__name__ + ": " + str(exc)[:2048]
        if driver.active_case is not None:
            driver.record_failure(driver.active_case, driver.active_stage, driver.stop_reason)
            report = driver.reports.get(driver.active_case)
            if report is not None:
                grade = getattr(exc, "grade", "EVIDENCE_FAILURE_STOP")
                report["terminal_stop_grade"] = grade
                if driver.unsafe_admission:
                    report["grade"] = "UNSAFE_ADMISSION_STOP"
                elif report["grade"] == "NOT_RUN":
                    report["grade"] = grade
    complete = driver.finish()
    return 0 if complete and driver.terminal == "COMPLETED_WITH_NONPASS_OR_GAPS" else 1

if __name__ == "__main__":
    raise SystemExit(main())
