"""Passive C observer V1. Derive roles from C syntax, never oracle values.

The bounded recognizer fails closed on unsupported emitter spelling. Insertions
are fprintf calls reading live scalar/record lvalues at existing sequence points.
No guest assignment, branch, helper state, authority, or synthetic old value.
"""
import json
import re


class Unobservable(RuntimeError):
    pass


IDENT = r"[A-Za-z_][A-Za-z_0-9]*"
SCALAR = r"(?:uint8_t|size_t|uint64_t|unsigned\s+char|bool)"


def mask(text):
    pattern = r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    return re.sub(pattern, lambda m: "".join("\n" if c == "\n" else " " for c in m[0]), text)


def close_brace(code, opening):
    depth = 1
    for pos in range(opening + 1, len(code)):
        depth += (code[pos] == "{") - (code[pos] == "}")
        if depth == 0:
            return pos
    raise Unobservable("unbalanced C scope")


def simple(expression):
    expression = re.sub(r"\((?:size_t|uint8_t|uint64_t|bool)\)", "", expression)
    return re.sub(r"[\s()]", "", expression)


def instrument(text):
    code = mask(text)
    types = {}
    # Named and typedef two-u8 C records, declaration order retained.
    structs = re.finditer(r"\b(typedef\s+)?struct(?:\s+(" + IDENT + r"))?\s*\{", code)
    for match in structs:
        end = close_brace(code, match.end() - 1)
        body = code[match.end():end]
        fields = re.findall(r"\buint8_t\s+(" + IDENT + r")\s*;", body)
        if len(fields) != 2 or re.sub(r"\buint8_t\s+" + IDENT + r"\s*;", "", body).strip():
            continue
        if match[1]:
            alias = re.match(r"\s*(" + IDENT + r")\s*;", code[end + 1:])
            if alias:
                types[alias[1]] = fields
        elif match[2]:
            types["struct " + match[2]] = fields
    roots = []
    for kind, fields in types.items():
        pattern = r"\b" + re.escape(kind) + r"\s+(" + IDENT + r")\s*(?:=|;)"
        for root in re.finditer(pattern, code):
            roots.append((root[1], fields, root.start()))
    loops = list(re.finditer(r"\b(?:for\s*\(\s*;\s*;\s*\)|while\s*\(\s*(?:1|true)\s*\))\s*\{", code))
    if len(loops) != 1:
        raise Unobservable(f"expected one syntactic unbounded C loop, got {len(loops)}")
    loop = loops[0]
    loop_end = close_brace(code, loop.end() - 1)
    roots = [root for root in roots if root[2] < loop.start()]
    if len(roots) != 1:
        raise Unobservable(f"expected one original two-u8 local, got {len(roots)}")
    root, fields, root_start = roots[0]
    declarations = []
    pattern = r"\b(" + SCALAR + r")\s+(" + IDENT + r")\s*(?:=\s*([^;{}]+))?;"
    for declaration in re.finditer(pattern, code):
        declarations.append({"name": declaration[2], "type": declaration[1],
                             "expression": declaration[3], "start": declaration.start(),
                             "end": declaration.end()})
    outer = {d["name"] for d in declarations
             if root_start < d["start"] < loop.start() and d["type"] == "size_t"}
    assignments = list(re.finditer(r"\b(" + IDENT + r")\s*=\s*([^;{}]+);", code))
    continues = list(re.finditer(r"\bcontinue\s*;", code[loop.end():loop_end]))
    breaks = list(re.finditer(r"\bbreak\s*;", code[loop.end():loop_end]))
    if len(continues) != 1 or len(breaks) != 1:
        raise Unobservable("expected one actual C Continue and one Break site")
    continue_pos = loop.end() + continues[0].start()
    break_pos = loop.end() + breaks[0].start()
    carry_candidates = [a for a in assignments if loop.end() < a.start() < continue_pos
                        and a[1] in outer]
    if not carry_candidates:
        raise Unobservable("no actual outer usize carry assignment before Continue")
    carry = carry_candidates[-1][1]
    break_candidates = [a for a in assignments if loop.end() < a.start() < break_pos
                        and a[1] in outer and a[1] != carry]
    if not break_candidates:
        raise Unobservable("no actual outer usize result assignment before Break")
    result = break_candidates[-1][1]
    comparisons = list(re.finditer(r"\b(" + IDENT + r")\s*<\s*(?:" + IDENT + r"|[0-9])", code[loop.end():loop_end]))
    if len(comparisons) != 1:
        raise Unobservable("no unique less-than guard")
    lhs = comparisons[0][1]
    guard_pos = loop.end() + comparisons[0].start()
    for _ in range(24):
        if lhs == carry:
            break
        definitions = [d for d in declarations if d["name"] == lhs and
                       d["expression"] and d["start"] < guard_pos]
        if not definitions:
            break
        lhs = simple(definitions[-1]["expression"])
    if lhs != carry:
        raise Unobservable("guard lhs is not the actual Continue carry")
    aliases = {}
    for match in re.finditer(r"\b(?:uint8_t\s*\*\s*)?(" + IDENT + r")\s*=\s*&\s*(" + IDENT + r")\s*\.\s*(" + IDENT + r")\s*;", code):
        if match[2] == root and match[3] in fields:
            aliases[match[1]] = fields.index(match[3])

    def target(expression):
        value = simple(expression)
        for index, field in enumerate(fields):
            if value == root + "." + field:
                return index
        if value.startswith("*") and value[1:] in aliases:
            return aliases[value[1:]]
        return None

    inserts = {}
    sites = []

    def emit(position, event, index=-1, value=None):
        if value is None:
            value = carry
        site = len(sites)
        sites.append({"site": site, "event": event, "index": index,
                      "value_lvalue": value, "offset": position})
        line = ('\nfprintf(stderr, "R1173|' + event + '|' + str(site) + '|' + str(index) +
                '|%zu|%zu|%zu|%zu|%p|%p|%p\\n", (size_t)(' + value +
                '), (size_t)(' + carry + '), (size_t)(' + root + '.' + fields[0] +
                '), (size_t)(' + root + '.' + fields[1] + '), (void *)&' + root +
                ', (void *)&' + root + '.' + fields[0] + ', (void *)&' + root +
                '.' + fields[1] + ');\n')
        inserts.setdefault(position, []).append(line)

    emit(loop.start(), "INIT")
    emit(loop.end(), "HEADER")
    # Both declaration loads and later assignments retain the real old lvalue.
    observed_loads = set()
    for declaration in declarations:
        if not (loop.end() < declaration["start"] < loop_end) or not declaration["expression"]:
            continue
        index = target(declaration["expression"])
        if index is not None:
            emit(declaration["end"], "LOAD", index, declaration["name"])
            observed_loads.add(declaration["end"])
    scalar_names = {d["name"] for d in declarations}
    for assignment in assignments:
        if not (loop.end() < assignment.start() < loop_end) or assignment.end() in observed_loads:
            continue
        index = target(assignment[2])
        if assignment[1] in scalar_names and index is not None:
            emit(assignment.end(), "LOAD", index, assignment[1])
    store_pattern = r"(?<![A-Za-z_0-9.])((?:" + re.escape(root) + r"\s*\.\s*" + IDENT + r")|(?:\*\s*\(?\s*" + IDENT + r"\s*\)?))\s*=\s*([^;{}]+);"
    store_sites = []
    for store in re.finditer(store_pattern, code):
        if not (loop.end() < store.start() < loop_end):
            continue
        index = target(store[1])
        if index is not None:
            emit(store.end(), "STORE", index, root + "." + fields[index])
            store_sites.append(index)
    if len(store_sites) != 1:
        raise Unobservable(f"expected one recognized actual field store, got {store_sites}")
    emit(continue_pos, "CONTINUE")
    emit(break_pos, "BREAK", value=result)
    emit(loop_end + 1, "FINAL", value=result)
    rewritten = text
    for position in sorted(inserts, reverse=True):
        rewritten = rewritten[:position] + "".join(inserts[position]) + rewritten[position:]
    rewritten = "#include <stdio.h>\n" + rewritten
    mapping = {"schema": "R1173_PASSIVE_C_V1", "root": root, "fields": fields,
               "carry": carry, "result": result, "aliases": aliases, "sites": sites,
               "oracle_used_by_instrumenter": False,
               "insertions": "read-only fprintf; no guest stores, branches or helper state"}
    return rewritten, mapping


def check_trace(stderr, case):
    events = []
    for line in stderr.splitlines():
        if not line.startswith("R1173|"):
            continue
        parts = line.split("|")
        if len(parts) != 11:
            raise AssertionError("malformed observer event: " + line)
        events.append({"event": parts[1], "site": int(parts[2]), "index": int(parts[3]),
                       "value": int(parts[4]), "carry": int(parts[5]),
                       "fields": [int(parts[6]), int(parts[7])], "identity": parts[8:11]})
    if not events:
        raise AssertionError("no passive native events")
    original = events[0]["identity"]
    if any(e["identity"] != original for e in events):
        raise AssertionError("original record/field physical identity changed")
    if original[1] == original[2] or any(x in ("(nil)", "0x0") for x in original):
        raise AssertionError("invalid or overlapping distinct child addresses")
    init = [e for e in events if e["event"] == "INIT"]
    if len(init) != 1 or init[0]["fields"] != case["initial_fields"]:
        raise AssertionError("actual original initial fields differ")
    headers = [e["carry"] for e in events if e["event"] == "HEADER"]
    if headers != case["headers"]:
        raise AssertionError(f"actual headers {headers} != frozen {case['headers']}")
    stores = [e for e in events if e["event"] == "STORE"]
    selected = case["selected_index"]
    if len(stores) != case["write_count"] or any(e["index"] != selected for e in stores):
        raise AssertionError("actual field write count/target differs")
    continued = [e["carry"] for e in events if e["event"] == "CONTINUE"]
    if continued != case["headers"][1:]:
        raise AssertionError("actual outgoing Continue carries differ")
    breaks = [e for e in events if e["event"] == "BREAK"]
    finals = [e for e in events if e["event"] == "FINAL"]
    if len(breaks) != 1 or len(finals) != 1 or any(
            e["value"] != case["break_result"] for e in breaks + finals):
        raise AssertionError("actual Break/result differs")
    if finals[0]["fields"][selected] != case["final_selected"] or finals[0]["fields"][1 - selected] != case["final_sibling"]:
        raise AssertionError("actual post-Break selected/sibling differs")
    groups = []
    current = None
    for event in events:
        if event["event"] == "HEADER":
            current = []
            groups.append(current)
        elif current is not None:
            current.append(event)
    old_values = []
    for group, expected in zip(groups, case["events"]):
        store_at = next(i for i, e in enumerate(group) if e["event"] == "STORE")
        preceding = [e for e in group[:store_at] if e["event"] == "LOAD" and e["index"] == selected]
        following = [e for e in group[store_at + 1:] if e["event"] == "LOAD" and e["index"] == selected]
        sibling = [e for e in group if e["event"] == "LOAD" and e["index"] != selected]
        if len(preceding) < 2 or not following or not sibling:
            raise Unobservable("insufficient separately materialized before/old/after/sibling loads")
        if preceding[0]["value"] != expected["before"] or preceding[-1]["value"] != expected["old"]:
            raise AssertionError("actual before/replace-old values differ")
        if any(e["value"] != expected["after"] for e in following) or any(
                e["value"] != expected["sibling"] for e in sibling):
            raise AssertionError("actual iteration selected/sibling read differs")
        if group[store_at]["fields"][selected] != expected["after"]:
            raise AssertionError("actual selected value immediately after store differs")
        if any(e["fields"][1 - selected] != expected["sibling"] for e in group):
            raise AssertionError("actual known-disjoint sibling changed")
        old_values.append(preceding[-1]["value"])
    return {"status": "PASS", "events": events, "headers": headers,
            "actual_old_values": old_values, "physical_identity": original,
            "current_fact_freshness": "not inferred from native values/addresses"}
