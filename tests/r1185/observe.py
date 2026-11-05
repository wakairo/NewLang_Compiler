"""R1185 passive C observer: original emitted C + its real Clang JSON AST only.

No guest NewLang source, oracle, expected values, replacement main, generated
guest call, arithmetic, assignment or control. Insertions are read-only logging
statements at existing compound-statement boundaries. The original C bytes are
preserved in order. This file is prepared as text; no local AST run is allowed.
"""
from __future__ import annotations

from collections import defaultdict
import hashlib
import json
from pathlib import Path
import re
import sys


class ObservationError(RuntimeError):
    pass


def digest(data):
    return hashlib.sha256(data).hexdigest()


def location(value):
    return value.get("expansionLoc", value)


def bounds(node):
    region = node.get("range", {})
    first = location(region.get("begin", {}))
    last = location(region.get("end", {}))
    if "offset" not in first or "offset" not in last:
        return None
    return first["offset"], last["offset"] + last.get("tokLen", 0)


def ctype(node):
    info = node.get("type", {})
    return info.get("desugaredQualType", info.get("qualType", ""))


def scalar_kind(kind):
    if "volatile" in kind or "_Atomic" in kind or "(" in kind or "[" in kind:
        return None
    if "*" in kind:
        return "pointer"
    bare = re.sub(r"\b(const|restrict|_Atomic)\b", "", kind).strip()
    if re.fullmatch(r"(?:(?:unsigned|signed) )?(?:char|short|int|long|long long)(?: int)?|_Bool|bool", bare):
        return "number"
    return None


def simple_atom(node):
    if "volatile" in ctype(node):
        return False
    kind = node.get("kind")
    children = node.get("inner", [])
    if kind in ("ImplicitCastExpr", "CStyleCastExpr", "ParenExpr"):
        return len(children) == 1 and simple_atom(children[0])
    if kind in ("DeclRefExpr", "IntegerLiteral", "CharacterLiteral"):
        return True
    if kind == "MemberExpr":
        return len(children) == 1 and simple_atom(children[0])
    if kind == "UnaryOperator" and node.get("opcode") in ("*", "&"):
        return len(children) == 1 and simple_atom(children[0])
    if kind == "ArraySubscriptExpr":
        return len(children) == 2 and all(simple_atom(x) for x in children)
    return False


def unwrapped(node):
    while node.get("kind") in ("ImplicitCastExpr", "CStyleCastExpr", "ParenExpr"):
        children = node.get("inner", [])
        if len(children) != 1:
            break
        node = children[0]
    return node


class Observer:
    def __init__(self, original, ast, filename):
        self.original = original
        self.ast = ast
        self.filename = str(Path(filename).resolve())
        self.insertions = defaultdict(list)
        self.nodes = []
        self.node_ids = {}
        self.parent = {}
        self.function = {}
        self.variables = {}
        self.declarations = {}
        self.ambiguous_declarations = set()
        self.decl_ids = {}
        self.records = {}
        self.records_by_ast_id = {}
        self.field_ids = {}
        self.functions = []
        self.events = []
        self.unobservable = []
        self.field_count = 0
        self._records(ast)
        self._typedef_records(ast)
        for candidate in ast.get("inner", []):
            if candidate.get("kind") != "FunctionDecl":
                continue
            bodies = [x for x in candidate.get("inner", [])
                      if x.get("kind") == "CompoundStmt"]
            if not bodies:
                continue
            loc = location(candidate.get("loc", {}))
            region = bounds(candidate)
            if (loc.get("includedFrom") or not region
                    or region[0] < 0 or region[1] > len(original)):
                continue
            if loc.get("file") and str(Path(loc["file"]).resolve()) != self.filename:
                continue
            if not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", candidate.get("name", "")):
                raise ObservationError("non-identifier function spelling")
            function_id = "f" + str(len(self.functions) + 1)
            self.functions.append({"id": function_id,
                                   "name": candidate["name"],
                                   "return_type": ctype(candidate)})
            self._index(candidate, None, function_id)
        if not self.functions:
            raise ObservationError("no source-owned function definition in actual AST")
        for node in self.nodes:
            if node.get("kind") in ("VarDecl", "ParmVarDecl"):
                if not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", node.get("name", "")):
                    raise ObservationError("non-identifier variable spelling")
                decl = "d" + str(len(self.variables) + 1)
                self.decl_ids[node["id"]] = decl
                self.declarations[decl] = node
                scope = self._scope(node)
                self.variables[decl] = {
                    "id": decl, "ast_id": node["id"], "name": node["name"],
                    "type": ctype(node), "node": self.node_ids[id(node)],
                    "scope": self.node_ids.get(id(scope)),
                    "initialized": node.get("kind") == "ParmVarDecl" or "init" in node,
                    "parameter": node.get("kind") == "ParmVarDecl",
                    "function": self.function[id(node)]}
        spellings = defaultdict(list)
        for decl, info in self.variables.items():
            spellings[(info["function"], info["name"])].append(decl)
        for declarations in spellings.values():
            if len(declarations) > 1:
                self.ambiguous_declarations.update(declarations)
                self.unobservable.append({"kind": "declaration_spelling_collision",
                    "declarations": declarations,
                    "reason": "all same-spelling declarations in this function omitted; no general lexical resolver"})

    def _records(self, node):
        if node.get("kind") == "RecordDecl" and node.get("completeDefinition"):
            fields = []
            for child in node.get("inner", []):
                if child.get("kind") != "FieldDecl":
                    continue
                self.field_count += 1
                field_id = "field" + str(self.field_count)
                self.field_ids[child.get("id")] = field_id
                fields.append({"id": field_id, "ast_id": child.get("id"),
                               "name": child.get("name"), "type": ctype(child),
                               "declared_index": len(fields)})
            if node.get("tagUsed") == "struct" and node.get("name"):
                self.records["struct " + node["name"]] = fields
            if node.get("tagUsed") == "struct" and node.get("id"):
                self.records_by_ast_id[node["id"]] = fields
        for child in node.get("inner", []):
            self._records(child)

    def _typedef_records(self, node):
        if node.get("kind") == "TypedefDecl" and node.get("name"):
            pending = [node]
            related = set()
            while pending:
                value = pending.pop()
                for key in ("ownedTagDecl", "decl"):
                    reference = value.get(key, {})
                    if reference.get("kind") == "RecordDecl" and reference.get("id") in self.records_by_ast_id:
                        related.add(reference["id"])
                pending.extend(value.get("inner", []))
            if len(related) == 1:
                fields = self.records_by_ast_id[next(iter(related))]
                self.records[node["name"]] = fields
                for name in (node.get("type", {}).get("qualType"),
                             node.get("type", {}).get("desugaredQualType")):
                    if name:
                        self.records[name] = fields
        for child in node.get("inner", []):
            self._typedef_records(child)

    def _index(self, node, parent, function):
        self.node_ids[id(node)] = "n" + str(len(self.nodes) + 1)
        self.nodes.append(node)
        self.parent[id(node)] = parent
        self.function[id(node)] = function
        for child in node.get("inner", []):
            self._index(child, node, function)

    def _scope(self, node):
        if node.get("kind") == "ParmVarDecl":
            parent = self.parent[id(node)]
            return next(x for x in parent["inner"] if x.get("kind") == "CompoundStmt")
        current = self.parent[id(node)]
        while current is not None:
            if current.get("kind") in ("CompoundStmt", "ForStmt"):
                return current
            current = self.parent[id(current)]
        return None

    def text(self, node):
        region = bounds(node)
        if not region or not 0 <= region[0] <= region[1] <= len(self.original):
            raise ObservationError("AST source range outside original C")
        return self.original[region[0]:region[1]].decode("utf-8")

    def after_statement(self, node):
        region = bounds(node)
        if not region:
            raise ObservationError("statement range absent")
        end = region[1]
        if self.original[max(region[0], end - 1):end] == b";":
            return end
        while end < len(self.original) and self.original[end:end + 1] in b" \t\r\n":
            end += 1
        if self.original[end:end + 1] != b";":
            raise ObservationError("statement lacks its real semicolon")
        return end + 1

    def event(self, kind, node, **extra):
        event_id = "e" + str(len(self.events) + 1)
        self.events.append({"id": event_id, "kind": kind,
                            "node": self.node_ids[id(node)],
                            "function": self.function[id(node)], **extra})
        return event_id

    def marker(self, event, kind):
        return f'(void)fprintf(stderr, "R1185|EVENT|{event}|{kind}\\n");\n'

    def log_variable(self, event, decl):
        if decl in self.ambiguous_declarations:
            return ""
        info = self.variables[decl]
        name = info["name"]
        kind = scalar_kind(info["type"])
        if kind == "number":
            return (f'(void)fprintf(stderr, "R1185|VALUE|{event}|{decl}|%llu|%p\\n", '
                    f'(unsigned long long)({name}), (void *)&{name});\n')
        if kind == "pointer":
            return (f'(void)fprintf(stderr, "R1185|POINTER|{event}|{decl}|%p|%p\\n", '
                    f'(void *)({name}), (void *)&{name});\n')
        bare = re.sub(r"\bconst\b", "", info["type"]).strip()
        fields = self.records.get(bare)
        if (fields and not bare.startswith("union ")
                and all(scalar_kind(field["type"]) == "number"
                        and re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", field["name"] or "")
                        for field in fields)):
            result = f'(void)fprintf(stderr, "R1185|ROOT|{event}|{decl}|%p\\n", (void *)&{name});\n'
            for field in fields:
                expression = name + "." + field["name"]
                result += (f'(void)fprintf(stderr, "R1185|FIELD|{event}|{decl}|{field["id"]}|%llu|%p\\n", '
                           f'(unsigned long long)({expression}), (void *)&({expression}));\n')
            return result
        return ""

    def active(self, position, function, records_only=False):
        for decl, info in self.variables.items():
            if info["function"] != function or not info["initialized"]:
                continue
            node = next(n for n in self.nodes if self.node_ids[id(n)] == info["node"])
            scope = self._scope(node)
            region = bounds(scope) if scope else None
            declaration = bounds(node)
            if (not region or not region[0] < position < region[1]
                    or not declaration or declaration[1] > position):
                continue
            if records_only and scalar_kind(info["type"]) is not None:
                continue
            yield decl

    def snapshot(self, event, position, function, records_only=False):
        return "".join(self.log_variable(event, decl)
                       for decl in self.active(position, function, records_only))

    def safe_direct_argument(self, node, position, function):
        node = unwrapped(node)
        if node.get("kind") in ("IntegerLiteral", "CharacterLiteral"):
            return True
        if node.get("kind") != "DeclRefExpr":
            return False
        decl = self.decl_ids.get(node.get("referencedDecl", {}).get("id"))
        if not decl or decl in self.ambiguous_declarations:
            return False
        info = self.variables[decl]
        declaration = self.declarations[decl]
        scope = self._scope(declaration)
        region = bounds(scope) if scope else None
        declared = bounds(declaration)
        return (info["function"] == function and info["initialized"]
                and scalar_kind(info["type"]) == "number"
                and declared is not None and declared[1] < position
                and region is not None and region[0] < position < region[1])

    def add(self, position, text):
        if text:
            if not 0 <= position <= len(self.original):
                raise ObservationError("insertion outside original C")
            self.insertions[position].append("\n" + text)

    def branch_entry(self, block, owner, kind):
        if block.get("kind") != "CompoundStmt":
            self.unobservable.append({"node": self.node_ids[id(owner)],
                                      "kind": kind, "reason": "noncompound body; no control rewrite"})
            return
        start = bounds(block)[0]
        if self.original[start:start + 1] != b"{":
            raise ObservationError("compound opening brace mismatch")
        event = self.event(kind, owner, body=self.node_ids[id(block)])
        text = self.marker(event, kind)
        if kind in ("LOOP_HEADER", "FUNCTION_ENTRY"):
            text += self.snapshot(event, start + 1, self.function[id(owner)])
        self.add(start + 1, text)

    def prepare(self):
        for node in self.nodes:
            kind = node.get("kind")
            parent = self.parent[id(node)]
            if kind == "FunctionDecl":
                body = next(x for x in node["inner"] if x.get("kind") == "CompoundStmt")
                self.branch_entry(body, node, "FUNCTION_ENTRY")
            elif kind in ("WhileStmt", "ForStmt", "DoStmt"):
                children = [x for x in node.get("inner", []) if x.get("kind")]
                body = children[0] if kind == "DoStmt" else children[-1]
                self.branch_entry(body, node, "LOOP_HEADER")
            elif kind == "IfStmt":
                children = node.get("inner", [])
                if node.get("hasElse"):
                    self.branch_entry(children[-2], node, "IF_TRUE")
                    self.branch_entry(children[-1], node, "IF_FALSE")
                else:
                    self.branch_entry(children[-1], node, "IF_TRUE")
            elif kind == "DeclStmt" and parent and parent.get("kind") == "CompoundStmt":
                event = self.event("DECLARATION", node)
                text = self.marker(event, "DECLARATION")
                for child in node.get("inner", []):
                    decl = self.decl_ids.get(child.get("id"))
                    if decl and self.variables[decl]["initialized"]:
                        text += self.log_variable(event, decl)
                self.add(self.after_statement(node), text)
                declarations = node.get("inner", [])
                direct = None
                if len(declarations) == 1 and declarations[0].get("kind") == "VarDecl":
                    initializer = declarations[0].get("inner", [])
                    if initializer:
                        direct = unwrapped(initializer[-1])
                if direct is not None and direct.get("kind") == "CallExpr":
                    position = bounds(node)[0]
                    call_children = direct.get("inner", [])
                    arguments = call_children[1:]
                    callee = unwrapped(call_children[0]) if call_children else {}
                    known_direct = (callee.get("kind") == "DeclRefExpr"
                                    and callee.get("referencedDecl", {}).get("kind") == "FunctionDecl")
                    safe = known_direct and all(scalar_kind(ctype(arg)) == "number"
                               and self.safe_direct_argument(arg, position, self.function[id(node)])
                               for arg in arguments)
                    if not safe:
                        self.unobservable.append({"node": self.node_ids[id(direct)],
                            "kind": "call_site", "reason": "arguments not proved initialized direct reads before single declarator"})
                        continue
                    call_event = self.event("CALL_SITE", direct)
                    before_call = self.marker(call_event, "CALL_SITE")
                    for index, argument in enumerate(arguments):
                        actual = self.text(argument)
                        before_call += (f'(void)fprintf(stderr, "R1185|CALL_ARGUMENT|{call_event}|{index}|%llu\\n", '
                                        f'(unsigned long long)({actual}));\n')
                    self.add(position, before_call)
            elif kind in ("ContinueStmt", "BreakStmt", "ReturnStmt"):
                if not parent or parent.get("kind") != "CompoundStmt":
                    self.unobservable.append({"node": self.node_ids[id(node)],
                                              "kind": kind, "reason": "noncompound boundary"})
                    continue
                label = {"ContinueStmt": "CONTINUE", "BreakStmt": "BREAK",
                         "ReturnStmt": "RETURN"}[kind]
                event = self.event(label, node)
                position = bounds(node)[0]
                text = self.marker(event, label)
                if label != "RETURN":
                    text += self.snapshot(event, position, self.function[id(node)])
                elif node.get("inner") and simple_atom(node["inner"][0]):
                    value = node["inner"][0]
                    if scalar_kind(ctype(value)) == "number":
                        expression = self.text(value)
                        text += (f'(void)fprintf(stderr, "R1185|RETURN_VALUE|{event}|%llu\\n", '
                                 f'(unsigned long long)({expression}));\n')
                self.add(position, text)
            elif (kind in ("BinaryOperator", "CompoundAssignOperator")
                  and node.get("opcode") in ("=", "+=", "-=")):
                if not parent or parent.get("kind") != "CompoundStmt":
                    self.unobservable.append({"node": self.node_ids[id(node)],
                                              "kind": "assignment", "reason": "nested expression; no rewrite"})
                    continue
                left, right = node.get("inner", [])
                if not simple_atom(left):
                    self.unobservable.append({"node": self.node_ids[id(node)],
                                              "kind": "assignment", "reason": "nonpassive lvalue"})
                    continue
                lhs = self.text(left)
                scalar = scalar_kind(ctype(left))
                if scalar != "number":
                    continue
                memory = unwrapped(left).get("kind") != "DeclRefExpr"
                label = "STORE" if memory else "ASSIGN"
                before = self.event(label + "_BEFORE", node,
                                    lhs=self.node_ids[id(left)], rhs=self.node_ids[id(right)])
                after = self.event(label + "_AFTER", node,
                                   lhs=self.node_ids[id(left)], rhs=self.node_ids[id(right)])
                first = self.marker(before, label + "_BEFORE")
                if memory:
                    first += f'(void)fprintf(stderr, "R1185|DESTINATION_ADDRESS|{before}|%p\\n", (void *)&({lhs}));\n'
                    self.unobservable.append({"node": self.node_ids[id(node)],
                        "kind": "additional_old_before_store_read",
                        "reason": "omitted conservatively; observe existing guest old-load/result declarations instead; no pointer initializer authority"})
                if node.get("opcode") == "=" and simple_atom(right) and scalar_kind(ctype(right)) == "number":
                    rhs = self.text(right)
                    first += (f'(void)fprintf(stderr, "R1185|INCOMING|{before}|%llu\\n", '
                              f'(unsigned long long)({rhs}));\n')
                last = self.marker(after, label + "_AFTER")
                last += (f'(void)fprintf(stderr, "R1185|DESTINATION|{after}|%p|%llu\\n", '
                         f'(void *)&({lhs}), (unsigned long long)({lhs}));\n')
                last += self.snapshot(after, self.after_statement(node), self.function[id(node)], True)
                self.add(bounds(node)[0], first)
                self.add(self.after_statement(node), last)
        observed_calls = {event["node"] for event in self.events
                          if event["kind"] == "CALL_SITE"}
        for node in self.nodes:
            if (node.get("kind") == "CallExpr"
                    and self.node_ids[id(node)] not in observed_calls):
                self.unobservable.append({"node": self.node_ids[id(node)],
                    "kind": "call_site", "reason": "no safe direct single-declarator insertion proved; nested/conditional calls never hoisted"})
        self.add(0, "#include <stdio.h>\n")
        result = bytearray()
        last = 0
        edits = []
        for position, pieces in sorted(self.insertions.items()):
            result.extend(self.original[last:position])
            addition = "".join(pieces).encode("utf-8")
            result.extend(addition)
            edits.append({"original_offset": position, "inserted_bytes": len(addition),
                          "sha256": digest(addition), "text": addition.decode("utf-8")})
            last = position
        result.extend(self.original[last:])
        # This is a byte identity assertion, not a guest execution or AST synthesis.
        restored = bytearray(result)
        shift = 0
        removals = []
        for edit in edits:
            removals.append((edit["original_offset"] + shift, edit["inserted_bytes"]))
            shift += edit["inserted_bytes"]
        for position, length in reversed(removals):
            del restored[position:position + length]
        if bytes(restored) != self.original:
            raise ObservationError("passive insertion restoration failed")
        metadata = []
        for node in self.nodes:
            entry = {"id": self.node_ids[id(node)], "kind": node.get("kind"),
                     "parent": self.node_ids.get(id(self.parent[id(node)])),
                     "function": self.function[id(node)], "range": bounds(node),
                     "children": [self.node_ids[id(x)] for x in node.get("inner", [])],
                     "type": ctype(node)}
            for field in ("opcode", "name", "value", "hasElse", "isArrow", "init"):
                if field in node:
                    entry[field] = node[field]
            if node.get("referencedDecl"):
                entry["ref_decl"] = self.decl_ids.get(node["referencedDecl"]["id"])
                entry["ref_name"] = node["referencedDecl"].get("name")
            if node.get("referencedMemberDecl"):
                entry["ref_field"] = self.field_ids.get(node["referencedMemberDecl"])
            entry["decl"] = self.decl_ids.get(node.get("id"))
            metadata.append(entry)
        model = {"schema": "R1185_PASSIVE_INSERT_ONLY_V1",
                 "original_c_sha256": digest(self.original),
                 "passive_c_sha256": digest(bytes(result)),
                 "original_restored_exactly": True,
                 "inputs": "original emitted C and compiler-produced AST only; no source/oracle",
                 "functions": self.functions, "records": self.records,
                 "variables": self.variables, "nodes": metadata,
                 "events": self.events, "insertions": edits,
                 "not_observable": self.unobservable,
                 "limits": ["no claim of machine CALL from C source call",
                            "store markers identify actual original-C AST boundaries, not proof an optimized machine STORE survives",
                            "no absolute/cross-run address equality",
                            "no invented semantic freshness or authority from logger addresses",
                            "uninitialized variables not read; only existing assigned lvalues logged",
                            "no rewritten guest expression, statement or control"]}
        return bytes(result), model


def main():
    if len(sys.argv) != 5:
        raise SystemExit("usage: observe.py ORIGINAL_C ACTUAL_AST_JSON PASSIVE_C MAP_JSON")
    original_path, ast_path, output_path, map_path = map(Path, sys.argv[1:])
    original = original_path.read_bytes()
    ast_bytes = ast_path.read_bytes()
    ast = json.loads(ast_bytes)
    observer = Observer(original, ast, original_path)
    output, model = observer.prepare()
    model["actual_ast_sha256"] = digest(ast_bytes)
    Path(output_path).write_bytes(output)
    Path(map_path).write_text(json.dumps(model, indent=2) + "\n")


if __name__ == "__main__":
    main()
