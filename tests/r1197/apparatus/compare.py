"""Independent R1197 comparison of actual logger records with frozen V1.

No old R decoder is imported. This code is prepared as text and unexecuted.
Role selection requires a separately justified map of the actual emitted C.
"""
from __future__ import annotations

import hashlib
import json
import re


class InterfaceUnresolved(RuntimeError):
    pass


class ObservationMismatch(RuntimeError):
    pass


def sha(data):
    return hashlib.sha256(data).hexdigest()


def require_actual_binding(case_name, binding_document, inputs, model):
    if binding_document.get("status") != "BOUND_TO_ACTUAL_OUTPUT":
        raise InterfaceUnresolved("actual C/AST/public-owner semantic role binding is unresolved")
    binding = binding_document.get("cases", {}).get(case_name)
    if binding is None:
        raise InterfaceUnresolved("no actual-output role mapping for " + case_name)
    for name, data in inputs.items():
        if binding.get(name + "_sha256") != sha(data):
            raise InterfaceUnresolved("role map is not bound to exact " + name)
    if not binding.get("mapping_evidence"):
        raise InterfaceUnresolved("actual source-owned identity mapping evidence is missing")
    if not model.get("original_restored_exactly") or not model.get("actual_ast_linkage"):
        raise InterfaceUnresolved("passive byte restoration/same-AST linkage is missing")
    nodes = {node["id"]: node for node in model["nodes"]}
    events = {event["id"]: event for event in model["events"]}
    variables = model["variables"]
    original = binding["original_declaration"]
    if original not in variables or not variables[original]["initialized"]:
        raise InterfaceUnresolved("original role is not an actual initialized C declaration")
    if variables[original]["function"] != binding["caller_function"]:
        raise InterfaceUnresolved("original declaration belongs to another C function")
    cursor = nodes[variables[original]["node"]]
    while cursor.get("parent"):
        cursor = nodes[cursor["parent"]]
        if cursor["kind"] in ("WhileStmt", "ForStmt", "DoStmt"):
            raise InterfaceUnresolved("mapped original is iteration-local, not the persistent original")
    record_type = re.sub(r"\bconst\b", "", variables[original]["type"]).strip()
    fields = model["records"].get(record_type)
    if fields is None or len(fields) != 2:
        raise InterfaceUnresolved("mapped original is not an observed complete two-field record")
    field_ids = {field["id"] for field in fields}
    if (binding["selected_field"] not in field_ids
            or binding["sibling_field"] not in field_ids
            or binding["selected_field"] == binding["sibling_field"]):
        raise InterfaceUnresolved("selected/sibling are not distinct actual fields of original")
    for role, selector in binding["values"].items():
        if not isinstance(selector.get("events"), list) or not selector["events"]:
            raise InterfaceUnresolved("no actual event selection for role " + role)
        if any(event not in events for event in selector["events"]):
            raise InterfaceUnresolved("role refers to absent actual event")
        declaration = selector.get("declaration")
        if declaration is not None and declaration not in variables:
            raise InterfaceUnresolved("role refers to absent actual declaration")
    if any(event not in events for event in binding["original_snapshot_events"]):
        raise InterfaceUnresolved("original snapshot event is absent from actual map")
    return binding


def parse_trace(stderr, model):
    known_events = {event["id"]: event for event in model["events"]}
    current = None
    occurrences = []
    fields_count = {
        "VALUE": 4, "POINTER": 4, "ROOT": 3, "FIELD": 5,
        "CALL_ARGUMENT": 3, "RETURN_VALUE": 2,
        "DESTINATION_ADDRESS": 2, "INCOMING": 2, "DESTINATION": 3,
    }
    numeric_positions = {
        "VALUE": (2,), "FIELD": (3,), "CALL_ARGUMENT": (1, 2),
        "RETURN_VALUE": (1,), "INCOMING": (1,), "DESTINATION": (2,),
    }
    for line in stderr.decode("utf-8", errors="strict").splitlines():
        if not line:
            continue
        parts = line.split("|")
        if len(parts) < 3 or parts[0] != "R1190":
            raise ObservationMismatch("non-observer stderr, including any sanitizer diagnostic")
        tag = parts[1]
        values = parts[2:]
        if tag == "EVENT":
            if len(values) != 2 or values[0] not in known_events:
                raise ObservationMismatch("unknown/malformed actual event")
            if known_events[values[0]]["kind"] != values[1]:
                raise ObservationMismatch("actual event kind differs from same-AST map")
            current = {"event": values[0], "kind": values[1], "records": []}
            occurrences.append(current)
            continue
        if tag not in fields_count or len(values) != fields_count[tag]:
            raise ObservationMismatch("malformed observer record")
        if current is None or values[0] != current["event"]:
            raise ObservationMismatch("observer record is not bound to its actual event occurrence")
        for index in numeric_positions.get(tag, ()):
            if re.fullmatch(r"[0-9]+", values[index]) is None:
                raise ObservationMismatch("non-decimal actual scalar observation")
            values[index] = int(values[index])
        current["records"].append({"tag": tag, "values": values})
    return occurrences


def selected_values(occurrences, selector):
    output = []
    for event in occurrences:
        if event["event"] not in selector["events"]:
            continue
        for record in event["records"]:
            if record["tag"] != selector["record"]:
                continue
            values = record["values"]
            if record["tag"] == "VALUE" and values[1] == selector.get("declaration"):
                output.append(values[2])
            elif record["tag"] == "FIELD" and values[1:3] == [selector.get("declaration"), selector.get("field")]:
                output.append(values[3])
            elif record["tag"] == "CALL_ARGUMENT" and values[1] == selector.get("argument"):
                output.append(values[2])
            elif record["tag"] in ("RETURN_VALUE", "INCOMING"):
                output.append(values[1])
            elif record["tag"] == "DESTINATION":
                output.append(values[2])
    return output


def compare(occurrences, binding, oracle, case_name):
    case = oracle["cases"][case_name]
    profile = oracle["profiles"][case["profile"]]
    post = oracle["common"]["post_loop"]
    expected = {
        "phase": [0, 1, 2],
        "delivered": profile["delivered"],
        "replaced": profile["replaced"],
        "current": profile["current"],
        "sibling": profile["sibling"],
        "old_snapshot": [post["old_snapshot"]],
        "loop_result": [post["loop_result"]],
        "final_sample": [post["final_sample"]],
        "final_guard": [post["final_guard"]],
        "snapshot_after": [post["snapshot_after"]],
    }
    actual = {}
    for role, wanted in expected.items():
        selector = binding["values"].get(role)
        if selector is None:
            raise InterfaceUnresolved("required role NOT OBSERVED: " + role)
        found = selected_values(occurrences, selector)
        actual[role] = found
        if found != wanted:
            raise ObservationMismatch("actual values differ from frozen oracle for " + role)
    roots, selected, siblings = [], [], []
    selected_values_after = []
    sibling_values_after = []
    for event in occurrences:
        if event["event"] not in binding["original_snapshot_events"]:
            continue
        for record in event["records"]:
            values = record["values"]
            if record["tag"] == "ROOT" and values[1] == binding["original_declaration"]:
                roots.append(values[2])
            if record["tag"] == "FIELD" and values[1] == binding["original_declaration"]:
                if values[2] == binding["selected_field"]:
                    selected.append(values[4])
                    selected_values_after.append(values[3])
                if values[2] == binding["sibling_field"]:
                    siblings.append(values[4])
                    sibling_values_after.append(values[3])
    if selected_values_after != [7] + profile["current"] or sibling_values_after != [211] * 4:
        raise ObservationMismatch("original snapshots do not match initialization and the three actual changes")
    if (len(roots) != 4 or len(selected) != 4 or len(siblings) != 4
            or len(set(roots)) != 1 or len(set(selected)) != 1
            or len(set(siblings)) != 1 or selected[0] == siblings[0]
            or any(value in ("(nil)", "0x0") for value in roots + selected + siblings)):
        raise ObservationMismatch("actual within-run original/field identity observations differ")
    return {
        "status": "FINITE_CALLER_VALUE_AND_PHYSICAL_OBSERVATIONS_MATCH",
        "actual": actual,
        "addresses": {"original": roots, "selected": selected, "sibling": siblings},
        "internal_condition_return": "Only independently bound direct events count; unbound internal operations remain NOT OBSERVED.",
        "authority_limit": "Address/value agreement grants no owner/issuer/currentness authority and is not a whole-loop validator.",
    }
