"""R1551 fixed-input calibrated Actions-only measurement apparatus."""

import base64
import difflib
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import traceback


MODE, CLI_TEXT, PROBE_TEXT, DATA_TEXT, WORK_TEXT, SANITIZER, FRONTEND = sys.argv[1:]
DATA = Path(DATA_TEXT).resolve()
WORK = Path(WORK_TEXT).resolve()
ROOT = DATA.parents[1]
CLI = Path(CLI_TEXT).resolve()
PROBE = Path(PROBE_TEXT).resolve()
MANIFEST = json.loads((DATA / "manifest.json").read_text(encoding="utf-8"))
OUT = WORK / MODE
FILES = []
COMMANDS = []
GUEST_ATTEMPTS = 0
GUEST_COMPLETED = 0
PASSED = False


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def save(name, data):
    path = OUT / name
    require(".." not in Path(name).parts and not Path(name).is_absolute(),
            "evidence path outside R allowlist")
    path.parent.mkdir(parents=True, exist_ok=True)
    if isinstance(data, str):
        data = data.encode("utf-8")
    path.write_bytes(data)
    if name not in FILES:
        FILES.append(name)
    return path


def save_json(name, value):
    return save(name, json.dumps(value, sort_keys=True, indent=2) + "\n")


def track(name):
    path = OUT / name
    require(path.is_file() and not path.is_symlink(), "missing R evidence file")
    if name not in FILES:
        FILES.append(name)
    return path


def run(command, label, expected=None, guest=False, archive=True):
    global GUEST_ATTEMPTS, GUEST_COMPLETED
    command = [str(x) for x in command]
    if guest:
        require(GUEST_ATTEMPTS < (2 if FRONTEND == "ON" else 4),
                "per-case guest bound would be exceeded")
        GUEST_ATTEMPTS += 1
    proc = subprocess.run(command, cwd=ROOT, capture_output=True, timeout=45)
    if guest:
        GUEST_COMPLETED += 1
    if archive:
        save(label + ".stdout", proc.stdout)
        save(label + ".stderr", proc.stderr)
    COMMANDS.append({"command": command, "label": label,
                     "returncode": proc.returncode, "guest": guest})
    if expected is not None:
        require(proc.returncode == expected,
                f"{label}: expected exit {expected}, actual {proc.returncode}")
    return proc


def check_frozen():
    actual = {}
    for name, item in MANIFEST["frozen"].items():
        raw = (DATA / "frozen" / name).read_bytes()
        actual[name] = {"bytes": len(raw), "sha256": digest(raw)}
        require(actual[name] == item, "frozen identity mismatch: " + name)
    save_json("frozen-identities.json", actual)


def identity():
    require(os.environ.get("GITHUB_ACTIONS") == "true",
            "R1551 may execute only in authorized GitHub Actions")
    require(os.environ.get("GITHUB_EVENT_NAME") == "pull_request",
            "semantic/native execution requires the natural R draft PR")
    require(os.environ.get("GITHUB_RUN_ATTEMPT") == "1",
            "manual rerun is not authorized")
    event = json.loads(Path(os.environ["GITHUB_EVENT_PATH"]).read_text())
    pr = event["pull_request"]
    require(pr["base"]["sha"] == MANIFEST["candidate"],
            "candidate base drift in actual PR event")
    require(pr["base"]["ref"] == MANIFEST["pr_base"], "wrong PR base")
    require(pr["head"]["ref"] == MANIFEST["branch"], "wrong R branch")
    require(pr["draft"] is True, "R PR must remain draft")
    # Exact-object metadata fetch accommodates the unchanged shallow checkout.
    run(["git", "fetch", "--no-tags", "--depth=8", "origin",
         MANIFEST["candidate"], pr["head"]["sha"]], "identity-object-fetch", 0)
    head = run(["git", "rev-parse", "HEAD"], "checkout-head", 0).stdout.strip().decode()
    tree = run(["git", "rev-parse", "HEAD^{tree}"], "checkout-tree", 0).stdout.strip().decode()
    rtree = run(["git", "rev-parse", pr["head"]["sha"] + "^{tree}"],
                "r-head-tree", 0).stdout.strip().decode()
    require(tree == rtree, "checkout tree differs from immutable R HEAD tree")
    run(["git", "merge-base", "--is-ancestor", MANIFEST["candidate"],
         pr["head"]["sha"]], "candidate-ancestry", 0)
    changed = run(["git", "diff", "--name-only", MANIFEST["candidate"],
                   pr["head"]["sha"]], "r-path-scope", 0).stdout.decode().splitlines()
    require(changed and all(x == "CMakeLists.txt" or x.startswith("tests/r1551/")
                            for x in changed), "changes outside released R paths")
    expected_cmake = (run(["git", "show", MANIFEST["candidate"] + ":CMakeLists.txt"],
                          "candidate-cmake", 0, archive=False).stdout)
    anchor = b"    find_package(Python3 3.12 REQUIRED COMPONENTS Interpreter)\n"
    require(expected_cmake.count(anchor) == 1, "CMake anchor not unique")
    expected_cmake = expected_cmake.replace(
        anchor, anchor + b"    include(tests/r1551/CMakeLists.txt)\n")
    require((ROOT / "CMakeLists.txt").read_bytes() == expected_cmake,
            "root CMake differs beyond one minimal R include")
    save_json("root-cmake-identity.json",
              {"bytes": len(expected_cmake), "sha256": digest(expected_cmake),
               "difference": "one tests/r1551 include only"})
    cache = {}
    for line in (WORK.parent / "CMakeCache.txt").read_text().splitlines():
        match = re.match(r"(CMAKE_BUILD_TYPE|CMAKE_C_COMPILER|NEWLANG_SANITIZER|"
                         r"NEWLANG_FRONTEND_ONLY):[^=]+=(.*)", line)
        if match:
            cache[match.group(1)] = match.group(2)
    require(cache["NEWLANG_SANITIZER"] == SANITIZER, "host sanitizer mismatch")
    require(cache["NEWLANG_FRONTEND_ONLY"] == FRONTEND, "frontend mode mismatch")
    save_json("execution-identity.json",
              {"candidate": MANIFEST["candidate"], "base": MANIFEST["base"],
               "r_head": pr["head"]["sha"], "checkout_head": head,
               "checkout_tree": tree, "r_tree": rtree,
               "branch": pr["head"]["ref"], "base_ref": pr["base"]["ref"],
               "pr": pr["number"], "run_id": os.environ["GITHUB_RUN_ID"],
               "attempt": os.environ["GITHUB_RUN_ATTEMPT"], "cache": cache,
               "evidence_label": MANIFEST["evidence_label"]})


def json_rows(raw):
    return [json.loads(line) for line in raw.decode("utf-8").splitlines() if line]


def one(rows, kind, **terms):
    found = [row for row in rows if row.get("record") == kind and
             all(row.get(k) == v for k, v in terms.items())]
    require(len(found) == 1, f"expected one {kind} {terms}, found {len(found)}")
    return found[0]


def scalar(value, expected, usize_type):
    require(value["type"] == usize_type and value["visible"] and value["known"],
            "quantity lacks genuine known usize package")
    require(value["dependencies"] == 0, "unexpected scalar dependency")
    require(value["number"] == expected,
            f"semantic quantity mismatch: {value['number']} != {expected}")


def result_of(node):
    require(node["result_count"] == 1 and len(node["results"]) == 1,
            "expected one genuine quantity result")
    return node["results"][0]


def items(nodes, root):
    found = []
    current = root["first_item"]
    while current:
        require(current in nodes and len(found) <= root["item_count"],
                "invalid/cyclic checked item linkage")
        item = nodes[current]
        found.append(item)
        current = item["next_item"]
    require(len(found) == root["item_count"], "checked item count mismatch")
    return found


def inspect_positive(label, spec, rows):
    target = one(rows, "target")
    require(target["size_t_bits"] in (32, 64), "target width outside frozen scope")
    require(target["size_max"] == (1 << target["size_t_bits"]) - 1,
            "unexpected target range")
    for stage in ("parse", "definition", "entry"):
        require(one(rows, "stage", stage=stage)["status"] == 0,
                label + ": unexpected first failure at " + stage)
    require(one(rows, "owned")["proof_class"] == 1,
            "ordinary input lacks machine-proved classification")
    require(one(rows, "complete")["entry_issued"], "no complete owned entry")
    graphs = {r["graph"]: r for r in rows if r["record"] == "graph"}
    require(len(graphs) == 5, "expected entry, main and exactly three helper bodies")
    nodes = {}
    for gid in graphs:
        graph = graphs[gid]
        nodes[gid] = {r["id"]: r for r in rows
                      if r["record"] == "node" and r["graph"] == gid}
        require(len(nodes[gid]) == graph["count"], "incomplete public node dump")
        require(graph["same_context"], "ordinary call/body world mismatch")
        if graph["parent"]:
            require(graph["source_in_original"],
                    "retained checked body source differs from frozen unit")
    types = one(rows, "types")
    usize_type = types["usize"]
    entry_graph = next(g for g in graphs.values() if g["parent"] == 0)
    entry = nodes[entry_graph["graph"]][entry_graph["root"]]
    require(entry["kind"] == "CALL" and entry["name"] == "main" and
            entry["body_backed"] and entry["argument_count"] == 0,
            "not the actual ordinary main entry")
    main_graph = next(g for g in graphs.values()
                      if g["parent"] == entry_graph["graph"] and
                      g["parent_call"] == entry["id"])
    main_nodes = nodes[main_graph["graph"]]
    main_root = main_nodes[main_graph["root"]]
    require(main_root["kind"] == "BLOCK" and not main_root["terminates"],
            "unit main normal edge mismatch")
    require(main_nodes[main_root["tail"]]["kind"] == "UNIT",
            "unit main must retain its actual unit tail")
    main_items = items(main_nodes, main_root)
    require([n["name"] for n in main_items] ==
            [b[0] for b in spec["bindings"]], "main source binding order mismatch")
    bindings = {}
    calls = []
    for node, (name, kind, number) in zip(main_items, spec["bindings"]):
        require(node["kind"] == "BINDING" and node["symbol"] != 0,
                "main item is not a genuine binding")
        init = main_nodes[node["initializer"]]
        require(init["kind"] == kind, "initializer kind mismatch: " + name)
        scalar(result_of(init), number, usize_type)
        bindings[name] = node
        if kind == "IDENTIFIER":
            origin = bindings[spec["copies"][name]]
            require(init["symbol"] == origin["symbol"] and init["use"] == 1,
                    "caller Copy source/availability linkage mismatch")
        elif kind in ("ADDITION", "SUBTRACTION"):
            recipe = spec["arithmetic"][name]
            left = main_nodes[init["first_argument"]]
            right = main_nodes[left["next_argument"]]
            require(left["kind"] == "IDENTIFIER" and left["use"] == 1 and
                    left["symbol"] == bindings[recipe[1]]["symbol"],
                    "Known arithmetic source identity mismatch")
            scalar(result_of(right), recipe[2], usize_type)
            receipt = one(rows, kind.lower(), graph=main_graph["graph"],
                          node=init["id"])
            require(receipt["present"] and receipt["valid"] and
                    receipt["origin_same"] and receipt["number"] == number and
                    receipt["result"] == result_of(init)["value"],
                    "arithmetic receipt/owned result mismatch")
        elif kind == "CALL":
            calls.append(init)
    require(len(calls) == 3, "main must execute exactly three helper calls")
    require(len({c["function"] for c in calls}) == 1, "helper identity changed")
    helper_maps = []
    formal_symbols = []
    parameter_notes = []
    for index, (call, expectation) in enumerate(zip(calls, spec["calls"])):
        actual, quotient = expectation
        require(call["body_backed"] and call["name"] == spec["helper"] and
                call["argument_count"] == 1, "wrong actual helper call")
        argument = main_nodes[call["first_argument"]]
        scalar(result_of(argument), actual, usize_type)
        actual_name = spec["call_actuals"][index]
        if actual_name:
            require(argument["kind"] == "IDENTIFIER" and argument["use"] == 1 and
                    argument["symbol"] == bindings[actual_name]["symbol"],
                    "call did not reuse the actual earlier binding")
        else:
            require(argument["kind"] == "USIZE_LITERAL",
                    "first call did not use the source literal")
        scalar(result_of(call), quotient, usize_type)
        child = next(g for g in graphs.values()
                     if g["parent"] == main_graph["graph"] and
                     g["parent_call"] == call["id"])
        child_nodes = nodes[child["graph"]]
        root = child_nodes[child["root"]]
        child_items = items(child_nodes, root)
        helper_bindings = [n for n in child_items if n["kind"] == "BINDING"]
        require([n["name"] for n in helper_bindings] == spec["helper_bindings"],
                "helper quotient/Copy binding order mismatch")
        helper_by_name = {n["name"]: n for n in helper_bindings}
        division = child_nodes[helper_bindings[0]["initializer"]]
        require(division["kind"] == "DIVISION", "missing single helper division")
        require(sum(n["kind"] == "DIVISION" for n in child_nodes.values()) == 1,
                "helper division count changed")
        receipt = one(rows, "division", graph=child["graph"], node=division["id"])
        require(receipt["present"] and receipt["valid"] and receipt["copy_valid"]
                and receipt["origin_same"], "division issuer validation failed")
        require(receipt["numerator_known"] and receipt["quotient_known"] and
                receipt["numerator"] == actual and
                receipt["divisor"] == spec["divisor"] and
                receipt["quotient"] == quotient and
                receipt["target_max"] == target["size_max"],
                "division operand/result/target mismatch")
        formal = child_nodes[receipt["left_node"]]
        require(formal["kind"] == "IDENTIFIER" and formal["use"] == 1 and
                formal["name"] == spec["formal"] and
                formal["symbol"] == receipt["formal_symbol"],
                "actual formal identity/Copy use mismatch")
        formal_symbols.append(formal["symbol"])
        scalar(result_of(division), quotient, usize_type)
        require(receipt["result"] == result_of(division)["value"],
                "division result-package mismatch")
        for index_copy in range(1, len(helper_bindings)):
            binding = helper_bindings[index_copy]
            init = child_nodes[binding["initializer"]]
            prior = helper_bindings[index_copy - 1]
            require(init["kind"] == "IDENTIFIER" and init["use"] == 1 and
                    init["symbol"] == prior["symbol"],
                    "helper Copy must preserve its source binding")
            scalar(result_of(init), quotient, usize_type)
        returned_symbol = helper_by_name[spec["return_name"]]["symbol"]
        if spec["control"] == "tail":
            require(not root["terminates"] and root["tail"],
                    "normal tail became terminating")
            expression = child_nodes[root["tail"]]
            responsibility = result_of(root)
            require(expression["kind"] == "IDENTIFIER" and
                    expression["symbol"] == returned_symbol and
                    expression["use"] == 1, "wrong retained normal tail")
            require(result_of(expression)["value"] == responsibility["value"],
                    "tail/block result custody mismatch")
        else:
            returns = [n for n in child_items if n["kind"] == "RETURN"]
            require(len(returns) == 1 and child_items[-1] is returns[0] and
                    returns[0]["terminates"] and not root["tail"],
                    "final RETURN/control edge mismatch")
            ret = returns[0]
            responsibility = ret["returned"]
            matches = [n for n in child_nodes.values()
                       if n["kind"] == "IDENTIFIER" and
                       n["symbol"] == returned_symbol and n["use"] == 1 and
                       n["span"][0] >= ret["span"][0] and
                       n["span"][1] <= ret["span"][1] and
                       n["result_count"] == 1 and
                       n["results"][0]["value"] == responsibility["value"]]
            require(len(matches) == 1, "RETURN lost its original quotient Copy")
        scalar(responsibility, quotient, usize_type)
        require(responsibility["value"] == result_of(call)["value"],
                "owned helper result/caller custody mismatch")
        parameters = one(rows, "parameters", graph=main_graph["graph"],
                         node=call["id"])
        if parameters["present"]:
            require(parameters["validation"] == 0,
                    "exposed actual/formal parameter receipt failed validation")
        parameter_notes.append(parameters)
        helper_maps.append({"graph": child["graph"],
                            "formal_symbol": formal["symbol"],
                            "bindings": {n["name"]: n["symbol"]
                                         for n in helper_bindings},
                            "return_symbol": returned_symbol})
    require(len(set(formal_symbols)) == 3, "fresh formal symbols were reused")
    mapping = {"main_function": entry["function"],
               "helper_function": calls[0]["function"],
               "main_bindings": {n["name"]: n["symbol"] for n in main_items},
               "helper_instances": helper_maps,
               "parameter_receipts": parameter_notes,
               "target": target,
               "limits": "Public graph/actual receipts plus ordinary emitter; "
                         "no claim of a general public whole-program theorem."}
    save_json(label + "-map.json", mapping)
    return mapping


def proof():
    for label, spec in MANIFEST["positives"].items():
        source = DATA / "frozen" / spec["file"]
        observed = run([PROBE, source], label + "-probe", 0)
        require(not observed.stderr, "positive public probe stderr")
        rows = json_rows(observed.stdout)
        inspect_positive(label, spec, rows)
        cli = run([CLI, "--check", source], label + "-cli-check", 0)
        require(not cli.stdout and not cli.stderr, "positive CLI CHECK diagnostics")
    for index, spec in enumerate(MANIFEST["negatives"], 1):
        source = DATA / "frozen" / spec["file"]
        prefix = "negative-" + str(index)
        observed = run([PROBE, source], prefix + "-probe", 0)
        require(not observed.stderr, "negative probe apparatus stderr")
        rows = json_rows(observed.stdout)
        require(one(rows, "target")["size_t_bits"] in (32, 64),
                "negative target width outside scope")
        require(one(rows, "stage", stage="parse")["status"] == 0,
                "semantic negative was rejected before complete source parsing")
        refused = one(rows, "stage", stage="definition")
        require(refused["status"] == 1 and refused["code"] == spec["code"],
                "negative has an unexplained first semantic refusal")
        require(refused["snapshot_unchanged"] and not refused["entry_issued"],
                "negative changed public state or issued an entry")
        require(not any(r["record"] in ("owned", "graph", "complete") for r in rows),
                "negative reached successful public-owned measurement")
        require(0 <= refused["span"][0] <= refused["span"][1] <=
                len(source.read_bytes()), "negative diagnostic span outside source")
        for suffix, command in (
            ("check", [CLI, "--check", source]),
            ("existing", [CLI, "--emit-c17-to", OUT / (prefix + ".existing.c"), source]),
            ("fresh", [CLI, "--emit-c17-to", OUT / (prefix + ".fresh.c"), source]),
        ):
            existing = OUT / (prefix + ".existing.c")
            fresh = OUT / (prefix + ".fresh.c")
            sentinel = b"R1551 output no-clobber sentinel\n"
            if suffix == "existing":
                save(existing.name, sentinel)
            require(not fresh.exists(), "fresh negative output unexpectedly exists")
            proc = run(command, prefix + "-cli-" + suffix, 3)
            require(not proc.stdout, "negative CLI emitted stdout")
            text = proc.stderr.decode("utf-8")
            require(re.search(r"\b" + re.escape(spec["code"]) + r"\b", text),
                    "negative CLI did not report the intended first refusal")
            require("V0-ENTRY" not in text and "unknown function" not in text.lower(),
                    "missing-main-only refusal cannot pass")
            if suffix == "existing":
                require(existing.read_bytes() == sentinel, "negative output clobbered")
            if suffix == "fresh":
                require(not fresh.exists(), "negative emitted a fresh C file")
            require(not list(OUT.glob("*.tmp.*")), "negative left a staging file")
    save_json("source-gate.json", {"passed": True, "positive_units": 2,
                                  "negative_units": 4,
                                  "newlang_native_guests": 0,
                                  "r_specific_oom": "NOT_RUN"})


def observation_copy(raw, mapping, spec):
    text = raw.decode("ascii")
    main_id = mapping["main_function"]
    helper_id = mapping["helper_function"]
    representative = mapping["helper_instances"][0]
    formal = representative["formal_symbol"]
    helper_header = f"static size_t nl_qty_fn_{helper_id}(size_t nl_local_{formal})\n{{\n"
    main_header = f"static void nl_qty_fn_{main_id}(void)\n{{\n"
    require(text.count(helper_header) == 1 and text.count(main_header) == 1,
            "emitted functions do not match actual public identities")
    helper_start = text.index(helper_header) + len(helper_header)
    helper_end = text.index("\n}\n", helper_start)
    main_start = text.index(main_header) + len(main_header)
    main_end = text.index("\n}\n", main_start)
    helper_body = text[helper_start:helper_end]
    main_body = text[main_start:main_end]
    return_line = f"    return nl_local_{representative['return_symbol']};"
    require(helper_body.count(return_line) == 1, "generated return changed source identity")
    require(main_body.count("    return;") == 1, "generated unit return missing")
    require(len(re.findall(rf"\bnl_qty_fn_{helper_id}\(", main_body)) == 3,
            "generated caller has wrong helper invocation count")
    require(text.count(f"    nl_qty_fn_{main_id}();") == 1,
            "ordinary C main does not call the genuine unit caller once")
    for name, symbol in mapping["main_bindings"].items():
        require(main_body.count(f"size_t nl_local_{symbol} = ") == 1,
                "missing actual caller local: " + name)
    for name, symbol in representative["bindings"].items():
        require(helper_body.count(f"size_t nl_local_{symbol} = ") == 1,
                "missing actual helper local: " + name)
    helper_names = [spec["formal"]] + spec["helper_bindings"]
    helper_symbols = [formal] + [representative["bindings"][name]
                                  for name in spec["helper_bindings"]]
    helper_format = "R1551_H " + " ".join(name + "=%zu" for name in helper_names)
    helper_add = ('    fprintf(stdout, "' + helper_format + '\\n", ' +
                  ", ".join("nl_local_" + str(s) for s in helper_symbols) + ');\n')
    main_names = [x[0] for x in spec["bindings"]]
    main_format = "R1551_M " + " ".join(name + "=%zu" for name in main_names)
    main_format += " width=%zu max=%zu"
    main_add = ('    fprintf(stdout, "' + main_format + '\\n", ' +
                ", ".join("nl_local_" + str(mapping["main_bindings"][name])
                          for name in main_names) +
                ", (size_t)(sizeof(size_t) * CHAR_BIT), (size_t)SIZE_MAX);\n")
    additions = [
        (0, "/* R1551 addition-only read-only observation copy. */\n"
            "#include <limits.h>\n#include <stdio.h>\n"),
        (helper_start + helper_body.index(return_line), helper_add),
        (main_start + main_body.index("    return;"), main_add),
    ]
    observed = raw
    for offset, addition in sorted(additions, reverse=True):
        observed = observed[:offset] + addition.encode("ascii") + observed[offset:]
    restored = []
    cursor = 0
    shift = 0
    insertion_records = []
    for offset, addition in sorted(additions):
        data = addition.encode("ascii")
        at = offset + shift
        require(observed[at:at + len(data)] == data, "observer insertion mismatch")
        restored.append(observed[cursor:at])
        cursor = at + len(data)
        insertion_records.append({"original_byte_offset": offset,
                                  "observed_byte_offset": at,
                                  "addition": addition, "bytes": len(data),
                                  "sha256": digest(data)})
        shift += len(data)
    restored.append(observed[cursor:])
    require(b"".join(restored) == raw, "removing observation additions changed raw C")
    save("observer.c", observed)
    save("restored.c", b"".join(restored))
    save_json("observation-insertions.json",
              {"additions": insertion_records, "raw_sha256": digest(raw),
               "observed_sha256": digest(observed),
               "restored_sha256": digest(b"".join(restored)),
               "restoration_exact": True})
    diff = "".join(difflib.unified_diff(text.splitlines(keepends=True),
                                      observed.decode("ascii").splitlines(keepends=True),
                                      fromfile="original.c", tofile="observer.c"))
    save("observation.diff", diff)
    return observed


def parse_observation(output, spec, target):
    lines = output.decode("ascii").splitlines()
    require(len(lines) == 4, "expected three actual helper records and one caller record")
    for index, expected in enumerate(spec["calls"]):
        require(lines[index].startswith("R1551_H "), "wrong helper observation order")
        values = dict(item.split("=") for item in lines[index].split()[1:])
        names = [spec["formal"]] + spec["helper_bindings"]
        require(list(values) == names, "helper observation identity mismatch")
        actual = {k: int(v) for k, v in values.items()}
        wanted = {spec["formal"]: expected[0]}
        wanted.update({name: expected[1] for name in spec["helper_bindings"]})
        require(actual == wanted, "native helper quantity mismatch")
    require(lines[3].startswith("R1551_M "), "missing actual caller observation")
    values = {k: int(v) for k, v in
              (item.split("=") for item in lines[3].split()[1:])}
    wanted = {name: number for name, _, number in spec["bindings"]}
    wanted.update(width=target["size_t_bits"], max=target["size_max"])
    require(values == wanted, "native caller reuse/arithmetic/target mismatch")
    return {"helper_records": lines[:3], "caller_record": lines[3]}


def native():
    spec = MANIFEST["positives"][MODE]
    gate = json.loads((WORK / "source" / "source-gate.json").read_text())
    require(gate["passed"], "source/owned/negative fixture did not pass")
    mapping = json.loads((WORK / "source" / (MODE + "-map.json")).read_text())
    save_json("public-symbol-map.json", mapping)
    source = DATA / "frozen" / spec["file"]
    original = OUT / "original.c"
    emitted = run([CLI, "--emit-c17-to", original, source], "ordinary-c17-emission", 0)
    require(not emitted.stdout and not emitted.stderr, "positive emission diagnostics")
    track("original.c")
    raw = original.read_bytes()
    require(raw and b"NewLang source-owned quantity division C17." in raw,
            "ordinary quantity C17 output unavailable")
    require(str(mapping["target"]["size_max"]).encode() in raw,
            "actual emitted target assertion absent")
    observation_copy(raw, mapping, spec)
    gcc = shutil.which("gcc-13") or shutil.which("gcc")
    require(gcc is not None, "available GCC missing")
    compilers = [("gcc", gcc)]
    if FRONTEND == "ON":
        save_json("clang-availability.json",
                  {"status": "NOT_AVAILABLE", "reason": "unchanged NO-LLVM PARTIAL"})
    else:
        sdk = os.environ.get("NEWLANG_SDK_ROOT")
        require(sdk, "pinned SDK prefix missing in full lane")
        clang = shutil.which("clang-23")
        require(clang and Path(clang).resolve().parent == (Path(sdk) / "bin").resolve(),
                "Clang is not from the existing pinned SDK")
        compilers.append(("clang23", clang))
    common = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
              "-O0", "-g"]
    require(SANITIZER in ("none", "address", "undefined"), "unknown sanitizer mode")
    if SANITIZER != "none":
        common += ["-fsanitize=" + SANITIZER, "-fno-omit-frame-pointer",
                   "-fno-sanitize-recover=all"]
    observations = []
    for name, compiler in compilers:
        version = run([compiler, "--version"], name + "-version", 0)
        if name == "clang23":
            require(b"23.1.3" in version.stdout, "pinned Clang version mismatch")
        flags = common + (["-Wno-error=gcc-install-dir-libstdcxx"]
                          if name == "clang23" else [])
        for variant in ("original", "observer"):
            source_c = OUT / (variant + ".c")
            executable = OUT / (name + "-" + variant)
            compiled = run([compiler] + flags + [source_c, "-o", executable],
                           name + "-" + variant + "-compile", 0)
            require(not compiled.stdout, "unexpected compiler stdout")
            executed = run([executable], name + "-" + variant + "-run", 0, guest=True)
            require(not executed.stderr, "guest stderr/sanitizer diagnostic")
            if variant == "original":
                require(not executed.stdout, "unedited C unexpectedly printed output")
                observations.append({"compiler": name, "variant": variant,
                                     "exit": 0, "stdout_bytes": 0, "stderr_bytes": 0})
            else:
                observations.append({"compiler": name, "variant": variant,
                                     "exit": 0,
                                     "observed": parse_observation(executed.stdout, spec,
                                                                  mapping["target"])})
            require(original.read_bytes() == raw, "unedited emitted C changed")
    require(GUEST_COMPLETED == (2 if FRONTEND == "ON" else 4),
            "unexpected guest process count")
    save_json("native-observations.json",
              {"runs": observations, "guest_count": GUEST_COMPLETED,
               "guest_sanitizer": SANITIZER, "raw_sha256": digest(raw),
               "source": spec["file"]})


def emit_evidence():
    # Transport only: no effect on guest computations or test verdict.
    records = {}
    total = 0
    for name in FILES:
        data = (OUT / name).read_bytes()
        total += len(data)
        if total > 450000:
            print("R1551 evidence retrieval limit: allowlisted payload exceeds bound",
                  flush=True)
            return
        records[name] = {"bytes": len(data), "sha256": digest(data),
                         "base64": base64.b64encode(data).decode("ascii")}
    packed = gzip.compress(json.dumps({"mode": MODE, "files": records},
                                      sort_keys=True).encode(), mtime=0)
    text = base64.b64encode(packed).decode("ascii")
    chunks = [text[i:i + 2800] for i in range(0, len(text), 2800)]
    lines = [f"R1551_BUNDLE_BEGIN {MODE} {digest(packed)} {len(packed)} {len(chunks)}"]
    lines += [f"R1551_BUNDLE_CHUNK {MODE} {i} {chunk}"
              for i, chunk in enumerate(chunks)]
    lines += [f"R1551_BUNDLE_END {MODE} {digest(packed)}"]
    stream = None
    try:
        stream = os.open(f"/proc/{os.getppid()}/fd/1", os.O_WRONLY | os.O_APPEND)
    except OSError:
        pass
    try:
        for line in lines:
            if stream is not None:
                os.write(stream, (line + "\n").encode("ascii"))
            else:
                print(line, flush=True)
    finally:
        if stream is not None:
            os.close(stream)


def main():
    global PASSED
    if os.environ.get("GITHUB_ACTIONS") != "true":
        raise SystemExit("R1551 execution is GitHub Actions only")
    OUT.mkdir(parents=True, exist_ok=False)
    failure = None
    try:
        require(MODE in ("source", "tail", "return"), "unreleased mode")
        identity()
        check_frozen()
        if MODE == "source":
            proof()
        else:
            native()
        check_frozen()
        PASSED = True
    except Exception:
        failure = traceback.format_exc()
        save("failure.txt", failure)
    finally:
        save_json("commands.json", COMMANDS)
        save_json("summary.json",
                  {"passed": PASSED, "mode": MODE,
                   "evidence_label": MANIFEST["evidence_label"],
                   "guest_attempts": GUEST_ATTEMPTS,
                   "guest_completed": GUEST_COMPLETED,
                   "sanitizer": SANITIZER, "frontend_only": FRONTEND,
                   "r_specific_oom": "NOT_RUN"})
        try:
            emit_evidence()
        except Exception as error:
            print("R1551 evidence retrieval limit:", type(error).__name__, flush=True)
    if failure is not None:
        print(failure, file=sys.stderr)
        return 1
    print("R1551", MODE, "PASS; calibrated fixed-input evidence;"
          " guest executions =", GUEST_COMPLETED, flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
