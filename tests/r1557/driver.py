#!/usr/bin/env python3
"""R1557 Actions-only measurement driver. Never run this locally."""

import argparse
import base64
import difflib
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import traceback

NAMES = [
    "r1557_v2_p1_tail.nl",
    "r1557_v2_p2_return_unitdivisor.nl",
    "r1557_auth_v1_division.nl",
    "r1557_v1_n1_zero_divisor.nl",
    "r1557_v1_n2_wrong_core_type.nl",
    "r1557_v1_n3_target_range.nl",
    "r1557_v1_n4_result_context.nl",
]
TESTS = [
    "r1557_remainder_cli_native.integration",
    "r1557_remainder_public_auth.unit",
]
CANDIDATE = "93fd2752972a9c2348d85e33f941bb7c786ec99b"
BASE = "be0ae7fa537b550ffdb8ad3640f49a504629062a"
COMMANDS = []


def digest(data):
    return hashlib.sha256(data).hexdigest()


def save(root, name, data):
    path = root / name
    path.parent.mkdir(parents=True, exist_ok=True)
    if not isinstance(data, bytes):
        data = data.encode("utf-8")
    path.write_bytes(data)
    return path


def save_json(root, name, value):
    return save(root, name, json.dumps(value, indent=2, sort_keys=True) + "\n")


def require(condition, message):
    if not condition:
        raise RuntimeError("R1557 STOP: " + message)


def run(root, label, command, *, input_data=None, timeout=90):
    item = {"label": label, "command": [str(x) for x in command]}
    try:
        result = subprocess.run(
            item["command"], input=input_data, capture_output=True,
            timeout=timeout, check=False,
        )
        item["returncode"] = result.returncode
        item["stdout_sha256"] = digest(result.stdout)
        item["stderr_sha256"] = digest(result.stderr)
        save(root, label + ".stdout", result.stdout)
        save(root, label + ".stderr", result.stderr)
    except Exception as error:
        item["exception"] = type(error).__name__ + ": " + str(error)
        COMMANDS.append(item)
        save_json(root, label + ".status.json", item)
        raise
    COMMANDS.append(item)
    save_json(root, label + ".status.json", item)
    return result


def frozen(root, evidence):
    manifest = json.loads((root / "frozen_manifest.json").read_text())
    observed = []
    for entry in manifest["sources"]:
        data = (root / "sources" / entry["name"]).read_bytes()
        require(len(data) == entry["utf8_bytes"], "frozen source byte count")
        require(digest(data) == entry["sha256"], "frozen source SHA")
        observed.append({**entry, "verified": True})
    require(sorted(x["name"] for x in observed) == sorted(NAMES),
            "exact seven-source inventory")
    save_json(evidence, "frozen_sources.json", observed)
    return observed


def identity(root, evidence, lane):
    fields = {}
    for name, arguments in [
        ("head", ["rev-parse", "HEAD"]),
        ("tree", ["rev-parse", "HEAD^{tree}"]),
        ("commit_object", ["cat-file", "-p", "HEAD"]),
        ("shallow", ["rev-parse", "--is-shallow-repository"]),
    ]:
        result = run(evidence, lane + ".git." + name,
                     ["git", "-C", str(root), *arguments])
        require(result.returncode == 0, "checkout metadata " + name)
        fields[name] = result.stdout.decode().strip()
    headers = fields["commit_object"].split("\n\n", 1)[0].splitlines()
    trees = [line[5:] for line in headers if line.startswith("tree ")]
    parents = [line[7:] for line in headers if line.startswith("parent ")]
    require(trees == [fields["tree"]], "checkout commit tree identity")
    fields["parents"] = parents
    fields["fixed_candidate"] = CANDIDATE
    fields["fixed_adopted_base"] = BASE
    fields["github"] = {
        name: os.environ.get(name, "")
        for name in (
            "GITHUB_EVENT_NAME", "GITHUB_SHA", "GITHUB_HEAD_REF",
            "GITHUB_BASE_REF", "GITHUB_RUN_ID", "GITHUB_RUN_ATTEMPT",
        )
    }
    require(fields["github"]["GITHUB_EVENT_NAME"] == "pull_request",
            "natural PR event required for measurement")
    require(fields["github"]["GITHUB_SHA"] == fields["head"],
            "actual checkout equals event checkout SHA")
    event_path = os.environ.get("GITHUB_EVENT_PATH")
    require(event_path, "actual PR event metadata available")
    event = json.loads(Path(event_path).read_text())
    pr = event.get("pull_request", {})
    fields["pull_request"] = {
        "head_sha": pr.get("head", {}).get("sha"),
        "head_ref": pr.get("head", {}).get("ref"),
        "base_sha": pr.get("base", {}).get("sha"),
        "base_ref": pr.get("base", {}).get("ref"),
    }
    require(fields["pull_request"]["base_sha"] == CANDIDATE,
            "fixed PR base drift")
    require(fields["pull_request"]["head_ref"] ==
            "r-fast/r1557-remainder-c1281", "own PR head ref")
    require(fields["pull_request"]["base_ref"] ==
            "p1554-authenticated-usize-remainder-public",
            "own fixed base ref")
    require(parents == [CANDIDATE, fields["pull_request"]["head_sha"]],
            "synthetic checkout actual ordered parents")
    candidate = run(evidence, lane + ".git.candidate_tree",
                    ["git", "-C", str(root),
                     "rev-parse", CANDIDATE + "^{tree}"])
    fields["candidate_tree_local_returncode"] = candidate.returncode
    if candidate.returncode == 0:
        fields["candidate_tree_local"] = candidate.stdout.decode().strip()
        require(fields["candidate_tree_local"] ==
                "74666c834b433b63e00aed3ebdb020f7a2b6d10b",
                "locally available candidate tree drift")
    else:
        require(fields["shallow"] == "true",
                "missing candidate object requires actual shallow checkout")
        fields["candidate_tree_local"] = None
    fields["candidate_tree_authoritative_api"] = {
        "commit": CANDIDATE,
        "tree": "74666c834b433b63e00aed3ebdb020f7a2b6d10b",
        "provenance": "C1281 fixed release plus R individual GitHub commit GET",
        "is_local_observation": False,
    }
    save_json(evidence, lane + ".identity.json", fields)


def public_test(args):
    root, evidence = Path(args.root), Path(args.evidence)
    frozen(root, evidence)
    identity(root, evidence, "public")
    result = run(evidence, "public.probe",
                 [args.probe, *(root / "sources" / name for name in NAMES)])
    save_json(evidence, "public.result.json", {
        "returncode": result.returncode,
        "test": "r1557_remainder_public_auth.unit",
        "guest_processes": 0,
    })
    require(result.returncode == 0, "public source/authentication probe failed")
    print("R1557 public source/authentication passed; guest processes0")


def function_map(raw):
    lines = raw.decode("utf-8").splitlines(keepends=True)
    functions = []
    current = None
    depth = 0
    for index, line in enumerate(lines):
        if current is None:
            match = re.fullmatch(
                r"\s*(?:static\s+)?(size_t|void|int)\s+"
                r"([A-Za-z_]\w*)\s*\(([^;{}]*)\)\s*(\{)?\s*\n?",
                line,
            )
            if match:
                parameters = re.findall(
                    r"(?:const\s+)?size_t\s+([A-Za-z_]\w*)",
                    match.group(3),
                )
                current = {
                    "return_type": match.group(1), "name": match.group(2),
                    "parameters": parameters, "start": index,
                    "declarations": [], "returns": [],
                }
                depth = 1 if match.group(4) else 0
            continue
        before = depth
        depth += line.count("{") - line.count("}")
        declaration = re.fullmatch(
            r"\s*(?:const\s+)?size_t\s+([A-Za-z_]\w*)"
            r"\s*=\s*(.*?)\s*;\s*\n?", line,
        )
        if declaration:
            current["declarations"].append({
                "name": declaration.group(1),
                "initializer": declaration.group(2), "line": index,
            })
        returned = re.fullmatch(r"\s*return(?:\s+(.*?))?;\s*\n?", line)
        if returned:
            current["returns"].append({
                "line": index, "expression": returned.group(1) or "",
            })
        if before > 0 and depth == 0:
            current["end"] = index
            functions.append(current)
            current = None
    require(current is None, "observer function boundary")
    return lines, functions


def observer(raw, evidence, label):
    lines, functions = function_map(raw)
    helpers = [f for f in functions if f["return_type"] == "size_t"
               and len(f["parameters"]) == 1]
    require(len(helpers) == 1, "one emitted unary quantity helper")
    helper = helpers[0]
    require(len(helper["returns"]) == 1, "one actual helper return point")
    returned = helper["returns"][0]["expression"].strip()
    require(re.fullmatch(r"[A-Za-z_]\w*", returned) is not None,
            "helper result must be an existing observable local")
    declarations = {d["name"]: d for d in helper["declarations"]}
    require(returned in declarations, "return local mapped to declaration")
    alias_chain = [returned]
    while True:
        expression = declarations[alias_chain[-1]]["initializer"].strip()
        if expression not in declarations:
            break
        require(expression not in alias_chain, "acyclic emitted result aliases")
        alias_chain.append(expression)
    callers = []
    for function in functions:
        calls = [d for d in function["declarations"]
                 if re.search(r"\b" + re.escape(helper["name"]) + r"\s*\(",
                              d["initializer"])]
        if len(calls) == 2:
            callers.append((function, calls))
    require(len(callers) == 1, "one caller with two original helper calls")
    caller, calls = callers[0]
    call_names = [d["name"] for d in calls]
    copy_names = []
    for name in call_names:
        copies = [d["name"] for d in caller["declarations"]
                  if d["initializer"].strip() == name]
        require(len(copies) == 1, "one original caller Copy for each result")
        copy_names.append(copies[0])
    mapping = {
        "helper": helper["name"], "helper_parameter": helper["parameters"][0],
        "helper_return": returned, "helper_alias_chain": alias_chain,
        "caller": caller["name"],
        "caller_values": dict(zip(
            ["first_result", "second_result", "first_copy", "second_copy"],
            call_names + copy_names,
        )),
        "functions": functions,
    }
    insertions = [(0, b"#include <stdio.h>\n")]
    offsets = []
    offset = 0
    for line in lines:
        offsets.append(offset)
        offset += len(line.encode("utf-8"))
    for function in [helper, caller]:
        variables = function["parameters"] + [
            d["name"] for d in function["declarations"]
        ]
        require(variables and len(variables) == len(set(variables)),
                "unique existing scalar variables for read-only observer")
        returns = function["returns"]
        if not returns:
            require(function["return_type"] == "void",
                    "only implicit unit return can use closing-brace point")
            returns = [{"line": function["end"], "expression": ""}]
        require(len(returns) == 1, "single selected return point")
        format_text = ("R1557_OBS|" + function["name"] + "|" +
                       "|".join(name + "=%zu" for name in variables) + r"\n")
        addition = (
            '    printf("' + format_text + '", ' +
            ", ".join(variables) + ");\n"
        ).encode()
        insertions.append((offsets[returns[0]["line"]], addition))
    insertions.sort(key=lambda item: item[0])
    pieces = []
    cursor = 0
    declared = []
    for position, addition in insertions:
        pieces.extend([raw[cursor:position], addition])
        cursor = position
        declared.append({
            "original_byte_offset": position,
            "addition_base64": base64.b64encode(addition).decode(),
        })
    pieces.append(raw[cursor:])
    instrumented = b"".join(pieces)
    restored = bytearray(instrumented)
    accumulated = 0
    placed = []
    for position, addition in insertions:
        placed.append((position + accumulated, addition))
        accumulated += len(addition)
    for position, addition in reversed(placed):
        require(bytes(restored[position:position + len(addition)]) == addition,
                "declared observer addition exact")
        del restored[position:position + len(addition)]
    require(bytes(restored) == raw, "observer removal restores exact raw C")
    save(evidence, label + ".observer.c", instrumented)
    save(evidence, label + ".observer.diff", "".join(difflib.unified_diff(
        raw.decode().splitlines(keepends=True),
        instrumented.decode().splitlines(keepends=True),
        fromfile=label + ".raw.c", tofile=label + ".observer.c",
    )))
    save_json(evidence, label + ".observer_map.json", {
        **mapping, "insertions": declared, "raw_sha256": digest(raw),
        "observer_sha256": digest(instrumented),
        "removal_matches_raw": True,
    })
    return mapping


def observed_values(stdout, mapping, inputs, expected):
    records = []
    for line in stdout.decode("utf-8").splitlines():
        fields = line.split("|")
        require(len(fields) >= 3 and fields[0] == "R1557_OBS",
                "actual observer stdout format")
        values = {}
        for field in fields[2:]:
            name, number = field.split("=", 1)
            require(re.fullmatch(r"\d+", number) is not None,
                    "actual observer scalar decimal")
            values[name] = int(number)
        records.append({"function": fields[1], "values": values})
    helpers = [r for r in records if r["function"] == mapping["helper"]]
    callers = [r for r in records if r["function"] == mapping["caller"]]
    require(len(records) == 3 and len(helpers) == 2 and len(callers) == 1,
            "two actual helper observations and one actual caller")
    for index, record in enumerate(helpers):
        values = record["values"]
        require(values[mapping["helper_parameter"]] == inputs[index],
                "actual helper argument source order")
        for name in mapping["helper_alias_chain"]:
            require(values[name] == expected[index],
                    "actual source helper result/Copy value")
    caller_expected = dict(zip(
        ["first_result", "second_result", "first_copy", "second_copy"],
        expected + expected,
    ))
    for source_name, c_name in mapping["caller_values"].items():
        require(callers[0]["values"][c_name] == caller_expected[source_name],
                "actual caller result/Copy for " + source_name)
    return {"records": records, "caller_source_values": caller_expected}


def compilers(args, evidence):
    result = [("gcc", args.gcc)]
    if args.frontend_only == "ON":
        save_json(evidence, "clang_availability.json", {
            "status": "NOT_AVAILABLE", "reason": "NO-LLVM PARTIAL scope",
        })
    else:
        prefix = os.environ.get("NEWLANG_SDK_ROOT")
        require(bool(prefix), "reviewed pinned Clang SDK prefix")
        clang = str(Path(prefix) / "bin" / "clang-23")
        version = run(evidence, "clang.version", [clang, "--version"])
        require(version.returncode == 0 and b"23.1.3" in version.stdout,
                "exact pinned Clang23.1.3 available")
        result.append(("clang", clang))
    for name, executable in result:
        version = run(evidence, name + ".version.used",
                      [executable, "--version"])
        require(version.returncode == 0, "available compiler " + name)
        target = run(evidence, name + ".target",
                     [executable, "-dumpmachine"])
        require(target.returncode == 0, "actual target " + name)
        macros = run(
            evidence, name + ".target_width",
            [executable, "-E", "-P", "-std=c17", "-x", "c", "-"],
            input_data=(
                b"#include <stdint.h>\n#include <stddef.h>\n"
                b"R1557_SIZE_T_BYTES __SIZEOF_SIZE_T__\n"
                b"R1557_SIZE_MAX SIZE_MAX\n"
            ),
        )
        require(macros.returncode == 0, "target preprocessing")
        lines = [line for line in macros.stdout.decode().splitlines()
                 if line.startswith("R1557_")]
        require(any(line == "R1557_SIZE_T_BYTES 8" for line in lines),
                "frozen T64 actual compiler target width")
        save(evidence, name + ".target_fields.txt", "\n".join(lines) + "\n")
    return result


def native_test(args):
    root, evidence = Path(args.root), Path(args.evidence)
    frozen(root, evidence)
    identity(root, evidence, "native")
    public = json.loads((evidence / "public.result.json").read_text())
    require(public["returncode"] == 0, "public fixture prerequisite")
    cases = [
        ("p1", NAMES[0], [143, 153], [7, 0]),
        ("p2", NAMES[1], [0, 65535], [0, 0]),
    ]
    for label, name, _, _ in cases:
        checked = run(evidence, label + ".check",
                      [args.compiler, "--check", root / "sources" / name])
        require(checked.returncode == 0 and not checked.stdout
                and not checked.stderr, "positive ordinary CLI CHECK " + label)
    diagnostics = [
        "USIZE-REMAINDER-ZERO", "USIZE-REMAINDER-TYPE",
        "USIZE-LITERAL-RANGE", "P8-BODY-RESULT",
    ]
    for index, name in enumerate(NAMES[3:]):
        checked = run(
            evidence, "n" + str(index + 1) + ".check",
            [args.compiler, "--check", root / "sources" / name],
        )
        require(checked.returncode != 0 and not checked.stdout and
                diagnostics[index].encode() in checked.stderr and
                b"BACKEND" not in checked.stderr,
                "genuine negative CHECK " + name)
    available = compilers(args, evidence)
    flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    if args.sanitizer != "none":
        require(args.sanitizer in ("address", "undefined"), "lane sanitizer")
        flags += [
            "-fsanitize=" + args.sanitizer, "-fno-omit-frame-pointer",
            "-fno-sanitize-recover=all",
        ]
    guests = 0
    numbers = {}
    for label, name, inputs, expected in cases:
        emitted = run(evidence, label + ".emit",
                      [args.compiler, root / "sources" / name])
        require(emitted.returncode == 0 and emitted.stdout
                and not emitted.stderr, "ordinary raw C emission " + label)
        raw = emitted.stdout
        raw_path = save(evidence, label + ".raw.c", raw)
        mapping = observer(raw, evidence, label)
        for cc_name, executable in available:
            cc_flags = flags.copy()
            if cc_name == "clang":
                cc_flags += ["-Wno-error=gcc-install-dir-libstdcxx"]
            for variant in ("raw", "observer"):
                source = evidence / (label + "." + variant + ".c")
                require(source.read_bytes() == raw if variant == "raw" else True,
                        "compile exact original raw bytes")
                binary = evidence / (label + "." + cc_name + "." + variant)
                prefix = label + "." + cc_name + "." + variant
                compiled = run(
                    evidence, prefix + ".compile",
                    [executable, *cc_flags, source, "-o", binary],
                )
                require(compiled.returncode == 0,
                        "strict actual guest C compilation " + prefix)
                executed = run(evidence, prefix + ".run", [binary], timeout=30)
                guests += 1
                require(executed.returncode == 0 and not executed.stderr,
                        "actual native status/sanitizer " + prefix)
                if variant == "raw":
                    require(not executed.stdout, "raw unit program stdout")
                else:
                    numbers[prefix] = observed_values(
                        executed.stdout, mapping, inputs, expected,
                    )
        require(raw_path.read_bytes() == raw, "raw bytes retained unchanged")
    expected_guests = 4 if args.frontend_only == "ON" else 8
    require(guests == expected_guests, "exact guest execution budget")
    save_json(evidence, "native.result.json", {
        "test": "r1557_remainder_cli_native.integration", "passed": True,
        "guest_processes": guests, "companion_guest_processes": 0,
        "sanitizer": args.sanitizer, "observations": numbers,
    })
    print("R1557 native passed; actual guest processes=" + str(guests))


def inventory(args):
    evidence = Path(args.evidence)
    environment = os.environ.copy()
    environment["R1557_REGISTRATION_ONLY"] = "1"
    result = subprocess.run(
        [args.ctest, "--test-dir", args.build_dir, "-N", "--show-only=json-v1"],
        capture_output=True, check=False, env=environment,
    )
    save(evidence, "inventory.ctest_complete.stdout", result.stdout)
    save(evidence, "inventory.ctest_complete.stderr", result.stderr)
    require(result.returncode == 0, "actual CTest -N inventory")
    text = result.stdout.decode("utf-8").lstrip()
    report, end = json.JSONDecoder().raw_decode(text)
    trailer = text[end:]
    save(evidence, "inventory.ctest_trailer.stdout", trailer)
    print("R1557 INVENTORY_TRAILER " + json.dumps({
        "bytes": len(trailer.encode()), "sha256": digest(trailer.encode()),
        "text": trailer if len(trailer) <= 8192 else "retained; too long to print",
    }, sort_keys=True))
    names = [entry["name"] for entry in report["tests"]]
    own = sorted(name for name in names if name.startswith("r1557_"))
    require(len(names) == len(set(names)) and own == TESTS,
            "exact own registration and unique total inventory")
    record = {
        "all_registered_count": len(names),
        "all_registered_names_sha256": digest(
            ("\n".join(sorted(names)) + "\n").encode()
        ),
        "own_registered": own, "own_count": len(own),
        "semantic_tests_executed_by_inventory": 0,
    }
    save_json(evidence, "inventory.json", record)
    print("R1557 ACTUAL_CTEST_N " + json.dumps(record, sort_keys=True))


def transport(args):
    if os.environ.get("R1557_REGISTRATION_ONLY") == "1":
        return
    evidence = Path(args.evidence)
    allowed = {".json", ".txt", ".stdout", ".stderr", ".c", ".diff"}
    payload = {}
    total = 0
    for path in sorted(evidence.glob("*")):
        if path.is_symlink() or not path.is_file() or path.suffix not in allowed:
            continue
        data = path.read_bytes()
        total += len(data)
        if len(data) > 2_000_000 or total > 3_000_000:
            print("R1557_TRANSPORT_LIMIT " + path.name, flush=True)
            return
        payload[path.name] = {
            "bytes": len(data), "sha256": digest(data),
            "base64": base64.b64encode(data).decode(),
        }
    missing = [name for name in
               ("inventory.json", "public.result.json", "native.result.json")
               if name not in payload]
    encoded = json.dumps(payload, sort_keys=True, separators=(",", ":")).encode()
    compressed = gzip.compress(encoded, mtime=0)
    text = base64.b64encode(compressed).decode()
    print("R1557_EVIDENCE_BEGIN " + json.dumps({
        "files": len(payload), "payload_bytes": len(encoded),
        "payload_sha256": digest(encoded), "gzip_sha256": digest(compressed),
        "base64_characters": len(text), "missing": missing,
        "transport_is_pass_gate": False,
    }, sort_keys=True), flush=True)
    for offset in range(0, len(text), 1600):
        print("R1557_EVIDENCE_CHUNK " + str(offset // 1600) + " " +
              text[offset:offset + 1600], flush=True)
    print("R1557_EVIDENCE_END", flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("public", "native", "inventory", "transport"))
    parser.add_argument("--root")
    parser.add_argument("--evidence", required=True)
    parser.add_argument("--probe")
    parser.add_argument("--compiler")
    parser.add_argument("--gcc")
    parser.add_argument("--frontend-only", choices=("ON", "OFF"), default="OFF")
    parser.add_argument("--sanitizer", default="none")
    parser.add_argument("--ctest")
    parser.add_argument("--build-dir")
    args = parser.parse_args()
    evidence = Path(args.evidence)
    evidence.mkdir(parents=True, exist_ok=True)
    if args.mode == "transport":
        try:
            transport(args)
        except Exception as error:
            print("R1557_TRANSPORT_UNAVAILABLE " + str(error), flush=True)
        return 0
    status = 0
    try:
        {"public": public_test, "native": native_test,
         "inventory": inventory}[args.mode](args)
    except Exception:
        status = 1
        error = traceback.format_exc()
        save(evidence, args.mode + ".failure.txt", error)
        print(error, file=sys.stderr, flush=True)
    finally:
        if COMMANDS:
            save_json(evidence, args.mode + ".commands.json", COMMANDS)
        save_json(evidence, args.mode + ".driver_status.json", {
            "returncode": status, "mode": args.mode,
        })
    return status


if __name__ == "__main__":
    raise SystemExit(main())
