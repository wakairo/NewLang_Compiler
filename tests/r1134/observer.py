"""R1134 passive mapping of its own emitted C through Clang's syntax tree."""
import re

def require(condition, message):
    if not condition:
        raise AssertionError("equipment mapping: " + message)

def one(items, label):
    require(len(items) == 1, "expected one " + label + ", got " + str(len(items)))
    return items[0]

def walk(node):
    yield node
    for child in node.get("inner", []):
        yield from walk(child)

def strip(node):
    while node.get("kind") in ("ImplicitCastExpr", "ParenExpr", "ConstantExpr",
                               "ExprWithCleanups"):
        inner = node.get("inner", [])
        require(len(inner) == 1, "non-unary scalar wrapper")
        node = inner[0]
    return node

def ctype(node):
    info = node.get("type", {})
    value = info.get("desugaredQualType", info.get("qualType", ""))
    return " ".join(re.sub(r"\bconst\b", "", value).split())

def u8(node):
    return ctype(node) in ("uint8_t", "unsigned char")

def offset(location):
    require(not any(key in location for key in
                    ("expansionLoc", "spellingLoc", "includedFrom")),
            "non-direct source location")
    require("offset" in location, "missing source offset")
    return location["offset"]

def bounds(node, code):
    region = node.get("range", {})
    require("begin" in region and "end" in region, "missing syntax range")
    begin = offset(region["begin"])
    end = offset(region["end"]) + region["end"].get("tokLen", 1)
    require(0 <= begin < end <= len(code), "foreign or invalid syntax range")
    return begin, end

def observe_c(code, ast, owned, case, source_path):
    require(code.isascii(), "exact AST byte offsets require ASCII C")
    public = {}
    for match in re.finditer(
            r"(?m)^R1134 NAME main \d+ (\d+) \d+ ([A-Za-z_]\w*)$", owned):
        public.setdefault(match[2], set()).add(match[1])
    needed = ["original", "remote"] + list(case["expected_locals"])
    names = {name: "nl_local_" + one(list(public.get(name, set())),
                                   "public source symbol " + name)
             for name in needed}
    nodes = {}
    parents = {}
    for node in walk(ast):
        if "id" in node:
            nodes[node["id"]] = node
        for child in node.get("inner", []):
            if "id" in child:
                parents[child["id"]] = node

    def parent(node):
        return parents.get(node.get("id"), {})

    def own_definition(node):
        if node.get("kind") != "FunctionDecl":
            return False
        if not any(n.get("kind") == "CompoundStmt" for n in node.get("inner", [])):
            return False
        try:
            location = node.get("loc", {})
            if location.get("file", source_path) != source_path:
                return False
            at = offset(location)
            name = node["name"]
            return code[at:at + len(name)] == name
        except (AssertionError, KeyError):
            return False

    definitions = [node for node in walk(ast) if own_definition(node)]
    owners = []
    for function in definitions:
        body = one([n for n in function["inner"]
                    if n.get("kind") == "CompoundStmt"], "function body")
        variables = [n for n in walk(body) if n.get("kind") == "VarDecl"]
        if all(any(n.get("name") == names[key] and
                   "nl_type_" in n.get("type", {}).get("qualType", "")
                   for n in variables) for key in ("original", "remote")):
            owners.append((function, body, variables))
    owner, body, variables = one(owners, "source original/remote caller")
    declarations = {
        name: one([n for n in variables if n.get("name") == cname],
                  "actual unshadowed variable " + name)
        for name, cname in names.items()
    }
    root_a, root_b = declarations["original"], declarations["remote"]
    require(root_a.get("init") and root_b.get("init"), "uninitialized roots")
    require(ctype(root_a) == ctype(root_b), "different original record types")
    root_a_stmt, root_b_stmt = parent(root_a), parent(root_b)
    require(root_a_stmt.get("kind") == root_b_stmt.get("kind") == "DeclStmt",
            "roots are not complete declarations")
    block = parent(root_a_stmt)
    require(block.get("kind") == "CompoundStmt" and parent(root_b_stmt) is block,
            "roots do not share the original lexical block")

    def statement_end(statement):
        _, end = bounds(statement, code)
        if code[end - 1] != ";":
            cursor = end
            while cursor < len(code) and code[cursor].isspace():
                cursor += 1
            if cursor < len(code) and code[cursor] == ";":
                end = cursor + 1
        return end

    loops = [n for n in walk(block) if n.get("kind") in ("ForStmt", "WhileStmt")]
    loop = one(loops, "actual generated loop")
    loop_begin, loop_end = bounds(loop, code)
    require(parent(loop) is block, "loop is not a direct compound statement")
    require(statement_end(root_a_stmt) < statement_end(root_b_stmt) <= loop_begin,
            "roots are not initialized before the loop")
    require(not any(n.get("kind") in ("GotoStmt", "IndirectGotoStmt", "LabelStmt",
                                     "SwitchStmt", "CaseStmt", "DefaultStmt",
                                     "DoStmt", "ReturnStmt") for n in walk(loop)),
            "unsupported loop control context")
    conditional = one([n for n in walk(loop) if n.get("kind") == "IfStmt"],
                      "guarded loop IF")
    require(parent(conditional).get("kind") == "CompoundStmt",
            "guard IF has an unbraced statement context")
    condition = strip(conditional["inner"][0])
    seen = set()
    logical_not_count = 0
    while True:
        if condition.get("kind") == "DeclRefExpr":
            ident = condition.get("referencedDecl", {}).get("id")
            require(ident not in seen and len(seen) < 16,
                    "cyclic or excessive condition alias")
            seen.add(ident)
            declaration = nodes.get(ident, {})
            require(declaration.get("kind") == "VarDecl" and declaration.get("init")
                    and declaration.get("inner"), "condition alias has no initializer")
            condition = strip(declaration["inner"][-1])
        elif condition.get("kind") == "UnaryOperator" and \
                condition.get("opcode") == "!":
            require(logical_not_count == 0 and len(condition.get("inner", [])) == 1,
                    "unsupported repeated logical-not guard wrapper")
            logical_not_count += 1
            condition = strip(condition["inner"][0])
        else:
            break
    require(condition.get("kind") == "BinaryOperator" and
            condition.get("opcode") == "<", "guard is not the actual less-than")
    operands = condition.get("inner", [])
    require(len(operands) == 2, "guard has wrong operand count")
    carried = strip(operands[0])
    require(carried.get("kind") == "DeclRefExpr", "carry is not a plain scalar")
    carry_id = carried.get("referencedDecl", {}).get("id")
    carry_decl = nodes.get(carry_id, {})
    require(carry_decl.get("kind") == "VarDecl" and ctype(carry_decl) in (
        "size_t", "uintptr_t", "unsigned int", "unsigned long",
        "unsigned long long", "unsigned long int", "unsigned long long int"),
        "carry is not a live ordinary unsigned scalar")
    carry = carry_decl.get("name")
    require(carry and sum(n.get("name") == carry for n in variables) == 1,
            "carry name absent or shadowed")
    guard_at = bounds(conditional, code)[0]
    require(offset(carry_decl["loc"]) < guard_at, "carry declared after guard")
    literal = strip(operands[1])
    while literal.get("kind") == "CStyleCastExpr":
        require(literal.get("castKind") in ("NoOp", "IntegralCast")
                and len(literal.get("inner", [])) == 1, "effectful guard literal")
        literal = strip(literal["inner"][0])
    require(literal.get("kind") == "IntegerLiteral", "guard bound is not literal")
    continued = one([n for n in walk(loop) if n.get("kind") == "ContinueStmt"],
                    "actual Continue")
    broken = one([n for n in walk(loop) if n.get("kind") == "BreakStmt"],
                 "actual Break")
    require(parent(continued).get("kind") == "CompoundStmt" and
            parent(broken).get("kind") == "CompoundStmt",
            "edge observer requires a verified braced statement context")

    def frame(tag, carry_name=None):
        a, b = names["original"], names["remote"]
        fmt = "R1134 " + tag
        args = []
        if carry_name is not None:
            fmt += " %zu"
            args.append("(size_t)" + carry_name)
        fmt += " %p %p %p %zu %p %p %p %zu %u %u %u %u\\n"
        args.extend([
            "(void *)&" + a, "(void *)&" + a + ".f0", "(void *)&" + a + ".f1",
            "sizeof(" + a + ")", "(void *)&" + b, "(void *)&" + b + ".f0",
            "(void *)&" + b + ".f1", "sizeof(" + b + ")",
            "(unsigned)" + a + ".f0", "(unsigned)" + a + ".f1",
            "(unsigned)" + b + ".f0", "(unsigned)" + b + ".f1"])
        return '\nprintf("' + fmt + '", ' + ", ".join(args) + ");\n"

    edits = [(statement_end(root_b_stmt), frame("INIT")),
             (guard_at, frame("HEADER", carry)),
             (bounds(continued, code)[0], frame("CONTINUE")),
             (bounds(broken, code)[0], frame("BREAK"))]
    local_points = {}
    separate_loop_assignment = None
    for name in case["expected_locals"]:
        declaration = declarations[name]
        require(u8(declaration), name + " is not an actual u8 scalar")
        statement = parent(declaration)
        require(statement.get("kind") == "DeclStmt" and parent(statement) is block,
                name + " is not in the original enclosing lexical block")
        at = statement_end(statement)
        if name == "loop_old" and at <= loop_begin:
            writes = []
            for node in walk(body):
                if node.get("kind") == "BinaryOperator" and node.get("opcode") == "=":
                    target = strip(node.get("inner", [{}])[0])
                    if target.get("kind") == "DeclRefExpr" and \
                            target.get("referencedDecl", {}).get("id") == declaration["id"]:
                        writes.append(node)
            assigned = one(writes, "actual B assignment to loop result")
            require(loop_begin < bounds(assigned, code)[0] <
                    bounds(broken, code)[0] < loop_end,
                    "loop result assignment is not before actual B")
            require(parent(assigned).get("kind") == "CompoundStmt",
                    "loop result assignment has an effectful expression context")
            current = assigned
            break_block = parent(broken)
            while parent(current) is not break_block:
                current = parent(current)
                require(current and current.get("kind") == "CompoundStmt",
                        "loop result assignment does not dominate B unconditionally")
            require(bounds(current, code)[1] <= bounds(broken, code)[0],
                    "loop result assignment does not precede B")
            separate_loop_assignment = {
                "variable": declaration["name"],
                "declaration": list(bounds(statement, code)),
                "actual_assignment": list(bounds(assigned, code)),
                "actual_break": list(bounds(broken, code)),
                "observation_after_normal_loop_exit": loop_end,
            }
            at = loop_end
        else:
            require(declaration.get("init") and declaration.get("inner"),
                    name + " has no definite initializer")
            if name == "loop_old":
                require(bounds(statement, code)[0] >= loop_end,
                        "loop result initializer is not after normal loop exit")
        local_points[name] = at
        edits.append((at, '\nprintf("R1134 LOCAL ' + name +
                      ' %u\\n", (unsigned)' + names[name] + ");\n"))
    edits.append((local_points["preserved_old"], frame("FINAL")))

    helpers = {}
    for function in definitions:
        params = [n for n in function.get("inner", [])
                  if n.get("kind") == "ParmVarDecl"]
        returned = function.get("type", {}).get("qualType", "").split("(", 1)[0].strip()
        if len(params) == 2 and returned in ("uint8_t", "unsigned char"):
            pointer = ctype(params[0])
            if pointer.endswith("*") and pointer[:-1].strip() in \
                    ("uint8_t", "unsigned char") and u8(params[1]):
                helpers[function["name"]] = (function, params)
    require(helpers, "no actual Copy-returning write helper")

    def callee(call):
        target = strip(call.get("inner", [{}])[0])
        return target.get("referencedDecl", {}).get("name")

    calls = [n for n in walk(body)
             if n.get("kind") == "CallExpr" and callee(n) in helpers]
    require(len(calls) == 2, "expected two actual ordinary helper calls")
    calls.sort(key=lambda node: bounds(node, code)[0])
    require(loop_begin < bounds(calls[0], code)[0] < loop_end <
            bounds(calls[1], code)[0], "helper calls are not at B then caller")
    used_helpers = sorted({callee(call) for call in calls})
    for name in used_helpers:
        function, params = helpers[name]
        helper_body = one([n for n in function["inner"]
                           if n.get("kind") == "CompoundStmt"], "helper body")
        opening = bounds(helper_body, code)[0]
        require(code[opening] == "{", "helper body opening is not direct")
        pointer, incoming = params[0]["name"], params[1]["name"]
        edits.append((opening + 1,
                      '\nprintf("R1134 HELPER %p %u %u\\n", (void *)' +
                      pointer + ", (unsigned)*" + pointer + ", (unsigned)" +
                      incoming + ");\n"))
    observed = code
    for at, insertion in sorted(edits, key=lambda item: item[0], reverse=True):
        observed = observed[:at] + insertion + observed[at:]
    metadata = {
        "public_symbols": names, "actual_carry": carry,
        "actual_guard_logical_not_count": logical_not_count,
        "caller_function": owner["name"], "actual_helper_functions": used_helpers,
        "separate_loop_result_assignment": separate_loop_assignment,
        "insertions": [{"offset": at, "text": insertion} for at, insertion in edits],
        "passive_only": True, "edge_bracing": "AST_VERIFIED",
        "mapping_scope": "Public source names plus actual emitted-C lexical declarations; no private issuer proof or cross-world authority comparison.",
    }
    return "#include <stdio.h>\n" + observed, metadata
