import json
import os
import pathlib
import re
import shutil
import sys

from common import Stop, invoke, require, sha, source_manifest, write_json
from observe import assess_stdout, make_observer


def map_public_sources(root, manifest, out, records):
    """Map actual retained body bytes to unique immutable source occurrences."""
    originals = {item["id"]: (root / item["name"]).read_bytes()
                 for item in manifest["sources"]}
    offsets = {}
    mappings = []
    for body in records:
        if body["kind"] != "BODY" or body["owner"] == "<entry>":
            continue
        unit = body["unit"]
        original = originals["P2" if unit == "P2DUP" else unit]
        retained = (out / ("public-source-" + body["label"] + ".nl")).read_bytes()
        require(retained, "empty actual retained body")
        start = original.find(retained)
        require(start >= 0 and original.find(retained, start + 1) < 0,
                "retained body has no unique immutable source occurrence")
        header = original[:start].rsplit(b"\n", 1)[-1]
        pattern = (rb"fn\s+" + re.escape(body["owner"].encode()) +
                   rb"\([^{}]*\)\s*->\s*(?:usize|unit)\s*")
        require(re.fullmatch(pattern, header) is not None,
                "retained body does not belong to actual callee declaration")
        offset = original[:start].count(b"\n")
        key = (unit, body["owner"])
        require(key not in offsets or offsets[key] == offset,
                "same declaration mapped to different source occurrences")
        offsets[key] = offset
        mappings.append({"unit": unit, "owner": body["owner"],
                         "body": body["body"], "source": body["source"],
                         "context": body["context"], "label": body["label"],
                         "retained_bytes": len(retained),
                         "retained_sha256": sha(retained),
                         "whole_source_sha256": sha(original),
                         "start_byte": start, "end_byte": start + len(retained),
                         "whole_line_offset": offset,
                         "mapping": "unique exact retained-byte substring; declaration name verified"})
    write_json(out / "source-mapping.json", mappings)
    return offsets


def public(config):
    root, manifest = source_manifest(config)
    out = pathlib.Path(config["evidence_dir"]) / "public"
    out.mkdir(parents=True, exist_ok=True)
    paths = [str(root / item["name"]) for item in manifest["sources"]]
    rc, stdout, stderr = invoke(out, "public-probe",
                                [config["probe"], str(out)] + paths)
    records = []
    for line in stdout.decode("utf-8").splitlines():
        records.append(json.loads(line))
    write_json(out / "records.json", records)
    require(rc == 0, "public probe failed; inspect original status/diagnostic")
    require(not stderr, "unexpected public probe stderr")
    offsets = map_public_sources(root, manifest, out, records)
    limits = []
    target = next(r for r in records if r["kind"] == "HOST")
    require(target["size_t_bytes"] == 8 and
            target["size_max"] == 18446744073709551615, "public host T64")
    expected_calls = {
        "P1": {3: ("sift_five", 53, 5, 10, "/"),
               5: ("pack_three", 10, 3, 3, "/"),
               6: ("pack_three", 10, 3, 3, "/")},
        "P2": {9: ("split_seventeen", 38, 17, 2, "/"),
               10: ("residue_six", 38, 6, 2, "%"),
               11: ("residue_six", 2, 6, 2, "%")}}
    expected_bindings = {
        "P1": {"load": 53, "first": 10, "saved": 10, "second": 3,
               "repeated": 3, "load_copy": 53, "second_copy": 3},
        "P2": {"cargo": 38, "quotient": 2, "residual": 2, "forwarded": 2,
               "quotient_copy": 2, "residual_copy": 2, "forwarded_copy": 2}}
    for unit in ("P1", "P2", "P2DUP"):
        original = "P2" if unit == "P2DUP" else unit
        calls = [r for r in records if r["kind"] == "CALL" and
                 r["unit"] == unit and r["caller"] == "main"]
        require(len(calls) == 3, "expected three actual ordinary calls: " + unit)
        identities = {}
        for call in calls:
            expected = expected_calls[original].get(call["line"] + offsets[(unit, "main")])
            require(expected is not None and call["name"] == expected[0],
                    "call/source identity mismatch")
            require(call["function"] > 0, "absent genuine function identity")
            require(call["caller_context"] == call["callee_context"],
                    "call/body public context mismatch")
            if call["name"] in identities:
                require(identities[call["name"]] == call["function"],
                        "same declaration changed identity")
            identities[call["name"]] = call["function"]
            if call["result"]["known"]:
                require(call["result"]["number"] == expected[3],
                        "public caller result mismatch")
            else:
                limits.append(unit + " call result Known fact not observable")
        require(len(set(identities.values())) == 2,
                "distinct declared helpers share a function identity")
        arithmetic = [r for r in records if r["kind"] == "ARITH" and
                      r["unit"] == unit]
        require(len(arithmetic) == 3, "three genuine leaf operations")
        for event in arithmetic:
            e = expected_calls[original].get(event["call_line"] + offsets[(unit, "main")])
            require(e is not None and event["owner"] == e[0] and
                    event["divisor"] == e[2] and event["op"] == e[4],
                    "operation/call/source association mismatch")
            require(event["target_max"] == 18446744073709551615,
                    "public quantity maximum differs from T64")
            require(event["observed"] and event["genuine"] and event["detached"],
                    "arithmetic self authentication failed")
            if event["numerator_known"]:
                require(event["numerator"] == e[1], "public numerator mismatch")
            else:
                limits.append(unit + " numerator Unknown")
            if event["result_known"]:
                require(event["result"] == e[3], "public operation mismatch")
            else:
                limits.append(unit + " operation result Unknown")
        bindings = [r for r in records if r["kind"] == "BIND" and
                    r["unit"] == unit and r["owner"] == "main"]
        for name, number in expected_bindings[original].items():
            matching = [r for r in bindings if r["name"] == name]
            require(len(matching) == 1, "caller source binding not mapped: " + name)
            binding = matching[0]
            observed = False
            for field in ("binding", "initializer_result"):
                view = binding[field]
                if view["present"] and view["known"]:
                    require(view["number"] == number,
                            "caller Copy/binding value mismatch: " + name)
                    observed = True
            if not observed:
                limits.append(unit + " " + name + " value NOT OBSERVED")
    negatives = {}
    # FIRST defines semantic obligations, not a diagnostic location or order.
    # These actual public code labels bind the unchanged obligations after FULL1.
    obligation_codes = {"N1": "USIZE-DIVISION-ZERO",
                        "N2": "USIZE-REMAINDER-ZERO",
                        "N3": "P3-TYPE-MISMATCH",
                        "N4": "P9-RETURN-TYPE"}
    for unit, code in obligation_codes.items():
        parsed = next(r for r in records if r["kind"] == "PARSE" and
                      r["unit"] == unit)
        registered = next(r for r in records if r["kind"] == "REGISTER" and
                          r["unit"] == unit)
        require(parsed["status"] == 0 and registered["status"] == 1,
                "negative did not reach intended semantic rejection: " + unit)
        require(registered["code"] == code,
                "negative did not reach frozen semantic obligation: " + unit)
        require(registered["code"] and registered["message"],
                "missing actual negative diagnostic")
        negatives[unit] = registered
    auth = [r for r in records if r["kind"] == "AUTH"]
    require(len(auth) == 6 and all(not r["accepted"] for r in auth),
            "foreign public evidence was accepted")
    challenge_anchors = {
        "P1_same_value_repeated_call": ("P1", 5, "P1", 6),
        "P1_different_leaf": ("P1", 3, "P1", 5),
        "P2_equal_remainders_distinct_actual": ("P2", 10, "P2", 11),
        "P2_other_actual_origin": ("P2", 10, "P2", 11),
        "P2_identical_bytes_foreign_context": ("P2", 10, "P2DUP", 10),
        "P2_foreign_source_only": ("P2", 10, "P2DUP", 10)}
    mapped_auth = []
    for challenge in auth:
        target = challenge["unit"]
        foreign = challenge["foreign_unit"]
        actual_anchor = (
            target, challenge["target_call_line"] + offsets[(target, "main")],
            foreign, challenge["foreign_call_line"] + offsets[(foreign, "main")])
        require(actual_anchor == challenge_anchors.get(challenge["challenge"]),
                "authentication challenge does not select frozen source calls")
        mapped_auth.append({**challenge, "whole_source_anchors": actual_anchor})
    write_json(out / "mapped-authentication.json", mapped_auth)
    copy_surface = [r for r in records if r["kind"] == "COPY_BODY_SURFACE"]
    for item in copy_surface:
        if not item["observed"]:
            limits.append(item["unit"] +
                          " specialized complete copy-body receipt NOT OBSERVED")
    result = {"status": "OBSERVED_WITH_LIMITS" if limits else "OBSERVED",
              "label": "NON-BLIND / implementation-exposed calibrated",
              "limits": limits, "negative_diagnostics": negatives,
              "auth_challenges": auth,
              "public_numeric_checks": "unchanged FIRST constants",
              "complete_parameter_receipt": "not presumed; see surface records"}
    write_json(out / "summary.json", result)


def native(config):
    root, manifest = source_manifest(config)
    out = pathlib.Path(config["evidence_dir"]) / "native"
    out.mkdir(parents=True, exist_ok=True)
    public_summary = json.loads((pathlib.Path(config["evidence_dir"]) /
                                "public" / "summary.json").read_text())
    compiler = pathlib.Path(config["newlangc"])
    raw_paths = {}
    for item in manifest["sources"]:
        case = item["id"]
        source = root / item["name"]
        rc, stdout, stderr = invoke(out, case + "-check",
                                    [str(compiler), "--check", str(source)])
        if case.startswith("N"):
            require(rc != 0, "CLI accepted intended negative " + case)
            code = public_summary["negative_diagnostics"][case]["code"].encode()
            require(code in stderr, "CLI negative diagnostic differs from public")
            continue
        require(rc == 0, "ordinary positive CLI CHECK failed: " + case)
        directory = out / case
        directory.mkdir()
        raw = directory / "raw.c"
        rc, stdout, stderr = invoke(
            out, case + "-emit",
            [str(compiler), "--emit-c17-to", str(raw), str(source)])
        require(rc == 0 and raw.is_file() and raw.stat().st_size > 0,
                "ordinary emitted-C availability: " + case)
        raw_paths[case] = raw
    gcc = shutil.which("gcc")
    require(gcc is not None, "GCC unavailable")
    compilers = [("gcc", gcc)]
    if config["frontend_only"] != "ON":
        clang = shutil.which("clang-23")
        require(clang is not None, "pinned Clang unavailable in FULL")
        compilers.append(("clang23", clang))
    else:
        write_json(out / "clang-status.json",
                   {"status": "NOT_AVAILABLE", "scope": "NO-LLVM PARTIAL",
                    "executed": False})
    versions = {}
    for name, executable in compilers:
        rc, stdout, stderr = invoke(out, name + "-version",
                                    [executable, "--version"])
        require(rc == 0, "native compiler version query failed")
        if name == "clang23":
            require(b"23.1.3" in stdout, "native Clang is not pinned23.1.3")
        rc, target, err = invoke(out, name + "-target",
                                 [executable, "-dumpmachine"])
        require(rc == 0 and target.decode().strip().startswith("x86_64"),
                "native compiler target is not released x86_64")
        versions[name] = {"path": executable, "version": stdout.decode(),
                          "target": target.decode().strip()}
    write_json(out / "compilers.json", versions)
    records = []
    sanitizer = config["sanitizer"]
    for case, raw in raw_paths.items():
        directory = raw.parent
        original = raw.read_bytes()
        mapping = make_observer(original, directory)
        write_json(directory / "mapping.json", mapping)
        for name, executable in compilers:
            flags = ["-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror"]
            if name == "clang23":
                flags.append("-Wno-error=gcc-install-dir-libstdcxx")
            if sanitizer != "none":
                flags += ["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all",
                          "-fno-omit-frame-pointer"]
            for variant in ("raw", "observer"):
                source = directory / (variant + ".c")
                binary = directory / (name + "-" + variant + ".bin")
                label = case + "-" + name + "-" + variant
                rc, stdout, stderr = invoke(
                    out, label + "-compile",
                    [executable] + flags + [str(source), "-o", str(binary)])
                require(rc == 0, "strict C17 compile failed: " + label)
                rc, stdout, stderr = invoke(out, label + "-run", [str(binary)],
                                            guest=True)
                require(rc == 0 and not stderr, "native/sanitizer discrepancy: " + label)
                if variant == "raw":
                    require(not stdout, "raw unit-main unexpected stdout")
                    observed = {"status": "RAW_EXIT0_ONLY"}
                else:
                    observed = assess_stdout(stdout, mapping, case)
                records.append({"case": case, "compiler": name,
                                "variant": variant, "flags": flags,
                                "source_bytes": source.stat().st_size,
                                "source_sha256": sha(source.read_bytes()),
                                "observation": observed})
                write_json(out / "observations.json", records)
        require(raw.read_bytes() == original, "raw C modified")
    write_json(out / "summary.json",
               {"label": "NON-BLIND / implementation-exposed calibrated",
                "status": "COMPLETED",
                "guest_attempts": len(json.loads(
                    (out / "guest-attempts.json").read_text())),
                "observations": records,
                "original_unmodified_C": True,
                "sanitizer": sanitizer,
                "environment": {k: os.environ.get(k) for k in
                                ("ASAN_OPTIONS", "UBSAN_OPTIONS", "LC_ALL")}})


def main():
    config = json.loads(pathlib.Path(sys.argv[2]).read_text())
    mode = sys.argv[1]
    directory = pathlib.Path(config["evidence_dir"]) / mode
    directory.mkdir(parents=True, exist_ok=True)
    try:
        if mode == "public":
            public(config)
        elif mode == "native":
            native(config)
        else:
            raise Stop("unknown mode")
    except Exception as exc:
        write_json(directory / "STOP.json",
                   {"status": "STOP", "exception_type": type(exc).__name__,
                    "message": str(exc),
                    "instruction": "retain FIRST and raw outputs; classify before repair"})
        print("R1562_STOP " + mode + " " + repr(exc), flush=True)
        return 1
    print("R1562_COMPLETE " + mode, flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
