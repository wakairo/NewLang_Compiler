"""R1197 text-prepared guard around the exactly allowlisted neutral observer.

This module must execute only in a separately released GitHub Actions job.
It has no oracle input and never fills an unobserved value or semantic role.
"""
from __future__ import annotations

import json
import os
from pathlib import Path
import sys

from neutral.observe import (
    Observer, ObservationError, bounds, ctype, digest, location,
    scalar_kind, simple_atom, unwrapped,
)


class GuardedObserver(Observer):
    def __init__(self, original, ast, filename):
        super().__init__(original, ast, filename)
        self.guard_receipt = []

    def descendants(self, node):
        yield node
        for child in node.get("inner", []):
            yield from self.descendants(child)

    def declaration_at(self, expression, position):
        expression = unwrapped(expression)
        if expression.get("kind") != "DeclRefExpr":
            return None
        decl = self.decl_ids.get(expression.get("referencedDecl", {}).get("id"))
        if decl is None:
            return None
        info = self.variables[decl]
        node = self.declarations[decl]
        scope = self._scope(node)
        region = bounds(scope) if scope else None
        declared = bounds(node)
        if (not region or not declared or not region[0] < position < region[1]
                or declared[1] > position):
            return None
        return decl

    def initialized(self, decl):
        return decl is not None and self.variables[decl]["initialized"]

    def pointer_stable(self, decl):
        for node in self.nodes:
            kind = node.get("kind")
            children = node.get("inner", [])
            if (kind in ("BinaryOperator", "CompoundAssignOperator")
                    and node.get("opcode") in ("=", "+=", "-=")
                    and children):
                lhs = unwrapped(children[0])
                found = self.decl_ids.get(lhs.get("referencedDecl", {}).get("id"))
                if lhs.get("kind") == "DeclRefExpr" and found == decl:
                    return False
            if kind == "UnaryOperator" and node.get("opcode") in ("++", "--"):
                for child in self.descendants(node):
                    found = self.decl_ids.get(child.get("referencedDecl", {}).get("id"))
                    if child.get("kind") == "DeclRefExpr" and found == decl:
                        return False
        return True

    def pointer_target(self, node, position, chain=()):
        node = unwrapped(node)
        children = node.get("inner", [])
        if node.get("kind") == "UnaryOperator" and node.get("opcode") == "&":
            return self.object_root(children[0], position, chain) if len(children) == 1 else None
        decl = self.declaration_at(node, position)
        if (not self.initialized(decl) or decl in chain or len(chain) >= 16
                or not self.pointer_stable(decl)):
            return None
        declaration = self.declarations[decl]
        initializers = declaration.get("inner", [])
        if declaration.get("kind") != "VarDecl" or not initializers:
            return None
        return self.pointer_target(initializers[-1], position, chain + (decl,))

    def object_root(self, node, position, chain=()):
        node = unwrapped(node)
        kind = node.get("kind")
        children = node.get("inner", [])
        if kind == "DeclRefExpr":
            return self.declaration_at(node, position)
        if kind == "MemberExpr" and len(children) == 1:
            if node.get("isArrow"):
                return self.pointer_target(children[0], position, chain)
            return self.object_root(children[0], position, chain)
        if kind == "UnaryOperator" and node.get("opcode") == "*" and len(children) == 1:
            return self.pointer_target(children[0], position, chain)
        # General array bounds, external pointers and conditional aliasing are
        # deliberately unresolved. No guessed range/owner grants are allowed.
        return None

    def safe_read(self, node, position):
        value = unwrapped(node)
        if value.get("kind") in ("IntegerLiteral", "CharacterLiteral"):
            return True
        return self.initialized(self.object_root(value, position))

    def audit_extra_reads(self):
        forbidden = {
            "GotoStmt", "IndirectGotoStmt", "LabelStmt", "SwitchStmt",
            "CaseStmt", "DefaultStmt", "GCCAsmStmt", "MSAsmStmt",
        }
        for node in self.nodes:
            kind = node.get("kind")
            if kind in forbidden:
                raise ObservationError("R1197 unresolved non-structured control: " + kind)
            if kind in ("VarDecl", "ParmVarDecl"):
                if "volatile" in ctype(node) or "_Atomic" in ctype(node):
                    raise ObservationError("R1197 refuses additional volatile/atomic reads")
            if kind == "FunctionDecl":
                loc = location(node.get("loc", {}))
                offset = loc.get("offset")
                name = node.get("name", "").encode("ascii")
                if (offset is None or self.original[offset:offset + len(name)] != name):
                    raise ObservationError("R1197 function name is not in actual raw-C bytes")
            parent = self.parent[id(node)]
            if not parent or parent.get("kind") != "CompoundStmt":
                continue
            position = bounds(node)[0]
            children = node.get("inner", [])
            if kind == "ReturnStmt" and children:
                value = children[0]
                if (simple_atom(value) and scalar_kind(ctype(value)) == "number"
                        and not self.safe_read(value, position)):
                    raise ObservationError("R1197 RETURN logger read lacks initialization/root proof")
            if (kind in ("BinaryOperator", "CompoundAssignOperator")
                    and node.get("opcode") in ("=", "+=", "-=")
                    and len(children) == 2):
                lhs, rhs = children
                if not simple_atom(lhs) or scalar_kind(ctype(lhs)) != "number":
                    continue
                target = self.object_root(lhs, position)
                if target is None:
                    raise ObservationError("R1197 lvalue logger has unresolved actual object root")
                if (unwrapped(lhs).get("kind") != "DeclRefExpr"
                        or node.get("opcode") != "=") and not self.initialized(target):
                    raise ObservationError("R1197 lvalue logger root lacks initialized declaration")
                for part in self.descendants(rhs):
                    if (part.get("kind") == "CallExpr"
                            or part.get("kind") == "CompoundAssignOperator"
                            or (part.get("kind") == "BinaryOperator" and part.get("opcode") == "=")
                            or (part.get("kind") == "UnaryOperator" and part.get("opcode") in ("++", "--"))):
                        raise ObservationError("R1197 assignment RHS can invalidate pre/post logger address")
                if (node.get("opcode") == "=" and simple_atom(rhs)
                        and scalar_kind(ctype(rhs)) == "number"
                        and not self.safe_read(rhs, position)):
                    raise ObservationError("R1197 INCOMING logger read lacks initialization/root proof")
        self.guard_receipt.append({
            "scope": "only indexed actual raw-C function definitions",
            "control": "structured lexical blocks only; nonlocal jump/switch/asm rejected",
            "extra_reads": "initialized declaration roots or literals; stable local pointer origins only",
            "unresolved": "general arrays/external pointers/nonlocal control are refused, not simulated",
        })

    def prepare(self):
        self.audit_extra_reads()
        output, model = super().prepare()
        model["r1197_guard"] = self.guard_receipt
        model["r1197_semantic_role_binding"] = "NOT_SUPPLIED_BY_OBSERVER"
        return output, model


def main():
    if len(sys.argv) != 5:
        raise SystemExit("observe_r1197.py RAW_C ACTUAL_AST PASSIVE_C MAP")
    binding = json.loads(Path(__file__).with_name("target_binding.json").read_text())
    if os.environ.get("GITHUB_ACTIONS") != "true" or not binding["execution_released"]:
        raise SystemExit("HOLD: R1197 observer requires separately released GitHub Actions execution")
    raw_path, ast_path, passive_path, map_path = map(Path, sys.argv[1:])
    raw, captured = raw_path.read_bytes(), ast_path.read_bytes()
    observer = GuardedObserver(raw, json.loads(captured), raw_path)
    output, model = observer.prepare()
    model["actual_ast_sha256"] = digest(captured)
    passive_path.write_bytes(output)
    map_path.write_text(json.dumps(model, indent=2) + "\n")


if __name__ == "__main__":
    main()
