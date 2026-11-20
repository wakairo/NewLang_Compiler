#!/usr/bin/env python3
"""R1568 own apparatus. No producer harness and no guest state injection."""
import argparse
import base64
import hashlib
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import traceback

parser = argparse.ArgumentParser()
parser.add_argument("mode", choices=("public", "controls", "native"))
parser.add_argument("--cli", required=True)
parser.add_argument("--probe", required=True)
parser.add_argument("--fixtures", required=True)
parser.add_argument("--output", required=True)
parser.add_argument("--build-dir", required=True)
parser.add_argument("--host-cc", required=True)
parser.add_argument("--sanitizer", required=True)
parser.add_argument("--build-type", required=True)
parser.add_argument("--frontend", required=True)
args = parser.parse_args()
root = pathlib.Path(args.fixtures)
out = pathlib.Path(args.output) / args.mode
out.mkdir(parents=True, exist_ok=True)
manifest = json.loads((root / "manifest.json").read_text())
command_number = 0
guest_pairs = []
result = {"mode": args.mode, "status": "STARTED", "observations": []}


class Stop(Exception):
    pass


def digest(data):
    return hashlib.sha256(data).hexdigest()


def save(name, data):
    if isinstance(data, str):
        data = data.encode()
    path = out / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    return path


def record(name, value):
    return save(name, json.dumps(value, indent=2, sort_keys=True) + "\n")


def command(argv, label, stdin=None, timeout=120):
    global command_number
    command_number += 1
    prefix = f"commands/{command_number:03d}-{label}"
    metadata = {"argv": list(map(str, argv)), "cwd": os.getcwd(),
                "stdout": prefix + ".stdout", "stderr": prefix + ".stderr"}
    record(prefix + ".json", metadata)
    try:
        proc = subprocess.run(list(map(str, argv)), input=stdin,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              timeout=timeout, check=False)
        rc, stdout, stderr = proc.returncode, proc.stdout, proc.stderr
    except subprocess.TimeoutExpired as exc:
        rc = -999
        stdout, stderr = exc.stdout or b"", exc.stderr or b""
        metadata["timeout"] = timeout
    except OSError as exc:
        rc, stdout, stderr = -998, b"", str(exc).encode()
    save(metadata["stdout"], stdout)
    save(metadata["stderr"], stderr)
    metadata.update(exit=rc, stdout_bytes=len(stdout), stderr_bytes=len(stderr),
                    stdout_sha256=digest(stdout), stderr_sha256=digest(stderr))
    record(prefix + ".json", metadata)
    return metadata, stdout, stderr


def case_path(case):
    entry = manifest["cases"][case]
    path = root / entry["source"]
    data = path.read_bytes()
    if len(data) != entry["bytes"] or digest(data) != entry["sha256"]:
        raise Stop("OWN_APPARATUS source byte/hash mismatch: " + case)
    save("inputs/" + entry["source"], data)
    oracle = (root / entry["oracle"]).read_bytes()
    if digest(oracle) != entry["oracle_sha256"]:
        raise Stop("OWN_APPARATUS oracle hash mismatch: " + case)
    save("inputs/" + entry["oracle"], oracle)
    return path


def metadata():
    values = {key: os.environ.get(key) for key in
              ("GITHUB_RUN_ID", "GITHUB_RUN_ATTEMPT", "GITHUB_SHA", "GITHUB_REF",
               "GITHUB_EVENT_NAME", "GITHUB_JOB")}
    values.update(host_cc=args.host_cc, sanitizer=args.sanitizer,
                  build_type=args.build_type, frontend=args.frontend)
    event_path = os.environ.get("GITHUB_EVENT_PATH")
    if event_path:
        event = json.loads(pathlib.Path(event_path).read_text())
        pr = event.get("pull_request", {})
        values["pr_head"] = pr.get("head", {}).get("sha")
        values["pr_base"] = pr.get("base", {}).get("sha")
        values["pr_base_ref"] = pr.get("base", {}).get("ref")
    record("environment.json", values)
    command(["git", "rev-parse", "HEAD"], "actual-head")
    command(["git", "cat-file", "-p", "HEAD"], "actual-head-object")
    for label, sha in (("fixed-p", manifest["fixed_p"]),
                       ("canonical", manifest["canonical"])):
        command(["git", "cat-file", "-e", sha + "^{commit}"],
                label + "-local-object")
    if args.mode == "public":
        command(["ctest", "--test-dir", args.build_dir, "-N"], "ctest-inventory")
        info, stdout, stderr = command(
            ["ctest", "--test-dir", args.build_dir, "-N", "--show-only=json-v1"],
            "ctest-json")
        parsed = {"exit": info["exit"], "stderr_bytes": len(stderr)}
        try:
            text = stdout.decode()
            start = text.index("{")
            data, used = json.JSONDecoder().raw_decode(text[start:])
            names = [test["name"] for test in data["tests"]]
            parsed.update(prefix=text[:start], trailer=text[start + used:],
                          names=names, unique=len(names) == len(set(names)))
            required = {"r1568.public.unit",
                        "r1568.cli_negative_control.integration",
                        "r1568.c17_native.integration"}
            parsed["own_present"] = sorted(required.intersection(names))
            if not parsed["unique"] or not required.issubset(names):
                raise Stop("OWN_APPARATUS CTest registration mismatch")
        except Stop:
            raise
        except Exception as exc:
            parsed["parse_error"] = str(exc)
            record("ctest-parsed.json", parsed)
            raise Stop("OWN_APPARATUS CTest JSON unavailable") from exc
        record("ctest-parsed.json", parsed)


def public():
    p1, p2 = case_path("P1"), case_path("V3_P2")
    info, stdout, stderr = command([args.probe, p1, p2], "public-probe")
    records = []
    for number, line in enumerate(stdout.splitlines(), 1):
        try:
            records.append(json.loads(line))
        except Exception:
            raise Stop(f"OWN_APPARATUS public JSON line {number}")
    record("public-records.json", records)
    if info["exit"] != 0:
        raise Stop("STOP public registration/entry/authentication first failure")
    expected = {
        0: {"actual": 26, "first_result": 6, "equal_result": 6,
            "reused_actual_result": 6, "forwarded_result": 1},
        1: {"actual": 41, "first_result": 6, "result_copy": 6,
            "forwarded_result": 6, "reused_result": 6},
        2: {"actual": 26, "first_result": 6, "equal_result": 6,
            "reused_actual_result": 6, "forwarded_result": 1},
    }
    seen = set()
    for row in records:
        if row.get("record") == "host_target" and row["size_max"] < 41:
            raise Stop("TARGET_PREMISE_UNSATISFIED")
        if row.get("record") == "arithmetic" and row.get("present"):
            if row["target_max"] < 41:
                raise Stop("TARGET_PREMISE_UNSATISFIED")
            if not row.get("valid"):
                raise Stop("STOP arithmetic authentication")
        if (row.get("record") == "node" and row.get("scope") == "main"
                and row.get("kind") == "binding"
                and row.get("name") in expected.get(row.get("cohort"), {})):
            key = (row["cohort"], row["name"])
            if row.get("observed") and row.get("known"):
                want = expected[row["cohort"]][row["name"]]
                if row["number"] != want:
                    raise Stop("STOP actual Known caller quantity mismatch")
                seen.add(key)
    result["known_main_bindings"] = [
        {"cohort": cohort, "name": name} for cohort, name in sorted(seen)]
    result["unobserved_main_bindings"] = [
        {"cohort": cohort, "name": name, "status": "NOT_OBSERVED"}
        for cohort, entries in expected.items() for name in entries
        if (cohort, name) not in seen]
    result["definition_time_body_record"] = "NOT_OBSERVED"
    result["complete_formal_call_world_certificate"] = "NOT_OBSERVED"


def controls():
    for case in ("N1", "N2", "N3", "N4", "P2"):
        path = case_path(case)
        info, stdout, stderr = command([args.cli, "--check", path],
                                       case + "-check")
        observation = {"case": case, "check": info,
                       "canonical_role": manifest["cases"][case]["role"]}
        result["observations"].append(observation)
        if info["exit"] < 0:
            raise Stop("STOP CLI signal/timeout/launch failure: " + case)
        if case in ("N1", "N2", "N3"):
            if info["exit"] == 0:
                raise Stop("STOP definite-invalid source accepted: " + case)
            observation["classification"] = (
                "REJECTED_AT_REPORTED_FIRST_LAYER; inspect actual diagnostic")
        else:
            observation["mandatory_reject"] = False
            if info["exit"] == 0:
                emitted, raw, errors = command([args.cli, path], case + "-emit")
                observation["emission"] = emitted
                save("coverage/" + case + ".raw.c", raw)
                if emitted["exit"] < 0:
                    raise Stop("STOP legal-control emitter signal")
            else:
                observation["emission"] = "NOT_RUN_AFTER_CHECK_REFUSAL"
            observation["native"] = "NOT_IN_NATIVE_COHORT"


def observed_c(raw):
    text = raw.decode("utf-8")
    lines = text.splitlines(keepends=True)
    inserts = {}
    mappings = []
    # Actual emitted identifiers are discovered, never predicted by an ID rule.
    function_pattern = re.compile(
        r"(?:static\s+)?(?:inline\s+)?(?:void|int|size_t|uint(?:32|64)_t)"
        r"\s+([A-Za-z_]\w*)\s*\(([^;{}]*)\)\s*\{")
    for match in function_pattern.finditer(text):
        offset = match.end()
        line = text.count("\n", 0, offset) + 1
        for param in match.group(2).split(","):
            found = re.fullmatch(
                r"\s*(?:const\s+)?(?:size_t|uint(?:32|64)_t)"
                r"\s+([A-Za-z_]\w*)\s*", param)
            if found:
                mappings.append((offset, "formal", found.group(1), line))
    decl = re.compile(
        r"^(\s+)(?:const\s+)?(?:size_t|uint(?:32|64)_t|uintptr_t)"
        r"\s+([A-Za-z_]\w*)\s*=\s*[^;]+;\s*(?://[^\n]*)?$")
    ret = re.compile(r"^(\s+)return\s+([A-Za-z_]\w*)\s*;\s*$")
    offset = 0
    for number, line in enumerate(lines, 1):
        found = decl.fullmatch(line.rstrip("\r\n"))
        if found:
            mappings.append((offset + len(line), "initialized_local",
                             found.group(2), number))
        offset += len(line)
    if not mappings:
        return None, [], []
    additions = [(0, '#include <inttypes.h>\n#include <stdio.h>\n')]
    for offset, kind, name, line in mappings:
        message = (f'R1568_VALUE line={line} kind={kind} name={name} value=')
        code = ('\n    fprintf(stdout, "' + message +
                '%" PRIuMAX "\\n", (uintmax_t)' + name + ');\n')
        additions.append((offset, code))
    additions.sort(key=lambda item: item[0])
    parts, position, observed_position = [], 0, 0
    spans = []
    for offset, code in additions:
        unchanged = text[position:offset]
        parts.append(unchanged)
        observed_position += len(unchanged.encode())
        encoded = code.encode()
        spans.append([observed_position, observed_position + len(encoded)])
        parts.append(code)
        observed_position += len(encoded)
        position = offset
    parts.append(text[position:])
    observed = "".join(parts).encode()
    recovered = bytearray()
    position = 0
    for start, end in spans:
        recovered.extend(observed[position:start])
        position = end
    recovered.extend(observed[position:])
    if bytes(recovered) != raw:
        raise Stop("OWN_APPARATUS addition-only reconstruction mismatch")
    return observed, mappings, spans


def native():
    compilers = [("GCC", shutil.which("gcc"))]
    if args.frontend.upper() in ("ON", "TRUE", "1"):
        result["clang"] = "NOT_EXECUTED: PARTIAL cohort GCC only"
    else:
        compilers.append(("Clang", shutil.which("clang-23")))
    versions = {}
    for label, cc in compilers:
        if not cc:
            raise Stop("OWN_APPARATUS required compiler unavailable: " + label)
        info, version, stderr = command([cc, "--version"], label + "-version")
        command([cc, "-dumpmachine"], label + "-target")
        _, macros, _ = command([cc, "-dM", "-E", "-x", "c", "-"],
                                label + "-target-macros", stdin=b"")
        if info["exit"] != 0:
            raise Stop("OWN_APPARATUS compiler version query failed")
        if label == "Clang" and b"23.1.3" not in version:
            raise Stop("STOP pinned Clang target drift")
        versions[label] = version.decode(errors="replace")
        size = re.search(rb"^#define __SIZEOF_SIZE_T__ (\d+)$", macros, re.M)
        result.setdefault("target_sizes", {})[label] = (
            int(size.group(1)) if size else "NOT_OBSERVED")
        maximum = re.search(rb"^#define __SIZE_MAX__ (.+)$", macros, re.M)
        measured_maximum = "NOT_OBSERVED"
        if maximum:
            token = maximum.group(1).strip().strip(b"()")
            token = re.sub(rb"[uUlL]+$", b"", token)
            try:
                measured_maximum = int(token, 0)
            except ValueError:
                pass
        result.setdefault("native_target_maximum", {})[label] = measured_maximum
        if isinstance(measured_maximum, int) and measured_maximum < 41:
            raise Stop("TARGET_PREMISE_UNSATISFIED")
    for case in ("P1", "V3_P2"):
        path = case_path(case)
        check, stdout, stderr = command([args.cli, "--check", path],
                                        case + "-check")
        if check["exit"] != 0 or stdout or stderr:
            raise Stop("STOP positive ordinary CHECK mismatch: " + case)
        emit, raw, stderr = command([args.cli, path], case + "-emit")
        save(case + "/raw.c", raw)
        if emit["exit"] != 0 or not raw:
            raise Stop("STOP positive ordinary C17 emission mismatch: " + case)
        observed, mapping, spans = observed_c(raw)
        record(case + "/observer-map.json",
               {"mapping": mapping, "inserted_spans": spans,
                "raw_sha256": digest(raw),
                "source_binding_association": "NOT_AUTOMATICALLY_ASSERTED"})
        variants = [("raw", raw)]
        if observed is None:
            result["observations"].append(
                {"case": case, "observer": "NOT_OBSERVED"})
        else:
            save(case + "/observer.c", observed)
            import difflib
            diff = "".join(difflib.unified_diff(
                raw.decode().splitlines(keepends=True),
                observed.decode().splitlines(keepends=True),
                fromfile="raw.c", tofile="observer.c"))
            save(case + "/observer.diff", diff)
            variants.append(("observer", observed))
        for label, cc in compilers:
            flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
            flags += (["-O3", "-DNDEBUG"] if args.build_type == "Release"
                      else ["-O0", "-g"])
            if label == "Clang":
                flags.append("-Wno-error=gcc-install-dir-libstdcxx")
            if args.sanitizer != "none":
                flags += ["-fsanitize=" + args.sanitizer,
                          "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
            for variant, data in variants:
                if len(guest_pairs) >= 16:
                    raise Stop("STOP guest compile/run budget exhausted")
                cfile = out / case / (variant + ".c")
                binary = out / case / (label.lower() + "-" + variant)
                pair = {"case": case, "compiler": label, "variant": variant,
                        "source_bytes": len(data), "source_sha256": digest(data),
                        "flags": flags, "sequence": len(guest_pairs) + 1}
                guest_pairs.append(pair)
                compiled, _, _ = command(
                    [cc, *flags, cfile, "-o", binary],
                    case + "-" + label + "-" + variant + "-compile")
                pair["compile"] = compiled
                record("guest-ledger.json", guest_pairs)
                if compiled["exit"] != 0:
                    raise Stop("STOP guest compile failure: " + case + "/" +
                               variant + " (classify raw vs observer)")
                run, stdout, stderr = command(
                    [binary], case + "-" + label + "-" + variant + "-run")
                pair["run"] = run
                record("guest-ledger.json", guest_pairs)
                if run["exit"] != 0 or stderr:
                    raise Stop("STOP guest native/sanitizer failure: " + case +
                               "/" + variant)
                if variant == "observer":
                    pair["observed_stdout"] = stdout.decode(errors="replace")
                    pair["quantity_binding_association"] = (
                        "REQUIRES_ACTUAL_C_STDOUT_READING; not exit0 proof")
                elif stdout:
                    raise Stop("STOP unexpected raw guest stdout")
        result["observations"].append(
            {"case": case, "raw_native": "COMPLETED",
             "internal_quantities": "SEE_ACTUAL_OBSERVER_MAP_AND_STDOUT"})
    result["guest_pairs"] = len(guest_pairs)


def transport():
    fd = None
    method = "captured-child-stdout"
    try:
        parent = os.getppid()
        if pathlib.Path(f"/proc/{parent}/comm").read_text().strip() == "ctest":
            fd = os.open(f"/proc/{parent}/fd/1", os.O_WRONLY | os.O_APPEND)
            method = "direct-CTest-parent-stdout"
    except OSError:
        pass
    if fd is None:
        fd = sys.stdout.fileno()
    def emit(kind, value):
        line = "\nR1568_" + kind + " " + json.dumps(
            value, sort_keys=True, separators=(",", ":")) + "\n"
        data = line.encode()
        while data:
            written = os.write(fd, data)
            data = data[written:]
    try:
        emit("TRANSPORT", {"mode": args.mode, "method": method})
        total = 0
        for path in sorted(out.rglob("*")):
            if not path.is_file() or path.is_symlink():
                continue
            # Executables are excluded; exact source/output bytes are retained.
            if path.stat().st_mode & 0o111:
                continue
            data = path.read_bytes()
            name = str(path.relative_to(out))
            if len(data) > 524288 or total + len(data) > 6291456:
                emit("MISSING", {"mode": args.mode, "path": name,
                                 "bytes": len(data), "sha256": digest(data),
                                 "status": "BOUNDED_TRANSPORT_TRUNCATED"})
                continue
            total += len(data)
            encoded = base64.b64encode(data).decode()
            chunks = [encoded[i:i + 1800]
                      for i in range(0, len(encoded), 1800)]
            emit("FILE", {"mode": args.mode, "path": name, "bytes": len(data),
                          "sha256": digest(data), "chunks": len(chunks)})
            for index, chunk in enumerate(chunks):
                emit("CHUNK", {"mode": args.mode, "path": name,
                               "index": index, "data": chunk})
        emit("TRANSPORT_END", {"mode": args.mode, "bytes": total,
                               "semantic_status": result["status"]})
    except Exception as exc:
        print("R1568_TRANSPORT_UNAVAILABLE", repr(exc), flush=True)
    finally:
        if method == "direct-CTest-parent-stdout":
            os.close(fd)


exit_code = 0
try:
    metadata()
    {"public": public, "controls": controls, "native": native}[args.mode]()
    result["status"] = "COMPLETED_WITH_EXPLICIT_OBSERVATION_LIMITS"
except Exception as exc:
    exit_code = 1
    result["status"] = "STOP"
    result["first_failure"] = str(exc)
    save("traceback.txt", traceback.format_exc())
finally:
    record("result.json", result)
    record("guest-ledger.json", guest_pairs)
    transport()
sys.exit(exit_code)
