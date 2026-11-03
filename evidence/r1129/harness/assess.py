#!/usr/bin/env python3
"""Interpret only R1129 frozen expectations and observations of its own source."""
from instrument import ObservationLimit


class MaterialFinding(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise MaterialFinding(message)


def owned(records, mode):
    fragments = {r["fragment"]: r for r in records if r["event"] == "fragment"}
    nodes = [r for r in records if r["event"] == "node"]
    types = {(r["world"], r["type"]): r for r in records if r["event"] == "type"}
    values = {(r["world"], r["value"]): r for r in records if r["event"] == "value"}
    bindings = {(r["world"], r["symbol"]): r
                for r in records if r["event"] == "binding"}
    places = {(r["world"], r["place"]): r for r in records if r["event"] == "place"}
    require(fragments and nodes, "successful public CHECK has no owned graph")
    entries = [r for r in fragments.values() if r["edge"] == "entry"]
    require(len(entries) == 1, "public entry fragment identity is not unique")
    entry = entries[0]
    roots = [n for n in nodes if n["fragment"] == entry["fragment"]
             and n["node"] == entry["root"]]
    require(len(roots) == 1 and roots[0]["kind_name"] == "CALL"
            and roots[0]["body_backed"], "entry is not a genuine body-backed call")
    require(any(f["parent_fragment"] == entry["fragment"]
                and f["owner_node"] == entry["root"] and f["edge"] == "call"
                for f in fragments.values()), "ordinary main body unavailable")
    loops = [n for n in nodes if n["kind_name"] == "LOOP"]
    limits = [
        "No universal public current-valid or source-to-C storage validator exists.",
        "Historical point-in-time field facts are not final live-place assertions.",
        "No semantic IDs are compared across different world labels.",
        "Public check and CLI emission use separate genuine checks of identical bytes.",
        "Header evidence is issuer-reported; finite native traces do not prove all H.",
    ]
    require(len(loops) <= 1, "unexpected duplicate source loop in owned graph")
    if mode == "normal":
        require(len(loops) == 1, "normal Continue source has no owned loop")
    if loops:
        loop = loops[0]
        require(loop["header_inductive"], "owned loop lacks published closure")
        require(loop["break_edges"] == 1 and loop["return_edges"] == 0,
                "owned normal Break/Return edge inventory differs")
        if mode == "normal":
            require(loop["continue_edges"] == 1, "owned normal Continue edge missing")
        else:
            require(loop["continue_edges"] in (0, 1),
                    "immediate control has unexpected Continue edge count")
        require(any(f["parent_fragment"] == loop["fragment"]
                    and f["owner_node"] == loop["node"] and f["edge"] == "loop"
                    for f in fragments.values()), "owned abstract body unavailable")
    else:
        limits.append("Immediate control loop was absent/pruned in the public graph.")
    controls = {k: [n for n in nodes if n["kind_name"] == k]
                for k in ("CONTINUE", "BREAK", "IF", "USIZE_EQUALITY", "LOAN")}
    if mode == "normal":
        require(controls["CONTINUE"] and controls["BREAK"] and controls["IF"],
                "normal owned graph omits a reachable control site")
        require(controls["USIZE_EQUALITY"], "normal phase comparison absent")
    for node in controls["USIZE_EQUALITY"]:
        typ = types.get((node["world"], node["type"]), {})
        require(typ.get("bool"), "phase equality is not source-issued bool")
    for node in controls["LOAN"]:
        require(node["loan_nonescape"], "accepted loan lacks nonescape evidence")

    expected = {
        "prior": 37, "returned_old": 37, "after_target": 58,
        "stable_sibling": 146, "other_target": 211, "other_sibling": 89,
        "caller_old": 146, "transferred_old": 211, "final_target": 58,
        "final_sibling": 37, "final_other": 58, "preserved_other": 89,
    }
    named = []
    for node in nodes:
        if node["name"] not in expected or not node["symbol"]:
            continue
        binding = bindings.get((node["world"], node["symbol"]))
        if not binding:
            continue
        value = values.get((node["world"], binding["value"]))
        if value:
            item = {"name": node["name"], "world": node["world"],
                    "symbol": node["symbol"], "value": binding["value"],
                    "known": value["known"], "scalar": value["scalar"],
                    "expected": expected[node["name"]]}
            if value["known"]:
                require(value["scalar"] == item["expected"],
                        "source-issued known scalar disagrees: " + str(item))
            if item not in named:
                named.append(item)
    fields = [n for n in nodes if n["field_present"]]
    for node in fields:
        require(node["field_compatible"], "field dependency compatibility absent")
        require(node["field_parent"] and node["field_child"]
                and node["parent_incarnation"] and node["child_incarnation"],
                "field evidence lacks source-issued identity")
        if node["name"] in ("signal", "checkpoint"):
            require(node["field_index"] == ("signal", "checkpoint").index(node["name"]),
                    "field name resolved to wrong declared index")
    names = {}
    for node in nodes:
        if node["name"] in ("original", "bystander") and node["symbol"]:
            binding = bindings.get((node["world"], node["symbol"]))
            if binding and binding["place"]:
                names.setdefault(node["world"], {})[node["name"]] = binding["place"]
    identity = []
    for world, roots in names.items():
        if len(roots) != 2:
            continue
        require(roots["original"] != roots["bystander"],
                "two source originals have one historical root")
        row = {"world": world, "bindings": roots, "places": {}}
        for label, place in roots.items():
            view = places.get((world, place))
            if view:
                require(view["independent_root"], "source original is not a root")
                row["places"][label] = view
        identity.append(row)
    if not identity:
        limits.append("Named historical root binding pair not exposed by this graph.")
    known_names = sorted({r["name"] for r in named if r["known"]})
    unknown_names = sorted(set(expected).difference(known_names))
    if unknown_names:
        limits.append("No known public scalar assertion for: " + ", ".join(unknown_names))
    return {
        "status": "SOURCE_ISSUED_PUBLIC_OBSERVATIONS",
        "entry": entry,
        "loop_nodes": loops, "control_counts": {k: len(v) for k, v in controls.items()},
        "known_named_scalars": named, "historical_identity": identity,
        "field_evidence": [{k: n[k] for k in (
            "world", "fragment", "node", "kind_name", "name", "field_index",
            "field_parent", "field_child", "parent_incarnation", "child_incarnation",
            "parent_fact", "child_fact", "parent_post_fact", "child_post_fact",
            "old_value", "new_value", "field_compatible",
        )} for n in fields],
        "limits": limits,
    }


def native(trace, mode, mapping):
    phases = [r["value"] for r in trace if r["event"] == "phase"]
    states = [r for r in trace if r["event"] == "state"]
    calls = [r for r in trace if r["event"] == "call"]
    expected_phases = [4, 19] if mode == "normal" else [19]
    if mapping["emitted_loop"]:
        require(phases == expected_phases, "actual phase trace differs: " + str(phases))
    else:
        require(mode == "immediate_break" and not phases,
                "normal loop cannot be replaced by immediate-control evidence")
    tags = [r["tag"] for r in states]
    require(tags.count("before_loop") == 1, "before-loop storage snapshot missing")
    require(tags.count("continue") == (1 if mode == "normal" else 0),
            "actual Continue count differs")
    require(tags.count("break") == (1 if mapping["emitted_loop"] else 0),
            "actual Break count differs")
    require(tags.count("header") == (len(expected_phases) if mapping["emitted_loop"] else 0),
            "actual body-entry count differs")
    for tag in ("after_call_0", "after_loop", "after_call_1", "after_call_2"):
        require(tags.count(tag) == 1, "missing/duplicate storage stage: " + tag)
    expected_tags = ["before_loop"]
    if mapping["emitted_loop"]:
        expected_tags += (["header", "continue", "header"]
                          if mode == "normal" else ["header"])
    expected_tags += ["after_call_0"]
    if mapping["emitted_loop"]:
        expected_tags += ["break"]
    expected_tags += ["after_loop", "after_call_1", "after_call_2"]
    require(tags == expected_tags, "actual state order differs: " + str(tags))
    expected_state = {
        "before_loop": (146, 37, 89, 211),
        "header": (146, 37, 89, 211),
        "continue": (146, 37, 89, 211),
        "after_call_0": (146, 58, 89, 211),
        "break": (146, 58, 89, 211),
        "after_loop": (146, 58, 89, 211),
        "after_call_1": (37, 58, 89, 211),
        "after_call_2": (37, 58, 89, 58),
    }
    address_keys = (
        "original", "original_signal", "original_checkpoint",
        "bystander", "bystander_signal", "bystander_checkpoint",
    )
    baseline = {key: states[0][key] for key in address_keys}
    require(baseline["original"] != baseline["bystander"],
            "simultaneous original storage aliases")
    require(len({baseline[k] for k in (
        "original_signal", "original_checkpoint",
        "bystander_signal", "bystander_checkpoint",
    )}) == 4, "live sibling/other-original fields alias")
    for row in states:
        got = tuple(row[k] for k in (
            "signal", "checkpoint", "other_signal", "other_checkpoint",
        ))
        require(got == expected_state[row["tag"]],
                "live storage values differ at " + row["tag"] + ": " + str(got))
        require({k: row[k] for k in address_keys} == baseline,
                "original/child storage changed before scope end")
    require(len(calls) == 3, "actual helper operation count differs")
    expected_calls = [
        ("original_checkpoint", 58, 37, 58),
        ("original_signal", 37, 146, 37),
        ("bystander_checkpoint", 58, 211, 58),
    ]
    for row, (destination, new, old, current) in zip(calls, expected_calls):
        require(row["destination"] == baseline[destination],
                "actual write argument is not the observed original child")
        require((row["new"], row["old"], row["current"]) == (new, old, current),
                "actual helper old/current/result lineage differs: " + str(row))
    events = [r["event"] if r["event"] != "state" else r["tag"] for r in trace]
    require(events.index("call") < events.index("after_call_0"),
            "helper return observation is not before caller observation")
    return {
        "status": "FINITE_INSTRUMENTED_NATIVE_OBSERVATIONS_MATCH",
        "phases": phases, "state_order": tags, "calls": calls,
        "stable_addresses": baseline,
        "limits": [
            "Observer writes only its own output; all guest values come from live C storage.",
            "Addresses belong to the passive variant, not the unmodified C process.",
            "Actual C effect-graph mapping is not a universal public issuer certificate.",
            "This finite trace establishes neither all-H induction nor general termination.",
        ],
    }
