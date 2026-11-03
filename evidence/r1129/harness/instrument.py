#!/usr/bin/env python3
"""Passive instrumentation of this R cohort's own emitted C, never P fixtures."""
import json
import re


class ObservationLimit(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise ObservationLimit(message)


def strip(node):
    while node.get("kind") in (
        "ImplicitCastExpr", "ParenExpr",
        "ExprWithCleanups", "ConstantExpr",
    ) and len(node.get("inner", [])) == 1:
        node = node["inner"][0]
    return node


def children(node):
    yield node
    for child in node.get("inner", []):
        yield from children(child)


def offset(location):
    require(not any(key in location for key in
                    ("expansionLoc", "spellingLoc", "includedFrom")),
            "macro/header source location is unsupported")
    if "offset" in location:
        return location["offset"]
    raise ObservationLimit("missing direct C source offset")


def bounds(node, code):
    region = node.get("range", {})
    a = offset(region["begin"])
    end = region["end"]
    b = offset(end) + end.get("tokLen", 1)
    require(0 <= a < b <= len(code), "foreign/header C range")
    return a, b


def ctype(node):
    typ = node.get("type", {})
    return re.sub(r"\bconst\b", "", typ.get(
        "desugaredQualType", typ.get("qualType", "")
    )).strip()


def is_u8(typ):
    return " ".join(typ.split()) in ("uint8_t", "unsigned char")


def own_definition(node, code, source_path):
    if node.get("kind") != "FunctionDecl":
        return False
    if not any(x.get("kind") == "CompoundStmt" for x in node.get("inner", [])):
        return False
    try:
        if node.get("loc", {}).get("file", source_path) != source_path:
            return False
        at = offset(node["loc"])
        return code[at:at + len(node["name"])] == node["name"]
    except (KeyError, ObservationLimit):
        return False


def instrument(code, ast, mode, source_path):
    require(code.isascii(), "Clang byte offsets require ASCII emitted C here")
    require(mode in ("normal", "immediate_break"), "unknown observation mode")
    require(not re.search(r"\br1129_(state|phase|ret)\b", code),
            "observer name collision")
    nodes = {}
    parents = {}
    for node in children(ast):
        if "id" in node:
            nodes[node["id"]] = node
        for child in node.get("inner", []):
            if "id" in child:
                parents[child["id"]] = node
    definitions = [n for n in children(ast) if own_definition(n, code, source_path)]
    helpers = {}
    for fn in definitions:
        params = [n for n in fn.get("inner", [])
                  if n.get("kind") == "ParmVarDecl"]
        result = fn.get("type", {}).get("qualType", "").split("(", 1)[0].strip()
        if len(params) == 2 and is_u8(result):
            pointer = ctype(params[0])
            if pointer.endswith("*") and is_u8(pointer[:-1].strip()):
                if is_u8(ctype(params[1])):
                    helpers[fn["name"]] = (fn, params)
    require(helpers, "no passive single-replace helper surface")

    def callee(call):
        xs = call.get("inner", [])
        if not xs:
            return None
        first = strip(xs[0])
        return first.get("referencedDecl", {}).get("name")

    bodies = []
    for fn in definitions:
        body = next(n for n in fn["inner"] if n.get("kind") == "CompoundStmt")
        calls = [n for n in children(body)
                 if n.get("kind") == "CallExpr" and callee(n) in helpers]
        if len(calls) == 3:
            bodies.append((fn, body, sorted(calls, key=lambda n: bounds(n, code)[0])))
    require(len(bodies) == 1, "no unique three-operation caller body")
    owner, body, calls = bodies[0]
    require(len([n for n in children(body) if n.get("kind") == "CallExpr"]) == 3,
            "unmapped caller call could escape or mutate observed storage")
    require(not any(n.get("kind") in (
        "GotoStmt", "IndirectGotoStmt", "LabelStmt", "SwitchStmt",
        "CaseStmt", "DefaultStmt", "DoStmt",
    ) for n in children(body)), "unsupported caller control context")
    used_helpers = {callee(call) for call in calls}
    helpers = {name: pair for name, pair in helpers.items() if name in used_helpers}
    loops = [n for n in children(body) if n.get("kind") in ("ForStmt", "WhileStmt")]
    require(len(loops) <= 1, "multiple C loops")
    require(loops or mode == "immediate_break",
            "normal source has no observable emitted C loop")
    loop = loops[0] if loops else None
    start = bounds(loop if loop else calls[0], code)[0]
    require(all(bounds(call, code)[0] > start for call in calls)
            if loop else True, "call before source loop")
    if loop:
        la, lb = bounds(loop, code)
        require(la < bounds(calls[0], code)[0] < lb,
                "first helper call not within loop")
        require(all(bounds(c, code)[0] > lb for c in calls[1:]),
                "caller operations not after loop")
    aliases = set()

    def place(node):
        node = strip(node)
        kind = node.get("kind")
        if kind == "MemberExpr":
            require(not node.get("isArrow", False), "indirect record base")
            base = strip(node["inner"][0])
            require(base.get("kind") == "DeclRefExpr", "nonlocal record base")
            ident = base.get("referencedDecl", {}).get("id")
            require(ident in nodes, "missing root declaration")
            return ident, "." + node["name"], node.get("referencedMemberDecl")
        if kind == "ArraySubscriptExpr":
            base, index = (strip(x) for x in node["inner"][:2])
            require(base.get("kind") == "DeclRefExpr", "nonlocal array base")
            require(index.get("kind") == "IntegerLiteral", "dynamic field index")
            ident = base.get("referencedDecl", {}).get("id")
            require(ident in nodes, "missing array declaration")
            value = int(index["value"])
            require(value in (0, 1), "not a two-field array")
            return ident, "[" + str(value) + "]", value
        raise ObservationLimit("unsupported emitted field lvalue")

    def address(node, depth=0):
        require(depth < 16, "pointer alias chain limit")
        node = strip(node)
        if node.get("kind") == "UnaryOperator" and node.get("opcode") == "&":
            return place(node["inner"][0])
        if node.get("kind") == "DeclRefExpr":
            ident = node.get("referencedDecl", {}).get("id")
            declaration = nodes.get(ident, {})
            require(declaration.get("kind") == "VarDecl", "nonlocal pointer")
            require(declaration.get("init") and declaration.get("inner"),
                    "uninitialized pointer alias")
            aliases.add(ident)
            return address(declaration["inner"][-1], depth + 1)
        raise ObservationLimit("unmapped write argument")

    targets = [address(call["inner"][1]) for call in calls]
    original, checkpoint, checkpoint_id = targets[0]
    require(targets[1][0] == original, "second call has a foreign original")
    signal, signal_id = targets[1][1], targets[1][2]
    bystander, other_checkpoint, other_id = targets[2]
    require(bystander != original, "other original aliases first")
    require(signal != checkpoint, "caller overwrites same field")
    require(checkpoint == other_checkpoint and checkpoint_id == other_id,
            "other-original field mapping differs")
    root_a, root_b = nodes[original], nodes[bystander]
    require(root_a.get("kind") == root_b.get("kind") == "VarDecl",
            "root is not an actual emitted variable")
    require(ctype(root_a) == ctype(root_b), "different emitted record types")
    for root in (root_a, root_b):
        require(offset(root["loc"]) < start, "root created during/after loop")
        require(root.get("init"), "root has no complete declaration initializer")
    if isinstance(checkpoint_id, str):
        declaration = nodes.get(checkpoint_id)
        require(declaration, "missing emitted field declaration")
        record = parents.get(checkpoint_id)
        require(record and record.get("kind") == "RecordDecl",
                "missing record layout")
        fields = [n for n in record.get("inner", [])
                  if n.get("kind") == "FieldDecl"]
        require(len(fields) == 2 and all(is_u8(ctype(f)) for f in fields),
                "not a two-u8 emitted record")
        require({f["id"] for f in fields} == {signal_id, checkpoint_id},
                "unmapped sibling field")
    else:
        require(re.sub(r"\s+", "", ctype(root_a)) in
                ("uint8_t[2]", "unsignedchar[2]"), "not exactly emitted u8[2]")
    for n in children(body):
        if n.get("kind") == "BinaryOperator" and n.get("opcode") == "=":
            lhs = strip(n["inner"][0])
            if lhs.get("kind") == "DeclRefExpr":
                ident = lhs.get("referencedDecl", {}).get("id")
                require(ident not in aliases, "write-reference alias reassigned")
                require(ident not in (original, bystander),
                        "whole original storage reassigned")

    same_type_roots = [n for n in children(body)
                       if n.get("kind") == "VarDecl" and ctype(n) == ctype(root_a)]
    require({n["id"] for n in same_type_roots} == {original, bystander},
            "additional same-type storage prevents a unique original map")
    for root in (root_a, root_b):
        declaration = parents.get(root["id"], {})
        require(declaration.get("kind") == "DeclStmt"
                and parents.get(declaration.get("id")) is body,
                "observed root is not in the caller's enclosing lexical block")
        require(sum(n.get("name") == root["name"] and n.get("kind") in
                    ("VarDecl", "ParmVarDecl") for n in children(owner)) == 1,
                "observed root name is shadowed")
    for n in children(body):
        if n.get("kind") != "DeclRefExpr":
            continue
        ident = n.get("referencedDecl", {}).get("id")
        if ident not in aliases:
            continue
        current = n
        parent = parents.get(current.get("id"), {})
        while parent.get("kind") in ("ImplicitCastExpr", "ParenExpr"):
            current, parent = parent, parents.get(parent.get("id"), {})
        admitted_call = (parent.get("kind") == "CallExpr" and parent in calls
                         and len(parent.get("inner", [])) > 1
                         and parent["inner"][1] is current)
        admitted_chain = parent.get("kind") == "VarDecl" and parent.get("id") in aliases
        admitted_discard = (parent.get("kind") == "CStyleCastExpr"
                            and ctype(parent) == "void")
        require(admitted_call or admitted_chain or admitted_discard,
                "pointer alias is mutated, indirectly accessed, or escapes")
    for n in children(body):
        if n.get("kind") == "UnaryOperator" and n.get("opcode") == "&":
            operand = strip(n.get("inner", [{}])[0])
            require(not (operand.get("kind") == "DeclRefExpr"
                         and operand.get("referencedDecl", {}).get("id")
                         in (original, bystander)),
                    "whole original address escapes finite mapping")

    a, b = root_a["name"], root_b["name"]
    expressions = [
        "&" + a, "&" + a + signal, "&" + a + checkpoint,
        "(unsigned)" + a + signal, "(unsigned)" + a + checkpoint,
        "&" + b, "&" + b + signal, "&" + b + checkpoint,
        "(unsigned)" + b + signal, "(unsigned)" + b + checkpoint,
    ]
    edits = {}

    def add(at, text):
        edits.setdefault(at, []).append("\n" + text + "\n")

    def state(tag):
        return 'r1129_state("' + tag + '", ' + ", ".join(expressions) + ");"

    def statement(node):
        current = node
        while True:
            parent = parents.get(current.get("id"))
            require(parent, "statement parent missing")
            if parent.get("kind") == "CompoundStmt":
                return current
            current = parent

    def statement_end(node):
        _, end = bounds(statement(node), code)
        if code[end - 1] != ";":
            cursor = end
            while cursor < len(code) and code[cursor].isspace():
                cursor += 1
            if cursor < len(code) and code[cursor] == ";":
                end = cursor + 1
        return end

    def compound_child(node, label):
        require(parents.get(node.get("id"), {}).get("kind") == "CompoundStmt",
                label + " is not a direct compound-statement child")

    insertion_start = start if loop else bounds(statement(calls[0]), code)[0]
    for root in (root_a, root_b):
        require(bounds(parents[root["id"]], code)[1] <= insertion_start,
                "storage snapshot would precede complete root initialization")
    if loop:
        compound_child(loop, "loop")
    for call in calls:
        stmt = statement(call)
        require(stmt.get("kind") in ("DeclStmt", "BinaryOperator", "CallExpr",
                                     "CStyleCastExpr"),
                "helper call has unsupported/unbraced statement context")
        require(len([n for n in children(stmt) if n.get("kind") == "CallExpr"]) == 1,
                "multiple calls in one enclosing statement")
        if stmt.get("kind") == "BinaryOperator":
            require(stmt.get("opcode") == "=", "helper result in effectful expression")
        if stmt.get("kind") == "CStyleCastExpr":
            require(ctype(stmt) == "void", "unsupported helper expression statement")
    add(insertion_start, state("before_loop"))
    for index, call in enumerate(calls):
        add(statement_end(call), state("after_call_" + str(index)))
    phase_expression = None
    if loop:
        conditionals = [n for n in children(loop) if n.get("kind") == "IfStmt"]
        require(len(conditionals) == 1, "unrecognized/additional emitted IF")

        def comparison(node):
            node = strip(node)
            if node.get("kind") == "DeclRefExpr":
                declaration = nodes.get(node.get("referencedDecl", {}).get("id"), {})
                require(declaration.get("init") and declaration.get("inner"),
                        "condition variable lacks initializer")
                return comparison(declaration["inner"][-1])
            if node.get("kind") == "BinaryOperator" and node.get("opcode") in ("==", "!="):
                operands = [strip(x) for x in node["inner"][:2]]
                dynamic = [x for x in operands
                           if any(y.get("kind") == "DeclRefExpr" for y in children(x))]
                require(len(dynamic) == 1, "comparison has no unique carried operand")
                literal = next(x for x in operands if x is not dynamic[0])
                while literal.get("kind") in (
                    "CStyleCastExpr", "ImplicitCastExpr", "ParenExpr",
                    "ConstantExpr",
                ):
                    require(len(literal.get("inner", [])) == 1,
                            "unsupported integer-literal wrapper")
                    if literal.get("kind") in ("CStyleCastExpr", "ImplicitCastExpr"):
                        require(literal.get("castKind") in ("NoOp", "IntegralCast"),
                                "comparison literal cast is not a pure integer cast")
                    literal = literal["inner"][0]
                require(literal.get("kind") == "IntegerLiteral",
                        "non-phase operand is not a pure integer literal/cast")
                require(all(y.get("kind") not in ("CallExpr", "BinaryOperator",
                                                "CompoundAssignOperator")
                            for y in children(dynamic[0])),
                        "phase observation would repeat an effect")
                for y in children(dynamic[0]):
                    if y.get("kind") == "UnaryOperator":
                        require(y.get("opcode") not in ("++", "--"),
                                "phase observation changes guest state")
                operand = dynamic[0]
                require(operand.get("kind") == "DeclRefExpr",
                        "phase must be a plain scalar, never volatile/pointer read")
                ident = operand.get("referencedDecl", {}).get("id")
                declaration = nodes.get(ident, {})
                require(declaration.get("kind") == "VarDecl"
                        and ctype(declaration) in (
                            "size_t", "uintptr_t", "unsigned int",
                            "unsigned long", "unsigned long long",
                            "unsigned long int", "unsigned long long int",
                        ), "phase is not a live ordinary unsigned scalar")
                require(sum(n.get("name") == declaration.get("name")
                            and n.get("kind") in ("VarDecl", "ParmVarDecl")
                            for n in children(owner)) == 1, "phase name shadowed")
                scope = declaration
                while scope.get("kind") not in ("CompoundStmt", "ForStmt"):
                    scope = parents.get(scope.get("id"), {})
                    require(scope, "phase lexical scope missing")
                ca, cb = bounds(conditionals[0], code)
                sa, sb = bounds(scope, code)
                require(sa <= ca < cb <= sb
                        and offset(declaration["loc"]) < ca,
                        "phase not live at observation point")
                # Ordering needs only a direct begin, never a macro-end slice.
                comparison_at = offset(node["range"]["begin"])
                for use in children(body):
                    if use.get("kind") == "UnaryOperator" and use.get("opcode") == "&":
                        target = strip(use.get("inner", [{}])[0])
                        require(target.get("referencedDecl", {}).get("id") != ident,
                                "phase scalar address escapes")
                    if use.get("kind") in ("BinaryOperator", "CompoundAssignOperator",
                                            "UnaryOperator", "CallExpr"):
                        # Preserve strict direct locations at insertion/slice sites.
                        ua = offset(use["range"]["begin"])
                        if comparison_at <= ua < ca:
                            require(use is node or use.get("kind") == "BinaryOperator"
                                    and use.get("opcode") in ("==", "!="),
                                    "effects between comparison and phase observation")
                begin, finish = bounds(operand, code)
                return code[begin:finish]
            raise ObservationLimit("condition not a scalar comparison")

        chosen = []
        for conditional in conditionals:
            try:
                chosen.append((conditional, comparison(conditional["inner"][0])))
            except ObservationLimit:
                pass
        require(len(chosen) == 1, "no unique phase-comparison IF")
        conditional, phase_expression = chosen[0]
        compound_child(conditional, "phase IF")
        at = bounds(conditional, code)[0]
        add(at, 'r1129_phase((unsigned long long)(' + phase_expression + "));")
        add(at, state("header"))
        continues = [n for n in children(loop) if n.get("kind") == "ContinueStmt"]
        breaks = [n for n in children(loop) if n.get("kind") == "BreakStmt"]
        require(len(continues) == len(breaks) == 1,
                "emitted control sites not C1+B1")
        compound_child(continues[0], "Continue")
        compound_child(breaks[0], "Break")
        add(bounds(continues[0], code)[0], state("continue"))
        add(bounds(breaks[0], code)[0], state("break"))
        add(bounds(loop, code)[1], state("after_loop"))
    else:
        add(statement_end(calls[0]), state("after_loop"))

    for name, (fn, params) in helpers.items():
        helper_body = next(n for n in fn["inner"] if n.get("kind") == "CompoundStmt")
        locals_ = [n for n in children(helper_body) if n.get("kind") == "VarDecl"]
        require(all(is_u8(ctype(n)) for n in locals_), "helper has pointer/foreign locals")
        allowed_ids = {n["id"] for n in params + locals_}
        parameter_ids = {n["id"] for n in params}
        pointer_id = params[0]["id"]
        require(not any(n.get("kind") in (
            "CallExpr", "IfStmt", "ForStmt", "WhileStmt", "DoStmt", "SwitchStmt",
            "GotoStmt", "IndirectGotoStmt", "LabelStmt",
        ) for n in children(helper_body)), "helper effects/lifetime not finite straight-line")
        pointer_stores = 0
        for n in children(helper_body):
            kind = n.get("kind")
            if kind == "CompoundStmt":
                require(n is helper_body, "nested helper scope/shadowing")
            if kind == "DeclRefExpr":
                require(n.get("referencedDecl", {}).get("id") in allowed_ids,
                        "helper observes foreign state")
            if kind == "UnaryOperator":
                require(n.get("opcode") != "&", "helper address escape")
                if n.get("opcode") == "*":
                    pointee = strip(n["inner"][0])
                    require(pointee.get("kind") == "DeclRefExpr"
                            and pointee.get("referencedDecl", {}).get("id") == pointer_id,
                            "helper reads an unproved pointer")
            mutation = (kind == "CompoundAssignOperator"
                        or kind == "BinaryOperator" and n.get("opcode") == "="
                        or kind == "UnaryOperator" and n.get("opcode") in ("++", "--"))
            if mutation:
                lhs = strip(n["inner"][0])
                if lhs.get("kind") == "DeclRefExpr":
                    require(lhs.get("referencedDecl", {}).get("id")
                            in allowed_ids.difference(parameter_ids),
                            "helper parameter mutation/lifetime mapping unsupported")
                else:
                    require(kind == "BinaryOperator" and n.get("opcode") == "="
                            and lhs.get("kind") == "UnaryOperator"
                            and lhs.get("opcode") == "*",
                            "unsupported helper indirect mutation")
                    target = strip(lhs["inner"][0])
                    require(target.get("referencedDecl", {}).get("id") == pointer_id,
                            "helper store is not through its stable destination")
                    pointer_stores += 1
        require(pointer_stores <= 1, "multiple hidden destination writes in helper")
        returns = [n for n in children(fn) if n.get("kind") == "ReturnStmt"]
        require(len(returns) == 1, "helper requires one actual return")
        for returned in returns:
            compound_child(returned, "helper Return")
            require(len(returned.get("inner", [])) == 1,
                    "helper return is not a value")
            value = strip(returned["inner"][0])
            require(value.get("kind") == "DeclRefExpr" and is_u8(ctype(value)),
                    "passive old-Copy return observation unavailable")
            x, y = bounds(value, code)
            old = code[x:y]
            destination, replacement = params[0]["name"], params[1]["name"]
            add(bounds(returned, code)[0],
                'r1129_ret("' + name + '", ' + destination + ", (unsigned)" +
                replacement + ", (unsigned)" + old + ", (unsigned)*" +
                destination + ");")

    prefix = r'''#include <stdio.h>
#include <stdint.h>
static void r1129_phase(unsigned long long phase)
{
    printf("R1129_TRACE {\"event\":\"phase\",\"value\":%llu}\n", phase);
}
static void r1129_ret(const char *name, const void *destination,
                       unsigned replacement, unsigned old, unsigned current)
{
    printf("R1129_TRACE {\"event\":\"call\",\"function\":\"%s\","
           "\"destination\":\"%p\",\"new\":%u,\"old\":%u,\"current\":%u}\n",
           name, destination, replacement, old, current);
}
static void r1129_state(const char *tag,
                       const void *a, const void *as, const void *ac,
                       unsigned avs, unsigned avc,
                       const void *b, const void *bs, const void *bc,
                       unsigned bvs, unsigned bvc)
{
    printf("R1129_TRACE {\"event\":\"state\",\"tag\":\"%s\","
           "\"original\":\"%p\",\"original_signal\":\"%p\","
           "\"original_checkpoint\":\"%p\",\"signal\":%u,\"checkpoint\":%u,"
           "\"bystander\":\"%p\",\"bystander_signal\":\"%p\","
           "\"bystander_checkpoint\":\"%p\",\"other_signal\":%u,"
           "\"other_checkpoint\":%u}\n",
           tag, a, as, ac, avs, avc, b, bs, bc, bvs, bvc);
}
'''
    if not loop:
        prefix = prefix.replace(
            'static void r1129_phase(unsigned long long phase)\n'
            '{\n'
            '    printf("R1129_TRACE {\\"event\\":\\"phase\\",\\"value\\":%llu}\\n", phase);\n'
            '}\n', ""
        )
    observed = code
    for at in sorted(edits, reverse=True):
        observed = observed[:at] + "".join(edits[at]) + observed[at:]
    mapping = {
        "mapping": "finite emitted-C effect graph; not a public issuer proof",
        "owner_function": owner["name"],
        "original_variable": a,
        "bystander_variable": b,
        "signal_member": signal,
        "checkpoint_member": checkpoint,
        "phase_expression": phase_expression,
        "emitted_loop": bool(loop),
        "helper_functions": sorted(helpers),
        "insertions": [{"offset": k, "text": "".join(edits[k])}
                       for k in sorted(edits)],
        "limits": [
            "Mapping follows actual write arguments and pre-loop storage, not values.",
            "No source-to-C public storage-map validator is claimed.",
            "Instrumented addresses are separate from unmodified-C execution.",
            "Unsupported syntax fails before instrumented compilation; no guessed mapping.",
        ],
    }
    return prefix + observed, mapping
