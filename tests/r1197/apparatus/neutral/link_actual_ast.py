"""Exact same-AST linkage for captured R maps; pure JSON/text, no C execution.

No oracle or case ID is accepted. Match each indexed function subtree back to
the already captured AST by kind/range/type/children, then retain real function
declaration and previousDecl IDs. This is identity linkage, not a C interpreter.
"""
import hashlib
import json
from pathlib import Path
import sys


def bounds(node):
    region = node.get("range", {})
    first = region.get("begin", {})
    last = region.get("end", {})
    first = first.get("expansionLoc", first)
    last = last.get("expansionLoc", last)
    if "offset" not in first or "offset" not in last:
        return None
    return [first["offset"], last["offset"] + last.get("tokLen", 0)]


def ctype(node):
    value = node.get("type", {})
    return value.get("desugaredQualType", value.get("qualType", ""))


def one(values, reason):
    values = list(values)
    if len(values) != 1:
        raise ValueError(reason + ": nonunique same-AST identity")
    return values[0]


def link(model_bytes, ast_bytes):
    model = json.loads(model_bytes)
    ast = json.loads(ast_bytes)
    digest = hashlib.sha256(ast_bytes).hexdigest()
    if digest != model.get("actual_ast_sha256"):
        raise ValueError("captured AST hash does not match the original map")
    nodes = {x["id"]: x for x in model["nodes"]}
    if len(nodes) != len(model["nodes"]):
        raise ValueError("duplicate mapped node identity")
    function_declarations = [x for x in ast.get("inner", []) if x.get("kind") == "FunctionDecl"]
    declarations = {x["id"]: x for x in function_declarations}
    if len(declarations) != len(function_declarations):
        raise ValueError("duplicate captured function declaration identity")
    raw_by_model = {}
    families = {}
    proofs = []

    def pair(mapped, raw):
        if (mapped.get("kind") != raw.get("kind")
                or mapped.get("range") != bounds(raw)
                or mapped.get("type") != ctype(raw)
                or len(mapped["children"]) != len(raw.get("inner", []))):
            raise ValueError("mapped subtree differs from its captured AST")
        for key in ("name", "opcode", "value", "hasElse", "init", "isArrow"):
            if mapped.get(key) != raw.get(key):
                raise ValueError("mapped AST property differs: " + key)
        raw_by_model[mapped["id"]] = raw
        mapped["actual_ast_id"] = raw.get("id")
        for child, actual in zip(mapped["children"], raw.get("inner", [])):
            pair(nodes[child], actual)

    for function in model["functions"]:
        mapped = one((x for x in nodes.values() if x["kind"] == "FunctionDecl"
                      and x["function"] == function["id"]), "mapped definition")
        raw = one((x for x in declarations.values()
                   if x.get("name") == function["name"]
                   and bounds(x) == mapped["range"] and ctype(x) == mapped["type"]
                   and any(c.get("kind") == "CompoundStmt" for c in x.get("inner", []))),
                  "captured definition")
        pair(mapped, raw)
        chain = []
        current = raw
        while current is not None:
            ident = current["id"]
            if ident in chain or (ident in families and families[ident] != function["id"]):
                raise ValueError("cyclic/ambiguous function declaration family")
            if current.get("name") != raw.get("name") or ctype(current) != ctype(raw):
                raise ValueError("previousDecl has a different name/type")
            chain.append(ident)
            families[ident] = function["id"]
            previous = current.get("previousDecl")
            if previous is None:
                break
            if previous not in declarations:
                raise ValueError("previousDecl is not in the captured translation unit")
            current = declarations[previous]
        function["actual_ast_id"] = raw["id"]
        function["definition_node"] = mapped["id"]
        function["declaration_chain"] = chain
        proofs.append({"function": function["id"], "definition": raw["id"],
                       "previousDecl_chain": chain})
    if len(raw_by_model) != len(nodes):
        raise ValueError("not every mapped node was linked to the same captured AST")
    for info in model["variables"].values():
        if raw_by_model[info["node"]].get("id") != info["ast_id"]:
            raise ValueError("variable declaration identity differs")
    for mapped in nodes.values():
        raw = raw_by_model[mapped["id"]]
        reference = raw.get("referencedDecl", {})
        if reference.get("kind") == "FunctionDecl":
            mapped["ref_function_ast_id"] = reference["id"]
            mapped["ref_function"] = families.get(reference["id"])
    model["actual_ast_linkage"] = {
        "ast_sha256": digest,
        "original_map_sha256": hashlib.sha256(model_bytes).hexdigest(),
        "definitions": proofs,
        "method": "exact indexed subtree plus actual FunctionDecl/previousDecl/DeclRef identity",
        "runtime_output_unchanged": True,
    }
    return model


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit("link_actual_ast.py ORIGINAL_MAP SAME_CAPTURED_AST LINKED_MAP")
    original, captured, output = map(Path, sys.argv[1:])
    result = link(original.read_bytes(), captured.read_bytes())
    output.write_text(json.dumps(result, indent=2) + "\n")
