"""R1190 post-capture comparison. Role decoding never receives the oracle.

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
            if len(pieces) < 3 or pieces[0] != "R1190":
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

    def copy_path(self, operand, stop=None):
        """Actual immutable C identifier path; no arithmetic/interpreter."""
        path, seen = [], set()
        node = self.unwrap(operand)
        while node['kind'] == 'DeclRefExpr' and node.get('ref_decl'):
            decl = node['ref_decl']
            if decl == stop:
                return list(reversed(path)), {'kind': 'declaration', 'id': decl}
            if decl in seen or decl in self.assignments or decl not in self.initializers:
                raise Incomplete('Copy path is mutable, cyclic, or not initialized: ' + decl)
            if not usize_type(self.variables[decl]['type']):
                raise Incomplete('Copy path leaves scalar usize')
            seen.add(decl)
            path.append(decl)
            node = self.unwrap(self.initializers[decl])
        if stop is not None or node['kind'] != 'IntegerLiteral':
            raise Incomplete('Copy path does not end at its actual declaration/literal origin')
        return list(reversed(path)), {'kind': 'literal', 'value': int(node['value'])}

    def read_field(self, event, root, field):
        row = one((r for r in self.data(event, 'FIELD') if r[2] == root and r[3] == field),
                  'direct original-root field read at actual event')
        return int(row[4]), row[5]

    def direct_value(self, event, decl):
        row = one((r for r in self.data(event, 'VALUE') if r[2] == decl),
                  'direct declaration VALUE at actual event')
        return int(row[3])

    def fresh_values(self, declarations, lower, upper):
        values = {}
        for decl in declarations:
            hits = []
            for event in self.matching('DECLARATION'):
                if lower < event['sequence'] < upper:
                    hits.extend(int(r[3]) for r in self.data(event, 'VALUE') if r[2] == decl)
            values[decl] = one(hits, 'one direct Copy/load declaration in this dynamic interval')
        return values

    def decode(self):
        loop = one((n for n in self.nodes.values() if n['kind'] in ('WhileStmt', 'ForStmt', 'DoStmt')),
                   'one actual C loop')
        if loop['kind'] == 'DoStmt':
            raise Incomplete('do-loop requires another actual edge decoder')
        if loop['kind'] == 'ForStmt' and any(self.nodes[x].get('kind') for x in loop['children'][:-1]):
            raise Incomplete('nonempty for clauses require another actual edge decoder')
        desc = list(self.descendants(loop)); ids = {n['id'] for n in desc}
        body = self.nodes[loop['children'][-1]]
        break_node = one((n for n in desc if n['kind'] == 'BreakStmt'), 'one actual Break')
        outer = one((n for n in desc if n['kind'] == 'IfStmt' and not n.get('hasElse')
                     and n.get('parent') == body['id']
                     and any(x['kind'] == 'ContinueStmt' for x in self.descendants(n))),
                    'outer true Continue / false fallthrough Break')
        if (body['kind'] != 'CompoundStmt' or break_node.get('parent') != body['id']
                or len(outer['children']) != 2 or outer['range'][1] >= break_node['range'][0]):
            raise Incomplete('actual guard/Continue/Break structure outside finite decoder')
        counter = one((d for d in self.dependencies(outer['children'][0])
                       if d in self.assignments and usize_type(self.variables[d]['type'])
                       and any(n in ids for n in self.assignments[d])), 'actual loop carry')
        headers = self.matching('LOOP_HEADER', loop['id'])
        if not headers:
            raise Incomplete('no actual loop header event')
        phases = [self.value(e, counter) for e in headers]
        intervals, guards = [], []
        for i, header in enumerate(headers):
            end = headers[i + 1]['sequence'] if i + 1 < len(headers) else len(self.events)
            true = [e for e in self.matching('IF_TRUE', outer['id']) if header['sequence'] < e['sequence'] < end]
            brk = [e for e in self.matching('BREAK', break_node['id']) if header['sequence'] < e['sequence'] < end]
            if len(true) == 1 and not brk:
                guards.append(True)
            elif len(brk) == 1 and not true:
                guards.append(False)
            else:
                raise Incomplete('header interval has no unique executed true/Break boundary')
            intervals.append((header['sequence'], end))
        helper = self.helper(n for n in desc if n['id'] != outer['id'])
        actual_path, _ = self.copy_path(helper['argument'], counter)
        formal_path, _ = self.copy_path(helper['comparison']['children'][0], helper['formal'])
        store, place = one(((n, self.place(n['children'][0])) for n in desc
                           if n['kind'] == 'BinaryOperator' and n.get('opcode') == '='
                           and self.place(n['children'][0]) and self.place(n['children'][0])[1] is not None),
                          'one original selected-child store')
        root, selected = place
        root_node = self.nodes[self.variables[root]['node']]
        if root_node['range'][1] >= loop['range'][0]:
            raise Incomplete('original record not declared before loop')
        fields = self.model['records'].get(self.variables[root]['type'].replace('const ', '').strip(), [])
        sibling = one((f['id'] for f in fields if f['id'] != selected), 'one original sibling field')
        if helper['result'] not in self.dependencies(store['children'][1]):
            raise Incomplete('store incoming does not follow actual caller result')
        first_root = one((e for e in self.events if e['sequence'] < headers[0]['sequence']
                          and any(r[2] == root for r in self.data(e, 'ROOT'))), 'initial original record declaration')
        initial_selected, selected_address = self.read_field(first_root, root, selected)
        initial_sibling, sibling_address = self.read_field(first_root, root, sibling)
        if selected_address == sibling_address:
            raise Mismatch('selected and sibling C addresses coincide in this run')
        root_addresses = [r[3] for e in self.events for r in self.data(e, 'ROOT') if r[2] == root]
        if len(set(root_addresses)) != 1:
            raise Mismatch('original C record address changed within run')
        for event in self.events:
            for row in self.data(event, 'FIELD'):
                if row[2] == root and row[3] in (selected, sibling):
                    wanted = selected_address if row[3] == selected else sibling_address
                    if row[5] != wanted:
                        raise Mismatch('original C child address changed within run')
        before_loads, entry_loads, current_loads, sibling_loads, post_selected, post_sibling = [], [], [], [], [], []
        for decl, initializer in self.initializers.items():
            if not u8_type(self.variables[decl]['type']):
                continue
            n = self.nodes[self.variables[decl]['node']]; origin = self.place(initializer)
            if origin == place:
                if n['range'][1] < loop['range'][0]: entry_loads.append(decl)
                elif n['id'] in ids and n['range'][0] < store['range'][0]: before_loads.append(decl)
                elif n['id'] in ids and n['range'][0] > store['range'][1]: current_loads.append(decl)
                elif n['range'][0] > loop['range'][1]: post_selected.append(decl)
            if origin == (root, sibling):
                if n['id'] in ids and n['range'][0] > store['range'][1]: sibling_loads.append(decl)
                elif n['range'][0] > loop['range'][1]: post_sibling.append(decl)
        if not entry_loads or not before_loads or not current_loads or not sibling_loads or not post_selected or not post_sibling:
            raise Incomplete('original entry/before/old/current/sibling/post read roles absent')
        # The last selected-child load dominating this standalone store is the
        # replace-old load. Earlier selected loads are retained separately.
        old_load = max(before_loads, key=lambda d: self.nodes[self.variables[d]['node']]['range'][0])
        def copies(origins, region):
            result = []
            for decl, initializer in self.initializers.items():
                n = self.nodes[self.variables[decl]['node']]
                if (u8_type(self.variables[decl]['type']) and n['function'] == loop['function']
                        and region(n) and any(o in self.dependencies(initializer) for o in origins)):
                    result.append(decl)
            return result
        inside = lambda n: n['id'] in ids
        after_loop = lambda n: n['range'][0] > loop['range'][1]
        normal_copies = copies([helper['result']], inside)
        old_copies = copies([old_load], lambda n: inside(n) and n['range'][0] > store['range'][1])
        entry_each = copies(entry_loads, inside)
        entry_after = copies(entry_loads, after_loop)
        if not normal_copies or not old_copies or not entry_each or not entry_after:
            raise Incomplete('caller normal/replace-old/retained entry Copy paths absent')
        stores_before = self.matching('STORE_BEFORE', store['id'])
        stores_after = self.matching('STORE_AFTER', store['id'])
        continues = [e for e in self.matching('CONTINUE') if e['static']['node'] in ids]
        breaks = self.matching('BREAK', break_node['id']); brk = one(breaks, 'one actual dynamic Break')
        active = sum(guards)
        if len(helper['observations']) != active or len(stores_before) != active or len(stores_after) != active or len(continues) != active:
            raise Incomplete('actual active-header/call/store/Continue counts differ')
        actual_copies, formal_copies, before_values, old_values = [], [], [], []
        currents, siblings, incoming, entry_values = [], [], [], []
        current_reads, sibling_reads, normal_values, old_copy_values = [], [], [], []
        for j, (low, high) in enumerate(x for x, go in zip(intervals, guards) if go):
            before, after, cont, call = stores_before[j], stores_after[j], continues[j], helper['observations'][j]
            if not (low < call['call_sequence'] < call['caller_result_sequence'] < before['sequence'] < after['sequence'] < cont['sequence'] < high):
                raise Incomplete('helper/replace/Continue event order not proved in this interval')
            actual_copies.append(self.fresh_values(actual_path, low, call['call_sequence']))
            formal_copies.append(self.fresh_values(formal_path, call['entry_sequence'], call['return_sequence']))
            loads = self.fresh_values(before_loads, low, before['sequence'])
            before_values.append(loads); old_values.append(loads[old_load])
            address = one(self.data(before, 'DESTINATION_ADDRESS'), 'original C store address')[2]
            dest = self.destination(after)
            if address != selected_address or dest['address'] != selected_address:
                raise Mismatch('store did not target the original selected child')
            incoming.append(int(one(self.data(before, 'INCOMING'), 'captured incoming value')[2]))
            currents.append(dest['value']); siblings.append(self.read_field(after, root, sibling)[0])
            current_reads.append(self.fresh_values(current_loads, after['sequence'], cont['sequence']))
            sibling_reads.append(self.fresh_values(sibling_loads, after['sequence'], cont['sequence']))
            normal_values.append(self.fresh_values(normal_copies, call['caller_result_sequence'], before['sequence']))
            old_copy_values.append(self.fresh_values(old_copies, after['sequence'], cont['sequence']))
            entry_values.append(self.fresh_values(entry_each, after['sequence'], cont['sequence']))
        tail_ids = body['children'][body['children'].index(outer['id']) + 1:body['children'].index(break_node['id'])]
        targets = set()
        for item in tail_ids:
            for n in self.descendants(item):
                if n['kind'] == 'BinaryOperator' and n.get('opcode') == '=':
                    left = self.unwrap(n['children'][0]); decl = left.get('ref_decl')
                    if left['kind'] == 'DeclRefExpr' and decl != counter and decl and usize_type(self.variables[decl]['type']):
                        targets.add(decl)
        loop_result = one(targets, 'source loop normal result written before actual Break')
        result_copies, outside_seed = {}, {}
        for decl, initializer in self.initializers.items():
            n = self.nodes[self.variables[decl]['node']]
            if n['function'] != loop['function'] or not after_loop(n) or not usize_type(self.variables[decl]['type']):
                continue
            values = self.declared_values(decl)
            if loop_result in self.dependencies(initializer): result_copies[decl] = values
            else: outside_seed[decl] = values
        if not result_copies:
            raise Incomplete('outside Break-result Copy observations absent')
        initial = self.initializers.get(counter)
        if initial is None:
            writes = [self.nodes[n] for n in self.assignments[counter] if self.nodes[n]['range'][1] < loop['range'][0]]
            initial = one(writes, 'actual pre-loop carry initialization')['children'][1]
        seed_path, seed_origin = self.copy_path(initial)
        seed_values = {d: self.declared_values(d, before=loop['range'][0]) for d in seed_path}
        post = {'break_value': self.value(brk, loop_result), 'actual_break_phase': self.value(brk, counter),
                'break_copies': result_copies,
                'post_selected': {d:self.declared_values(d) for d in post_selected},
                'post_sibling': {d:self.declared_values(d) for d in post_sibling},
                'entry_after': {d:self.declared_values(d) for d in entry_after}, 'outside_seed': outside_seed}
        aliases = [d for d, init in self.initializers.items() if '*' in self.variables[d]['type'] and self.place(init) == place]
        for event in self.events:
            for row in self.data(event, 'POINTER'):
                if row[2] in aliases and row[3] != selected_address:
                    raise Mismatch('observed loan C alias address differs from selected original child')
        return {'kind':'loop','phase_headers':phases,'outer_guard_results':guards,
                'selected_at_headers':[self.read_field(e,root,selected)[0] for e in headers],
                'sibling_at_headers':[self.read_field(e,root,sibling)[0] for e in headers],
                'helper_arguments':helper['arguments'],'fresh_formal_values':helper['formals'],
                'helper_branch_results':helper['choices'],'helper_normal_results':helper['result_values'],
                'helper_observations':helper['observations'],'actual_C_copy_values':actual_copies,'formal_C_copy_values':formal_copies,
                'before_selected_reads':before_values,'replace_old_results':old_values,'actual_incoming':incoming,
                'selected_after_each_store':currents,'sibling_after_each_store':siblings,
                'current_reads':current_reads,'sibling_reads':sibling_reads,'caller_result_copies':normal_values,
                'old_result_copies':old_copy_values,'entry_copies_in_iterations':entry_values,
                'continue_arguments':[self.value(e,counter) for e in continues],
                'helper_call_count':len(helper['observations']),'store_count':len(stores_after),
                'continue_count':len(continues),'break_count':len(breaks),'initial_selected':initial_selected,
                'initial_sibling':initial_sibling,'seed':{'origin':seed_origin,'C_copy_values':seed_values},'post':post,
                'roles':{'loop':loop['id'],'counter':counter,'outer_if':outer['id'],'loop_result':loop_result,
                         'helper_call':helper['call']['id'],'helper_formal':helper['formal'],'store':store['id'],
                         'old_load':old_load,'entry_loads':entry_loads,'actual_copy_path':actual_path,
                         'formal_copy_path':formal_path,'seed_copy_path':seed_path},
                'identity':{'original_C_root_decl':root,'selected_C_field':selected,'sibling_C_field':sibling,
                            'root_address':root_addresses[0],'selected_address':selected_address,
                            'sibling_address':sibling_address,'aliases':aliases},
                'limits':['finite original-C graph/runtime values; no machine CALL/STORE survival claim',
                          'helper ternary choice inferred from captured operands and exact actual AST',
                          'false guard inferred from actual fallthrough Break; no source expression reevaluation',
                          'C Copy path may contain receiving/temporary bindings; no automatic one-to-one source link count',
                          'addresses are in-run only, not source authority or a semantic freshness certificate']}


def main():
    if len(sys.argv) != 4:
        raise SystemExit('decode.py LINKED_ACTUAL_MAP RAW_TRACE OUTPUT_JSON')
    model_path, trace_path, output_path = map(Path,sys.argv[1:])
    report={'schema':'R1190_ACTUAL_DECODE_V1','status':'INCOMPLETE','whole_loop_validator':'NOT_RUN/NO_APPLICABLE_PUBLIC_API'}
    result=1
    try:
        model_bytes=model_path.read_bytes(); raw=trace_path.read_bytes()
        report['map_sha256']=hashlib.sha256(model_bytes).hexdigest(); report['raw_sha256']=hashlib.sha256(raw).hexdigest()
        report['observed']=Decoder(json.loads(model_bytes),raw).decode()
        report['status']='ACTUAL_VALUES_DECODED';result=0
    except Mismatch as error:
        report.update(status='OBSERVED_IDENTITY_MISMATCH',error=str(error));result=42
    except Exception as error:
        report.update(status='R_DECODER_INCOMPLETE',error=str(error))
    output_path.write_text(json.dumps(report,indent=2)+'\n');print(report['status'])
    return result

if __name__=='__main__':
    raise SystemExit(main())
