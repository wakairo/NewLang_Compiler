"""R1581 frozen-source driver. All execution belongs in released Actions."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
SOURCES = ROOT / "frozen"
POSITIVE = ["r1581-p1.nl", "r1581-p2.nl"]
INVALID = ["r1581-n1-zero.nl", "r1581-n2-underflow.nl"]
COVERAGE = ["r1581-e1-named-divisor.nl", "r1581-e2-multiplication.nl"]
TESTS = {
    "r1581.public_positive",
    "r1581.true_invalid",
    "r1581.legal_coverage",
    "r1581.cli_c17_native",
}
BOUNDS = dict(zip(POSITIVE + INVALID + COVERAGE,
                  [251, 191, 73, 61, 173, 83]))


class Stop(Exception):
    pass


def need(condition, message):
    if not condition:
        raise Stop(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def expectations(case):
    """Literal FIRST predictions, keyed by observed owned-graph paths."""
    values = {}
    returns = {}
    main = "entry/body"

    def put(base, **items):
        values.update({base + "/" + key: value
                       for key, value in items.items()})

    def sift(call, actual, remainder, quotient):
        values[call + "/arg0"] = actual
        put(call + "/body", remainder=remainder, quotient=quotient)
        returns[call + "/body"] = quotient

    def cut(call, actual, quotient, residue):
        values[call + "/arg0"] = actual
        put(call + "/body", quotient=quotient, residue=residue)
        returns[call + "/body"] = residue

    def trim(call, actual, residue, quotient):
        values[call + "/arg0"] = actual
        put(call + "/body", residue=residue, quotient=quotient,
            tail=quotient)

    if case == POSITIVE[0]:
        put(main, seed=203, shared=203, west=1, east=1, close=2,
            later=1, boundary=0, cross=0, total=1, rest=0,
            joined=2, tallied=3, final=3)
        rows = [
            ("west", 203, 29, 3, 3, 1, 1),
            ("east", 203, 29, 3, 3, 1, 1),
            ("close", 214, 30, 4, 4, 2, 2),
            ("later", 244, 34, 8, 8, 4, 1),
            ("boundary", 251, 35, 9, 0, 0, 0),
        ]
        for name, actual, q, r, sr, sq, result in rows:
            call = main + "/" + name
            values[call + "/arg0"] = actual
            body = call + "/body"
            put(body, quotient=q, residue=r, copied=r, small=sq,
                tail=result)
            sift(body + "/small", r, sr, sq)
        sift(main + "/cross", 1, 1, 0)
    else:
        put(main, seed=191, top=4, nested=2, same=2, repeat=1,
            sum=6, delta=4, final=5)
        for name, actual, bounded, iq, ir, oq, folded, r, result in [
            ("top", 191, 94, 47, 18, 9, 9, 9, 4),
            ("repeat", 153, 56, 28, 28, 14, 14, 3, 1)]:
            call = main + "/" + name
            values[call + "/arg0"] = actual
            body = call + "/body"
            put(body, bounded=bounded, folded=folded, residue=r,
                output=result)
            cut(body + "/folded/arg0", bounded, iq, ir)
            cut(body + "/folded", ir, oq, folded)
            trim(body + "/output", r, r, result)
            returns[body] = result
        cut(main + "/nested/arg0", 95, 47, 18)
        trim(main + "/nested", 18, 5, 2)
        trim(main + "/same", 5, 5, 2)
    return values, returns


class Driver:
    def __init__(self, args):
        self.args = args
        self.out = Path(args.build) / "r1581-results" / args.group
        self.out.mkdir(parents=True, exist_ok=True)
        self.reports = []
        self.manifest = json.loads((ROOT / "frozen-manifest.json").read_text())
        for row in self.manifest:
            raw = (SOURCES / row["name"]).read_bytes()
            need(len(raw) == row["bytes"] and digest(raw) == row["sha256"],
                 "FROZEN_INPUT_CHANGED: " + row["name"])
            need(b"\r" not in raw and raw.endswith(b"\n"),
                 "FROZEN_BYTE_ENVELOPE: " + row["name"])
        head = subprocess.run(
            ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
        need(head.returncode == 0, "APPARATUS: checkout identity unavailable")
        self.identity = {
            "checkout": head.stdout.strip(),
            "event_sha": os.getenv("GITHUB_SHA"),
            "event": os.getenv("GITHUB_EVENT_NAME"),
            "ref": os.getenv("GITHUB_REF"),
            "attempt": os.getenv("GITHUB_RUN_ATTEMPT"),
            "job": os.getenv("GITHUB_JOB"),
            "group": args.group,
        }
        print("R1581_IDENTITY " + json.dumps(self.identity, sort_keys=True),
              flush=True)

    def run(self, label, command, timeout=90):
        try:
            result = subprocess.run(command, stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE, timeout=timeout,
                                    check=False)
        except subprocess.TimeoutExpired as exc:
            (self.out / (label + ".stdout")).write_bytes(exc.stdout or b"")
            (self.out / (label + ".stderr")).write_bytes(exc.stderr or b"")
            raise Stop("PROCESS_TIMEOUT: " + label) from exc
        (self.out / (label + ".stdout")).write_bytes(result.stdout)
        (self.out / (label + ".stderr")).write_bytes(result.stderr)
        (self.out / (label + ".command.json")).write_text(
            json.dumps({"argv": command, "exit": result.returncode}, indent=2)
            + "\n")
        return result

    def probe(self, name, inspect):
        raw = (SOURCES / name).read_bytes()
        row = {"case": name, "source_sha256": digest(raw),
               "source_bytes": len(raw)}
        result = self.run(
            name + ".probe",
            [self.args.probe, str(SOURCES / name),
             "inspect" if inspect else "status"])
        records = [line.split("\t")
                   for line in result.stdout.decode("utf-8").splitlines()]
        inputs = [x[1] for x in records if x[0] == "INPUT_HEX"]
        need(len(inputs) == 1 and bytes.fromhex(inputs[0]) == raw,
             "SOURCE_INGESTION_MISMATCH: " + name)
        stages = {x[1]: x[2] for x in records if x[0] == "STAGE"}
        row["stages"] = stages
        row["probe_exit"] = result.returncode
        row["diagnostic"] = result.stderr.decode("utf-8", errors="replace")
        row["records"] = records
        self.reports.append(row)
        if result.returncode != 0:
            print(result.stdout.decode("utf-8", errors="replace"), flush=True)
            print(row["diagnostic"], file=sys.stderr, flush=True)
        need(result.returncode == 0,
             "PUBLIC_OR_APPARATUS_FAILURE: " + name)
        return row

    def positive(self, name):
        row = self.probe(name, True)
        stages = row["stages"]
        need(all(stages.get(k) == "OK"
                 for k in ("parse", "register", "main", "public")),
             "UNEXPECTED_POSITIVE_STAGE: " + name + " " + str(stages))
        values, returns = expectations(name)
        actual = {"VALUE": {}, "RETURN": {}}
        arith_checks = 0
        arith_unknown = 0
        for rec in row["records"]:
            if rec[0] in actual:
                path, known, number = rec[1], int(rec[2]), int(rec[3])
                actual[rec[0]].setdefault(path, []).append(
                    (bool(known), number))
            elif rec[0] == "ARITH":
                path, op = rec[1], rec[2]
                nk, rk, number, divisor, value, maximum = map(int, rec[3:])
                need(divisor != 0 and maximum >= BOUNDS[name],
                     "TARGET_OR_DIVISOR_FAILURE: " + name + " " + path)
                if nk and rk:
                    expect = number // divisor if op == "/" else number % divisor
                    need(value == expect,
                         "AUTHENTIC_ARITHMETIC_MISMATCH: " + name + " " + path)
                    arith_checks += 1
                else:
                    arith_unknown += 1
        compared = []
        missing = []
        for tag, expected in [("VALUE", values), ("RETURN", returns)]:
            for path, number in expected.items():
                found = actual[tag].get(path, [])
                known = [v for flag, v in found if flag]
                if not known:
                    missing.append(tag + ":" + path)
                    continue
                need(all(v == number for v in known),
                     "FROZEN_ORACLE_MISMATCH: " + name + " " + tag + " " +
                     path + " actual=" + repr(known) + " expected=" + str(number))
                compared.append({"tag": tag, "path": path, "actual": known,
                                 "expected": number, "comparison": "PASS"})
        row["arithmetic_relation_assertions"] = arith_checks
        row["unknown_arithmetic_records"] = arith_unknown
        row["frozen_numeric_comparisons"] = compared
        row["unobserved_frozen_points"] = missing
        row["whole_call_result_custody"] = "NOT_OBSERVABLE"
        row["source_body_whole_file_equality"] = "NOT_REQUIRED"
        row["numeric_scope"] = (
            "Known getter observations along the returned owned graph; "
            "local division/remainder validity; not a whole-call custody seal")
        row["verdict"] = "PASS_SCOPED" if compared else "NOT_OBSERVABLE_NUMERIC"
        final_path = "entry/body/final"
        final = actual["VALUE"].get(final_path, [])
        print("R1581_PUBLIC " + json.dumps({
            "case": name, "source_sha256": row["source_sha256"],
            "stages": stages, "frozen_comparisons": len(compared),
            "unobserved_points": len(missing), "arith_assertions": arith_checks,
            "arith_unknown": arith_unknown, "final_actual": final,
            "whole_call_result_custody": "NOT_OBSERVABLE",
            "verdict": row["verdict"]}, sort_keys=True), flush=True)
        for item in compared:
            if item["path"].endswith(("/final", "/tail", "/folded")):
                print("R1581_MEASURED " + json.dumps(item, sort_keys=True),
                      flush=True)
        return row

    def control(self, name, invalid, cli):
        row = self.probe(name, False)
        stages = row["stages"]
        failed = [(k, v) for k, v in stages.items()
                  if v not in ("OK", "NOT_RUN")]
        accepted = stages.get("main") == "OK"
        if invalid:
            need(not accepted, "TRUE_INVALID_ACCEPTED: " + name)
            need(bool(failed), "CONTROL_WITHOUT_REJECTION: " + name)
            need(all(v not in ("OOM", "RESOURCE", "INTERNAL", "PRECISION")
                     for _, v in failed),
                 "INVALID_GATE_NOT_REACHED: " + name)
            row["verdict"] = "REJECTED"
            row["intended_gate"] = (
                "SEMANTIC_REJECTION" if any(v == "SEMANTIC_ERROR"
                                            for _, v in failed)
                else "NOT_REACHED_EARLY_SOURCE_BARRIER")
        else:
            need(not any(v in ("OOM", "RESOURCE", "INTERNAL")
                         for _, v in failed),
                 "COVERAGE_INFRASTRUCTURE_FAILURE: " + name)
            row["verdict"] = "LEGAL_ACCEPTED" if accepted else "LEGAL_EXCLUDED"
            row["mandatory_reject"] = False
        if cli:
            check = self.run(
                name + ".cli-check",
                [self.args.compiler, "--check", str(SOURCES / name)])
            row["cli_check_exit"] = check.returncode
            if invalid:
                need(check.returncode == 3 and not check.stdout,
                     "INVALID_CLI_REJECTION_MISMATCH: " + name)
                emit = self.run(
                    name + ".cli-emit", [self.args.compiler, str(SOURCES / name)])
                need(emit.returncode == 3 and not emit.stdout,
                     "INVALID_CLI_EMITTED_C_OR_SUCCEEDED: " + name)
                row["cli_emit_exit"] = emit.returncode
                row["emitted_c_bytes"] = len(emit.stdout)
            else:
                need(check.returncode in (0, 3),
                     "COVERAGE_CLI_INFRASTRUCTURE_FAILURE: " + name)
                need((check.returncode == 0) == accepted,
                     "COVERAGE_API_CLI_DISAGREEMENT: " + name)
        print("R1581_CONTROL " + json.dumps({
            k: v for k, v in row.items()
            if k not in ("records", "diagnostic")}, sort_keys=True), flush=True)

    def native(self, name):
        raw_source = (SOURCES / name).read_bytes()
        check = self.run(name + ".native-check",
                         [self.args.compiler, "--check", str(SOURCES / name)])
        need(check.returncode == 0 and not check.stdout and not check.stderr,
             "POSITIVE_CLI_CHECK_FAILURE: " + name)
        emit = self.run(name + ".emit", [self.args.compiler, str(SOURCES / name)])
        need(emit.returncode == 0 and bool(emit.stdout),
             "POSITIVE_C17_EMISSION_FAILURE: " + name)
        raw = self.out / (name + ".raw.c")
        raw.write_bytes(emit.stdout)
        original_hash = digest(emit.stdout)
        binary = self.out / (name + ".native")
        flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
        flags += shlex.split(self.args.cflags)
        flags += ["-O2", "-DNDEBUG"] if self.args.config == "Release" else ["-O0"]
        if self.args.sanitizer != "none":
            flags += ["-fsanitize=" + self.args.sanitizer,
                      "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
        compile_result = self.run(
            name + ".compile", [self.args.cc] + flags +
            [str(raw), "-o", str(binary)], timeout=120)
        need(compile_result.returncode == 0,
             "PRISTINE_C17_COMPILE_FAILURE: " + name)
        run = self.run(name + ".native", [str(binary)], timeout=30)
        need(run.returncode == 0, "PRISTINE_NATIVE_FAILURE: " + name)
        need(digest(raw.read_bytes()) == original_hash,
             "PRISTINE_C_CHANGED: " + name)
        row = {
            "case": name, "verdict": "PRISTINE_UNIT_EXIT0_ONLY",
            "source_sha256": digest(raw_source), "raw_c_sha256": original_hash,
            "raw_c_bytes": len(emit.stdout), "compile_exit": compile_result.returncode,
            "native_exit": run.returncode, "native_stdout_hex": run.stdout.hex(),
            "native_stderr_hex": run.stderr.hex(),
            "native_internal_quantity": "NOT_OBSERVABLE",
            "source_c_event_mapping": "NOT_OBSERVABLE",
            "read_only_observer": "NOT_IMPLEMENTED_NO_AUTHENTIC_MAP",
            "sanitizer": self.args.sanitizer,
        }
        self.reports.append(row)
        print("R1581_NATIVE " + json.dumps(row, sort_keys=True), flush=True)

    def c17_diagnostic(self):
        """C1303: record actual P2 commands; no native acceptance change."""
        name = POSITIVE[1]
        source = SOURCES / name
        raw_source = source.read_bytes()
        compiler = Path(self.args.compiler).resolve()
        compiler_bytes = compiler.read_bytes()
        print("R1581_C17_DIAGNOSTIC_INPUT " + json.dumps({
            "case": name, "source": str(source),
            "source_bytes": len(raw_source),
            "source_sha256": digest(raw_source),
            "source_hex": raw_source.hex(),
            "compiler": str(compiler),
            "compiler_bytes": len(compiler_bytes),
            "compiler_sha256": digest(compiler_bytes),
            "cwd": str(Path.cwd()), "build": self.args.build,
            "config": self.args.config, "cflags": self.args.cflags,
            "sanitizer": self.args.sanitizer,
            "python": sys.version,
            "compile_native": "NOT_RUN",
        }, sort_keys=True), flush=True)
        commands = [
            ("check", [self.args.compiler, "--check", str(source)]),
            ("emit", [self.args.compiler, str(source)]),
        ]
        for stage, argv in commands:
            result = self.run(name + ".diagnostic-" + stage, argv, timeout=30)
            exact_stdout = result.returncode != 0 or len(result.stdout) <= 16384
            row = {
                "case": name, "stage": stage, "argv": argv,
                "source_bytes": len(raw_source),
                "source_sha256": digest(raw_source),
                "returncode": result.returncode,
                "stdout_bytes": len(result.stdout),
                "stdout_sha256": digest(result.stdout),
                "stdout_hex": result.stdout.hex() if exact_stdout else None,
                "stdout_runner_file": str(
                    self.out / (name + ".diagnostic-" + stage + ".stdout")),
                "stdout_transport": (
                    "EXACT_HEX" if exact_stdout else
                    "LARGE_SUCCESS_HASH_ONLY_RUNNER_FILE_NOT_RETRIEVED"),
                "stderr_bytes": len(result.stderr),
                "stderr_sha256": digest(result.stderr),
                "stderr_hex": result.stderr.hex(),
                "stderr_text": result.stderr.decode("utf-8", errors="backslashreplace"),
                "verdict": "RECORDED_ONLY_NOT_LANGUAGE_CAPABILITY_PASS",
                "compile_native": "NOT_RUN",
            }
            self.reports.append(row)
            print("R1581_C17_DIAGNOSTIC " + json.dumps(row, sort_keys=True),
                  flush=True)
            if stage == "check":
                need(result.returncode == 0 and not result.stdout and not result.stderr,
                     "DIAGNOSTIC_UNEXPECTED_POSITIVE_CHECK: " + name)
            need(result.returncode >= 0,
                 "DIAGNOSTIC_PROCESS_SIGNAL: " + name)

    def inventory(self):
        result = self.run(
            "own-inventory",
            [self.args.ctest, "--test-dir", self.args.build,
             "-N", "-R", r"^r1581[.]"])
        need(result.returncode == 0, "APPARATUS_INVENTORY_FAILURE")
        text = result.stdout.decode("utf-8")
        names = re.findall(r"Test\s+#\d+:\s+(r1581\.\S+)", text)
        need(len(names) == len(TESTS) and set(names) == TESTS,
             "OWN_CTEST_REGISTRATION_MISMATCH: " + repr(names))
        print("R1581_ACTUAL_CTEST_N_BEGIN\n" + text +
              "R1581_ACTUAL_CTEST_N_END", flush=True)
        print("R1581 inventory only: 0 CTests executed", flush=True)

    def finish(self, error=None):
        output = {"identity": self.identity, "reports": self.reports,
                  "error": error,
                  "whole_call_result_custody": "NOT_OBSERVABLE"}
        (self.out / "results.json").write_text(
            json.dumps(output, indent=2, sort_keys=True) + "\n")
        summary = os.getenv("GITHUB_STEP_SUMMARY")
        if summary:
            compact = [{
                k: v for k, v in row.items()
                if k not in ("records", "diagnostic", "frozen_numeric_comparisons",
                             "unobserved_frozen_points")}
                for row in self.reports]
            with open(summary, "a", encoding="utf-8") as stream:
                stream.write("\n### R1581 " + self.args.group + "\n\n")
                stream.write("Actual measurements; complete custody NOT_OBSERVABLE.\n\n")
                stream.write("~~~json\n" + json.dumps({
                    "identity": self.identity, "reports": compact, "error": error},
                    indent=2, sort_keys=True) + "\n~~~\n")
        print("R1581_RESULT " + json.dumps({
            "group": self.args.group, "error": error,
            "cases": len(self.reports),
            "scope": "measured statuses/comparisons; no general soundness"}, sort_keys=True),
              flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--group", required=True,
                        choices=["preflight", "public", "invalid", "coverage", "native",
                                 "c17-diagnostic"])
    for name in ("probe", "compiler", "cc", "ctest", "build"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--config", default="Debug")
    parser.add_argument("--cflags", default="")
    parser.add_argument("--sanitizer", default="none",
                        choices=["none", "address", "undefined"])
    args = parser.parse_args()
    driver = Driver(args)
    try:
        if args.group == "preflight":
            print("R1581 BUILD-TIME PREFLIGHT: six frozen sources; "
                  "parse/register/main/public only; native NOT_RUN; "
                  "separate from FAST ctest -N", flush=True)
            driver.inventory()
            for name in POSITIVE:
                driver.positive(name)
            for name in INVALID:
                driver.control(name, True, False)
            for name in COVERAGE:
                driver.control(name, False, False)
        elif args.group == "c17-diagnostic":
            driver.c17_diagnostic()
        elif args.group == "public":
            for name in POSITIVE:
                driver.positive(name)
        elif args.group == "invalid":
            for name in INVALID:
                driver.control(name, True, True)
        elif args.group == "coverage":
            for name in COVERAGE:
                driver.control(name, False, True)
        else:
            for name in POSITIVE:
                driver.native(name)
    except (Stop, ValueError, KeyError, OSError) as exc:
        driver.finish(str(exc))
        raise
    driver.finish()


if __name__ == "__main__":
    main()
