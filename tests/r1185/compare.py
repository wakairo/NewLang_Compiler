"""R1185 post-capture comparison. Role decoding never receives the oracle.

The finite decoder uses actual C/AST graph relations and captured logger events.
Unknown/ambiguous shapes return equipment-incomplete, never a fabricated PASS.
Numeric expectations are loaded only after decoding has completed.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sys

from direct_helper import decode_direct_helper


class Incomplete(RuntimeError):
    pass


class Mismatch(RuntimeError):
    pass


def one(values, description):
    values = list(values)
    if len(values) != 1:
        raise Incomplete(description + ": expected one structural candidate, found " + str(len(values)))
    return values[0]


def u8_type(value):
    return value.replace("const ", "").strip() in ("unsigned char", "uint8_t", "__uint8_t")


def usize_type(value):
    return value.replace("const ", "").strip() in (
        "unsigned long", "unsigned long int", "unsigned long long", "size_t", "uintptr_t")


class Decoder:
    def __init__(self, model, raw):
        self.model = model
        self.nodes = {x["id"]: x for x in model["nodes"]}
        self.variables = model["variables"]
        self.static_events = {x["id"]: x for x in model["events"]}
        self.initializers = {}
        self.assignments = {}
        for decl, info in self.variables.items():
            node = self.nodes[info["node"]]
            if node.get("init") and node["children"]:
                self.initializers[decl] = node["children"][-1]
        for node in self.nodes.values():
            if node["kind"] in ("BinaryOperator", "CompoundAssignOperator") and node.get("opcode") in ("=", "+=", "-="):
                if len(node["children"]) == 2:
                    left = self.unwrap(node["children"][0])
                    if left["kind"] == "DeclRefExpr" and left.get("ref_decl"):
                        self.assignments.setdefault(left["ref_decl"], []).append(node["id"])
        # None is reserved for a static same-AST role check. It creates no raw
        # events and cannot complete decode(), which requires actual boundaries.
        self.events = [] if raw is None else self.parse(raw)

    def unwrap(self, node):
        node = self.nodes[node] if isinstance(node, str) else node
        while node["kind"] in ("ImplicitCastExpr", "CStyleCastExpr", "ParenExpr") and len(node["children"]) == 1:
            node = self.nodes[node["children"][0]]
        return node

    def descendants(self, node):
        node = self.nodes[node] if isinstance(node, str) else node
        yield node
        for child in node["children"]:
            yield from self.descendants(child)

    def dependencies(self, node, visited=None):
        visited = set() if visited is None else visited
        node = self.nodes[node] if isinstance(node, str) else node
        if node["id"] in visited:
            return set()
        visited.add(node["id"])
        result = set()
        if node["kind"] == "DeclRefExpr" and node.get("ref_decl"):
            decl = node["ref_decl"]
            result.add(decl)
            if decl in self.initializers and decl not in self.assignments:
                result |= self.dependencies(self.initializers[decl], visited)
            elif len(self.assignments.get(decl, [])) == 1:
                write = self.nodes[self.assignments[decl][0]]
                right = self.unwrap(write["children"][1])
                if (write.get("opcode") == "=" and right["kind"] == "DeclRefExpr"
                        and write.get("range") and node.get("range")
                        and write["range"][1] < node["range"][0]
                        and "*" not in self.variables[decl]["type"]):
                    result |= self.dependencies(right, visited)
        for child in node["children"]:
            result |= self.dependencies(child, visited)
        return result

    def expanded(self, node, visited=None):
        visited = set() if visited is None else visited
        node = self.nodes[node] if isinstance(node, str) else node
        if node["id"] in visited:
            return
        visited.add(node["id"])
        yield node
        if node["kind"] == "DeclRefExpr" and node.get("ref_decl"):
            decl = node["ref_decl"]
            if decl in self.initializers and decl not in self.assignments:
                yield from self.expanded(self.initializers[decl], visited)
            elif len(self.assignments.get(decl, [])) == 1:
                write = self.nodes[self.assignments[decl][0]]
                right = self.unwrap(write["children"][1])
                admissible = (right["kind"] == "DeclRefExpr" or
                              (right["kind"] == "BinaryOperator" and right.get("opcode") in ("<", "==")))
                if (write.get("opcode") == "=" and admissible
                        and write.get("range") and node.get("range")
                        and write["range"][1] < node["range"][0]
                        and "*" not in self.variables[decl]["type"]):
                    yield from self.expanded(right, visited)
        for child in node["children"]:
            yield from self.expanded(child, visited)

    def place(self, node, visited=None):
        visited = set() if visited is None else visited
        node = self.unwrap(node)
        if node["id"] in visited:
            return None
        visited.add(node["id"])
        if node["kind"] == "MemberExpr" and len(node["children"]) == 1:
            base = self.place(node["children"][0], visited)
            if base and base[1] is None and node.get("ref_field"):
                return base[0], node["ref_field"]
            return None
        if node["kind"] == "UnaryOperator" and node.get("opcode") in ("*", "&") and len(node["children"]) == 1:
            return self.place(node["children"][0], visited)
        if node["kind"] == "DeclRefExpr" and node.get("ref_decl"):
            decl = node["ref_decl"]
            kind = self.variables[decl]["type"].replace("const ", "").strip()
            if kind in self.model["records"] and not kind.startswith("union "):
                return decl, None
            if "*" in kind and decl in self.initializers and decl not in self.assignments:
                return self.place(self.initializers[decl], visited)
        return None

    def parse(self, raw):
        events = []
        current = None
        state = {}
        for line in raw.decode("utf-8", errors="strict").splitlines():
            pieces = line.split("|")
            if len(pieces) < 3 or pieces[0] != "R1185":
                raise Incomplete("nonlogger stderr retained; cannot silently discard it")
            if pieces[1] == "EVENT":
                if current is not None:
                    current["state"] = dict(state)
                if pieces[2] not in self.static_events:
                    raise Incomplete("event not present in actual observer map")
                static = self.static_events[pieces[2]]
                if len(pieces) != 4 or pieces[3] != static["kind"]:
                    raise Incomplete("event kind differs from actual observer map")
                current = {"sequence": len(events), "static": static,
                           "records": [], "state": None}
                events.append(current)
                continue
            if current is None or pieces[2] != current["static"]["id"]:
                raise Incomplete("data record does not belong to the live event")
            current["records"].append(pieces[1:])
            if pieces[1] == "VALUE":
                if pieces[3] not in self.variables:
                    raise Incomplete("unknown declaration in value record")
                state[pieces[3]] = {"value": int(pieces[4]), "address": pieces[5],
                                    "event": current["sequence"]}
            if pieces[1] == "DESTINATION" and current["static"]["kind"] == "ASSIGN_AFTER":
                left = self.unwrap(current["static"]["lhs"])
                if left["kind"] == "DeclRefExpr" and left.get("ref_decl"):
                    state[left["ref_decl"]] = {"value": int(pieces[4]), "address": pieces[3],
                                               "event": current["sequence"]}
        if current is not None:
            current["state"] = dict(state)
        if not events:
            raise Incomplete("no captured passive events")
        return events

    def matching(self, kind, node=None):
        return [e for e in self.events if e["static"]["kind"] == kind
                and (node is None or e["static"]["node"] == node)]

    def value(self, event, decl):
        if decl not in event["state"]:
            raise Incomplete("declaration has no captured initialized value: " + decl)
        return event["state"][decl]["value"]

    def data(self, event, kind):
        return [record for record in event["records"] if record[0] == kind]

    def destination(self, event):
        row = one(self.data(event, "DESTINATION"), "actual destination observation")
        return {"address": row[2], "value": int(row[3])}

    def helper(self, nodes):
        return decode_direct_helper(self, list(nodes), Incomplete, Mismatch,
                                    one, u8_type, usize_type)

    def declared_values(self, decl, after=None, before=None):
        result = []
        for event in self.matching("DECLARATION"):
            node = self.nodes[event["static"]["node"]]
            start = node["range"][0]
            if after is not None and start <= after:
                continue
            if before is not None and start >= before:
                continue
            for row in self.data(event, "VALUE"):
                if row[2] == decl:
                    result.append(int(row[3]))
        return result

    def decode(self):
        loops = [n for n in self.nodes.values() if n["kind"] in ("WhileStmt", "ForStmt", "DoStmt")]
        if not loops:
            return self.control()
        loop = one(loops, "one actual C loop")
        descendants = list(self.descendants(loop))
        ids = {n["id"] for n in descendants}
        # Increment expressions would execute after Continue. This finite decoder
        # does not pretend that a pre-Continue counter read observes that update.
        if loop["kind"] == "ForStmt":
            prior = [self.nodes[x] for x in loop["children"][:-1]]
            if any(n.get("kind") for n in prior):
                raise Incomplete("nonempty C for clauses need a distinct actual-edge decoder")
        if loop["kind"] == "DoStmt":
            raise Incomplete("do-loop observation requires a distinct edge decoder")
        breaks = [n for n in descendants if n["kind"] == "BreakStmt"]
        break_node = one(breaks, "actual loop Break statement")
        body = self.nodes[loop["children"][-1]]
        if body["kind"] != "CompoundStmt" or break_node.get("parent") != body["id"]:
            raise Incomplete("finite loop requires a direct fallthrough Break in its compound body")
        outer = one((n for n in descendants if n["kind"] == "IfStmt" and not n.get("hasElse")
                     and n.get("parent") == body["id"]
                     and any(x["kind"] == "ContinueStmt" for x in self.descendants(n))),
                    "outer guard IF whose true region continues before fallthrough Break")
        if len(outer["children"]) != 2 or outer["range"][1] >= break_node["range"][0]:
            raise Incomplete("outer true region and fallthrough Break are not finite ordered siblings")
        tail_ids = body["children"][body["children"].index(outer["id"]) + 1:
                                    body["children"].index(break_node["id"])]
        break_tail = [n for node in tail_ids for n in self.descendants(node)]
        condition = outer["children"][0]
        counter = one((d for d in self.dependencies(condition)
                       if d in self.assignments and usize_type(self.variables[d]["type"]) and
                       any(n in ids for n in self.assignments[d])), "loop carry read by outer guard")
        header_events = self.matching("LOOP_HEADER", loop["id"])
        if not header_events:
            raise Incomplete("actual loop header events absent")
        phase_headers = [self.value(e, counter) for e in header_events]
        outer_events = self.matching("IF_TRUE", outer["id"])
        outer_choices = []
        for index, header in enumerate(header_events):
            stop = header_events[index + 1]["sequence"] if index + 1 < len(header_events) else len(self.events)
            true_hits = [e for e in outer_events if header["sequence"] < e["sequence"] < stop]
            break_hits = [e for e in self.matching("BREAK", break_node["id"])
                          if header["sequence"] < e["sequence"] < stop]
            if len(true_hits) == 1 and not break_hits:
                outer_choices.append(True)
            elif len(break_hits) == 1 and not true_hits:
                outer_choices.append(False)
            else:
                raise Incomplete("actual header has no unique true-entry or fallthrough-Break path")
        helper = self.helper(n for n in descendants if n["id"] != outer["id"])
        if counter not in self.dependencies(helper["argument"]):
            raise Incomplete("actual C helper operand path does not establish direct carry flow")
        store_nodes = []
        for node in descendants:
            if node["kind"] == "BinaryOperator" and node.get("opcode") == "=":
                place = self.place(node["children"][0])
                if place and place[1] is not None:
                    store_nodes.append((node, place))
        store, selected_place = one(store_nodes, "one selected-field store inside loop")
        root, selected_field = selected_place
        root_node = self.nodes[self.variables[root]["node"]]
        if root_node["range"][1] >= loop["range"][0]:
            raise Incomplete("original root is not established outside the actual C loop")
        fields = self.model["records"].get(self.variables[root]["type"].replace("const ", "").strip(), [])
        sibling_field = one((f["id"] for f in fields if f["id"] != selected_field), "one known-disjoint C sibling field")
        if helper["result"] not in self.dependencies(store["children"][1]):
            raise Incomplete("actual store RHS is not connected to actual IF u8 result")
        old_loads = []
        entry_selected = []
        entry_sibling = []
        post_selected = []
        post_sibling = []
        for decl, initializer in self.initializers.items():
            if not u8_type(self.variables[decl]["type"]):
                continue
            node = self.nodes[self.variables[decl]["node"]]
            origin = self.place(initializer)
            if origin == selected_place and node["range"][1] < loop["range"][0]:
                entry_selected.append(decl)
            if origin == (root, sibling_field) and node["range"][1] < loop["range"][0]:
                entry_sibling.append(decl)
            if origin == selected_place and node["id"] in ids and node["range"][0] < store["range"][0]:
                old_loads.append(decl)
            if origin == selected_place and node["range"][0] > loop["range"][1]:
                post_selected.append(decl)
            if origin == (root, sibling_field) and node["range"][0] > loop["range"][1]:
                post_sibling.append(decl)
        old_load = one(old_loads, "actual selected old-load declaration before store")
        before_stores = self.matching("STORE_BEFORE", store["id"])
        after_stores = self.matching("STORE_AFTER", store["id"])
        if len(before_stores) != len(after_stores):
            raise Incomplete("store before/after event count differs")
        roots = []
        field_rows = []
        for event in self.events:
            roots.extend(row[3] for row in self.data(event, "ROOT") if row[2] == root)
            field_rows.extend((event, row) for row in self.data(event, "FIELD") if row[2] == root)
        initial_selected = one((row for event, row in field_rows if row[3] == selected_field
                                and event["sequence"] < header_events[0]["sequence"]),
                               "initial selected-child observation")
        initial_sibling = one((row for event, row in field_rows if row[3] == sibling_field
                               and event["sequence"] < header_events[0]["sequence"]),
                              "initial sibling observation")
        selected_address, sibling_address = initial_selected[5], initial_sibling[5]
        if selected_address == sibling_address:
            raise Mismatch("observed selected and sibling addresses coincide")
        if len(set(roots)) != 1:
            raise Mismatch("observed original root address changed within this run")
        for event, row in field_rows:
            if row[3] in (selected_field, sibling_field):
                wanted = selected_address if row[3] == selected_field else sibling_address
                if row[5] != wanted:
                    raise Mismatch("observed original child address changed within this run")
        old_values = []
        incoming = []
        current = []
        siblings = []
        for before, after in zip(before_stores, after_stores):
            address = one(self.data(before, "DESTINATION_ADDRESS"), "real store destination address")[2]
            if address != selected_address:
                raise Mismatch("real store destination is not the original selected child")
            last_header = max((e for e in header_events if e["sequence"] < before["sequence"]), key=lambda e: e["sequence"])
            if old_load not in before["state"] or before["state"][old_load]["event"] <= last_header["sequence"]:
                raise Incomplete("old result is not an observed fresh guest load in this iteration")
            old_values.append(self.value(before, old_load))
            incoming.append(int(one(self.data(before, "INCOMING"), "actual incoming scalar read")[2]))
            after_value = self.destination(after)
            if after_value["address"] != selected_address:
                raise Mismatch("post-store lvalue address is not selected child")
            current.append(after_value["value"])
            sibling = one((row for row in self.data(after, "FIELD")
                           if row[2] == root and row[3] == sibling_field), "post-store sibling read")
            siblings.append(int(sibling[4]))
        alias_decls = [d for d, initializer in self.initializers.items()
                       if "*" in self.variables[d]["type"] and self.place(initializer) == selected_place]
        for event in self.events:
            for row in self.data(event, "POINTER"):
                if row[2] in alias_decls and row[3] != selected_address:
                    raise Mismatch("observed selected alias has a different destination")
        continue_events = [e for e in self.matching("CONTINUE") if e["static"]["node"] in ids]
        break_events = self.matching("BREAK", break_node["id"])
        break_event = one(break_events, "actual dynamic Break")
        if not entry_selected or not entry_sibling:
            raise Incomplete("pre-loop field Copy declarations not structurally identified")
        entry_selected_after = []
        entry_sibling_after = []
        break_values = {row[2]: int(row[3]) for row in self.data(break_event, "VALUE")}
        for declarations, observed in ((entry_selected, entry_selected_after),
                                       (entry_sibling, entry_sibling_after)):
            for decl in declarations:
                if decl not in break_values:
                    raise Incomplete("entry Copy not directly observed at actual Break")
                observed.append(break_values[decl])
        break_targets = set()
        for node in break_tail:
            if node["kind"] == "BinaryOperator" and node.get("opcode") == "=":
                left = self.unwrap(node["children"][0])
                decl = left.get("ref_decl") if left["kind"] == "DeclRefExpr" else None
                if decl and decl != counter and usize_type(self.variables[decl]["type"]):
                    break_targets.add(decl)
        loop_result = one(break_targets, "actual outside loop-result assignment in Break arm")
        copied_results = []
        starting_phase_copies = []
        for decl, initializer in self.initializers.items():
            node = self.nodes[self.variables[decl]["node"]]
            if (node["function"] != loop["function"] or node["range"][0] <= loop["range"][1]
                    or not usize_type(self.variables[decl]["type"])):
                continue
            values = self.declared_values(decl)
            if loop_result in self.dependencies(initializer):
                copied_results.extend(values)
            else:
                starting_phase_copies.extend(values)
        if not copied_results or not starting_phase_copies:
            raise Incomplete("outside loop-result or starting-phase Copy observation absent")
        selected_post_values = [v for d in post_selected for v in self.declared_values(d)]
        sibling_post_values = [v for d in post_sibling for v in self.declared_values(d)]
        if not selected_post_values or not sibling_post_values:
            raise Incomplete("post-loop current/sibling loads not observed")
        helper_results = helper["result_values"]
        normal_copies = [d for d, initializer in self.initializers.items()
                         if u8_type(self.variables[d]["type"])
                         and helper["result"] in self.dependencies(initializer)
                         and self.nodes[self.variables[d]["node"]]["id"] in ids]
        if not normal_copies:
            raise Incomplete("caller u8 result Copy path absent from actual C graph")
        copy_values = {d: self.declared_values(d) for d in normal_copies}
        old_copies = [d for d, initializer in self.initializers.items()
                      if u8_type(self.variables[d]["type"])
                      and old_load in self.dependencies(initializer)
                      and self.nodes[self.variables[d]["node"]]["id"] in ids]
        if not old_copies:
            raise Incomplete("replace old-result Copy path absent from actual C graph")
        return {"kind": "loop", "phase_headers": phase_headers,
                "outer_guard_results": outer_choices,
                "outer_guard_evidence_limit": "true entry observed; false is actual fallthrough-Break path inference within each observed loop-header interval",
                "helper_arguments": helper["arguments"],
                "fresh_formal_values": helper["formals"],
                "formal_evidence_limit": helper["evidence_limit"],
                "helper_branch_results": helper["choices"],
                "helper_observations": helper["observations"],
                "helper_normal_results": helper_results,
                "replace_old_results": old_values,
                "selected_after_each_store": current, "sibling_after_each_store": siblings,
                "continue_arguments": [self.value(e, counter) for e in continue_events],
                "helper_call_count": len(helper["arguments"]), "store_count": len(after_stores),
                "continue_count": len(continue_events), "break_count": len(break_events),
                "actual_incoming": incoming, "caller_result_copies": copy_values,
                "old_result_copies": {d: self.declared_values(d) for d in old_copies},
                "initial_selected": int(initial_selected[4]), "initial_sibling": int(initial_sibling[4]),
                "post": {"break_result": self.value(break_event, loop_result),
                         "actual_break_phase": self.value(break_event, counter),
                         "selected": selected_post_values[-1], "sibling": sibling_post_values[-1],
                         "entry_selected_copy": entry_selected_after[-1],
                         "entry_sibling_copy": entry_sibling_after[-1],
                         "starting_phase_copy": starting_phase_copies[-1]},
                "entry_selected_copies_at_break": entry_selected_after,
                "entry_sibling_copies_at_break": entry_sibling_after,
                "outside_result_copies": copied_results,
                "outside_starting_phase_copies": starting_phase_copies,
                "post_selected_loads": selected_post_values, "post_sibling_loads": sibling_post_values,
                "identity": {"original_C_root_decl": root, "selected_C_field": selected_field,
                             "sibling_C_field": sibling_field, "root_address": roots[0],
                             "selected_address": selected_address, "sibling_address": sibling_address,
                             "aliases": alias_decls,
                             "limit": "within this passive run only; no C layout/source-index equivalence or cross-run/ghost authority"},
                "roles": {"loop": loop["id"], "counter": counter, "outer_if": outer["id"],
                          "helper_conditional": helper["if"]["id"], "helper_call_result": helper["result"],
                          "helper_call": helper["call"]["id"], "callee_function": helper["callee_function"],
                          "helper_operand": helper["operand"], "store": store["id"],
                          "old_load": old_load, "loop_result": loop_result}}

    def control(self):
        if any(n["kind"] in ("BinaryOperator", "CompoundAssignOperator")
               and n.get("opcode") in ("=", "+=", "-=") for n in self.nodes.values()):
            raise Incomplete("ordinary helper-only control contains an actual assignment/store")
        helper = self.helper(self.nodes.values())
        observation = one(helper["observations"], "ordinary actual helper invocation")
        copies = [d for d, initializer in self.initializers.items()
                  if u8_type(self.variables[d]["type"])
                  and self.variables[d]["function"] == helper["caller_function"]
                  and helper["result"] in self.dependencies(initializer)]
        copy_values = {d: self.declared_values(d) for d in copies}
        if not copies or any(not v for v in copy_values.values()):
            raise Incomplete("ordinary helper caller Copy observation absent")
        operand_dependencies = self.dependencies(helper["argument"])
        phase_origins = []
        for decl in operand_dependencies:
            if decl not in self.initializers or not usize_type(self.variables[decl]["type"]):
                continue
            dependencies = self.dependencies(self.initializers[decl])
            if not any(usize_type(self.variables[d]["type"]) for d in dependencies):
                phase_origins.append(decl)
        origin = one(phase_origins, "ordinary actual-phase origin in C Copy graph")
        post_phase = []
        for decl, initializer in self.initializers.items():
            node = self.nodes[self.variables[decl]["node"]]
            if (usize_type(self.variables[decl]["type"])
                    and node["function"] == helper["caller_function"]
                    and node["range"][0] > helper["call"]["range"][1]
                    and origin in self.dependencies(initializer)):
                post_phase.extend(self.declared_values(decl))
        if not post_phase:
            raise Incomplete("ordinary caller phase-after Copy path not observed")
        return {"kind": "ordinary_control", "expected": {
                    "actual": observation["actual_argument"], "fresh_formal": observation["formal_value"],
                    "condition": observation["conditional_choice_inferred"],
                    "normal_result": observation["caller_call_result_observed"],
                    "normal_copy": next(iter(copy_values.values()))[-1],
                    "phase_after": post_phase[-1],
                    "calls": len(helper["observations"]), "stores": 0, "loops": 0},
                "helper_observations": helper["observations"],
                "caller_result_copies": copy_values,
                "post_phase_copies": post_phase,
                "roles": {"helper_conditional": helper["if"]["id"], "helper_call_result": helper["result"],
                          "helper_call": helper["call"]["id"], "callee_function": helper["callee_function"],
                          "helper_operand": helper["operand"], "actual_origin": origin},
                "limits": ["helper-only evidence; no loop/store claim",
                           helper["evidence_limit"]]}


def compare(observed, expected):
    differences = []
    def equal(key, actual, wanted):
        if actual != wanted:
            differences.append({"key": key, "actual": actual, "expected": wanted})
    if expected["kind"] == "ordinary_control":
        if observed["kind"] != "ordinary_control":
            raise Incomplete("ordinary control unexpectedly decoded as loop")
        for key in ("actual", "fresh_formal", "condition", "normal_result", "normal_copy", "phase_after", "calls", "stores", "loops"):
            equal(key, observed["expected"][key], expected["expected"][key])
        for decl, values in observed["caller_result_copies"].items():
            equal("caller_result_copy." + decl, values, [expected["expected"]["normal_result"]] * len(values))
        equal("post_phase_copies", observed["post_phase_copies"],
              [expected["expected"]["phase_after"]] * len(observed["post_phase_copies"]))
        return differences
    if observed["kind"] != "loop":
        raise Incomplete("loop case has no decoded actual loop")
    for key in ("phase_headers", "outer_guard_results", "helper_arguments", "fresh_formal_values",
                "helper_branch_results", "helper_normal_results", "replace_old_results",
                "selected_after_each_store", "sibling_after_each_store", "continue_arguments",
                "helper_call_count", "store_count", "continue_count", "break_count",
                "initial_selected", "initial_sibling"):
        equal(key, observed[key], expected[key])
    for key in ("break_result", "selected", "sibling", "entry_selected_copy", "entry_sibling_copy", "starting_phase_copy"):
        equal("post." + key, observed["post"][key], expected["post"][key])
    equal("actual_break_phase", observed["post"]["actual_break_phase"], expected["post"]["break_result"])
    equal("actual_incoming", observed["actual_incoming"], expected["helper_normal_results"])
    for decl, values in observed["caller_result_copies"].items():
        equal("caller_result_copy." + decl, values, expected["helper_normal_results"])
    for decl, values in observed["old_result_copies"].items():
        equal("old_result_copy." + decl, values, expected["replace_old_results"])
    for key, wanted in (("outside_result_copies", expected["post"]["break_result"]),
                        ("outside_starting_phase_copies", expected["post"]["starting_phase_copy"]),
                        ("entry_selected_copies_at_break", expected["post"]["entry_selected_copy"]),
                        ("entry_sibling_copies_at_break", expected["post"]["entry_sibling_copy"]),
                        ("post_selected_loads", expected["post"]["selected"]),
                        ("post_sibling_loads", expected["post"]["sibling"])):
        equal(key, observed[key], [wanted] * len(observed[key]))
    return differences


def main():
    if len(sys.argv) != 6:
        raise SystemExit("compare.py ORACLE CASE OBSERVER_MAP RAW_TRACE OUTPUT_JSON")
    oracle_path, case_id, map_path, trace_path, output_path = sys.argv[1:]
    report = {"schema": "R1185_POST_CAPTURE_COMPARISON_DRAFT", "case": case_id,
              "status": "INCOMPLETE", "whole_loop_validator": "NOT_RUN/NO_APPLICABLE_PUBLIC_API"}
    result = 1
    try:
        model_bytes = Path(map_path).read_bytes()
        trace = Path(trace_path).read_bytes()
        report["map_sha256"] = hashlib.sha256(model_bytes).hexdigest()
        report["raw_trace_sha256"] = hashlib.sha256(trace).hexdigest()
        decoder = Decoder(json.loads(model_bytes), trace)
        observed = decoder.decode()
        report["observed"] = observed
        # No oracle is available to Decoder or to any role-selection operation.
        oracle = json.loads(Path(oracle_path).read_text())
        expected = one((c for c in oracle["cases"] if c["id"] == case_id), "frozen case")
        differences = compare(observed, expected)
        report["differences"] = differences
        if differences:
            report["status"], result = "FROZEN_ORACLE_MISMATCH", 42
        else:
            report["status"], result = "DECODED_FINITE_VALUES_MATCH", 0
        report["limit"] = "No semantic authority or general soundness follows; public facts and machine CALL observability remain separate."
    except Mismatch as error:
        report["status"], report["error"], result = "OBSERVED_IDENTITY_MISMATCH", str(error), 42
    except Exception as error:
        report["status"], report["error"], result = "R_DECODER_INCOMPLETE", str(error), 1
    Path(output_path).write_text(json.dumps(report, indent=2) + "\n")
    print(report["status"])
    return result


if __name__ == "__main__":
    raise SystemExit(main())
