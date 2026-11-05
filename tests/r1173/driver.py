"""R1173 finite independent cohort; semantic/native execution is Actions-only."""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import traceback

from observer import Unobservable, check_trace, instrument

CLI, PROBE, HOST_CC, SAN, OWN, OUT = sys.argv[1:7]
OWN, OUT = Path(OWN).resolve(), Path(OUT).resolve()
REPO = OWN.parent.parent
if os.environ.get("GITHUB_ACTIONS") != "true":
    raise SystemExit("R1173 STOP: compiler/native execution is authorized in Actions only")
OUT.mkdir(parents=True, exist_ok=False)
COMMANDS = []
RESULTS = []
FAILURES = []
SEQUENCE = 0


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def command(label, argv, timeout=40):
    global SEQUENCE
    SEQUENCE += 1
    prefix = OUT / "commands" / (str(SEQUENCE).zfill(4) + "-" + label)
    prefix.parent.mkdir(exist_ok=True)
    started = datetime.now(timezone.utc).isoformat()
    timed_out = False
    try:
        result = subprocess.run([str(x) for x in argv], stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, timeout=timeout, check=False,
                                cwd=REPO)
        rc, stdout, stderr = result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired as error:
        rc, timed_out = -999, True
        stdout, stderr = error.stdout or b"", error.stderr or b""
    except OSError as error:
        rc, stdout, stderr = -998, b"", repr(error).encode()
    prefix.with_suffix(".stdout").write_bytes(stdout)
    prefix.with_suffix(".stderr").write_bytes(stderr)
    record = {"sequence": SEQUENCE, "label": label, "command": [str(x) for x in argv],
              "cwd": str(REPO), "started_utc": started, "returncode": rc,
              "timed_out": timed_out, "stdout_sha256": hashlib.sha256(stdout).hexdigest(),
              "stderr_sha256": hashlib.sha256(stderr).hexdigest(),
              "stdout": str(prefix.with_suffix(".stdout").relative_to(OUT)),
              "stderr": str(prefix.with_suffix(".stderr").relative_to(OUT))}
    write_json(prefix.with_suffix(".json"), record)
    COMMANDS.append(record)
    print("R1173 COMMAND " + json.dumps(record, sort_keys=True), flush=True)
    return rc, stdout, stderr


def failure(case, layer, category, detail):
    entry = {"case": case, "layer": layer, "category": category, "detail": detail}
    FAILURES.append(entry)
    print("R1173 RED " + json.dumps(entry, sort_keys=True), flush=True)


def check_frozen():
    manifest = json.loads((OWN / "execution-freeze-v3.json").read_text())
    for entry in manifest["files"]:
        path = REPO / entry["path"]
        if path.stat().st_size != entry["bytes"] or digest(path) != entry["sha256"]:
            raise RuntimeError("execution freeze mismatch: " + entry["path"])
    for path in OWN.glob("*.json"):
        if path.name not in ("fixed-tree.json", "fixed-commit-metadata.json"):
            shutil.copyfile(path, OUT / path.name)
    for path in OWN.glob("*.py"):
        shutil.copyfile(path, OUT / path.name)
    shutil.copyfile(OWN / "probe.c", OUT / "probe.c")
    shutil.copytree(OWN / "sources", OUT / "sources")
    fixed = OUT.parent / "r1173-fixed-context.json"
    if not fixed.is_file():
        raise RuntimeError("verified fixed-P context receipt missing")
    shutil.copyfile(fixed, OUT / fixed.name)
    identities = {}
    for name, args in (("head", ["rev-parse", "HEAD"]),
                       ("tree", ["rev-parse", "HEAD^{tree}"]),
                       ("parents", ["show", "-s", "--format=%P", "HEAD"])):
        rc, raw, _ = command("identity-" + name, ["git", *args])
        if rc:
            raise RuntimeError("checkout Git identity unavailable")
        identities[name] = raw.decode().strip()
    identities.update({key: os.environ.get(key) for key in (
        "GITHUB_EVENT_NAME", "GITHUB_SHA", "GITHUB_REF", "GITHUB_RUN_ID",
        "GITHUB_RUN_ATTEMPT", "GITHUB_JOB", "RUNNER_OS", "RUNNER_ARCH")})
    identities.update(host_compiler=HOST_CC, host_sanitizer=SAN,
                      cli_sha256=digest(Path(CLI)), probe_sha256=digest(Path(PROBE)))
    write_json(OUT / "identity.json", identities)
    return json.loads((OWN / "oracle-v1.json").read_text())


def native(case, c_path, mode, compilers, sanitizer="none", mapping=None):
    records = []
    for compiler in compilers:
        for opt in ("O0", "O2"):
            label = "-".join((case["name"], mode, Path(compiler).name, opt, sanitizer))
            binary = OUT.parent / ("r1173-bin-" + label)
            args = [compiler, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-" + opt, str(c_path), "-o", str(binary)]
            if sanitizer != "none":
                args.extend(["-g", "-fno-omit-frame-pointer", "-fsanitize=" + sanitizer,
                             "-fno-sanitize-recover=all"])
            rc, _, stderr = command(label + "-compile", args)
            row = {"compiler": compiler, "optimization": opt, "guest_sanitizer": sanitizer,
                   "mode": mode, "compile_returncode": rc, "run": "NOT_RUN"}
            records.append(row)
            if rc:
                category = "OBSERVER_OR_TOOLCHAIN_REVIEW" if mode == "passive" else "C17_OR_TOOLCHAIN_STOP"
                failure(case["name"], "native-compile", category, label)
                continue
            row["binary_sha256"] = digest(binary)
            rc, stdout, stderr = command(label + "-run", [str(binary)], timeout=10)
            row.update(run_returncode=rc, run="PASS" if rc == 0 else "RED")
            if rc or stdout:
                failure(case["name"], "native", "AFFECTED_STOP", label + " returncode/stdout")
            elif mode == "passive":
                try:
                    observed = check_trace(stderr.decode("utf-8", "replace"), case)
                    write_json(OUT / "observations" / (label + ".json"), observed)
                    row["observation"] = "PASS"
                except Unobservable as error:
                    row["observation"] = "NOT_RUN_COMPLETE_COMPARISON"
                    failure(case["name"], "passive", "MACHINERY_NOT_RUN", str(error))
                except (AssertionError, ValueError) as error:
                    row["observation"] = "RED"
                    failure(case["name"], "passive", "AFFECTED_STOP", str(error))
            elif stderr:
                failure(case["name"], "native", "AFFECTED_STOP", label + " unexpected stderr")
            binary.unlink(missing_ok=True)
    return records


def positive(case, compilers):
    name = case["name"]
    source = OWN / case["source"]
    result = {"case": name, "kind": "positive", "check": "NOT_RUN", "owned": "NOT_RUN",
              "c17": "NOT_RUN", "unmodified": "NOT_RUN", "passive": "NOT_RUN",
              "guest_sanitizer": "NOT_RUN"}
    RESULTS.append(result)
    rc, _, _ = command(name + "-check", [CLI, "--check", source])
    result["check"] = "PASS" if rc == 0 else "RED"
    if rc:
        failure(name, "CHECK", "AFFECTED_STOP", "declared finite positive rejected")
        return
    rc, raw, _ = command(name + "-owned", [PROBE, source])
    result["owned"] = "PASS" if rc == 0 else "RED"
    if rc:
        failure(name, "owned", "AFFECTED_STOP_OR_PUBLIC_ADAPTER_REVIEW", "source-issued probe rejected")
        return
    owned = [json.loads(line) for line in raw.decode().splitlines() if line.startswith("{")]
    write_json(OUT / "owned" / (name + ".json"), owned)
    result["public_observations"] = next((x for x in owned if x.get("event") == "summary"), None)
    rc, raw, _ = command(name + "-emit", [CLI, source])
    result["c17"] = "PASS" if rc == 0 else "RED"
    if rc:
        failure(name, "C17", "AFFECTED_STOP", "declared finite positive emission rejected")
        return
    c_path = OUT / (name + "-unmodified.c")
    c_path.write_bytes(raw)
    result["generated_c_sha256"] = digest(c_path)
    result["unmodified"] = native(case, c_path, "unmodified", compilers)
    try:
        observed_c, mapping = instrument(raw.decode("utf-8"))
    except (Unobservable, UnicodeDecodeError) as error:
        result["passive"] = "NOT_RUN_OBSERVER_RECOGNIZER"
        failure(name, "passive", "MACHINERY_NOT_RUN", str(error))
        return
    observer_path = OUT / (name + "-passive.c")
    observer_path.write_text(observed_c)
    write_json(OUT / (name + "-observer-map.json"), mapping)
    result["passive_c_sha256"] = digest(observer_path)
    result["passive"] = native(case, observer_path, "passive", compilers, mapping=mapping)
    if SAN in ("address", "undefined") and name in ("multi_first", "multi_second", "same_bits"):
        clang = next((c for c in compilers if "clang" in Path(c).name), None)
        if clang is None:
            failure(name, "guest-sanitizer", "MACHINERY_NOT_RUN", "pinned Clang missing")
        else:
            result["guest_sanitizer"] = (
                native(case, c_path, "unmodified", [clang], sanitizer=SAN) +
                native(case, observer_path, "passive", [clang], sanitizer=SAN, mapping=mapping))


def negative(case):
    name = case["name"]
    source = OWN / case["source"]
    result = {"case": name, "kind": "negative", "native": "NOT_RUN_BY_DESIGN"}
    RESULTS.append(result)
    rc, _, stderr = command(name + "-negative-check", [CLI, "--check", source])
    result["check_returncode"] = rc
    if rc:
        result.update(rejected_at="CHECK", diagnostic=stderr.decode("utf-8", "replace"))
        if rc < 0 or not stderr.strip():
            failure(name, "negative-CHECK", "AFFECTED_STOP", "unstructured rejection/crash")
        return
    if name not in ("overflow_edge", "wrong_guard_arm"):
        result["rejected_at"] = None
        failure(name, "negative-CHECK", "UNSAFE_ACCEPTANCE_STOP", "invalid type/access/scope accepted")
        return
    rc, raw, stderr = command(name + "-negative-emit", [CLI, source])
    (OUT / (name + "-negative-generated.c")).write_bytes(raw)
    result.update(emit_returncode=rc, rejected_at="C17" if rc else None,
                  diagnostic=stderr.decode("utf-8", "replace"))
    if rc == 0:
        failure(name, "negative-C17", "UNSAFE_ACCEPTANCE_STOP", "unsafe ADD/phase accepted through emission")
    elif rc < 0 or not stderr.strip():
        failure(name, "negative-C17", "AFFECTED_STOP", "unstructured rejection/crash")


try:
    oracle = check_frozen()
    compilers = [path for path in (shutil.which("gcc-13"), shutil.which("clang-23")) if path]
    cache = (OUT.parent / "CMakeCache.txt").read_text()
    frontend = "NEWLANG_FRONTEND_ONLY:BOOL=ON" in cache
    if not compilers or (not frontend and len(compilers) != 2):
        raise RuntimeError("required native GCC13/Clang23 toolchain unavailable")
    for compiler in compilers:
        command("toolchain-" + Path(compiler).name, [compiler, "--version"])
    for case in oracle["positives"]:
        positive(case, compilers)
    for case in oracle["negatives"]:
        negative(case)
except Exception as error:
    failure("runner", "equipment", "REVIEW_REQUIRED", repr(error))
    (OUT / "runner-exception.txt").write_text(traceback.format_exc())
finally:
    summary = {"schema": "R1173_FIRST_V1", "host_sanitizer": SAN, "host_compiler": HOST_CC,
               "cases": RESULTS, "failures": FAILURES, "commands": len(COMMANDS),
               "public_negative_mutations": "NOT_RUN: no matching public validator in inspected API",
               "ghost_limits": "read-only source-issued public field/loop views only; native values are not freshness proof",
               "first_evidence": "immutable natural job result; no retry/replay inside runner",
               "status": "PASS" if not FAILURES else "RED"}
    write_json(OUT / "FIRST.json", summary)
    write_json(OUT / "commands.json", COMMANDS)
    print("R1173 FIRST " + json.dumps(summary, sort_keys=True), flush=True)
    subprocess.run([sys.executable, str(OWN / "export.py"), str(OUT)], check=False)
raise SystemExit(1 if FAILURES else 0)
