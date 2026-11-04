#!/usr/bin/env python3
"""R1134 C223: frozen independent inputs, actual observations, no state repair."""
import base64
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import traceback

ROOT = Path(__file__).resolve().parent
COMPILER, PROBE, HOST_CC, SANITIZER, RAW_ARG = sys.argv[1:]
RAW = Path(RAW_ARG).resolve()
RECORDS = []
FILES = {}
SUMMARY = []
CURRENT = {"case": "equipment", "stage": "start"}

def digest(data):
    return hashlib.sha256(data).hexdigest()

def emit(kind, value):
    print("R1134_" + kind + " " + json.dumps(value, sort_keys=True), flush=True)

def require(condition, message):
    if not condition:
        raise AssertionError(message)

def keep(name, data):
    if isinstance(data, str):
        data = data.encode("utf-8")
    path = RAW / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    FILES[name] = {"bytes": len(data), "sha256": digest(data),
                   "base64": base64.b64encode(data).decode("ascii")}
    return path

def command(case, stage, args, timeout=30):
    CURRENT.update(case=case, stage=stage)
    args = [str(arg) for arg in args]
    timed_out = False
    try:
        result = subprocess.run(args, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, timeout=timeout,
                                check=False)
        code, stdout, stderr = result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired as error:
        timed_out = True
        code, stdout, stderr = None, error.stdout or b"", error.stderr or b""
    receipt = {"case": case, "stage": stage, "argv": args,
               "timeout_seconds": timeout, "timed_out": timed_out,
               "returncode": code,
               "stdout_base64": base64.b64encode(stdout).decode("ascii"),
               "stderr_base64": base64.b64encode(stderr).decode("ascii")}
    RECORDS.append(receipt)
    emit("COMMAND", {key: value for key, value in receipt.items()
                     if key not in ("stdout_base64", "stderr_base64")})
    if stdout:
        print(stdout.decode("utf-8", errors="replace"), end="", flush=True)
    if stderr:
        print(stderr.decode("utf-8", errors="replace"), end="", flush=True)
    require(not timed_out, case + ": timeout; later stages NOT_RUN")
    return code, stdout, stderr

def checked_ok(result, stage, empty_stdout=False):
    code, stdout, stderr = result
    require(code == 0 and not stderr and (not empty_stdout or not stdout),
            stage + ": expected clean success, got rc=" + str(code))

def verified_bundle(prefix, expected_manifest):
    manifest_path = ROOT / prefix / "manifest.json"
    data = manifest_path.read_bytes()
    require(digest(data) == expected_manifest, prefix + ": manifest changed")
    manifest = json.loads(data)
    for entry in manifest["entries"]:
        contents = (ROOT / entry["path"]).read_bytes()
        require(len(contents) == entry["bytes"] and
                digest(contents) == entry["sha256"],
                "frozen payload changed: " + entry["path"])
    SUMMARY.append({"stage": "freeze_identity", "bundle": prefix,
                    "manifest_sha256": digest(data),
                    "entries_verified": len(manifest["entries"])})

def mask_c(text):
    pattern = re.compile(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|'
                         r"'(?:\\.|[^'\\])*'")
    return pattern.sub(lambda match: "".join(
        "\n" if char == "\n" else " " for char in match.group()), text)

def paired(mask, left, right):
    stack = []
    result = {}
    for index, char in enumerate(mask):
        if char == left:
            stack.append(index)
        elif char == right:
            require(bool(stack), "equipment: unmatched C delimiter")
            result[stack.pop()] = index
    require(not stack, "equipment: unmatched C delimiter")
    return result

def unique(matches, description):
    require(len(matches) == 1, "equipment: expected one " + description +
            ", observed " + str(len(matches)))
    return matches[0]

def observe_c(text, owned, case):
    mask = mask_c(text)
    braces = paired(mask, "{", "}")
    parens = paired(mask, "(", ")")
    names = {}
    for match in re.finditer(
            r"(?m)^R1134 NAME main \d+ (\d+) \d+ ([A-Za-z_]\w*)$", owned):
        names.setdefault(match[2], set()).add(match[1])
    needed = ["original", "remote"] + list(case["expected_locals"])
    c_names = {}
    for name in needed:
        symbol = unique(list(names.get(name, set())), "public symbol for " + name)
        c_names[name] = "nl_local_" + symbol

    declarations = {}
    for name in ("original", "remote"):
        pattern = re.compile(r"\b(?:const\s+)?nl_type_\d+\s+" +
                             re.escape(c_names[name]) + r"\s*=\s*\{")
        start = unique(list(pattern.finditer(mask)), "actual " + name + " object")
        opening = mask.index("{", start.start())
        closing = braces[opening]
        end = re.match(r"\s*;", mask[closing + 1:])
        require(end is not None, "equipment: object declaration has no semicolon")
        declarations[name] = (start.start(), closing + 1 + end.end())
    require(declarations["original"][0] < declarations["remote"][0],
            "equipment: unexpected source declaration order")
    containers = [(a, b) for a, b in braces.items()
                  if a < declarations["original"][0] and
                  b > declarations["remote"][1]]
    require(bool(containers), "equipment: no enclosing main body")
    main_start, main_end = min(containers, key=lambda item: item[1] - item[0])
    loop_pattern = re.compile(r"\b(?:for\s*\(\s*;\s*;\s*\)|"
                              r"while\s*\(\s*(?:1|true)\s*\))\s*\{")
    loop = unique([match for match in loop_pattern.finditer(mask)
                   if main_start < match.start() < main_end], "generated loop")
    loop_open = mask.index("{", loop.start())
    loop_close = braces[loop_open]
    condition_match = unique(list(re.finditer(
        r"\bif\s*\(", mask[loop_open + 1:loop_close])), "guarded loop if")
    if_start = loop_open + 1 + condition_match.start()
    if_paren = mask.index("(", if_start)
    condition_end = parens[if_paren]
    condition = mask[if_paren + 1:condition_end]
    candidates = set(re.findall(r"\b(nl_[A-Za-z0-9_]+)\s*\)*\s*<", condition))
    if not candidates:
        candidates = set(re.findall(
            r"\b(nl_[A-Za-z0-9_]+)\s*\)*\s*<",
            mask[loop_open + 1:if_start]))
    carry = unique(list(candidates), "actual carry operand in emitted guard")
    edits = []

    def frame(tag, carry_name=None):
        original, remote = c_names["original"], c_names["remote"]
        fmt = "R1134 " + tag
        arguments = []
        if carry_name is not None:
            fmt += " %zu"
            arguments.append("(size_t)" + carry_name)
        fmt += " %p %p %p %zu %p %p %p %zu %u %u %u %u\\n"
        arguments.extend([
            "(void *)&" + original, "(void *)&" + original + ".f0",
            "(void *)&" + original + ".f1", "sizeof(" + original + ")",
            "(void *)&" + remote, "(void *)&" + remote + ".f0",
            "(void *)&" + remote + ".f1", "sizeof(" + remote + ")",
            "(unsigned)" + original + ".f0", "(unsigned)" + original + ".f1",
            "(unsigned)" + remote + ".f0", "(unsigned)" + remote + ".f1"])
        return '\nprintf("' + fmt + '", ' + ", ".join(arguments) + ");\n"

    edits.append((declarations["remote"][1], frame("INIT")))
    edits.append((if_start, frame("HEADER", carry)))
    for keyword, tag in (("continue", "CONTINUE"), ("break", "BREAK")):
        found = unique(list(re.finditer(r"\b" + keyword + r"\s*;",
                                       mask[loop_open + 1:loop_close])),
                       "actual " + keyword + " edge")
        edits.append((loop_open + 1 + found.start(), frame(tag)))
    local_locations = {}
    for name in case["expected_locals"]:
        pattern = re.compile(r"\b(?:const\s+)?uint8_t\s+" +
                             re.escape(c_names[name]) + r"\s*=[^;]*;")
        decl = unique([match for match in pattern.finditer(mask)
                       if main_start < match.start() < main_end],
                      "actual scalar binding " + name)
        local_locations[name] = decl.end()
        edits.append((decl.end(), '\nprintf("R1134 LOCAL ' + name +
                      ' %u\\n", (unsigned)' + c_names[name] + ");\n"))
    edits.append((local_locations["preserved_old"], frame("FINAL")))
    helper_pattern = re.compile(
        r"\b(?:static\s+)?uint8_t\s+(nl_[A-Za-z0-9_]+)\s*\(\s*"
        r"uint8_t\s*\*\s*(?:const\s+)?([A-Za-z_]\w*)\s*,\s*"
        r"(?:const\s+)?uint8_t\s+(?:const\s+)?([A-Za-z_]\w*)\s*\)\s*\{")
    helper = unique(list(helper_pattern.finditer(mask)), "Copy-returning helper")
    pointer, incoming = helper[2], helper[3]
    edits.append((helper.end(),
                  '\nprintf("R1134 HELPER %p %u %u\\n", (void *)' +
                  pointer + ", (unsigned)*" + pointer + ", (unsigned)" +
                  incoming + ");\n"))
    observed = text
    for position, insertion in sorted(edits, key=lambda item: item[0], reverse=True):
        observed = observed[:position] + insertion + observed[position:]
    metadata = {"public_symbols": c_names, "actual_carry": carry,
                "actual_helper": helper[1], "helper_pointer": pointer,
                "helper_incoming": incoming,
                "insertions": [{"offset": pos, "text": insertion}
                               for pos, insertion in edits],
                "passive_only": True}
    return "#include <stdio.h>\n" + observed, metadata

def validate_trace(output, case):
    rows = {}
    for line in output.decode("ascii").splitlines():
        parts = line.split()
        require(len(parts) >= 2 and parts[0] == "R1134",
                "unexpected guest output: " + line)
        rows.setdefault(parts[1], []).append(parts[2:])

    def one(tag):
        return unique(rows.get(tag, []), tag + " trace")

    def frame_values(parts):
        require(len(parts) == 12, "malformed actual frame")
        identity = [int(value, 10 if index in (3, 7) else 16)
                    for index, value in enumerate(parts[:8])]
        values = [int(value) for value in parts[8:]]
        return identity, values

    identity, values = frame_values(one("INIT"))
    require(values == case["initial"] + case["other"], "initial frame mismatch")
    a, a0, a1, size_a, b, b0, b1, size_b = identity
    require(a > 0 and b > 0 and size_a > 0 and size_b > 0,
            "invalid actual object addresses")
    require(a0 != a1 and a <= a0 < a + size_a and a <= a1 < a + size_a,
            "original fields are not distinct live object subobjects")
    require(b0 != b1 and b <= b0 < b + size_b and b <= b1 < b + size_b,
            "remote fields are not distinct live object subobjects")
    require(a + size_a <= b or b + size_b <= a,
            "original and remote are not disjoint actual objects")
    trace = []
    for parts in rows.get("HEADER", []):
        trace.append(int(parts[0]))
        current_identity, current_values = frame_values(parts[1:])
        require(current_identity == identity and
                current_values == case["initial"] + case["other"],
                "header did not retain original frame")
    require(trace == case["trace"], "actual carry trace mismatch")
    continues = rows.get("CONTINUE", [])
    require(len(continues) == case["continue_count"],
            "actual Continue count mismatch")
    for parts in continues:
        current_identity, current_values = frame_values(parts)
        require(current_identity == identity and
                current_values == case["initial"] + case["other"],
                "Continue changed original frame")
    expected_break = case["initial"].copy()
    expected_break[case["field"]] = case["change"]
    break_identity, break_values = frame_values(one("BREAK"))
    require(break_identity == identity and
            break_values == expected_break + case["other"],
            "actual normal Break frame mismatch")
    expected_final = case["initial"].copy()
    expected_final[case["field"]] = case["caller"]
    final_identity, final_values = frame_values(one("FINAL"))
    require(final_identity == identity and
            final_values == expected_final + case["other"],
            "actual subsequent caller frame mismatch")
    local_rows = rows.get("LOCAL", [])
    require(all(len(row) == 2 for row in local_rows), "malformed local trace")
    local_values = {name: int(value) for name, value in local_rows}
    require(len(local_rows) == len(case["expected_locals"]) and
            local_values == case["expected_locals"],
            "actual Copy/current/sibling/remote/caller scalar mismatch")
    helpers = rows.get("HELPER", [])
    require(len(helpers) == 2 and all(len(row) == 3 for row in helpers),
            "actual ordinary helper count mismatch")
    selected_address = identity[case["field"] + 1]
    expected_calls = [(case["initial"][case["field"]], case["change"]),
                      (case["change"], case["caller"])]
    for row, (old, incoming) in zip(helpers, expected_calls):
        require(int(row[0], 16) == selected_address and
                (int(row[1]), int(row[2])) == (old, incoming),
                "helper did not receive original field ref and actual values")
    require(set(rows) <= {"INIT", "HEADER", "CONTINUE", "BREAK",
                          "FINAL", "LOCAL", "HELPER"}, "unknown observer tag")
    return {"carry_trace": trace, "continue_count": len(continues),
            "break_count": 1, "identity_within_run": identity,
            "locals": local_values, "helpers": helpers,
            "original_disjoint_from_remote": True}

def run():
    require(os.environ.get("GITHUB_ACTIONS") == "true", "Actions-only equipment")
    RAW.mkdir(parents=True, exist_ok=True)
    verified_bundle("c222-r-v1",
                    "3f047b8690c99331a8787762a3d44c5a7c54b4658be3fefcce70d30305659843")
    verified_bundle("c223-r-v2",
                    "c1a536546e8143cbd675b88adddba16c72012aa2ae405777e56b1abc50bab862")
    oracle = json.loads((ROOT / "c223-r-v2/oracle.json").read_bytes())
    for args in (["git", "rev-parse", "HEAD"],
                 ["git", "rev-list", "--parents", "-n", "1", "HEAD"],
                 [COMPILER, "--version"], [HOST_CC, "--version"]):
        checked_ok(command("equipment", "identity", args), "equipment identity")
    gcc = shutil.which("gcc")
    clang = shutil.which("clang-23") or shutil.which("clang")
    require(gcc is not None and clang is not None,
            "equipment: independent GCC and Clang unavailable")
    for cc in (gcc, clang):
        checked_ok(command("equipment", "cc_version", [cc, "--version"]),
                   "compiler identity")
    for case in oracle["cases"]:
        if case["native_allowed"]:
            continue
        source = ROOT / case["path"]
        keep(case["id"] + "/frozen.nl", source.read_bytes())
        code, stdout, stderr = command(case["id"], "NEGATIVE_CHECK",
                                       [COMPILER, "--check", source])
        diagnostic = re.search(
            r"error\((semantic|unsupported)\)\[([A-Z][A-Z0-9-]+)\]:\s*([^\r\n]+)",
            stderr.decode("utf-8", errors="replace"))
        if case["profile_prediction"] == "CONSERVATIVE_REJECTION_ALLOWED":
            require((code == 0 and not stdout and not stderr) or
                    (code == 3 and not stdout and diagnostic is not None),
                    "precision control: unexpected CHECK failure layer")
            verdict = "ACCEPT" if code == 0 else "CONSERVATIVE_REJECT"
        else:
            require(code == 3 and not stdout and diagnostic is not None,
                    case["id"] + ": unsafe negative not rejected at CHECK")
            verdict = "REJECT"
        SUMMARY.append({"case": case["id"], "stage": "NEGATIVE_CHECK",
                        "verdict": verdict, "returncode": code,
                        "rejection_layer": diagnostic[1] if diagnostic else None,
                        "diagnostic_code": diagnostic[2] if diagnostic else None,
                        "diagnostic_message": diagnostic[3] if diagnostic else None,
                        "native": "NOT_RUN_BY_FROZEN_CONTRACT"})
        emit("CASE", SUMMARY[-1])
    modes = [(cc_name, cc, opt, "none")
             for cc_name, cc in (("gcc", gcc), ("clang", clang))
             for opt in ("O0", "O2")]
    if SANITIZER in ("address", "undefined"):
        modes.extend(("clang", clang, opt, SANITIZER) for opt in ("O0", "O2"))
    for case in oracle["cases"]:
        if not case["native_allowed"]:
            continue
        case_id = case["id"]
        source = ROOT / case["path"]
        keep(case_id + "/frozen.nl", source.read_bytes())
        checked_ok(command(case_id, "CHECK", [COMPILER, "--check", source]),
                   case_id + " CHECK", empty_stdout=True)
        public = command(case_id, "SOURCE_ISSUED_PUBLIC_OWNED", [PROBE, source])
        checked_ok(public, case_id + " public owned")
        require(b"R1134 OWNED SOURCE_REGISTERED MAIN_CHECKED READ_ONLY PASS" in
                public[1], "public owned receipt absent")
        keep(case_id + "/public-owned.txt", public[1])
        emission = command(case_id, "C17_EMIT", [COMPILER, source])
        checked_ok(emission, case_id + " emission")
        require(bool(emission[1]), "empty generated translation unit")
        original_c = keep(case_id + "/generated.c", emission[1])
        CURRENT.update(case=case_id, stage="PASSIVE_EQUIPMENT_MAPPING")
        observed, mapping = observe_c(emission[1].decode("utf-8"),
                                     public[1].decode("utf-8"), case)
        passive_c = keep(case_id + "/passive.c", observed)
        keep(case_id + "/passive-map.json",
             json.dumps(mapping, sort_keys=True, indent=2) + "\n")
        for cc_name, cc, optimization, sanitizer in modes:
            mode = cc_name + "-" + optimization + "-" + sanitizer
            for kind, c_source in (("unmodified", original_c), ("passive", passive_c)):
                exe = RAW / case_id / (mode + "-" + kind + ".exe")
                args = [cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra",
                        "-Wpedantic", "-Werror", "-" + optimization]
                if sanitizer != "none":
                    args += ["-fsanitize=" + sanitizer, "-fno-omit-frame-pointer",
                             "-fno-sanitize-recover=all"]
                args += [c_source, "-o", exe]
                built = command(case_id, "STRICT_C17_COMPILE:" + mode + ":" + kind,
                                args, timeout=30)
                checked_ok(built, case_id + " strict C17 " + mode + ":" + kind)
                executable = exe.read_bytes()
                SUMMARY.append({"case": case_id, "mode": mode, "kind": kind,
                                "exe_bytes": len(executable),
                                "exe_sha256": digest(executable)})
                native = command(case_id, "NATIVE:" + mode + ":" + kind,
                                 [exe], timeout=10)
                checked_ok(native, case_id + " native " + mode + ":" + kind,
                           empty_stdout=kind == "unmodified")
                observation = None
                if kind == "passive":
                    CURRENT.update(case=case_id, stage="PASSIVE_TRACE_ASSERT:" + mode)
                    observation = validate_trace(native[1], case)
                    keep(case_id + "/" + mode + "-observation.json",
                         json.dumps(observation, sort_keys=True, indent=2) + "\n")
                SUMMARY.append({"case": case_id, "mode": mode, "kind": kind,
                                "verdict": "PASS", "observation": observation})
        emit("CASE", {"case": case_id, "verdict": "PASS", "modes": len(modes),
                      "unmodified_and_passive": True})
    SUMMARY.append({"stage": "COMPLETE", "verdict": "ALL_FINITE_CHECKS_PASS"})
    emit("COMPLETE", SUMMARY[-1])

def finish(success, error):
    RAW.mkdir(parents=True, exist_ok=True)
    payload = {"schema": "newlang.r1134.c223.raw.v1", "success": success,
               "first_failure": None if success else dict(CURRENT),
               "error": error,
               "github": {key: os.environ.get(key) for key in (
                   "GITHUB_SHA", "GITHUB_REF", "GITHUB_EVENT_NAME", "GITHUB_RUN_ID",
                   "GITHUB_RUN_ATTEMPT", "GITHUB_JOB", "GITHUB_HEAD_REF",
                   "GITHUB_BASE_REF")},
               "host_sanitizer": SANITIZER, "commands": RECORDS,
               "files": FILES, "summary": SUMMARY,
               "later_unrecorded_stages": "NOT_RUN",
               "v1": "BYTE_PRESERVED_NOT_EXECUTED",
               "scope": "SCOPE-CALIBRATED / implementation-fixture-blind",
               "limits": "Finite source/public-owned/native evidence only; private inference, general soundness and audit-locality burden unmeasured; GO624 PAUSED; H1 HOLD."}
    data = (json.dumps(payload, sort_keys=True, separators=(",", ":")) + "\n").encode()
    (RAW / "raw.json").write_bytes(data)
    emit("RAW_READY", {"bytes": len(data), "sha256": digest(data),
                       "path": str(RAW / "raw.json"), "success": success})
    summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary_path:
        with open(summary_path, "a", encoding="utf-8") as stream:
            stream.write("\n## R1134 C223 independent finite execution\n\n")
            stream.write(("PASS" if success else "FIRST RED") + " - raw SHA256 " +
                         digest(data) + "\n\n")
            stream.write("CTest post-test transport preserves exact raw JSON in this job log. V1 unchanged and unexecuted; V2 scope-calibrated. General soundness and audit-locality burden remain unmeasured.\n")
    emit("FINAL", {"success": success, "first_failure": payload["first_failure"],
                   "raw_sha256": digest(data)})

if __name__ == "__main__":
    success = False
    error = None
    try:
        run()
        success = True
    except Exception:
        error = traceback.format_exc()
        print(error, flush=True)
    finally:
        finish(success, error)
    raise SystemExit(0 if success else 1)
