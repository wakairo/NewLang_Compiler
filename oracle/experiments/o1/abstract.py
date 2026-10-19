"""Independent finite-abstract checker. Does NOT import/use exact or old semantics.
A proof succeeds only for singleton current facts. Budget/unknown -> PRECISION.
"""
from dataclasses import dataclass
from neutral import (Change, Claim, CloseLoan, DropPayload, HoldPayload,
                     OpenLoan, Option, Outcome, Pointer, Program, Unmodeled)


@dataclass(frozen=True)
class LatticeCell:
    options: frozenset[Option]
    facts: frozenset[str]
    occurrences: frozenset[str | None]


def run(program: Program, event_budget: int = 12, alternative_budget: int = 2) -> Outcome:
    if event_budget < 0 or alternative_budget < 1:
        return Outcome("PRECISION", None, "ABSTRACT_RESOURCE_BUDGET")
    origin = {}
    states = {}
    active_write = {}
    issued_loan_names = set()
    borrowed_payloads = {}
    visited = set()
    for r in program.originals:
        if (r.root in origin or r.field_count != 3 or
                any((r.region == v.region or r.allocation == v.allocation or
                     r.domain == v.domain or r.incarnation == v.incarnation)
                    for v in origin.values())):
            return Outcome("SEMANTIC_REJECT", None, "ORIGINAL_IDENTITY")
        origin[r.root] = r
        for f in range(r.field_count):
            states[r.root, f] = LatticeCell(frozenset((Option(),)),
                                             frozenset((f"initial:{r.root}:{f}",)),
                                             frozenset((None,)))
    if len(program.operations) != len(program.evidence):
        return Outcome("SEMANTIC_REJECT", None, "TAPE_BIJECTION")

    changes = 0
    for ix, (action, assertion) in enumerate(zip(program.operations, program.evidence)):
        label = action.node
        if label in visited or assertion.node != label or assertion.ordinal != ix or assertion.kind != type(action).__name__:
            return Outcome("SEMANTIC_REJECT", label, "TAPE_CHRONOLOGY")
        visited.add(label)
        if isinstance(action, Unmodeled):
            return Outcome("UNSUPPORTED", label, "OUTSIDE_O1_SLICE")
        if isinstance(action, OpenLoan):
            known = origin.get(action.root)
            if known is None or action.field < 0 or action.field >= known.field_count:
                return Outcome("SEMANTIC_REJECT", label, "ROOT_OR_PROJECTION")
            if action.domain != known.domain:
                return Outcome("SEMANTIC_REJECT", label, "WRONG_ORIGINAL_DOMAIN")
            if action.mode != "write":
                return Outcome("SEMANTIC_REJECT", label, "NO_WRITE_AMPLIFICATION")
            if action.loan in issued_loan_names:
                return Outcome("SEMANTIC_REJECT", label, "LOAN_REISSUED")
            issued_loan_names.add(action.loan)
            active_write[action.loan] = (action.root, action.field)
        elif isinstance(action, CloseLoan):
            if action.loan not in active_write:
                return Outcome("SEMANTIC_REJECT", label, "LOAN_NOT_LIVE")
            active_write.pop(action.loan)
        elif isinstance(action, HoldPayload):
            state = states.get((action.root, action.field))
            if state is None or len(state.occurrences) != 1:
                return Outcome("PRECISION", label, "OCCURRENCE_NOT_DEFINITE")
            occurrence = next(iter(state.occurrences))
            if occurrence is None or occurrence != action.occurrence or action.hold in borrowed_payloads:
                return Outcome("SEMANTIC_REJECT", label, "STALE_PAYLOAD_HOLD")
            borrowed_payloads[action.hold] = (action.root, action.field)
        elif isinstance(action, DropPayload):
            if action.hold not in borrowed_payloads:
                return Outcome("SEMANTIC_REJECT", label, "UNKNOWN_PAYLOAD_HOLD")
            borrowed_payloads.pop(action.hold)
        elif isinstance(action, Change):
            changes += 1
            if changes > event_budget:
                return Outcome("PRECISION", label, "FIELD_EVENT_BUDGET")
            place = active_write.get(action.loan)
            if place is None or place[1] != action.field:
                return Outcome("SEMANTIC_REJECT", label, "NO_LIVE_FIELD_WRITE_REF")
            if action.alias != "known":
                return Outcome("PRECISION", label, "ALIAS_ANALYSIS_UNKNOWN")
            if not isinstance(action.value, Option):
                return Outcome("SEMANTIC_REJECT", label, "PTR_CANNOT_GRANT_FIELD_VALUE")
            if action.value.payload is not None:
                p = action.value.payload
                source = origin.get(p.root) if isinstance(p, Pointer) else None
                if source is None or p.region != source.region or p.incarnation != source.incarnation:
                    return Outcome("SEMANTIC_REJECT", label, "STALE_PTR_PROVENANCE")
            if place in borrowed_payloads.values():
                return Outcome("SEMANTIC_REJECT", label, "SURVIVING_OCCURRENCE_DEP")
            state = states[place]
            if max(len(state.options), len(state.facts), len(state.occurrences)) > alternative_budget:
                return Outcome("PRECISION", label, "ALTERNATIVE_LIMIT")
            if not (len(state.options) == len(state.facts) == len(state.occurrences) == 1):
                return Outcome("PRECISION", label, "BEFORE_NOT_SINGLETON")
            actual = Claim(next(iter(state.options)), next(iter(state.facts)), next(iter(state.occurrences)))
            root, field = place
            others = tuple((k, states[root, k]) for k in range(origin[root].field_count) if k != field)
            if any(len(s.options) != 1 or len(s.facts) != 1 or len(s.occurrences) != 1 for _, s in others):
                return Outcome("PRECISION", label, "SIBLING_NOT_DEFINITE")
            independent_siblings = tuple((k, Claim(next(iter(s.options)), next(iter(s.facts)),
                                                   next(iter(s.occurrences)))) for k, s in others)
            if assertion.before != actual or assertion.siblings_before != independent_siblings:
                return Outcome("SEMANTIC_REJECT", label, "STALE_OLD_OR_SIBLING_BEFORE")
            # Abstract transfer, independently implemented: singleton fresh
            # occurrence replaces prior possibility, siblings not touched.
            incoming = LatticeCell(frozenset((action.value,)), frozenset((f"fact:{label}",)),
                                   frozenset((f"occ:{label}" if action.value.payload else None,)))
            states[place] = incoming
            post = Claim(next(iter(incoming.options)), next(iter(incoming.facts)),
                         next(iter(incoming.occurrences)))
            if assertion.after != post or assertion.siblings_after != independent_siblings:
                return Outcome("SEMANTIC_REJECT", label, "STALE_NEW_OR_SIBLING_AFTER")
        else:
            return Outcome("UNSUPPORTED", label, "UNKNOWN_IR_OPERATION")
    if active_write or borrowed_payloads:
        return Outcome("SEMANTIC_REJECT", None, "LEXICAL_SCOPE_LEAK")
    return Outcome("ACCEPT")
