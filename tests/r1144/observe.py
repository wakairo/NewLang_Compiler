"""R1144: fixed source -> genuine owned -> unchanged C17 + passive native trace."""
import base64
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import traceback

CLI, PROBE, CC, CC_ID, SAN, ROOT, OUT, BUILD_TYPE, FRONTEND = sys.argv[1:]
ROOT = Path(ROOT).resolve()
OUT = Path(OUT).resolve()
OUT.mkdir(parents=True, exist_ok=False)
P_HEAD = "773643d2709a5e5db24650069d67c0d9032fefb0"
ORACLE_SHA = "2e29cceb34c9e6fa323bbcb45629a076253aeb48241b0fad3d5259ef3f0e8176"
records = []
files = {}
summary = {
    "schema": "r1144-observation-v1",
    "status": "NOT_RUN",
    "v1": "NOT_RUN; original oracle retained; known CLI/source-profile boundary",
    "v2": ("pre-execution frozen independent cohort / "
           "post-freeze interface-calibrated V2 / "
           "incidental old-P formatter exposure after freeze"),
    "first_red": None,
    "cases": [],
    "compiler": CC,
    "compiler_id": CC_ID,
    "sanitizer": SAN,
    "build_type": BUILD_TYPE,
    "frontend_only": FRONTEND,
    "native_optimizations": ["-O0", "-O2"],
    "audit_locality": "UNMEASURED",
    "GO624": "PAUSED",
    "H1": "HOLD",
    "P1036_PR1050": "STOP",
    "clock_JST": datetime.datetime.now(
        datetime.timezone(datetime.timedelta(hours=9))).isoformat(),
    "workflow": {key: os.environ.get(key) for key in [
        "GITHUB_RUN_ID", "GITHUB_RUN_ATTEMPT", "GITHUB_EVENT_NAME",
        "GITHUB_REF", "GITHUB_SHA", "GITHUB_JOB", "RUNNER_ARCH"]},
}


class Stop(Exception):
    pass


def require(condition, detail):
    if not condition:
        raise Stop(detail)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def save(name, raw):
    if isinstance(raw, str):
        raw = raw.encode()
    path = OUT / name
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("xb") as stream:
        stream.write(raw)
    files[name] = {"sha256": sha(raw), "bytes": len(raw),
                   "base64": base64.b64encode(raw).decode()}
    return path


def command(stage, argv, timeout=30):
    row = {"stage": stage, "argv": [str(x) for x in argv]}
    records.append(row)
    try:
        proc = subprocess.run(row["argv"], cwd=ROOT, capture_output=True,
                              timeout=timeout, check=False)
        row.update(returncode=proc.returncode,
                   stdout=proc.stdout.decode("utf-8", "backslashreplace"),
                   stderr=proc.stderr.decode("utf-8", "backslashreplace"))
        return proc
    except subprocess.TimeoutExpired as error:
        row.update(returncode="TIMEOUT",
                   stdout=(error.stdout or b"").decode("utf-8", "backslashreplace"),
                   stderr=(error.stderr or b"").decode("utf-8", "backslashreplace"))
        raise Stop(stage + ": bounded execution timeout") from error


def successful(stage, argv, timeout=30):
    proc = command(stage, argv, timeout)
    require(proc.returncode == 0, stage + ": nonzero exit")
    return proc


def once(text, old, new):
    require(text.count(old) == 1, "passive observer structural match: " + old)
    return text.replace(old, new, 1)


def observe(raw, mapping):
    text = raw.decode("utf-8")
    entry = "static void nl_fn_" + str(mapping["entry"]) + "(void)\n{\n"
    require(text.count(entry) == 1, "single source-issued entry definition")
    left, rest = text.split(entry, 1)
    require("\n}\n" in rest, "entry definition boundary")
    main, right = rest.split("\n}\n", 1)
    after = str(mapping["after"])
    copy = str(mapping["mirrored"])
    selected = str(mapping["selected"])
    call = str(mapping["call"])
    loop = str(mapping["loop"])
    old = "    (void)nl_local_" + after + ";\n"
    main = once(main, old, old +
                '    r1144_observe("outer", nl_local_' + after + ");\n")
    old = ("    const size_t nl_local_" + copy +
           " = nl_local_" + after + ";\n")
    main = once(main, old, old +
                '    r1144_observe("copy", nl_local_' + copy + ");\n")
    old = ("    const uint8_t nl_local_" + selected +
           " = nl_call_" + call + ";\n")
    main = once(main, old, old +
                '    r1144_observe("selected", nl_local_' + selected + ");\n")
    found = re.findall(r"(?m)^([ ]*)continue;\n", main)
    require(len(found) == 1, "single existing Continue control point")
    old = found[0] + "continue;\n"
    main = once(main, old, found[0] +
                'r1144_observe("continue", nl_loop_carry_' + loop + ");\n" + old)
    text = left + entry + main + "\n}\n" + right
    helper = ("static uint8_t nl_fn_" + str(mapping["callee"]) +
              "(size_t nl_local_" + str(mapping["parameter"]) + ")\n{\n")
    text = once(text, helper, helper +
                '    r1144_observe("actual", nl_local_' +
                str(mapping["parameter"]) + ");\n")
    # Only inserted read-only observer calls and one include differ.
    stripped = "\n".join(line for line in text.split("\n")
                         if "r1144_observe(" not in line)
    require(stripped == raw.decode("utf-8"),
            "observer must preserve every original generated C line")
    return '#include "observer.h"\n' + text


HEADER = r"""#ifndef R1144_OBSERVER_H
#define R1144_OBSERVER_H
#include <stddef.h>
#include <stdio.h>
static void r1144_observe(const char *label, size_t value)
{
    (void)printf("R1144 %s %zu\n", label, value);
}
#define NL_SCALAR_PHASE(value) r1144_observe("phase", (value))
#define NL_SCALAR_RESULT(value) r1144_observe("break", (value))
#endif
"""


def expected_trace(row):
    answer = []
    values = row["expected_header_values"]
    for index, value in enumerate(values):
        answer.append(["phase", value])
        if index + 1 < len(values):
            answer.append(["continue", values[index + 1]])
    answer += [["break", row["expected_break"]],
               ["outer", row["expected_outer_binding"]],
               ["copy", row["expected_copy"]],
               ["actual", row["expected_actual"]],
               ["selected", row["expected_source_result"]]]
    return answer


def positive(row, state):
    source = ROOT / row["path"]
    prefix = row["id"] + "/"
    state["stage"] = "CHECK"
    checked = successful(row["id"] + "/CHECK", [CLI, "--check", source])
    require(not checked.stderr, row["id"] + ": CHECK diagnostic on success")
    state["stage"] = "owned"
    owned = successful(row["id"] + "/owned", [PROBE, "owned", source])
    mapping = json.loads(owned.stdout)
    state["owned"] = mapping
    require(mapping.get("source_issued") and
            mapping.get("private_relation_valid") and
            mapping.get("ordinary_copy") and mapping.get("actual_copy"),
            row["id"] + ": genuine owned chain not observed")
    save(prefix + "owned.json", json.dumps(mapping, sort_keys=True, indent=2) + "\n")
    state["stage"] = "C17"
    emitted = successful(row["id"] + "/C17", [CLI, source])
    require(emitted.stdout and not emitted.stderr,
            row["id"] + ": C17 output missing or diagnostic present")
    guest = save(prefix + "guest.c", emitted.stdout)
    passive = save(prefix + "observed.c", observe(emitted.stdout, mapping))
    save(prefix + "observer.h", HEADER)
    state["generated_c_sha256"] = sha(emitted.stdout)
    state["native"] = []
    base = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    if CC_ID == "Clang":
        base.append("-Wno-error=gcc-install-dir-libstdcxx")
    if SAN != "none":
        base += ["-fsanitize=" + SAN, "-fno-sanitize-recover=all",
                 "-fno-omit-frame-pointer"]
    for optimization in ["-O0", "-O2"]:
        state["stage"] = "native " + optimization
        result = {"optimization": optimization, "guest": "NOT_RUN",
                  "passive": "NOT_RUN"}
        state["native"].append(result)
        for kind, code in [("guest", guest), ("passive", passive)]:
            binary = OUT / prefix / (kind + optimization)
            successful(row["id"] + "/" + optimization + "/" + kind + "/compile",
                       [CC, *base, optimization, code, "-o", binary], 60)
            ran = successful(row["id"] + "/" + optimization + "/" + kind + "/run",
                             [binary], 5)
            save(prefix + kind + optimization + ".stdout", ran.stdout)
            save(prefix + kind + optimization + ".stderr", ran.stderr)
            require(not ran.stderr, row["id"] + ": native diagnostic")
            if kind == "guest":
                require(ran.stdout == b"", row["id"] + ": unexpected guest stdout")
                result["guest"] = "exit0; no value claim from exit status"
            else:
                trace = []
                for line in ran.stdout.decode("utf-8").splitlines():
                    found = re.fullmatch(r"R1144 ([a-z]+) ([0-9]+)", line)
                    require(found is not None, row["id"] + ": malformed trace")
                    trace.append([found[1], int(found[2])])
                result["trace"] = trace
                require(trace == expected_trace(row),
                        row["id"] + ": frozen value/control oracle mismatch")
                result["passive"] = "PASS"
    state.update(status="PASS", stage="complete",
                 predicate_branch=row["expected_predicate_branch"],
                 predicate_branch_evidence="inferred from distinct source-selected result",
                 selected_binding=row["expected_source_result"])


def negative(row, state):
    source = ROOT / row["path"]
    state["native"] = "NOT_RUN by frozen policy"
    state["canonical_classification"] = row["classification"]
    state["stage"] = "CHECK"
    cli = command(row["id"] + "/CHECK", [CLI, "--check", source])
    probe = command(row["id"] + "/first-layer", [PROBE, "check", source])
    require(probe.returncode in (0, 2), row["id"] + ": host/probe failure")
    observed = json.loads(probe.stdout)
    state["first_layer"] = observed
    require((cli.returncode == 0) == observed["accepted"],
            row["id"] + ": CLI/probe acceptance disagreement")
    if row["expected_check"] == "reject":
        require(cli.returncode != 0 and not observed["accepted"],
                row["id"] + ": invalid source unexpectedly accepted")
        require(observed["status"] not in (
                    "OUT_OF_MEMORY", "RESOURCE_LIMIT", "INTERNAL_ERROR",
                    "HOST_ERROR"),
                row["id"] + ": resource failure is not semantic rejection")
    else:
        precision_codes = {
            "USIZE-SUBTRACTION-PRECISION", "P14-LOOP-PRECISION"}
        precision = (observed["stage"] in ("registration", "CHECK") and
                     observed["status"] == "ANALYSIS_PRECISION_LIMIT" and
                     observed.get("code") in precision_codes)
        require(observed["accepted"] or precision,
                row["id"] + ": refusal is not the frozen quantity/cyclic precision boundary")
        state["interpretation"] = (
            "Canonical legal [2,0]; accepted by CHECK; native deliberately NOT_RUN."
            if observed["accepted"] else
            "Canonical legal [2,0]; exact quantity/cyclic precision refusal; "
            "not an invalid-program result. Raw status/code retained.")
    state.update(status="PASS", stage="complete")


def emit_bundle():
    summary["command_count"] = len(records)
    serialized = json.dumps({"summary": summary, "commands": records,
                             "files": files}, sort_keys=True,
                            separators=(",", ":")).encode()
    with (OUT / "raw-evidence.json").open("xb") as stream:
        stream.write(serialized)
    encoded = base64.b64encode(serialized).decode()
    digest = sha(serialized)
    token = str(os.environ.get("GITHUB_RUN_ID", "unknown"))
    lines = ["R1144_EVIDENCE_BEGIN " + token + " " + digest +
             " " + str(len(serialized))]
    lines += ["R1144_EVIDENCE_DATA " + token + " " + encoded[i:i + 3000]
              for i in range(0, len(encoded), 3000)]
    lines += ["R1144_EVIDENCE_END " + token + " " + digest,
              "R1144_RESULT " + json.dumps(summary, sort_keys=True)]
    # CTest hides successful child stdout. Its own stdout is the unchanged
    # workflow's log sink. This writes R-only evidence, not guest state.
    parent = Path("/proc") / str(os.getppid())
    if os.environ.get("GITHUB_ACTIONS") == "true":
        require((parent / "comm").read_text().strip() == "ctest",
                "raw evidence sink parent must be CTest")
        with (parent / "fd/1").open("w", buffering=1) as sink:
            for line in lines:
                sink.write(line + "\n")
    else:
        print("\n".join(lines))
    print("R1144 raw evidence SHA256", digest, "bytes", len(serialized))


try:
    manifest = json.loads((ROOT / "tests/r1144/harness-manifest.json").read_bytes())
    for path, expected in manifest["files"].items():
        require(sha((ROOT / path).read_bytes()) == expected, "frozen input: " + path)
    oracle_raw = (ROOT / "tests/r1144/oracle.v2.json").read_bytes()
    require(sha(oracle_raw) == ORACLE_SHA, "V2 immutable oracle")
    oracle = json.loads(oracle_raw)
    summary["cases"] = [{"id": row["id"], "status": "NOT_RUN", "stage": "NOT_RUN"}
                        for row in oracle["positive"] + oracle["negative"]]
    for row in oracle["positive"] + oracle["negative"]:
        require(sha((ROOT / row["path"]).read_bytes()) == row["sha256"],
                "source-to-oracle hash: " + row["id"])
    event_path = os.environ.get("GITHUB_EVENT_PATH")
    if event_path:
        event = json.loads(Path(event_path).read_bytes())
        pr = event.get("pull_request")
        require(pr is not None, "semantic R execution requires natural PR event")
        summary["pr_head"] = pr["head"]["sha"]
        summary["pr_base"] = pr["base"]["sha"]
        require(pr["base"]["sha"] == P_HEAD, "fixed P base drift")
    else:
        raise Stop("R execution is authorized only in Actions")
    summary["checkout_head"] = successful("checkout HEAD",
                                          ["git", "rev-parse", "HEAD"]).stdout.decode().strip()
    summary["checkout_tree"] = successful("checkout tree",
                                          ["git", "rev-parse", "HEAD^{tree}"]).stdout.decode().strip()
    summary["host_version"] = successful("compiler identity", [CC, "--version"]).stdout.decode()
    inventory = OUT.parent / "r1144-registration.txt"
    require(inventory.is_file(), "actual CTest inventory missing")
    save("registration.txt", inventory.read_bytes())
    for row, state in zip(oracle["positive"] + oracle["negative"], summary["cases"]):
        if row in oracle["positive"]:
            positive(row, state)
        else:
            negative(row, state)
    summary["status"] = "PASS"
except Exception as error:
    summary["status"] = "FIRST_RED"
    summary["first_red"] = {"type": type(error).__name__, "detail": str(error)}
    for state in summary["cases"]:
        if state["stage"] != "NOT_RUN" and state["status"] != "PASS":
            state["status"] = "FIRST_RED"
            break
    summary["traceback"] = traceback.format_exc()
finally:
    emit_bundle()
sys.exit(0 if summary["status"] == "PASS" else 1)
