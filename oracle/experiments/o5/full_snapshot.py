"""O5 independent exact *full world snapshot* verifier.

Deliberately simple: copy the complete semantic dictionary on every Change,
then compare complete before and after worlds. No import of incremental model.
"""
from dataclasses import dataclass, replace
from model import Cell, Original, Program, Value, field_key, root_key


@dataclass(frozen=True)
class Verdict:
    status: str
    rule: str = "OK"
    first: str | None = None
    cloned_slots: int = 0
    shared_nodes: int = 0
    visited_slots: int = 0
    peak_stored_slots: int = 0


def check(program: Program, event_budget: int = 1000) -> Verdict:
    clones=0
    visits=0
    history=[]
    state=dict(program.initial)
    originals={o.name:o for o in program.originals}
    def out(status,rule,sid=None):
        return Verdict(status,rule,sid,clones,0,visits,
                       sum(len(h) for h in history)+len(state))
    if len(originals)!=len(program.originals) or len(originals)!=5 or len(state)!=25:
        return out("SEMANTIC_REJECT","INITIAL_SHAPE")
    if len({o.backing for o in originals.values()})!=5 or len({o.incarnation for o in originals.values()})!=5 or len({o.allocation for o in originals.values()})!=5 or len({o.domain for o in originals.values()})!=5 or len({o.owner_binding for o in originals.values()})!=5:
        return out("SEMANTIC_REJECT","OWNER_IDENTITY")
    for o in originals.values():
        if not o.allocation_live or not o.domain_live or root_key(o.name) not in state:
            return out("SEMANTIC_REJECT","ORIGINAL_CUSTODY")
        for f in range(4):
            cell=state.get(field_key(o.name,f))
            if not isinstance(cell,Cell) or cell.value.kind != ("u8" if f==3 else "Option"):
                return out("SEMANTIC_REJECT","FIELD_SHAPE")
    expected=0
    seen=set()
    for index,step in enumerate(program.steps):
        if step.sid in seen:
            return out("SEMANTIC_REJECT","DUPLICATE_SOURCE_NODE",step.sid)
        seen.add(step.sid)
        if step.kind=="copy":
            if step.effect!="known": return out("PRECISION","UNKNOWN_COPY_EFFECT",step.sid)
            if expected<len(program.checked) and program.checked[expected].source_index==index:
                return out("SEMANTIC_REJECT","COPY_NOT_CHANGE",step.sid)
            continue
        if step.kind=="unknown" or step.effect=="unknown":
            return out("PRECISION","UNKNOWN_GAP_EFFECT",step.sid)
        if step.kind!="change":
            return out("UNSUPPORTED","UNMODELED_ROOT_OPERATION",step.sid)
        if expected >= event_budget:
            return out("PRECISION","EVENT_BUDGET",step.sid)
        if expected>=len(program.checked):
            return out("SEMANTIC_REJECT","MISSING_SOURCE_CHANGE",step.sid)
        event=program.checked[expected]
        if event.ordinal!=expected or event.source_index!=index or event.sid!=step.sid:
            return out("SEMANTIC_REJECT","UNRECORDED_CHANGE_OR_ORDER",step.sid)
        original=originals.get(step.root)
        if original is None or step.field not in range(4):
            return out("SEMANTIC_REJECT","UNKNOWN_ROOT_OR_FIELD",step.sid)
        if step.read_domain!=original.domain or not step.read_access:
            return out("SEMANTIC_REJECT","WRONG_ORIGINAL_READ_DOMAIN",step.sid)
        if not step.write_ref or step.ref_root!=step.root or step.ref_incarnation!=original.incarnation or step.projection!=step.field:
            return out("SEMANTIC_REJECT","DERIVED_WRITE_REF",step.sid)
        if not step.scope or step.scope!=step.derived_scope or not step.scope_closed:
            return out("SEMANTIC_REJECT","LEXICAL_SCOPE",step.sid)
        if step.value is None or step.value.kind != ("u8" if step.field==3 else "Option"):
            return out("SEMANTIC_REJECT","TYPE_MISMATCH",step.sid)
        if step.field==3 and (step.value.scalar is None or not 0<=step.value.scalar<=255):
            return out("SEMANTIC_REJECT","U8_VALUE",step.sid)
        if step.value.carries_allocation or step.value.carries_domain:
            return out("SEMANTIC_REJECT","COPY_PTR_CANNOT_MINT_OWNER",step.sid)
        if step.value.pointer_root is not None:
            target=originals.get(step.value.pointer_root)
            if target is None or (step.value.pointer_backing,step.value.pointer_incarnation)!=(target.backing,target.incarnation):
                return out("SEMANTIC_REJECT","STALE_TARGET_POINTER",step.sid)
        visits+=len(state)
        if tuple(sorted(state.items())) != event.before:
            return out("SEMANTIC_REJECT","BEFORE_WORLD_CURRENTNESS",step.sid)
        old=state[field_key(step.root,step.field)]
        if not isinstance(old,Cell) or old.fact!=event.old_fact or old.occurrence!=event.old_occurrence:
            return out("SEMANTIC_REJECT","OLD_FACT_OR_OCCURRENCE",step.sid)
        if event.sibling_key==field_key(step.root,step.field) or not isinstance(state.get(event.sibling_key),Cell) or state[event.sibling_key].fact != event.sibling_fact:
            return out("SEMANTIC_REJECT","SIBLING_CURRENTNESS",step.sid)
        # A full cloned semantic checkpoint (25 root/field entries), never
        # a bytewise private representation comparison.
        before=state
        after=before.copy()
        clones+=len(before)
        new_cell=replace(old,value=step.value,fact=f"fact.new.{step.sid}",
                         occurrence=f"occ.new.{step.sid}" if step.value.pointer_root else None)
        after[field_key(step.root,step.field)] = new_cell
        after[root_key(step.root)] = f"rootfact.new.{step.sid}"
        if old.place!=new_cell.place or old.incarnation!=new_cell.incarnation:
            return out("SEMANTIC_REJECT","FIXED_SUBOBJECT_ID",step.sid)
        if new_cell.fact!=event.new_fact or new_cell.occurrence!=event.new_occurrence:
            return out("SEMANTIC_REJECT","NEW_FACT_OR_OCCURRENCE",step.sid)
        visits+=len(after)
        if tuple(sorted(after.items()))!=event.after:
            return out("SEMANTIC_REJECT","AFTER_WORLD_CURRENTNESS",step.sid)
        history.append(after)
        state=after
        expected+=1
    if expected!=len(program.checked):
        return out("SEMANTIC_REJECT","ORPHAN_CHECKED_EVENT")
    return out("ACCEPT","OK")
