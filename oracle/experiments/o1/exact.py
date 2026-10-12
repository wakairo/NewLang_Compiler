"""Exact bounded concrete model: no import of abstract/finite checker."""
from neutral import (Change, Claim, CloseLoan, DropPayload, HoldPayload,
                     OpenLoan, Option, Outcome, Pointer, Program, Unmodeled)


def run(program: Program) -> Outcome:
    roots = {}
    cells = {}
    loans = {}
    holds = {}
    seen_nodes = set()
    seen_loans = set()
    for original in program.originals:
        if (original.root in roots or original.field_count != 3 or
                any(x.region == original.region or x.allocation == original.allocation
                    or x.domain == original.domain or x.incarnation == original.incarnation
                    for x in roots.values())):
            return Outcome("SEMANTIC_REJECT", None, "ORIGINAL_IDENTITY")
        roots[original.root] = original
        for k in range(original.field_count):
            cells[original.root, k] = Claim(Option(), f"initial:{original.root}:{k}", None)

    if len(program.operations) != len(program.evidence):
        return Outcome("SEMANTIC_REJECT", None, "TAPE_BIJECTION")
    for position, (op, proof) in enumerate(zip(program.operations, program.evidence)):
        node = op.node
        if node in seen_nodes or proof.node != node or proof.ordinal != position or proof.kind != type(op).__name__:
            return Outcome("SEMANTIC_REJECT", node, "TAPE_CHRONOLOGY")
        seen_nodes.add(node)
        if isinstance(op, Unmodeled):
            return Outcome("UNSUPPORTED", node, "OUTSIDE_O1_SLICE")
        if isinstance(op, OpenLoan):
            root = roots.get(op.root)
            if root is None or op.field not in range(root.field_count):
                return Outcome("SEMANTIC_REJECT", node, "ROOT_OR_PROJECTION")
            if op.domain != root.domain:
                return Outcome("SEMANTIC_REJECT", node, "WRONG_ORIGINAL_DOMAIN")
            if op.mode != "write":
                return Outcome("SEMANTIC_REJECT", node, "NO_WRITE_AMPLIFICATION")
            if op.loan in seen_loans:
                return Outcome("SEMANTIC_REJECT", node, "LOAN_REISSUED")
            seen_loans.add(op.loan)
            loans[op.loan] = (op.root, op.field)
        elif isinstance(op, CloseLoan):
            if op.loan not in loans:
                return Outcome("SEMANTIC_REJECT", node, "LOAN_NOT_LIVE")
            del loans[op.loan]
        elif isinstance(op, HoldPayload):
            current = cells.get((op.root, op.field))
            if current is None or current.occurrence is None or current.occurrence != op.occurrence or op.hold in holds:
                return Outcome("SEMANTIC_REJECT", node, "STALE_PAYLOAD_HOLD")
            holds[op.hold] = (op.root, op.field, current.occurrence)
        elif isinstance(op, DropPayload):
            if op.hold not in holds:
                return Outcome("SEMANTIC_REJECT", node, "UNKNOWN_PAYLOAD_HOLD")
            del holds[op.hold]
        elif isinstance(op, Change):
            target = loans.get(op.loan)
            if target is None or target[1] != op.field:
                return Outcome("SEMANTIC_REJECT", node, "NO_LIVE_FIELD_WRITE_REF")
            if op.alias != "known":
                return Outcome("UNSUPPORTED", node, "UNKNOWN_ALIAS_WORLD")
            if not isinstance(op.value, Option):
                return Outcome("SEMANTIC_REJECT", node, "PTR_CANNOT_GRANT_FIELD_VALUE")
            if op.value.payload is not None:
                p = op.value.payload
                real = roots.get(p.root) if isinstance(p, Pointer) else None
                if real is None or (p.region, p.incarnation) != (real.region, real.incarnation):
                    return Outcome("SEMANTIC_REJECT", node, "STALE_PTR_PROVENANCE")
            root_name, field = target
            if any(r == root_name and f == field for r, f, _ in holds.values()):
                return Outcome("SEMANTIC_REJECT", node, "SURVIVING_OCCURRENCE_DEP")
            prior = cells[root_name, field]
            siblings = tuple((i, cells[root_name, i]) for i in range(roots[root_name].field_count) if i != field)
            if proof.before != prior or proof.siblings_before != siblings:
                return Outcome("SEMANTIC_REJECT", node, "STALE_OLD_OR_SIBLING_BEFORE")
            # A whole-Option Change/Reset retains root and fixed-field incarnation;
            # its conditional Some occurrence is always renewed, even same ptr.
            updated = Claim(op.value, f"fact:{node}", f"occ:{node}" if op.value.payload else None)
            cells[root_name, field] = updated
            after_siblings = tuple((i, cells[root_name, i]) for i in range(roots[root_name].field_count) if i != field)
            if proof.after != updated or proof.siblings_after != after_siblings:
                return Outcome("SEMANTIC_REJECT", node, "STALE_NEW_OR_SIBLING_AFTER")
        else:
            return Outcome("UNSUPPORTED", node, "UNKNOWN_IR_OPERATION")
    if loans or holds:
        return Outcome("SEMANTIC_REJECT", None, "LEXICAL_SCOPE_LEAK")
    return Outcome("ACCEPT")
