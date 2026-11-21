"""R1625 forward v1: classify bounded public records, never certify substitution."""
class ObservationStop(Exception):
    def __init__(self, reason, boundary=False):
        super().__init__(reason)
        self.boundary = boundary

def require(condition, reason, boundary=False):
    if not condition:
        raise ObservationStop(reason, boundary)

def unique(records, record, key):
    result = {}
    for row in records:
        if row.get("record") != record:
            continue
        identity = key(row)
        require(identity not in result, "duplicate_receipt_identity:" + record)
        result[identity] = row
    return result

def span_bytes(data, span):
    require(isinstance(span, list) and len(span) == 2 and
            all(type(x) is int for x in span), "malformed_source_span")
    a, b = span
    require(0 <= a <= b <= len(data), "source_span_out_of_bounds")
    return data[a:b]

def classify_public(records, cid, original):
    """An observation procedure with named gaps, not a semantic/capability PASS."""
    require(cid in ("p1", "p2", "c4", "s1"), "not_a_lawful_graph_case")
    require(len(records) <= 8192, "receipt_count_limit", True)
    constants = [r for r in records if r.get("record") == "public_constants"]
    require(len(constants) == 1 and
            constants[0].get("schema") == "R1625_FORWARD_OBS_V1",
            "public_constants_schema_missing", True)
    k = constants[0]
    fragments = unique(records, "fragment", lambda r: r["fragment"])
    sources = unique(records, "source", lambda r: r["source"])
    nodes = unique(records, "node", lambda r: (r["fragment"], r["node"]))
    types = unique(records, "node_type", lambda r: (r["fragment"], r["node"]))
    values = unique(records, "value",
                    lambda r: (r["fragment"], r["node"], r["value"]))
    declarations = unique(records, "declaration",
                          lambda r: bytes.fromhex(r["name_hex"]).decode("ascii"))
    require(len(fragments) <= 32 and len(sources) <= 32 and len(nodes) <= 2048,
            "graph_receipt_budget", True)
    text = {sid: bytes.fromhex(r["bytes_hex"]) for sid, r in sources.items()}
    require(all(len(b) <= 4096 for b in text.values()), "source_receipt_byte_cap")
    originals = [sid for sid, r in sources.items() if r.get("original_owner")]
    adapters = [sid for sid, r in sources.items() if r.get("adapter_owner")]
    require(len(originals) == 1 and text[originals[0]] == original,
            "original_source_receipt_mismatch")
    require(len(adapters) == 1 and text[adapters[0]] == b"main()\n",
            "adapter_source_receipt_mismatch")
    require("main" in declarations, "main_declaration_missing", True)
    body_spans = {name: row["body_span"] for name, row in declarations.items()}
    for span in body_spans.values():
        span_bytes(original, span)
    offsets = {}
    for sid, data in text.items():
        if sid in adapters:
            offsets[sid] = None
        elif data == original:
            offsets[sid] = 0
        else:
            matches = [span[0] for span in body_spans.values()
                       if span_bytes(original, span) == data]
            require(len(matches) == 1, "retained_body_source_ambiguous", True)
            offsets[sid] = matches[0]
    for (fid, nid), node in nodes.items():
        require(fid in fragments and fragments[fid]["source"] in text,
                "node_fragment_source_missing", True)
        fragment = fragments[fid]
        require(node["world"] == fragment["world"] and
                node["source"] == fragment["source"], "node_world_source_mismatch")
        span_bytes(text[fragment["source"]], node["span"])
        require((fid, nid) in types, "typed_node_record_missing", True)
    links = {}
    for row in records:
        if row.get("record") not in ("fragment", "fragment_repeat"):
            continue
        child = row["fragment"]
        require(child in fragments, "edge_fragment_unavailable", True)
        key = (row["parent"], row["via"], row["edge"])
        require(key not in links, "duplicate_or_ambiguous_fragment_edge", True)
        links[key] = child
    entries = [r for r in fragments.values() if r["edge"] == "entry_adapter"]
    require(len(entries) == 1 and entries[0]["parent"] == 0,
            "entry_adapter_fragment_ambiguous", True)

    def node(fid, nid):
        require((fid, nid) in nodes, "qualified_node_unavailable", True)
        return nodes[(fid, nid)]

    def token(fid, row, field="name_span"):
        return span_bytes(text[fragments[fid]["source"]], row[field])

    def absolute_span(fid, row):
        sid = fragments[fid]["source"]
        require(offsets[sid] is not None, "original_call_in_adapter_source")
        return [offsets[sid] + x for x in row["span"]]

    def node_type(fid, nid, expected):
        row, typ = node(fid, nid), types[(fid, nid)]
        require(typ["world"] == row["world"] and typ["type"] == row["type"],
                "type_receipt_context_mismatch")
        require(typ.get("present") and typ.get("view"),
                "required_public_type_unavailable", True)
        require(typ["kind"] == k["type_" + expected],
                "observed_type_contradicts_source:" + expected)
        require(typ.get("copy") is True and typ.get("discardable") is True,
                "observed_scalar_copy_discardable_mismatch")
        return typ["type"]

    def scalar(fid, nid, expected_type, expected_value):
        row = node(fid, nid)
        typ = node_type(fid, nid, expected_type)
        require(row["result_count"] == 1 and row["results"][0][0] == typ,
                "observed_scalar_result_shape_mismatch")
        value_id = row["results"][0][1]
        value = values.get((fid, nid, value_id))
        require(value is not None and value.get("view"),
                "qualified_value_observation_missing", True)
        require(value["world"] == row["world"] and value["type"] == typ,
                "value_type_world_mismatch")
        require(type(value.get("scalar_known")) is bool,
                "malformed_knownness")
        if value["scalar_known"]:
            require(type(value["scalar_value"]) is int and
                    value["scalar_value"] == expected_value,
                    "known_scalar_contradicts_call_associated_oracle")
        else:
            require(value["scalar_value"] is None,
                    "unknown_value_was_reported_as_quantity")
        return {"fragment": fid, "node": nid, "world": row["world"],
                "type": typ, "value": value_id, "known": value["scalar_known"],
                "observed_scalar": value["scalar_value"],
                "source_expected_scalar": expected_value}

    def formals(name, expected, result):
        require(name in declarations, "callee_declaration_unavailable", True)
        decl = declarations[name]
        require(bytes.fromhex(decl["result_type_hex"]) == result.encode("ascii") and
                decl["parameter_count"] == len(expected),
                "declared_signature_contradicts_frozen_source")
        actual = decl.get("formal_syntax")
        require(isinstance(actual, list) and len(actual) == len(expected),
                "formal_syntax_observation_missing", True)
        for pos, (row, pair) in enumerate(zip(actual, expected)):
            require(row["position"] == pos and
                    bytes.fromhex(row["name_hex"]) == pair[0].encode("ascii") and
                    bytes.fromhex(row["type_hex"]) == pair[1].encode("ascii") and
                    span_bytes(original, row["name_span"]) == pair[0].encode("ascii") and
                    span_bytes(original, row["type_span"]) == pair[1].encode("ascii"),
                    "source_formal_position_type_mismatch")
        require(decl.get("formal_entry_authentication") == "NOT_ESTABLISHED",
                "formal_syntax_promoted_to_authentication")
        return [{"position": i, "name": n, "source_type": t}
                for i, (n, t) in enumerate(expected)]

    def body_of(parent, call_id, name):
        call = node(parent, call_id)
        require(call["kind"] == k["registered_call"] and
                call.get("body_backed") is True and
                token(parent, call) == name.encode("ascii"),
                "call_source_or_body_backed_mismatch")
        child = links.get((parent, call_id, "call_body_observation"))
        require(child is not None, "genuine_call_body_edge_missing", True)
        require(fragments[child]["world"] == fragments[parent]["world"],
                "ordinary_call_body_context_mismatch")
        root = node(child, fragments[child]["root"])
        require(root["kind"] == k["block"] and
                absolute_span(child, root) == body_spans[name],
                "call_body_source_declaration_mismatch")
        return child

    formals("main", [], "unit")
    entry = entries[0]["fragment"]
    entry_call = node(entry, fragments[entry]["root"])
    require(entry_call["arguments"] == 0, "main_adapter_argument_mismatch")
    main_body = body_of(entry, entry_call["node"], "main")
    require(len(declarations) == (1 if cid == "p1" else 2),
            "unexpected_function_declaration_set")
    specifications = []
    if cid in ("p2", "c4"):
        specifications = [{
            "spelling": b"select_quantity(seed, flag)", "callee": "select_quantity",
            "actuals": [("seed", "usize", 53), ("flag", "bool", 0)],
            "formals": [("input", "usize"), ("take_quotient", "bool")],
            "numerator": 53, "flag": 0, "quantity": 4}]
    elif cid == "s1":
        specifications = [
            {"spelling": b"choose_mixed(first_flag, first_input)", "callee": "choose_mixed",
             "actuals": [("first_flag", "bool", 1), ("first_input", "usize", 43)],
             "formals": [("take_quotient", "bool"), ("input", "usize")],
             "numerator": 43, "flag": 1, "quantity": 6},
            {"spelling": b"choose_mixed(second_flag, second_input)", "callee": "choose_mixed",
             "actuals": [("second_flag", "bool", 0), ("second_input", "usize", 26)],
             "formals": [("take_quotient", "bool"), ("input", "usize")],
             "numerator": 26, "flag": 0, "quantity": 5}]
    main_calls = [n for (fid, _nid), n in nodes.items()
                  if fid == main_body and n["kind"] == k["registered_call"]]
    require(len(main_calls) == len(specifications),
            "source_call_site_count_unavailable_or_extra", True)
    groups, used_calls = [], set()
    for order, spec in enumerate(specifications):
        matches = [n for n in main_calls
                   if token(main_body, n, "span").strip() == spec["spelling"]]
        require(len(matches) == 1, "source_call_site_ambiguous", True)
        call = matches[0]
        require(call["node"] not in used_calls, "two_source_calls_conflated", True)
        used_calls.add(call["node"])
        declared = formals(spec["callee"], spec["formals"], "usize")
        require(call["arguments"] == 2, "typed_actual_count_mismatch")
        arg_id, actuals, seen_args = call["edges"][0], [], set()
        for position, (name, kind, number) in enumerate(spec["actuals"]):
            require(arg_id != 0 and arg_id not in seen_args,
                    "actual_argument_chain_missing_or_cyclic", True)
            seen_args.add(arg_id)
            arg = node(main_body, arg_id)
            require(arg["kind"] == k["identifier"] and
                    token(main_body, arg) == name.encode("ascii") and
                    token(main_body, arg, "span").strip() == name.encode("ascii") and
                    arg["value_use"] == k["value_copied"],
                    "actual_source_name_or_copy_use_mismatch")
            actual = scalar(main_body, arg_id, kind, number)
            actual.update(position=position, source_name=name, source_type=kind)
            actuals.append(actual)
            arg_id = arg["edges"][1]
        require(arg_id == 0, "extra_actual_argument")
        result = scalar(main_body, call["node"], "usize", spec["quantity"])
        body = body_of(main_body, call["node"], spec["callee"])
        groups.append({
            "source_call_order": order, "source_call_span": absolute_span(main_body, call),
            "source_call_text": spec["spelling"].decode("ascii"),
            "call_fragment": main_body, "call_node": call["node"],
            "call_world": call["world"], "body_fragment": body,
            "body_world": fragments[body]["world"],
            "declared_body_span": body_spans[spec["callee"]],
            "formal_source_syntax": declared, "actual_observations": actuals,
            "call_result_observation": result, "expected_numerator": spec["numerator"],
            "expected_flag": spec["flag"], "expected_selected_quantity": spec["quantity"],
            "formal_entry_substitution_authentication": "NOT_ESTABLISHED"})
    if cid == "p1":
        groups = [{"source_call_order": 0, "source_call_text": "main() adapter",
                   "call_fragment": entry, "call_node": entry_call["node"],
                   "call_world": fragments[entry]["world"], "body_fragment": main_body,
                   "body_world": fragments[main_body]["world"],
                   "declared_body_span": body_spans["main"],
                   "formal_source_syntax": [], "actual_observations": [],
                   "expected_numerator": 0, "expected_flag": 1,
                   "expected_selected_quantity": 0,
                   "formal_entry_substitution_authentication": "NOT_ESTABLISHED"}]
    arithmetic = [r for r in records if r.get("record") in ("division", "remainder")]
    arith_keys = [(r["fragment"], r["node"]) for r in arithmetic]
    require(len(set(arith_keys)) == len(arith_keys), "duplicate_arithmetic_receipt")
    associated = set()
    for group in groups:
        paths = {group["body_fragment"]: [[]]}
        queue = [(group["body_fragment"], [], [group["body_fragment"]])]
        arm_paths, path_count = [], 1
        while queue:
            fid, current_path, ancestry = queue.pop(0)
            require(path_count <= 64 and len(current_path) <= 12,
                    "association_path_budget", True)
            for (parent, via, edge), child in links.items():
                if parent != fid:
                    continue
                require(edge in ("then_hypothetical", "else_hypothetical"),
                        "unexpected_nested_call_in_frozen_body", True)
                conditional = node(fid, via)
                require(conditional["kind"] == k["conditional"] and
                        conditional["normal_arms"] == 2,
                        "normal_if_arm_shape_mismatch")
                node_type(fid, via, "usize")
                require(fragments[child]["world"] != fragments[fid]["world"],
                        "hypothetical_arm_world_not_separate")
                path = current_path + [{"parent": fid, "via": via, "edge": edge,
                                        "child": child, "world": fragments[child]["world"]}]
                require(child not in ancestry, "cyclic_owned_arm_path", True)
                paths.setdefault(child, []).append(path)
                queue.append((child, path, ancestry + [child]))
                arm_paths.append(path)
                path_count += 1
        require(any(p[-1]["edge"] == "then_hypothetical" for p in arm_paths) and
                any(p[-1]["edge"] == "else_hypothetical" for p in arm_paths),
                "both_hypothetical_arms_not_observed", True)
        selected = [r for r in arithmetic if r["fragment"] in paths]
        require({r["record"] for r in selected} == {"division", "remainder"},
                "call_associated_arithmetic_coverage_missing", True)
        numerator = group["expected_numerator"]
        events = []
        for row in selected:
            fid, nid = row["fragment"], row["node"]
            observed_node = node(fid, nid)
            require(row.get("view") is True, "arithmetic_view_missing", True)
            require(all(row.get(name) is True for name in
                        ("origin_same", "event_valid", "copy_valid")),
                    "isolated_arithmetic_authenticity_failed")
            require(row.get("wrong_zero_accepted") is False,
                    "wrong_zero_candidate_accepted")
            foreign = "foreign_origin" if row["record"] == "division" else "foreign_source"
            require(row.get(foreign + "_checked") is True,
                    "foreign_candidate_not_checked", True)
            require(row.get(foreign + "_accepted") is False,
                    "foreign_candidate_accepted")
            if row["record"] == "remainder":
                require(row.get("source_same") is True, "remainder_source_mismatch")
            operator = b"/" if row["record"] == "division" else b"%"
            source = text[fragments[fid]["source"]]
            require(observed_node["kind"] == k[row["record"]] and
                    row["span"] == observed_node["span"] and
                    span_bytes(source, row["operator_span"]) == operator and
                    [span_bytes(source, s).strip() for s in row["operand_spans"]] ==
                    [b"copied", b"usize(7)"], "arithmetic_call_source_shape_mismatch")
            actual_span = absolute_span(fid, observed_node)
            body_start, body_end = group["declared_body_span"]
            require(body_start <= actual_span[0] <= actual_span[1] <= body_end,
                    "arithmetic_outside_call_associated_body_source")
            for path in paths[fid]:
                if cid == "c4":
                    require(not path, "hoisted_control_arithmetic_moved_into_arm")
                else:
                    expected_edge = ("then_hypothetical" if row["record"] == "division"
                                     else "else_hypothetical")
                    require(len(path) == 1 and path[0]["edge"] == expected_edge,
                            "arithmetic_hypothetical_source_arm_mismatch")
            typ = node_type(fid, nid, "usize")
            require(row["type"] == typ and row["target_max"] >= max(7, numerator) and
                    row["numbers"][1] == 7, "arithmetic_type_divisor_range_mismatch")
            for index, operand in enumerate(row["operands"]):
                op = node(fid, operand)
                require(op["span"] == row["operand_spans"][index],
                        "arithmetic_operand_span_mismatch")
                observation = scalar(fid, operand, "usize", numerator if index == 0 else 7)
                require(observation["value"] == row["values"][index],
                        "arithmetic_operand_package_mismatch")
            require(type(row["numerator_known"]) is bool and
                    type(row["result_known"]) is bool, "arithmetic_knownness_shape")
            require(row["numbers"][0] == (numerator if row["numerator_known"] else 0),
                    "known_numerator_or_unknown_placeholder_mismatch")
            expected = numerator // 7 if operator == b"/" else numerator % 7
            require(row["quantity"] == (expected if row["result_known"] else 0),
                    "known_result_or_unknown_placeholder_mismatch")
            result = scalar(fid, nid, "usize", expected)
            require(result["value"] == row["result"],
                    "arithmetic_result_package_mismatch")
            associated.add((fid, nid))
            events.append({"fragment": fid, "node": nid,
                           "world": fragments[fid]["world"],
                           "source": fragments[fid]["source"],
                           "paths": paths[fid], "operation": row["record"],
                           "numerator_known": row["numerator_known"],
                           "observed_numerator": row["numbers"][0] if row["numerator_known"] else None,
                           "result_known": row["result_known"],
                           "observed_quantity": row["quantity"] if row["result_known"] else None,
                           "source_expected_numerator": numerator,
                           "source_expected_quantity": expected,
                           "isolated_event_validation": "OBSERVED_TRUE",
                           "runtime_execution_of_this_arm": "NOT_OBSERVED"})
        group["hypothetical_arm_paths"] = arm_paths
        group["arithmetic_associations"] = events
    require(associated == set(arith_keys), "unassociated_arithmetic_outside_call_paths", True)
    ending = records[-1]
    require(ending.get("record") == "end" and
            ending.get("formal_entry_substitution_authentication") == "NOT_ESTABLISHED" and
            ending.get("runtime_selection_observed") is False and
            ending.get("whole_call_theorem_available") is False,
            "observer_claimed_unavailable_authentication_or_runtime_proof")
    return {
        "state": "BOUNDED_CALL_GRAPH_OBSERVATIONS_COMPLETE_WITH_EXPLICIT_GAPS",
        "call_associations": groups,
        "shared_body_fragment_observed": len({g["body_fragment"] for g in groups}) < len(groups),
        "formal_entry_substitution_authentication": "NOT_ESTABLISHED",
        "runtime_internal_quantities_order_selection": "NOT_OBSERVED",
        "whole_call_result_world_theorem": "NOT_ESTABLISHED",
        "capability_pass": "NOT_INFERRED",
    }
