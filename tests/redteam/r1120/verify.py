#!/usr/bin/env python3
"""R1120 evidence recorder. Execute only inside the authorized GitHub Actions."""

import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--source-dir", required=True)
    parser.add_argument("--output-dir", required=True)
    parser.add_argument("--cc", required=True)
    parser.add_argument("--sanitizer", required=True)
    parser.add_argument("--phase", choices=["v1"], required=True)
    args = parser.parse_args()
    if os.environ.get("GITHUB_ACTIONS") != "true":
        raise SystemExit("STOP: R1120 compiler/native execution is Actions-only")
    source = Path(args.source_dir).resolve()
    output = Path(args.output_dir).resolve()
    output.mkdir(parents=True, exist_ok=False)
    summary = {"phase": args.phase, "independence": "BLIND_ORIGINAL_SOURCE",
               "production_head": "5346a7e2ca2aacb4ed6ab50f3b09bb258b1e607c",
               "canonical": "Draft17.34", "cases": [], "native": "NOT_REACHED"}
    for key in ("GITHUB_SHA", "GITHUB_REF", "GITHUB_EVENT_NAME", "GITHUB_RUN_ID",
                "GITHUB_RUN_ATTEMPT", "GITHUB_JOB", "GITHUB_HEAD_REF"):
        summary[key] = os.environ.get(key)
    summary["checked_git_head"] = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], text=True).strip()
    summary["started_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    summary["cc"] = args.cc
    summary["sanitizer"] = args.sanitizer
    (output / "provenance.json").write_text(json.dumps(summary, indent=2) + "\n")
    log = output / "commands.jsonl"

    def record(label, command):
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                cwd=output, timeout=20, check=False)
        (output / (label + ".stdout")).write_bytes(result.stdout)
        (output / (label + ".stderr")).write_bytes(result.stderr)
        row = {"label": label, "argv": command, "returncode": result.returncode,
               "stdout_sha256": digest(result.stdout), "stdout_bytes": len(result.stdout),
               "stderr_sha256": digest(result.stderr), "stderr_bytes": len(result.stderr),
               "stderr": result.stderr.decode("utf-8", errors="replace"),
               "diagnostics": re.findall(r"\b[A-Z][A-Z0-9]+(?:-[A-Z0-9]+)+\b",
                                         result.stderr.decode("utf-8", errors="replace"))}
        with log.open("a", encoding="utf-8") as stream:
            stream.write(json.dumps(row, sort_keys=True) + "\n")
        print("R1120_COMMAND " + json.dumps(row, sort_keys=True), flush=True)
        return result, row

    try:
        integrity = []
        for item in json.loads((source / "v1_manifest.json").read_text()):
            content = (source / "v1" / item["path"]).read_bytes()
            actual = {"path": item["path"], "sha256": digest(content), "size": len(content)}
            integrity.append(actual)
            if actual != item:
                raise RuntimeError("STOP: original frozen bytes changed: " + item["path"])
        summary["original_integrity"] = integrity
        (output / "original_integrity.json").write_text(json.dumps(integrity, indent=2) + "\n")
        for item in integrity:
            name = item["path"]
            if not name.endswith(".nl"):
                continue
            case_id = name.split("_", 1)[0]
            path = str(source / "v1" / name)
            checked, check_row = record(case_id + "_check", [args.compiler, "--check", path])
            case = {"case": case_id, "source": item, "check": check_row,
                    "intended_negative_layer": "NOT_REACHED" if case_id.startswith("N") else None}
            summary["cases"].append(case)
            if checked.returncode not in (0, 3):
                raise RuntimeError("STOP: unexpected CHECK exit/sanitizer/environment " + case_id)
            if case_id.startswith("N") and checked.returncode == 0:
                raise RuntimeError("STOP: frozen negative unexpectedly accepted " + case_id)
            emitted, emit_row = record(case_id + "_emit", [args.compiler, path])
            case["emit"] = emit_row
            if emitted.returncode not in (0, 3):
                raise RuntimeError("STOP: unexpected C17 exit/sanitizer/environment " + case_id)
            if checked.returncode != 0 and (emitted.returncode == 0 or emitted.stdout):
                raise RuntimeError("STOP: rejected source emitted C17 " + case_id)
            if emitted.returncode != 0:
                if emitted.stdout:
                    raise RuntimeError("STOP: rejected emitter leaked C bytes " + case_id)
                sentinel = output / (case_id + ".sentinel")
                original = ("R1120_NO_CLOBBER_" + case_id + "\n").encode("ascii")
                sentinel.write_bytes(original)
                guarded, guarded_row = record(case_id + "_no_clobber",
                    [args.compiler, "--emit-c17-to", str(sentinel), path])
                case["no_clobber"] = guarded_row
                case["sentinel_unchanged"] = sentinel.read_bytes() == original
                if guarded.returncode != 3 or not case["sentinel_unchanged"]:
                    raise RuntimeError("STOP: failed output replaced destination " + case_id)
            else:
                (output / (case_id + "_original.c")).write_bytes(emitted.stdout)
                case["native"] = "NOT_TESTED: no authenticated runtime observer yet"
            case["disposition"] = (
                "FRONTEND_REJECTION_ONLY: inspect first diagnostic; named negative not proven"
                if case_id.startswith("N") else
                "BLIND_INCONCLUSIVE: actual first refusal retained; canonical legality separate"
                if checked.returncode != 0 or emitted.returncode != 0 else
                "CHECK_C17_ONLY: native observation not established")
        summary["verdict"] = "V1_BLIND_INCONCLUSIVE"
        summary["reason"] = "Original predictions unchanged; source/entry/profile and native coverage require first-result adjudication."
        status = 1
    except Exception as exc:
        summary["verdict"] = "STOP"
        summary["reason"] = str(exc)
        status = 1
    summary["finished_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    (output / "summary.json").write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n")
    print("R1120_SUMMARY " + json.dumps(summary, sort_keys=True), flush=True)
    return status


if __name__ == "__main__":
    sys.exit(main())
