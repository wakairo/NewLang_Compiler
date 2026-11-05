"""R1179 passive C observer. Inputs: unmodified C and its Clang AST only.

No NewLang source, oracle, expected values, or previous R inputs are read.
All guest expressions come from actual AST declarations and def-use relations.
Unrecognized shapes stop before passive compilation; original bytes are kept.
"""
import difflib
import json
from pathlib import Path
import re
import sys


class ShapeError(RuntimeError):
    pass


def require(condition, text):
    if not condition:
        raise ShapeError(text)


def descendants(node):
    yield node
    for child in node.get("inner", []):
        yield from descendants(child)


def loc(point):
    return point.get("expansionLoc", point)


def bounds(node):
    span = node.get("range", {})
    begin, end = loc(span.get("begin", {})), loc(span.get("end", {}))
    require("offset" in begin and "offset" in end, "AST source range missing")
    return begin["offset"], end["offset"] + end.get("tokLen", 1)


def unwrap(node):
    while node.get("kind") in (
        "ImplicitCastExpr", "CStyleCastExpr", "ParenExpr",
        "ConstantExpr", "ExprWithCleanups",
    ) and len(node.get("inner", [])) == 1:
        node = node["inner"][0]
    return node


def type_text(node):
    data = node.get("type", {})
    raw = data.get("desugaredQualType", data.get("qualType", ""))
    return " ".join(re.sub(r"\b(const|volatile|restrict|__restrict|__restrict__)\b",
                           "", raw).split())


def is_scalar(node):
    t = type_text(node)
    return "*" not in t and "[" not in t and (
        t in ("unsigned char", "unsigned long", "unsigned long long",
              "unsigned int", "int", "_Bool", "bool")
        or " ".join(re.sub(r"\b(const|volatile|restrict)\b", "",
                              node.get("type", {}).get("qualType", "")).split())
        in ("uint8_t", "size_t")
    )


def is_usize(node):
    return is_scalar(node) and (
        " ".join(re.sub(r"\b(const|volatile|restrict)\b", "",
                          node.get("type", {}).get("qualType", "")).split()) == "size_t"
        or type_text(node) in ("unsigned long", "unsigned long long")
    )


def instrument(source_path, ast_path, passive_path, map_path):
    source = Path(source_path).read_text(encoding="utf-8")
    tree = json.loads(Path(ast_path).read_text(encoding="utf-8"))
    require(source.isascii(), "this finite emitted-C mapping requires ASCII byte offsets")
    functions = {
        n["name"]: n for n in tree.get("inner", [])
        if n.get("kind") == "FunctionDecl"
        and any(x.get("kind") == "CompoundStmt" for x in n.get("inner", []))
    }
    require("main" in functions, "actual C main definition required")
    reachable, active, call_edges = set(), set(), []

    def visit_function(name):
        require(name not in active, "recursive C call graph is outside the finite mapping")
        if name in reachable:
            return
        active.add(name)
        function = functions[name]
        function_body = next(x for x in function["inner"]
                             if x.get("kind") == "CompoundStmt")
        for node in descendants(function_body):
            if node.get("kind") != "CallExpr":
                continue
            callee = unwrap(node["inner"][0])
            require(callee.get("kind") == "DeclRefExpr" and
                    callee.get("referencedDecl", {}).get("kind") == "FunctionDecl",
                    "indirect C call is outside the finite mapping")
            called = callee["referencedDecl"]["name"]
            if called in functions:
                call_edges.append({"caller": name, "callee": called,
                                   "offset": bounds(node)[0]})
                visit_function(called)
        active.remove(name)
        reachable.add(name)

    visit_function("main")
    loop_functions = []
    for name in reachable:
        function = functions[name]
        function_body = next(x for x in function["inner"]
                             if x.get("kind") == "CompoundStmt")
        found = [n for n in descendants(function_body)
                 if n.get("kind") in ("ForStmt", "WhileStmt", "DoStmt")]
        if found:
            loop_functions.append((name, function_body, found))
    require(len(loop_functions) == 1 and len(loop_functions[0][2]) == 1,
            "one actual loop in one reachable C function required")
    guest_function, body, _ = loop_functions[0]
    all_nodes = list(descendants(body))
    parent = {}
    for node in all_nodes:
        for child in node.get("inner", []):
            parent[id(child)] = node
    declarations = {n["id"]: n for n in all_nodes if n.get("kind") == "VarDecl"}
    require(not any(re.search(r"\b(volatile|_Atomic)\b",
                              json.dumps(n.get("type", {})))
                    for n in all_nodes if n.get("kind") in
                    ("VarDecl", "DeclRefExpr", "MemberExpr")),
            "volatile/atomic observations are outside this passive finite family")
    by_name = {}
    for did, node in declarations.items():
        name = node["name"]
        require(re.fullmatch(r"[A-Za-z_]\w*", name) is not None, "C name shape")
        by_name.setdefault(name, []).append(did)
    loops = [n for n in all_nodes if n.get("kind") in (
        "ForStmt", "WhileStmt", "DoStmt")]
    require(len(loops) == 1, "one actual C loop required")
    loop = loops[0]
    loop_start, loop_end = bounds(loop)
    loop_body = next((x for x in reversed(loop.get("inner", []))
                      if x.get("kind") == "CompoundStmt"), None)
    require(loop_body is not None, "compound loop body required")
    loop_nodes = list(descendants(loop_body))
    loop_ids = {id(n) for n in loop_nodes}
    breaks = [n for n in loop_nodes if n.get("kind") == "BreakStmt"]
    continues = [n for n in loop_nodes if n.get("kind") == "ContinueStmt"]
    require(len(breaks) == len(continues) == 1,
            "one real Break and one real Continue required")

    def statement_end(node):
        _, end = bounds(node)
        if source[end - 1:end] == ";":
            return end
        pos = end
        while pos < len(source) and source[pos].isspace():
            pos += 1
        endpoint = node.get("range", {}).get("end", {})
        if "expansionLoc" in endpoint and pos < len(source) and source[pos] == "(":
            # Clang's expansion endpoint may name UINTMAX_C but omit its
            # invocation arguments. Extend only this one balanced invocation,
            # never search forward for an unrelated semicolon or statement.
            start = loc(endpoint)["offset"]
            require(re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", source[start:end]) is not None,
                    "macro endpoint is not one identifier")
            depth = 0
            while pos < len(source):
                character = source[pos]
                require(character not in ";{}\"'\\/",
                        "unsupported token or statement boundary in macro invocation")
                if character == "(":
                    depth += 1
                elif character == ")":
                    depth -= 1
                    if depth == 0:
                        pos += 1
                        break
                pos += 1
            require(depth == 0, "unterminated macro invocation")
            while pos < len(source) and source[pos].isspace():
                pos += 1
        require(pos < len(source) and source[pos] == ";",
                "full statement does not terminate at its actual semicolon")
        return pos + 1

    def declaration_statement(node):
        p = parent.get(id(node))
        require(p is not None and p.get("kind") == "DeclStmt",
                "ordinary local declaration statement required")
        return p

    definitions = {}
    for did, node in declarations.items():
        if node.get("inner"):
            definitions.setdefault(did, []).append(
                (bounds(node)[0], node["inner"][-1], node, "bind"))
    assignments = [n for n in all_nodes
                   if n.get("kind") == "BinaryOperator" and n.get("opcode") == "="]
    for node in assignments:
        lhs = unwrap(node["inner"][0])
        if lhs.get("kind") == "DeclRefExpr":
            did = lhs["referencedDecl"]["id"]
            if did in declarations:
                definitions.setdefault(did, []).append(
                    (bounds(node)[0], node["inner"][1], node, "assign"))

    def field_target(expression, seen=()):
        expression = unwrap(expression)
        kind = expression.get("kind")
        if kind == "MemberExpr":
            base = field_target(expression["inner"][0], seen)
            return None if base is None else (base[0], base[1] + [expression["name"]])
        if kind == "UnaryOperator" and expression.get("opcode") in ("&", "*"):
            return field_target(expression["inner"][0], seen)
        if kind == "DeclRefExpr":
            did = expression["referencedDecl"]["id"]
            if did not in declarations or did in seen:
                return None
            decl = declarations[did]
            if "*" in type_text(decl):
                choices = definitions.get(did, [])
                require(len(choices) == 1, "pointer alias is not single-definition")
                return field_target(choices[0][1], seen + (did,))
            return did, []
        return None

    stores = []
    for node in assignments:
        target = field_target(node["inner"][0])
        if id(node) in loop_ids and target and len(target[1]) == 1:
            stores.append((node, target))
    require(len(stores) == 1, "one real original-field store in loop required")
    store, (root_id, selected_path) = stores[0]
    root_decl = declarations[root_id]
    root_name = root_decl["name"]
    root_type = type_text(root_decl)
    node_index = {n["id"]: n for n in descendants(tree) if "id" in n}
    record_ids = set()
    alias = node_index.get(root_decl.get("type", {}).get("typeAliasDeclId"))
    if alias is not None:
        require(alias.get("kind") == "TypedefDecl", "root type alias is not a typedef")
        for node in descendants(alias):
            for key in ("decl", "ownedTagDecl"):
                target = node.get(key, {})
                if target.get("kind") == "RecordDecl":
                    record_ids.add(target["id"])
    records = []
    for node in descendants(tree):
        if node.get("kind") == "RecordDecl":
            fs = [x for x in node.get("inner", []) if x.get("kind") == "FieldDecl"]
            name = node.get("name", "")
            matched = node.get("id") in record_ids or (
                name and re.search(r"\b" + re.escape(name) + r"\b", root_type))
            if matched and node.get("tagUsed") == "struct":
                if len(fs) == 2 and all(type_text(x) == "unsigned char" for x in fs):
                    records.append(fs)
    require(len(records) == 1, "root must resolve to one actual two-u8 C struct")
    same_type_roots = [
        n for name in reachable for n in descendants(functions[name])
        if n.get("kind") == "VarDecl" and type_text(n) == root_type
    ]
    require(len(same_type_roots) == 1 and same_type_roots[0]["id"] == root_id,
            "one physical original aggregate, with no substitute aggregate copy")
    fields = [f["name"] for f in records[0]]
    require(selected_path[0] in fields, "store does not select an actual root child")
    selected = fields.index(selected_path[0])

    outer_ifs = []
    for node in loop_nodes:
        if node.get("kind") == "IfStmt" and parent.get(id(node)) is loop_body:
            kinds = [x.get("kind") for x in descendants(node)]
            if "ContinueStmt" in kinds:
                outer_ifs.append(node)
    require(len(outer_ifs) == 1, "unique top-level Continue condition")
    outer_if = outer_ifs[0]
    nested_break = any(n is breaks[0] for n in descendants(outer_if))
    if not nested_break:
        require(parent.get(id(breaks[0])) is loop_body and
                bounds(outer_if)[1] < bounds(breaks[0])[0],
                "fallthrough Break must follow the completed top-level Continue arm")
    condition = outer_if["inner"][0]

    def phase_origins(node, visited=()):
        node = unwrap(node)
        if node.get("kind") == "DeclRefExpr":
            did = node["referencedDecl"]["id"]
            if did in visited or did not in declarations:
                return set()
            decl = declarations[did]
            if is_usize(decl) and bounds(decl)[0] < loop_start:
                return {did}
            result = set()
            for _, expr, _, _ in definitions.get(did, []):
                result.update(phase_origins(expr, visited + (did,)))
            return result
        result = set()
        for child in node.get("inner", []):
            result.update(phase_origins(child, visited))
        return result

    phase_ids = phase_origins(condition)
    require(len(phase_ids) == 1, "outer condition must trace to one carried usize")
    phase_id = next(iter(phase_ids))
    phase_name = declarations[phase_id]["name"]
    runtime_phase_ids = {
        n["referencedDecl"]["id"] for n in descendants(condition)
        if n.get("kind") == "DeclRefExpr"
        and n["referencedDecl"]["id"] in declarations
        and is_usize(declarations[n["referencedDecl"]["id"]])
        and loop_start < bounds(declarations[n["referencedDecl"]["id"]])[0]
        < bounds(outer_if)[0]
    }
    require(len(runtime_phase_ids) == 1, "one actual current-phase binding in the header")
    runtime_phase_id = next(iter(runtime_phase_ids))
    runtime_definitions = definitions.get(runtime_phase_id, [])
    require(len(runtime_definitions) == 1 and
            runtime_definitions[0][3] == "bind" and
            phase_origins(runtime_definitions[0][1]) == {phase_id},
            "actual runtime phase must receive the carried usize")
    runtime_phase = {"id": runtime_phase_id, "event": "bind",
                     "offset": runtime_definitions[0][0]}

    def direct_read(node):
        plain = unwrap(node)
        if plain.get("kind") == "MemberExpr" or (
                plain.get("kind") == "UnaryOperator" and plain.get("opcode") == "*"):
            return field_target(plain)
        return None

    reads = []
    for did, items in definitions.items():
        for offset, expr, node, event in items:
            target = direct_read(expr)
            if target and target[0] == root_id and len(target[1]) == 1:
                reads.append({"id": did, "offset": offset, "node": node,
                              "event": event, "index": fields.index(target[1][0])})
    store_start, _ = bounds(store)
    before = [r for r in reads if loop_start < r["offset"] < store_start]
    after = [r for r in reads if store_start < r["offset"] < loop_end]
    post = [r for r in reads if r["offset"] > loop_end]
    old_reads = sorted((r for r in before if r["index"] == selected),
                       key=lambda r: r["offset"])
    require(len(old_reads) >= 2, "prewrite and actual old-load must both exist")
    old_load = old_reads[-1]
    prewrite = old_reads[0]

    def only(items, index, description):
        found = [x for x in items if x["index"] == index]
        require(len(found) == 1, description)
        return found[0]

    sibling_before = only(before, 1 - selected, "one actual sibling-before load")
    current = only(after, selected, "one actual current-after load")
    sibling = only(after, 1 - selected, "one actual sibling-after load")
    post_current = only(post, selected, "one actual post-Break current load")
    post_sibling = only(post, 1 - selected, "one actual post-Break sibling load")

    def copied_from(did, origin, visited=()):
        if did == origin:
            return True
        if did in visited:
            return False
        for _, expr, _, _ in definitions.get(did, []):
            plain = unwrap(expr)
            if plain.get("kind") == "DeclRefExpr":
                if copied_from(plain["referencedDecl"]["id"], origin,
                               visited + (did,)):
                    return True
        return False

    old_copies = []
    for did, items in definitions.items():
        if did != old_load["id"] and copied_from(did, old_load["id"]):
            for offset, _, node, event in items:
                if store_start < offset < current["offset"]:
                    old_copies.append({"id": did, "offset": offset,
                                       "node": node, "event": event})
    require(len(old_copies) >= 2,
            "actual old result transfer and source Copy must both be observable")
    old_copies.sort(key=lambda x: x["offset"])

    incoming_bindings = []
    incoming_expression = unwrap(store["inner"][1])
    incoming_seen = set()
    while incoming_expression.get("kind") == "DeclRefExpr":
        incoming_id = incoming_expression["referencedDecl"]["id"]
        require(incoming_id in declarations and incoming_id not in incoming_seen,
                "incoming scalar carrier is not a finite local copy chain")
        incoming_seen.add(incoming_id)
        definitions_for_incoming = definitions.get(incoming_id, [])
        require(len(definitions_for_incoming) == 1 and
                type_text(declarations[incoming_id]) == "unsigned char",
                "incoming must use a single-definition u8 carrier")
        offset, expression, node, event = definitions_for_incoming[0]
        require(event == "bind" and loop_start < offset < store_start,
                "incoming source binding must precede the actual write in this iteration")
        incoming_bindings.append({"id": incoming_id, "offset": offset, "event": event})
        incoming_expression = unwrap(expression)
    incoming_bindings.reverse()
    if incoming_expression.get("kind") == "ConditionalOperator":
        selector = incoming_expression["inner"][0]
        require(phase_origins(selector) == {phase_id} and
                any(n.get("kind") == "DeclRefExpr" and
                    n.get("referencedDecl", {}).get("id") == runtime_phase_id
                    for n in descendants(selector)),
                "incoming conditional must use the actual current phase")
        require(len(incoming_bindings) >= 2 and
                all(unwrap(x).get("kind") == "IntegerLiteral"
                    for x in incoming_expression["inner"][1:]),
                "phase selection must yield literal u8 through ordinary copies")
        incoming_kind = "phase_conditional"
    else:
        require(incoming_expression.get("kind") == "IntegerLiteral" and
                not incoming_bindings,
                "literal control must be a direct literal without invented copies")
        incoming_kind = "literal"

    break_start, break_end = bounds(breaks[0])
    break_branch = parent[id(breaks[0])]
    require(break_branch.get("kind") == "CompoundStmt", "compound Break branch")
    b0, b1 = bounds(break_branch)
    result_candidates = []
    for did, items in definitions.items():
        if did == phase_id or not is_usize(declarations[did]):
            continue
        if bounds(declarations[did])[0] >= loop_start:
            continue
        for offset, expr, node, event in items:
            if b0 < offset < break_start and phase_id in phase_origins(expr):
                result_candidates.append((did, node))
    require(len(result_candidates) == 1, "one actual Break-result carrier assignment")
    result_id, result_assignment = result_candidates[0]
    finished = []
    for did, items in definitions.items():
        if did != result_id and copied_from(did, result_id):
            for offset, _, node, event in items:
                if offset > loop_end and is_usize(declarations[did]):
                    finished.append({"id": did, "offset": offset,
                                     "node": node, "event": event})
    # The Break assignment may already target the source's finished binding.
    # That same real carrier is also read after the loop, without inventing a
    # nonexistent post-loop Copy declaration.
    finished.sort(key=lambda x: x["offset"])
    result_name = declarations[result_id]["name"]

    insertions = {}
    events = []
    def add(position, code):
        insertions.setdefault(position, []).append(code)

    def emit(event, scalars=None, addresses=None, ident=None):
        scalars = dict(scalars or {})
        addresses = dict(addresses or {})
        labels = [('e', json.dumps(event))]
        args = []
        if ident is not None:
            labels.append(('id', json.dumps(ident)))
        for key, expression in scalars.items():
            labels.append((key, "%llu"))
            args.append("(unsigned long long)(" + expression + ")")
        for key, expression in addresses.items():
            labels.append((key, '"%p"'))
            args.append("(void *)(" + expression + ")")
        fmt = "R1179 {" + ",".join(json.dumps(k) + ":" + v for k, v in labels) + "}\n"
        return "printf(" + json.dumps(fmt) + (", " + ", ".join(args) if args else "") + ");"

    def state(event, phase=True, extra=None, addresses=None):
        vals = {"f0": root_name + "." + fields[0], "f1": root_name + "." + fields[1]}
        if phase:
            vals["phase"] = phase_name
        vals.update(extra or {})
        adds = {"root": "&" + root_name,
                "c0": "&" + root_name + "." + fields[0],
                "c1": "&" + root_name + "." + fields[1]}
        adds.update(addresses or {})
        return emit(event, vals, adds)

    for did, items in definitions.items():
        decl = declarations.get(did)
        if decl is None or not is_scalar(decl):
            continue
        for offset, _, node, event in items:
            stmt = declaration_statement(node) if event == "bind" else node
            if event == "assign":
                require(parent.get(id(node), {}).get("kind") == "CompoundStmt",
                        "scalar assignment must be an ordinary statement")
            end = statement_end(stmt)
            add(end, "\n" + emit(event, {"value": decl["name"]}, ident=did) + "\n")
            events.append({"event": event, "id": did, "name": decl["name"],
                           "type": type_text(decl), "offset": offset,
                           "statement": source[bounds(stmt)[0]:end]})

    baseline_end = statement_end(declaration_statement(root_decl))
    for node in assignments:
        target = field_target(node["inner"][0])
        if target and target[0] == root_id and bounds(node)[0] < loop_start:
            baseline_end = max(baseline_end, statement_end(node))
    require(baseline_end < loop_start, "original must exist before loop")
    add(baseline_end, "\n" + state("B0", phase=False) + "\n")
    add(bounds(loop_body)[0] + 1, "\n" + state("header") + "\n")
    store_end = statement_end(store)
    rhs_start = bounds(store["inner"][1])[0]
    rhs = source[rhs_start:store_end - 1].strip()
    lhs = source[bounds(store)[0]:bounds(store["inner"][0])[1]].strip()
    pure_kinds = {"DeclRefExpr", "IntegerLiteral", "ImplicitCastExpr",
                  "CStyleCastExpr", "ParenExpr", "ConstantExpr"}
    require(all(n.get("kind") in pure_kinds and is_scalar(n)
                for n in descendants(store["inner"][1])),
            "incoming re-read must be a scalar carrier/literal with only harmless casts")
    require(parent.get(id(store), {}).get("kind") == "CompoundStmt",
            "actual store must be a full ordinary statement")
    add(store_start, "\n" + state("store_before", extra={"incoming": rhs},
                                  addresses={"destination": "&(" + lhs + ")"}) + "\n")
    add(store_end, "\n" + state("store_after") + "\n")
    for node, name in [(continues[0], "continue"), (breaks[0], "break")]:
        require(parent.get(id(node), {}).get("kind") == "CompoundStmt",
                "control transfer must have its real compound scope")
        extra = {"finished": result_name} if name == "break" else None
        add(bounds(node)[0], "\n" + state(name, extra=extra) + "\n")
    add(loop_end, "\n" + state("loop_exit", extra={"finished": result_name}) + "\n")
    post_end = max(statement_end(declaration_statement(r["node"]))
                   if r["event"] == "bind" else statement_end(r["node"])
                   for r in (post_current, post_sibling))
    add(post_end, "\n" + state("post_loads", extra={"finished": result_name}) + "\n")

    role_names = {
        "runtime_phase": runtime_phase,
        "prewrite": prewrite, "sibling_before": sibling_before,
        "old_load": old_load, "current": current, "sibling": sibling,
        "post_current": post_current, "post_sibling": post_sibling,
    }
    mapping = {
        "schema": "R1179_AST_DERIVED_PASSIVE_V2",
        "guest_function": guest_function, "reachable_call_edges": call_edges,
        "unique_physical_original": True,
        "control_shape": "nested_break" if nested_break else "continue_if_then_fallthrough_break",
        "incoming": {"kind": incoming_kind, "bindings": incoming_bindings},
        "finished_storage": "actual Break receiving carrier, re-read after loop",
        "inputs": ["unmodified C bytes", "Clang AST of those exact C bytes"],
        "oracle_input": False, "original": root_name, "original_id": root_id,
        "fields_in_declaration_order": fields, "selected_declaration_index": selected,
        "phase": phase_name, "phase_id": phase_id,
        "break_result_id": result_id, "break_result_name": declarations[result_id]["name"],
        "roles": {key: {"id": value["id"], "event": value["event"],
                        "offset": value["offset"]} for key, value in role_names.items()},
        "old_result_and_copy": [{"id": x["id"], "event": x["event"],
                                "offset": x["offset"]} for x in old_copies],
        "finished": [{"id": x["id"], "event": x["event"], "offset": x["offset"]}
                     for x in finished],
        "events": events,
        "store_source": source[store_start:store_end],
        "insertions": [{"offset": p, "text": "\n".join(v)}
                       for p, v in sorted(insertions.items())],
        "boundary": "Reads actual C carriers; not ghost/ownership/whole-loop validation",
    }
    passive = source
    for position, additions in sorted(insertions.items(), reverse=True):
        passive = passive[:position] + "\n".join(additions) + passive[position:]
    passive = "#include <stdio.h>\n" + passive
    Path(passive_path).write_text(passive, encoding="utf-8")
    Path(map_path).write_text(json.dumps(mapping, indent=2) + "\n", encoding="utf-8")
    Path(str(passive_path) + ".diff").write_text(
        "".join(difflib.unified_diff(source.splitlines(True), passive.splitlines(True),
                                    fromfile="original.c", tofile="passive.c")),
        encoding="utf-8")
    return mapping


if __name__ == "__main__":
    try:
        instrument(*sys.argv[1:5])
    except (ShapeError, KeyError, ValueError, IndexError) as error:
        print("R1179 MECHANISM_RECOGNIZER_STOP: " + str(error), file=sys.stderr)
        raise SystemExit(4) from error
