import difflib
import json
import re

from common import require, sha


def masked(text):
    pattern = r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    return re.sub(pattern, lambda m: "".join("\n" if c == "\n" else " "
                                             for c in m[0]), text)


def close_brace(mask, opening):
    depth = 0
    for i in range(opening, len(mask)):
        depth += (mask[i] == "{") - (mask[i] == "}")
        if depth == 0:
            return i
    raise ValueError("unclosed emitted function")


def parse_functions(raw):
    mask = masked(raw)
    pattern = re.compile(
        r"\b(?:static\s+)?(?P<type>size_t|uint64_t|uintptr_t|void|int)\s+"
        r"(?P<name>[A-Za-z_]\w*)\s*\((?P<args>[^;{}]*)\)\s*\{")
    functions = []
    for match in pattern.finditer(mask):
        opening = match.end() - 1
        end = close_brace(mask, opening)
        if any(f["opening"] < opening < f["end"] for f in functions):
            continue
        arguments = raw[match.start("args"):match.end("args")]
        parameters = re.findall(
            r"\b(?:const\s+)?(?:size_t|uint64_t|uintptr_t)\s+([A-Za-z_]\w*)",
            arguments)
        declarations = []
        body_mask = mask[opening + 1:end]
        decl = re.compile(
            r"\b(?:const\s+)?(?:size_t|uint64_t|uintptr_t)\s+"
            r"(?P<name>[A-Za-z_]\w*)\s*=\s*(?P<rhs>[^;{}]+);")
        for item in decl.finditer(body_mask):
            offset = opening + 1
            declarations.append({
                "name": item["name"], "start": offset + item.start(),
                "end": offset + item.end(),
                "rhs": raw[offset + item.start("rhs"):offset + item.end("rhs")]})
        returns = []
        for item in re.finditer(r"\breturn\s+([A-Za-z_]\w*)\s*;", body_mask):
            returns.append({"name": item[1], "start": opening + 1 + item.start()})
        functions.append({"name": match["name"], "type": match["type"],
                          "parameters": parameters, "opening": opening,
                          "end": end, "declarations": declarations,
                          "returns": returns})
    return functions


def constant(expression, declarations, before, seen=()):
    text = masked(expression).strip()
    wrapped = re.fullmatch(r"(?:UINT64_C|UINTMAX_C)\s*\(\s*(\d+)\s*\)", text)
    if wrapped:
        return int(wrapped[1])
    text = re.sub(r"\(\s*(?:const\s+)?(?:size_t|uint64_t|uintptr_t)\s*\)", "", text)
    text = text.replace("(", "").replace(")", "").strip()
    literal = re.fullmatch(r"(\d+)[uUlL]*", text)
    if literal:
        return int(literal[1])
    if re.fullmatch(r"[A-Za-z_]\w*", text) and text not in seen:
        candidates = [d for d in declarations if d["name"] == text and
                      d["start"] < before]
        if candidates:
            d = candidates[-1]
            return constant(d["rhs"], declarations, d["start"], seen + (text,))
    return None


def make_observer(raw_bytes, directory):
    raw = raw_bytes.decode("utf-8")
    functions = parse_functions(raw)
    additions = []
    serial = 0

    def add(position, text, reason):
        nonlocal serial
        serial += 1
        block = ("\n/* R1562_OBSERVER_BEGIN " + str(serial) + " */\n" +
                 text + "\n/* R1562_OBSERVER_END */\n")
        additions.append({"position": position, "text": block, "reason": reason})

    add(0, "#include <stdio.h>\n#include <stdint.h>", "observation declarations")
    for function in functions:
        name = function["name"]
        for parameter in function["parameters"]:
            add(function["opening"] + 1,
                '    printf("R1562_OBS|P|' + name + "|" + parameter +
                "|%zu\\n\", (size_t)" + parameter + ");",
                {"function": name, "kind": "formal", "name": parameter})
        if name == "main":
            add(function["opening"] + 1,
                '    printf("R1562_TARGET|%zu|%zu\\n", '
                "(size_t)SIZE_MAX, sizeof(size_t));", "native target metadata")
        for declaration in function["declarations"]:
            variable = declaration["name"]
            add(declaration["end"],
                '    printf("R1562_OBS|V|' + name + "|" + variable +
                "|%zu\\n\", (size_t)" + variable + ");",
                {"function": name, "kind": "existing initialized scalar",
                 "name": variable, "rhs": declaration["rhs"]})
        available = set(function["parameters"])
        available.update(d["name"] for d in function["declarations"])
        for returned in function["returns"]:
            if returned["name"] in available:
                variable = returned["name"]
                add(returned["start"],
                    '    printf("R1562_OBS|R|' + name + "|" + variable +
                    "|%zu\\n\", (size_t)" + variable + ");",
                    {"function": name, "kind": "existing return value",
                     "name": variable})
    pieces, cursor = [], 0
    for addition in sorted(additions, key=lambda a: (a["position"], a["text"])):
        pieces.extend([raw[cursor:addition["position"]], addition["text"]])
        cursor = addition["position"]
    pieces.append(raw[cursor:])
    observed = "".join(pieces)
    restored = re.sub(
        r"\n/\* R1562_OBSERVER_BEGIN \d+ \*/\n[\s\S]*?"
        r"\n/\* R1562_OBSERVER_END \*/\n", "", observed)
    require(restored.encode() == raw_bytes, "observer removal reconstruction")
    (directory / "observer.c").write_bytes(observed.encode())
    (directory / "observer-removal.c").write_bytes(restored.encode())
    (directory / "observer.patch").write_text(
        "".join(difflib.unified_diff(raw.splitlines(True),
                                    observed.splitlines(True),
                                    fromfile="raw.c", tofile="observer.c")),
        encoding="utf-8")
    arithmetic = []
    for function in functions:
        for declaration in function["declarations"]:
            expression = masked(declaration["rhs"])
            operations = list(re.finditer(r"[/%%]", expression))
            if len(operations) == 1:
                op = operations[0]
                divisor = constant(declaration["rhs"][op.end():],
                                   function["declarations"],
                                   declaration["start"])
                arithmetic.append({"function": function["name"],
                                   "operator": op[0], "divisor": divisor,
                                   "result_variable": declaration["name"],
                                   "expression": declaration["rhs"]})
    mapping = {"raw_bytes": len(raw_bytes), "raw_sha256": sha(raw_bytes),
               "observer_bytes": len(observed.encode()),
               "observer_sha256": sha(observed.encode()),
               "removal_sha256": sha(restored.encode()),
               "removal_equal": True, "additions": additions,
               "functions": functions, "arithmetic": arithmetic}
    return mapping


def assess_stdout(stdout, mapping, case):
    text = stdout.decode("utf-8")
    values, targets, other = [], [], []
    for line in text.splitlines():
        if line.startswith("R1562_OBS|"):
            parts = line.split("|")
            require(len(parts) == 5 and parts[4].isdigit(), "observer row format")
            values.append({"kind": parts[1], "function": parts[2],
                           "name": parts[3], "value": int(parts[4])})
        elif line.startswith("R1562_TARGET|"):
            parts = line.split("|")
            targets.append([int(parts[1]), int(parts[2])])
        else:
            other.append(line)
    require(targets == [[18446744073709551615, 8]], "native T64 target mismatch")
    require(not other, "unexpected observer guest stdout")
    wanted = ([("/", 5, [53], [10]), ("/", 3, [10, 10], [3, 3])]
              if case == "P1" else
              [("/", 17, [38], [2]), ("%", 6, [38, 2], [2, 2])])
    selected, limits = [], []
    for operator, divisor, arguments, results in wanted:
        candidates = [a for a in mapping["arithmetic"]
                      if a["operator"] == operator and a["divisor"] == divisor]
        if len(candidates) != 1:
            limits.append("cannot uniquely map emitted leaf " +
                          operator + str(divisor))
            continue
        actual = candidates[0]
        function = actual["function"]
        params = [v for v in values if v["kind"] == "P" and
                  v["function"] == function]
        result_rows = [v for v in values if v["kind"] == "V" and
                       v["function"] == function and
                       v["name"] == actual["result_variable"]]
        if len(params) != len(arguments) or len(result_rows) != len(results):
            limits.append("emitted call structure not bijectively observed: " +
                          function)
        else:
            require([v["value"] for v in params] == arguments,
                    "native actual/formal values disagree: " + function)
            require([v["value"] for v in result_rows] == results,
                    "native operation result disagrees: " + function)
        selected.append(actual)
    call_results = []
    if len(selected) == 2:
        names = {a["function"] for a in selected}
        candidates = []
        for f in mapping["functions"]:
            calls = []
            for d in f["declarations"]:
                for name in names:
                    if re.search(r"\b" + re.escape(name) + r"\s*\(", d["rhs"]):
                        calls.append({**d, "callee": name})
            if len(calls) == 3:
                candidates.append((f, sorted(calls, key=lambda d: d["start"])))
        if len(candidates) == 1:
            f, calls = candidates[0]
            expected_results = [10, 3, 3] if case == "P1" else [2, 2, 2]
            for d, expected in zip(calls, expected_results):
                rows = [v for v in values if v["kind"] == "V" and
                        v["function"] == f["name"] and v["name"] == d["name"]]
                if len(rows) != 1:
                    limits.append("caller result cannot be uniquely observed")
                else:
                    require(rows[0]["value"] == expected,
                            "native caller result mismatch")
                    call_results.append({"callee": d["callee"],
                                         "variable": d["name"],
                                         "actual": rows[0]["value"]})
        else:
            limits.append("cannot uniquely map emitted ordinary caller")
    return {"status": "OBSERVED" if not limits else "PARTIAL_NOT_OBSERVED",
            "limits": limits, "targets": targets, "values": values,
            "mapped_arithmetic": selected, "caller_results": call_results}
