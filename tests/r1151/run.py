"""R1151 finite independent observations. Execute only in natural GitHub CI."""
from pathlib import Path
import base64
import datetime
import gzip
import hashlib
import io
import json
import os
import re
import subprocess
import sys
import tarfile
import traceback

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
OUT = Path(sys.argv[5]).resolve()
OUT.mkdir(parents=True, exist_ok=True)
CLI, PROBE, CC, SANITIZER = sys.argv[1:5]
COMMANDS = []
RESULTS = []
STATUS = "NOT_RUN"


class ApparatusError(RuntimeError):
    pass


class MaterialFinding(RuntimeError):
    pass


class RefusalStop(RuntimeError):
    pass


def require(condition, message, error=ApparatusError):
    if not condition:
        raise error(message)


def write_json(name, value):
    (OUT / name).write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")


def command(label, argv, cwd=None, timeout=25):
    prefix = "%03d-%s" % (len(COMMANDS), re.sub(r"[^A-Za-z0-9_.-]", "_", label))
    start = datetime.datetime.now(datetime.timezone.utc).isoformat()
    try:
        p = subprocess.run(argv, cwd=cwd or ROOT, stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE, timeout=timeout, check=False)
    except subprocess.TimeoutExpired as e:
        stdout, stderr = e.stdout or b"", e.stderr or b""
        (OUT / (prefix + ".stdout")).write_bytes(stdout)
        (OUT / (prefix + ".stderr")).write_bytes(stderr)
        COMMANDS.append({"label": label, "argv": [str(x) for x in argv],
                         "start": start, "rc": "TIMEOUT",
                         "stdout": prefix + ".stdout", "stderr": prefix + ".stderr"})
        raise MaterialFinding("finite execution timed out: " + label)
    (OUT / (prefix + ".stdout")).write_bytes(p.stdout)
    (OUT / (prefix + ".stderr")).write_bytes(p.stderr)
    COMMANDS.append({"label": label, "argv": [str(x) for x in argv],
                     "cwd": str(cwd or ROOT), "start": start,
                     "end": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                     "rc": p.returncode, "stdout": prefix + ".stdout",
                     "stderr": prefix + ".stderr"})
    return p


def clean_success(p, label):
    require(p.returncode == 0 and p.stderr == b"",
            "%s rc=%s stderr=%r" % (label, p.returncode, p.stderr[:1600]),
            MaterialFinding)


def one_sub(pattern, replacement, text, description):
    changed, count = re.subn(pattern, replacement, text, flags=re.MULTILINE)
    require(count == 1, "observer attachment %s: occurrences=%s" % (description, count))
    return changed


def function_span(text, fid):
    pattern = r"static (?:void|uint8_t) nl_fn_%d\([^;\n]*\)\n\{" % fid
    matches = list(re.finditer(pattern, text))
    require(len(matches) == 1, "function attachment %s" % fid)
    begin = matches[0].end()
    level = 1
    end = begin
    while end < len(text) and level:
        level += (text[end] == "{") - (text[end] == "}")
        end += 1
    require(level == 0, "unterminated generated function")
    return begin, end - 1, matches[0].group(0)


def instrument(text, rows):
    owned = rows[0]
    nodes = [r for r in rows if r.get("fragment") == "main" and "kind" in r]
    bindings = {r["name"]: r for r in nodes if r["binding"]}
    needed = {"original", "before", "result", "copied", "selected",
              "old", "updated", "sibling", "terminal_copy"}
    require(needed <= bindings.keys(), "public binding/source-name mapping missing")
    loops = [r for r in nodes if r["loop"]]
    calls = [r for r in nodes if r["call"] and r["name"] == "choose"]
    require(len(loops) == 1 and len(calls) == 1, "owned loop/predicate multiplicity")
    loop, call = loops[0], calls[0]
    require(loop["header_inductive"] and loop["continues"] == 1 and
            loop["breaks"] == 1, "public source-loop header/edge observation",
            MaterialFinding)
    lid = loop["id"]
    begin, end, _ = function_span(text, owned["entry_function"])
    body = text[begin:end]
    original = "nl_local_%d" % bindings["original"]["symbol"]
    binding_marker = r"^(\s*\(void\)%s;\n)" % original
    body = one_sub(
        binding_marker,
        lambda m: m.group(1) +
        "    r1151_first = &%s.f0;\n    r1151_second = &%s.f1;\n"
        '    r1151_record("bind", 0, NULL);\n' % (original, original),
        body, "live original")
    for key in sorted(needed - {"original"}):
        local = "nl_local_%d" % bindings[key]["symbol"]
        address = "NULL"
        if key in {"before", "updated", "sibling"}:
            read = re.findall(r"\b%s = (nl_local_\d+\.f[01]);" % local, body)
            require(len(read) == 1, "actual generated field read " + key)
            address = "&" + read[0]
        body = one_sub(
            r"^(\s*\(void\)%s;\n)" % local,
            lambda m, key=key, local=local, address=address:
            m.group(1) + '    r1151_record("%s", (size_t)%s, %s);\n'
            % (key, local, address),
            body, key)
    body = one_sub(
        r"^(\s*const size_t nl_loop_phase_%d = nl_loop_carry_%d;\n)" % (lid, lid),
        lambda m: m.group(1) +
        '        r1151_record("phase", nl_loop_phase_%d, NULL);\n' % lid,
        body, "actual loop header")
    body = one_sub(
        r"^(\s*nl_loop_carry_%d = [^\n]+;\n)" % lid,
        lambda m: m.group(1) +
        '            r1151_record("continue", nl_loop_carry_%d, NULL);\n' % lid,
        body, "actual continue")
    body = one_sub(
        r"^(\s*nl_local_%d = nl_loop_phase_%d;\n)" %
        (bindings["result"]["symbol"], lid),
        lambda m: m.group(1) +
        '        r1151_record("break", nl_loop_phase_%d, NULL);\n' % lid,
        body, "actual break")
    call_value = "nl_call_%d" % call["id"]
    body = one_sub(
        r"^(\s*\(void\)%s;\n)" % call_value,
        lambda m: m.group(1) +
        '    r1151_record("callee_return", (size_t)%s, NULL);\n' % call_value,
        body, "existing call-result temporary")
    writer = re.findall(r"uint8_t \*const (nl_local_\d+) = &(nl_local_\d+\.f[01]);",
                        body)
    require(len(writer) == 1, "actual scoped write reference")
    writer_name, writer_target = writer[0]
    body = one_sub(
        r"^(\s*uint8_t \*const %s = &%s;\n)" %
        (writer_name, re.escape(writer_target)),
        lambda m: m.group(1) +
        '        r1151_record("writer", (size_t)*%s, %s);\n' %
        (writer_name, writer_name), body, "writer address")
    old = re.findall(r"const uint8_t (nl_replace_old_\d+) = \*%s;" % writer_name,
                     body)
    require(len(old) == 1, "actual replace old load")
    body = one_sub(
        r"^(\s*const uint8_t %s = \*%s;\n)" % (old[0], writer_name),
        lambda m: m.group(1) +
        '        r1151_record("replace_old", (size_t)%s, %s);\n' %
        (old[0], writer_name), body, "old load")
    body = one_sub(
        r"^(\s*\*%s = [^\n]+;\n)" % writer_name,
        lambda m: m.group(1) +
        '        r1151_record("store", (size_t)*%s, %s);\n' %
        (writer_name, writer_name), body, "actual post-store value")
    text = text[:begin] + body + text[end:]
    begin, end, signature = function_span(text, call["function"])
    formal = re.findall(r"size_t (nl_local_\d+)", signature)
    require(len(formal) == 1, "actual predicate formal")
    text = text[:begin] + '\n    r1151_record("callee_actual", (size_t)%s, NULL);' \
        % formal[0] + text[begin:]
    return '#include "observer.h"\n' + text, bindings, nodes


def observed(output, case):
    records = []
    for line in output.decode().splitlines():
        parts = line.split("\t")
        require(len(parts) == 8 and parts[0] == "R1151",
                "malformed passive observation " + repr(line))
        records.append({"tag": parts[1], "value": int(parts[2]),
                        "first": parts[3], "second": parts[4],
                        "upper": int(parts[5]), "lower": int(parts[6]),
                        "address": parts[7]})
    require(records and records[0]["tag"] == "bind", "missing live original",
            MaterialFinding)
    first, second = records[0]["first"], records[0]["second"]
    require(first != second and first != "(nil)" and second != "(nil)",
            "invalid original field addresses", MaterialFinding)
    target = first if case["target_index"] == 0 else second
    sibling = second if case["target_index"] == 0 else first
    tags = {}
    stored = False
    for row in records:
        tags.setdefault(row["tag"], []).append(row)
        require(row["first"] == first and row["second"] == second,
                "original object identity changed", MaterialFinding)
        if row["tag"] == "store":
            require(not stored, "more than one actual store", MaterialFinding)
            stored = True
        expected = [case["upper"], case["lower"]]
        if stored:
            expected[case["target_index"]] = case["expected_selected"]
        require([row["upper"], row["lower"]] == expected,
                "original T fields disagree at " + row["tag"], MaterialFinding)
    single = {
        "bind": 0, "before": case["expected_old"],
        "break": case["expected_result"], "result": case["expected_result"],
        "copied": case["expected_result"], "callee_actual": case["expected_result"],
        "callee_return": case["expected_selected"],
        "selected": case["expected_selected"], "writer": case["expected_old"],
        "replace_old": case["expected_old"], "store": case["expected_selected"],
        "old": case["expected_old"], "updated": case["expected_selected"],
        "sibling": case["expected_sibling"], "terminal_copy": case["expected_return"],
    }
    require(set(tags) == set(single) | {"phase"} |
            ({"continue"} if case["expected_backedges"] else set()),
            "unexpected or missing observation tags", MaterialFinding)
    for tag, value in single.items():
        require(len(tags.get(tag, [])) == 1 and tags[tag][0]["value"] == value,
                "%s value/count differs from frozen oracle" % tag, MaterialFinding)
    require([r["value"] for r in tags["phase"]] == case["path"],
            "actual loop header path differs", MaterialFinding)
    require([r["value"] for r in tags.get("continue", [])] == case["path"][1:],
            "actual carried path differs", MaterialFinding)
    for tag in ["before", "updated", "writer", "replace_old", "store"]:
        require(tags[tag][0]["address"] == target,
                tag + " did not use original target address", MaterialFinding)
    require(tags["sibling"][0]["address"] == sibling,
            "sibling did not use original sibling address", MaterialFinding)
    require(stored, "no actual store", MaterialFinding)
    return records


def publish_archive():
    write_json("commands.json", COMMANDS)
    write_json("results.json", {"status": STATUS, "cases": RESULTS,
                               "manual_reruns": 0})
    members = {}
    for path in sorted(OUT.rglob("*")):
        if path.is_file() and path.name not in {"evidence.tar.gz", "manifest.json"}:
            members[str(path.relative_to(OUT))] = hashlib.sha256(path.read_bytes()).hexdigest()
    write_json("manifest.json", members)
    archive = io.BytesIO()
    with tarfile.open(fileobj=archive, mode="w") as tar:
        for name in sorted(list(members) + ["manifest.json"]):
            raw = (OUT / name).read_bytes()
            info = tarfile.TarInfo(name)
            info.size = len(raw)
            info.mode = 0o644
            info.mtime = 0
            tar.addfile(info, io.BytesIO(raw))
    data = gzip.compress(archive.getvalue(), mtime=0)
    (OUT / "evidence.tar.gz").write_bytes(data)
    encoded = base64.b64encode(data).decode()
    identity = "%s-%s-%s-%s" % (os.environ.get("GITHUB_RUN_ID", "none"),
                               os.environ.get("GITHUB_RUN_ATTEMPT", "none"),
                               Path(CC).name, SANITIZER)
    lines = ["R1151_ARCHIVE_BEGIN " + identity + " " +
             hashlib.sha256(data).hexdigest() + " " + str(len(data))]
    chunks = [encoded[i:i + 2400] for i in range(0, len(encoded), 2400)]
    lines += ["R1151_ARCHIVE_CHUNK %s %d/%d %s" % (identity, i + 1, len(chunks), value)
              for i, value in enumerate(chunks)]
    lines += ["R1151_ARCHIVE_END " + identity,
              "R1151_STATUS " + STATUS + " " + identity]
    report = "\n".join(lines) + "\n"
    # CTest captures normal successful child stdout. Its own stdout is the
    # existing Actions log. Preserve the finite archive there without changing
    # workflows, rerunning a semantic experiment, or obtaining network tokens.
    try:
        with open("/proc/%d/fd/1" % os.getppid(), "w") as runner:
            runner.write(report)
            runner.flush()
    except OSError as error:
        print(report, end="", flush=True)
        raise ApparatusError("cannot preserve success evidence in runner log: " + str(error))


def main():
    require(os.environ.get("GITHUB_ACTIONS") == "true",
            "R1151 execution is authorized only in GitHub Actions")
    environment = {name: os.environ.get(name) for name in
                   ["GITHUB_RUN_ID", "GITHUB_RUN_ATTEMPT", "GITHUB_SHA",
                    "GITHUB_REF", "GITHUB_EVENT_NAME", "GITHUB_WORKFLOW",
                    "GITHUB_JOB", "RUNNER_OS", "ImageOS", "ImageVersion",
                    "ASAN_OPTIONS", "UBSAN_OPTIONS"]}
    environment.update({"compiler": CC, "sanitizer": SANITIZER})
    for key, args in [
        ("checkout", ["git", "rev-parse", "HEAD"]),
        ("tree", ["git", "rev-parse", "HEAD^{tree}"]),
        ("parents", ["git", "show", "-s", "--format=%P", "HEAD"]),
        ("compiler_version", [CC, "--version"]),
        ("cli_version", [CLI, "--version"]),
        ("tracked_changes", ["git", "diff", "--name-only"]),
        ("ctest_inventory", ["ctest", "--test-dir", str(Path(CLI).parent), "-N"]),
    ]:
        result = command("environment-" + key, args)
        require(result.returncode == 0, "environment capture failed " + key)
        environment[key] = result.stdout.decode()
    require(not environment["tracked_changes"], "tracked source changed in runner")
    names = re.findall(r"Test\s+#\d+:\s+(\S+)", environment["ctest_inventory"])
    require(names.count("r1151_original_t_write.integration") == 1 and
            len(names) == len(set(names)), "unique R CTest registration")
    environment["ctest_count"] = len(names)
    write_json("environment.json", environment)
    frozen = json.loads((HERE / "source-manifest.json").read_text())
    for row in frozen:
        data = (ROOT / row["path"]).read_bytes()
        require(len(data) == row["bytes"] and
                hashlib.sha256(data).hexdigest() == row["sha256"],
                "source freeze mismatch " + row["path"])
    oracle = json.loads((HERE / "v3/oracle-v3.json").read_text())
    (OUT / "oracle-v3.json").write_bytes((HERE / "v3/oracle-v3.json").read_bytes())
    (OUT / "observer.h").write_bytes((HERE / "observer.h").read_bytes())
    for case in oracle["negative"]:
        filename = case["id"] + ".nl"
        source = HERE / filename
        (OUT / filename).write_bytes(source.read_bytes())
        p = command(case["id"] + "-CHECK-only", [CLI, "--check", str(source)])
        require(p.returncode > 0, "negative accepted/crashed: " + case["id"],
                MaterialFinding)
        require(b"Sanitizer" not in p.stderr and b"runtime error:" not in p.stderr,
                "negative host sanitizer report", MaterialFinding)
        RESULTS.append({"id": case["id"], "status": "REJECTED_AT_FIRST_CLI_LAYER",
                        "class": case["class"], "rc": p.returncode,
                        "diagnostic": p.stderr.decode(errors="replace"),
                        "owned": "NOT_RUN", "c17": "NOT_RUN", "native": "NOT_RUN"})
    for case in oracle["positive"]:
        cid = case["id"]
        source = HERE / "v3" / (cid + ".nl")
        (OUT / (cid + ".nl")).write_bytes(source.read_bytes())
        row = {"id": cid, "status": "STARTED", "check": "NOT_RUN",
               "owned": "NOT_RUN", "c17": "NOT_RUN", "native": []}
        RESULTS.append(row)
        p = command(cid + "-CHECK", [CLI, "--check", str(source)])
        if p.returncode != 0:
            row.update({"status": "CHECK_REFUSAL_STOP", "check": p.returncode,
                        "diagnostic": p.stderr.decode(errors="replace")})
            require(p.returncode > 0 and b"Sanitizer" not in p.stderr and
                    b"runtime error:" not in p.stderr, "host crash", MaterialFinding)
            raise RefusalStop("frozen canonical-legal CHECK refusal: " + cid)
        clean_success(p, cid + " CHECK")
        require(p.stdout == b"", "unexpected CHECK output")
        row["check"] = "PASS"
        p = command(cid + "-owned-public", [PROBE, str(source)])
        clean_success(p, cid + " owned")
        rows = [json.loads(line) for line in p.stdout.decode().splitlines()]
        require(rows and rows[0].get("authentic_owned_entry"),
                "missing authentic owned root")
        row["owned"] = "PUBLIC_SOURCE_ISSUED_VIEW"
        generated = OUT / (cid + "-guest.c")
        p = command(cid + "-emit", [CLI, "--emit-c17-to", str(generated), str(source)])
        if p.returncode != 0:
            row.update({"status": "C17_PROFILE_REFUSAL_STOP", "c17": p.returncode,
                        "diagnostic": p.stderr.decode(errors="replace")})
            require(p.returncode > 0 and b"Sanitizer" not in p.stderr and
                    b"runtime error:" not in p.stderr, "emitter crash", MaterialFinding)
            raise RefusalStop("ordinary C17 finite-profile refusal: " + cid)
        clean_success(p, cid + " emit")
        require(p.stdout == b"" and generated.is_file(), "missing generated C")
        row["c17"] = "ACCEPTED_PRIVATE_CONSUMER"
        original_bytes = generated.read_bytes()
        code, bindings, nodes = instrument(original_bytes.decode(), rows)
        observed_c = OUT / (cid + "-observer.c")
        observed_c.write_text(code)
        write_json(cid + "-owned-mapping.json", {"bindings": bindings, "nodes": nodes})
        for optimization in ["-O0", "-O2"]:
            flags = ["-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror",
                     optimization]
            if SANITIZER != "none":
                flags += ["-fsanitize=" + SANITIZER, "-fno-sanitize-recover=all",
                          "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
            for kind, cfile in [("guest", generated), ("observer", observed_c)]:
                label = cid + "-" + kind + "-" + optimization[1:]
                exe = OUT / (label + ".exe")
                p = command(label + "-compile", [CC, *flags, str(cfile), "-o", str(exe)])
                clean_success(p, label + " strict compile")
                p = command(label + "-run", [str(exe)])
                clean_success(p, label + " native")
                if kind == "guest":
                    require(p.stdout == b"", "unmodified guest stdout changed",
                            MaterialFinding)
                else:
                    records = observed(p.stdout, case)
                    write_json(label + "-observations.json", records)
                row["native"].append({"kind": kind, "optimization": optimization,
                                      "sanitizer": SANITIZER, "status": "PASS"})
                exe.unlink()
        require(generated.read_bytes() == original_bytes,
                "unmodified guest changed")
        row["status"] = "PASS"
        row["guest_sha256"] = hashlib.sha256(original_bytes).hexdigest()
        row["observer_sha256"] = hashlib.sha256(observed_c.read_bytes()).hexdigest()
    require(len(RESULTS) == 12, "cohort accounting")
    return "PASS"


if __name__ == "__main__":
    exit_code = 0
    try:
        STATUS = main()
    except (ApparatusError, MaterialFinding, RefusalStop) as error:
        STATUS = type(error).__name__ + ": " + str(error)
        (OUT / "failure.txt").write_text(traceback.format_exc())
        exit_code = 1
    except Exception as error:
        STATUS = "UNCLASSIFIED_STOP: " + repr(error)
        (OUT / "failure.txt").write_text(traceback.format_exc())
        exit_code = 1
    try:
        publish_archive()
    except Exception as error:
        print("R1151 evidence preservation failed", repr(error), flush=True)
        exit_code = 1
    print("R1151", STATUS, "case records", len(RESULTS), flush=True)
    raise SystemExit(exit_code)
