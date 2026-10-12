"""O5 separately authored persistent-delta / forward-frontier algorithm.

No full-world clone. Each typed Change creates one structurally shared Node
containing at most {root fact, changed fixed-cell}. Flat current frontier is a
small mutable cache for low-cost CURRENT checks; version chain retains history.
No import of full_snapshot or its transition/acceptance helpers.
"""
from __future__ import annotations
from dataclasses import dataclass
from model import Program, Cell, field_key, root_key

@dataclass(frozen=True)
class VersionNode:
    parent: 'VersionNode | None'
    updates: tuple[tuple[str, object], ...]
    source_index: int

@dataclass(frozen=True)
class Result:
    status: str
    rule: str = "OK"
    first: str | None = None
    cloned_slots: int = 0
    shared_nodes: int = 0
    visited_slots: int = 0
    peak_stored_slots: int = 0


def check(program: Program, event_budget: int = 1000) -> Result:
    truth=dict(program.initial)
    identities={x.name:x for x in program.originals}
    head=VersionNode(None,(),-1)
    linked=0
    delta_slots=0
    visits=0
    seen=set()
    cursor=0
    # Keep all historical frames alive: a small roll-forward allocator model.
    nodes=[head]
    def result(status,why,sid=None):
        return Result(status,why,sid,delta_slots,linked,visits,
                      len(truth)+sum(len(x.updates) for x in nodes))
    if len(identities)!=5 or len(identities)!=len(program.originals) or len(truth)!=25:
        return result("SEMANTIC_REJECT","BOOTSTRAP_SHAPE")
    for attr in ("backing","allocation","incarnation","domain","owner_binding"):
        if len({getattr(x,attr) for x in identities.values()})!=5:
            return result("SEMANTIC_REJECT","OWNER_IDENTITY")
    for source in identities.values():
        if not (source.domain_live and source.allocation_live) or root_key(source.name) not in truth:
            return result("SEMANTIC_REJECT","ORIGINAL_CUSTODY")
        for k in range(4):
            c=truth.get(field_key(source.name,k))
            if not isinstance(c,Cell) or c.value.kind != ("u8" if k==3 else "Option"):
                return result("SEMANTIC_REJECT","FOUR_FIELDS")
    for step_index,step in enumerate(program.steps):
        if step.sid in seen: return result("SEMANTIC_REJECT","SOURCE_NODE_COLLISION",step.sid)
        seen.add(step.sid)
        if step.kind=="copy":
            if step.effect!="known":return result("PRECISION","UNKNOWN_COPY",step.sid)
            if cursor<len(program.checked) and program.checked[cursor].source_index==step_index:
                return result("SEMANTIC_REJECT","COPY_EVENT_FORGED",step.sid)
            continue
        if step.kind=="unknown" or step.effect=="unknown":
            return result("PRECISION","MAY_CHANGE_UNMODELED",step.sid)
        if step.kind!="change": return result("UNSUPPORTED","OWNER_OPERATION_NOT_MODELED",step.sid)
        if cursor>=event_budget: return result("PRECISION","RESOURCE_LIMIT",step.sid)
        if cursor>=len(program.checked):return result("SEMANTIC_REJECT","MISSING_LEDGER_ENTRY",step.sid)
        ledger=program.checked[cursor]
        # Source/event coverage via monotone event-index, no borrowed reference
        # to another algorithm's acceptance or to producer witness authority.
        if ledger.source_index!=step_index or ledger.ordinal!=cursor or ledger.sid!=step.sid:
            return result("SEMANTIC_REJECT","SOURCE_EVENT_BIJECTION",step.sid)
        origin=identities.get(step.root)
        if origin is None or not 0<=step.field<4:return result("SEMANTIC_REJECT","BAD_FIELD_PATH",step.sid)
        if not step.read_access or step.read_domain!=origin.domain:
            return result("SEMANTIC_REJECT","READ_STABILITY_NOT_ORIGINAL_D",step.sid)
        if not step.write_ref or step.ref_root!=origin.name or step.ref_incarnation!=origin.incarnation or step.projection!=step.field:
            return result("SEMANTIC_REJECT","REF_PROJECTION_CHAIN",step.sid)
        if not step.scope or step.scope!=step.derived_scope or not step.scope_closed:
            return result("SEMANTIC_REJECT","SCOPE_NOT_CLOSED",step.sid)
        cell_name=field_key(step.root,step.field)
        old=truth.get(cell_name)
        if not isinstance(old,Cell):return result("SEMANTIC_REJECT","NO_CURRENT_FIXED_FIELD",step.sid)
        expected_kind="u8" if step.field==3 else "Option"
        if step.value is None or step.value.kind!=expected_kind:
            return result("SEMANTIC_REJECT","VALUE_KIND",step.sid)
        if expected_kind=="u8" and (step.value.scalar is None or not 0<=step.value.scalar<256):
            return result("SEMANTIC_REJECT","SCALAR_RANGE",step.sid)
        if step.value.carries_domain or step.value.carries_allocation:
            return result("SEMANTIC_REJECT","COPY_PTR_NO_ALLOCATION_OR_DOMAIN",step.sid)
        if step.value.pointer_root is not None:
            referent=identities.get(step.value.pointer_root)
            if referent is None or step.value.pointer_backing!=referent.backing or step.value.pointer_incarnation!=referent.incarnation:
                return result("SEMANTIC_REJECT","BAD_COPY_TARGET",step.sid)
        # Full claimed snapshot check using shared forward frontier. No clone;
        # proves all siblings from CURRENT state, not just same numerical values.
        if len(ledger.before)!=len(truth):return result("SEMANTIC_REJECT","BEFORE_SIZE",step.sid)
        for key,witness in ledger.before:
            visits+=1
            if key not in truth or truth[key]!=witness:
                return result("SEMANTIC_REJECT","BEFORE_FRONTIER",step.sid)
        if ledger.old_fact!=old.fact or ledger.old_occurrence!=old.occurrence:
            return result("SEMANTIC_REJECT","STALE_OLD",step.sid)
        sibling=truth.get(ledger.sibling_key)
        if ledger.sibling_key==cell_name or not isinstance(sibling,Cell) or sibling.fact!=ledger.sibling_fact:
            return result("SEMANTIC_REJECT","SIBLING_FACT",step.sid)
        # Independent immutable transfer. Changed field + enclosing *value fact*
        # are the only modified entries; 23 others remain structural shares.
        fresh=Cell(old.place,old.incarnation,step.value,
                   "fact.new."+step.sid,
                   "occ.new."+step.sid if step.value.pointer_root is not None else None)
        if fresh.fact!=ledger.new_fact or fresh.occurrence!=ledger.new_occurrence:
            return result("SEMANTIC_REJECT","STALE_NEW",step.sid)
        updates=((cell_name,fresh),(root_key(step.root),"rootfact.new."+step.sid))
        # Check claimed successor WITHOUT committing until both checks pass.
        view=dict(updates)
        if len(ledger.after)!=len(truth):return result("SEMANTIC_REJECT","AFTER_SIZE",step.sid)
        for key,expected in ledger.after:
            visits+=1
            got=view[key] if key in view else truth.get(key)
            if got!=expected:
                return result("SEMANTIC_REJECT","AFTER_FRONTIER",step.sid)
        head=VersionNode(head,updates,step_index)
        nodes.append(head)
        linked+=1
        delta_slots+=len(updates)
        for key,value in updates:truth[key]=value
        cursor+=1
    if cursor!=len(program.checked):return result("SEMANTIC_REJECT","ORPHAN_EVENT")
    return result("ACCEPT","OK")
