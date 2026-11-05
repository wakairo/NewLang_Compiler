"""Finite linkage of captured direct u8 calls to their single ternary helper.

No oracle/case name is accepted. This module never executes C or creates ASTs.
The only branch inference is an LT/EQ on captured scalar operands in an exact
same-AST, single-conditional, side-effect-free normal-return helper. A caller
call-result VALUE is the result observation; RETURN_VALUE is kept separately.
"""


def decode_direct_helper(decoder, nodes, incomplete, mismatch, one, u8_type, usize_type):
    d = decoder
    if not d.model.get("actual_ast_linkage"):
        raise incomplete("direct helper requires verified same-captured-AST declaration linkage")
    call = one((n for n in nodes if n["kind"] == "CallExpr" and u8_type(n["type"])),
               "unique direct u8 call in the measured caller region")
    if len(call["children"]) != 2:
        raise incomplete("finite helper call requires one actual argument")
    reference = d.unwrap(call["children"][0])
    if reference["kind"] != "DeclRefExpr" or not reference.get("ref_function"):
        raise incomplete("callee is not an actual AST-linked function declaration")
    function = one((f for f in d.model["functions"] if f["id"] == reference["ref_function"]),
                   "callee definition reached by actual declaration identity")
    if function["id"] == call["function"] or not u8_type(function["return_type"].split(" (")[0]):
        raise incomplete("recursive/non-u8 callee is outside finite helper profile")
    definition = d.nodes[function["definition_node"]]
    callee_nodes = list(d.descendants(definition))
    parameters = [k for k, v in d.variables.items()
                  if v["function"] == function["id"] and v.get("parameter")]
    formal = one(parameters, "one actual helper formal")
    if not usize_type(d.variables[formal]["type"]):
        raise incomplete("actual helper formal is not usize")
    argument = d.unwrap(call["children"][1])
    if (argument["kind"] != "DeclRefExpr" or not argument.get("ref_decl")
            or not usize_type(d.variables[argument["ref_decl"]]["type"])):
        raise incomplete("finite actual argument must be a direct initialized usize read")
    result_node = d.nodes.get(call.get("parent"))
    while result_node and result_node["kind"] in ("ImplicitCastExpr", "CStyleCastExpr", "ParenExpr"):
        result_node = d.nodes.get(result_node.get("parent"))
    if result_node is None or result_node["kind"] != "VarDecl":
        raise incomplete("direct call is not an entire initialized declaration")
    result = one((k for k, v in d.variables.items() if v["node"] == result_node["id"]),
                 "actual caller call-result declaration")
    declaration = d.nodes.get(result_node.get("parent"))
    if (result not in d.initializers or result in d.assignments
            or d.unwrap(d.initializers[result])["id"] != call["id"]
            or declaration is None or declaration["kind"] != "DeclStmt"
            or declaration["children"] != [result_node["id"]]
            or d.nodes[declaration["parent"]]["kind"] != "CompoundStmt"):
        raise incomplete("call-result is not the finite immediate declaration observation")
    conditional = one((n for n in callee_nodes if n["kind"] == "ConditionalOperator"),
                      "single actual helper ConditionalOperator")
    returned = one((n for n in callee_nodes if n["kind"] == "ReturnStmt"),
                   "single actual helper normal return")
    if len(returned["children"]) != 1 or len(conditional["children"]) != 3:
        raise incomplete("helper normal return/conditional expression is incomplete")
    forbidden = ("CallExpr", "IfStmt", "WhileStmt", "ForStmt", "DoStmt", "SwitchStmt",
                 "GotoStmt", "IndirectGotoStmt", "CompoundAssignOperator", "UnaryOperator")
    if any(n["kind"] in forbidden or (n["kind"] == "BinaryOperator" and n.get("opcode") not in ("<", "=="))
           for n in callee_nodes):
        raise incomplete("helper contains operations outside captured finite pure ternary profile")
    if conditional["id"] not in {n["id"] for n in d.expanded(returned["children"][0])}:
        raise incomplete("normal return is not linked through immutable copies to the conditional")
    comparison = d.unwrap(conditional["children"][0])
    if (comparison["kind"] != "BinaryOperator" or comparison.get("opcode") not in ("<", "==")
            or len(comparison["children"]) != 2
            or sum(n["kind"] == "BinaryOperator" for n in callee_nodes) != 1):
        raise incomplete("helper predicate is not its unique LT/EQ comparison")

    def copy_leaf(node, seen=None):
        seen = set() if seen is None else seen
        node = d.unwrap(node)
        if node["kind"] == "IntegerLiteral":
            return ("literal", int(node["value"]))
        if node["kind"] != "DeclRefExpr" or not node.get("ref_decl"):
            raise incomplete("predicate operand is not a finite scalar Copy/literal")
        decl = node["ref_decl"]
        if decl == formal:
            return ("formal", decl)
        if decl in seen or decl in d.assignments or decl not in d.initializers:
            raise incomplete("predicate Copy is mutable/cyclic/uninitialized")
        if d.variables[decl]["function"] != function["id"] or not usize_type(d.variables[decl]["type"]):
            raise incomplete("predicate Copy crosses the actual callee usize scope")
        seen.add(decl)
        return copy_leaf(d.initializers[decl], seen)

    if copy_leaf(comparison["children"][0]) != ("formal", formal):
        raise incomplete("helper left operand is not an immutable formal Copy path")
    if copy_leaf(comparison["children"][1])[0] != "literal":
        raise incomplete("helper right operand is not a fixed scalar literal Copy path")
    arms = [d.unwrap(x) for x in conditional["children"][1:]]
    if any(x["kind"] != "IntegerLiteral" or not 0 <= int(x["value"]) <= 255 for x in arms):
        raise incomplete("helper arms are not two actual u8 literals")
    calls = d.matching("CALL_SITE", call["id"])
    entries = d.matching("FUNCTION_ENTRY", definition["id"])
    returns = d.matching("RETURN", returned["id"])
    results = d.matching("DECLARATION", declaration["id"])
    if not (len(calls) == len(entries) == len(returns) == len(results)):
        raise incomplete("actual call/entry/normal-return/caller-result counts differ")
    arguments, formals, choices, values, observations = [], [], [], [], []
    prior_result = -1

    def captured_operand(node, event, entry):
        node = d.unwrap(node)
        if node["kind"] == "IntegerLiteral":
            return int(node["value"]), "actual AST literal"
        if node["kind"] != "DeclRefExpr" or not node.get("ref_decl"):
            raise incomplete("unrecognized predicate operand at captured return")
        decl = node["ref_decl"]
        if decl not in event["state"] or event["state"][decl]["event"] < entry["sequence"]:
            raise incomplete("predicate operand was not captured in this invocation")
        return d.value(event, decl), "captured VALUE for " + decl

    for site, entry, ret, observed_result in zip(calls, entries, returns, results):
        if not (prior_result < site["sequence"] < entry["sequence"] < ret["sequence"] < observed_result["sequence"]):
            raise incomplete("call/entry/return/result events are not uniquely ordered")
        prior_result = observed_result["sequence"]
        actual_row = one(d.data(site, "CALL_ARGUMENT"), "actual one-argument call observation")
        if actual_row[2] != "0":
            raise incomplete("actual argument index is not zero")
        formal_row = one((r for r in d.data(entry, "VALUE") if r[2] == formal),
                         "direct formal observation at this actual function entry")
        result_row = one((r for r in d.data(observed_result, "VALUE") if r[2] == result),
                         "direct caller call-result observation after normal return")
        actual_value, formal_value, result_value = int(actual_row[3]), int(formal_row[3]), int(result_row[3])
        if actual_value != formal_value:
            raise mismatch("actual call argument and same-invocation formal differ")
        left, left_basis = captured_operand(comparison["children"][0], ret, entry)
        right, right_basis = captured_operand(comparison["children"][1], ret, entry)
        if left != formal_value:
            raise mismatch("captured helper formal Copy changed before comparison")
        chosen = left < right if comparison["opcode"] == "<" else left == right
        if result_value != int(arms[0 if chosen else 1]["value"]):
            raise mismatch("observed caller result differs from actual helper selected literal arm")
        internal = d.data(ret, "RETURN_VALUE")
        if len(internal) > 1 or (internal and int(internal[0][2]) != result_value):
            raise mismatch("captured internal RETURN_VALUE disagrees with caller call-result")
        arguments.append(actual_value)
        formals.append(formal_value)
        choices.append(chosen)
        values.append(result_value)
        observations.append({
            "call_sequence": site["sequence"], "entry_sequence": entry["sequence"],
            "return_sequence": ret["sequence"], "caller_result_sequence": observed_result["sequence"],
            "actual_argument": actual_value, "formal_value": formal_value,
            "left_operand": left, "right_operand": right,
            "operand_basis": [left_basis, right_basis],
            "conditional_choice_inferred": chosen,
            "conditional_choice_basis": "captured scalar operands plus actual single LT/EQ ConditionalOperator; no direct ternary branch marker",
            "caller_call_result_observed": result_value,
            "internal_RETURN_VALUE": int(internal[0][2]) if internal else None,
            "internal_RETURN_VALUE_status": "OBSERVED" if internal else "NOT_OBSERVED",
        })
    return {"if": conditional, "result": result, "comparison": comparison,
            "operand": formal, "argument": argument, "call": call,
            "result_declaration": declaration, "caller_function": call["function"],
            "callee_function": function["id"], "formal": formal,
            "branches": [], "results": results, "arguments": arguments,
            "formals": formals, "choices": choices, "result_values": values,
            "observations": observations,
            "evidence_limit": "actual C direct callee/formal/normal return/caller-result linkage; branch choice inferred from operands and AST; no machine CALL or fresh-address claim"}
