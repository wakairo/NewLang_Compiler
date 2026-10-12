"""Deterministic synthetic declaration and witness builder, NOT either checker.

Fixture builder deliberately assumes Original; it cannot issue real source rights.
"""
from dataclasses import replace
import random
from model import Original, Value, Cell, Step, Witness, Program, field_key, root_key

ROOTS=("directory", "slot2", "slot4", "schedulerX", "schedulerY")

def fixtures(motif: str, changes: int, *, seed: int = 20261012,
             scalar_gap: str = "none", same_target: bool = False) -> Program:
    if motif not in ("directory", "scheduler") or changes < 2 or scalar_gap not in ("none", "hidden", "recorded", "copy", "unknown"):
        raise ValueError((motif, changes, scalar_gap))
    rng = random.Random(seed)
    originals = tuple(Original(n, f"R.{i}", f"O.{i}", f"A.{i}",
                               f"D.{i}", f"owner.{i}") for i,n in enumerate(ROOTS))
    state: dict[str, object] = {}
    for i,o in enumerate(originals):
        state[root_key(o.name)] = f"rootfact.initial.{i}"
        for f in range(4):
            v=Value("u8", scalar=i+1) if f==3 else Value("Option")
            state[field_key(o.name,f)] = Cell(f"place.{o.name}.{f}", f"inc.{o.name}.{f}", v,
                                                f"fact.initial.{o.name}.{f}")
    steps: list[Step] = []
    checked: list[Witness] = []
    # DECLARATIVE fixture emitter. It emits claimed worlds; validators implement
    # their independent transition and must neither import nor call this emitter.
    def append(step: Step, record: bool = True):
        before=tuple(sorted(state.items()))
        old=state.get(field_key(step.root, step.field))
        if step.kind == "change":
            assert isinstance(old, Cell) and step.value is not None
            # Old Some occurrence resets even when pointer target is unchanged.
            state[field_key(step.root, step.field)] = replace(
                old, value=step.value, fact=f"fact.new.{step.sid}",
                occurrence=(f"occ.new.{step.sid}" if step.value.pointer_root else None))
            state[root_key(step.root)]=f"rootfact.new.{step.sid}"
        after=tuple(sorted(state.items()))
        steps.append(step)
        if record:
            assert isinstance(old, Cell) and step.kind=="change"
            now=state[field_key(step.root,step.field)]
            sibling=field_key("slot4" if step.root!="slot4" else "directory",3)
            # Probe an unrelated fourth scalar, not another link in same root.
            witness=Witness(len(checked),step.sid,len(steps)-1,before,after,
                            old.fact,now.fact,old.occurrence,now.occurrence,
                            sibling,state[sibling].fact)
            checked.append(witness)
    for i in range(changes):
        if motif == "directory":
            root,field="directory",2
            target=("slot2",None,"slot4",None)[i%4]
        else:
            opts=(("schedulerX",0,"schedulerY"),
                  ("schedulerY",1,"schedulerX"),
                  ("schedulerX",0,"schedulerY"),
                  ("schedulerY",1,None),
                  ("schedulerX",0,"schedulerY"),
                  ("schedulerY",1,"schedulerY"))
            root,field,target=opts[i%len(opts)]
        if same_target and i>=2 and motif=="scheduler":
            # Guarantee a real Some(same ptr) -> Some(same ptr) Change, fresh occ.
            old=state[field_key(root,field)].value.pointer_root
            if old: target=old
        original=next(o for o in originals if o.name==root)
        sid=f"change:{i:02}:{rng.getrandbits(32):08x}"
        target_original=next((o for o in originals if o.name==target),None)
        ptr_value=Value("Option",pointer_root=target,
                        pointer_backing=target_original.backing if target_original else None,
                        pointer_incarnation=target_original.incarnation if target_original else None)
        append(Step(sid,"change",root,field,ptr_value,
                    read_domain=original.domain,ref_root=root,
                    ref_incarnation=original.incarnation,projection=field,
                    scope=f"scope.{i}",derived_scope=f"scope.{i}"))
        if i==0 and scalar_gap!="none":
            # Same literal u8. A whole scalar Change/Reset still renews ValueFact.
            scalar_root="slot4"
            old=state[field_key(scalar_root,3)].value
            sid=f"scalar:gap:{rng.getrandbits(32):08x}"
            sc=next(o for o in originals if o.name==scalar_root)
            if scalar_gap in ("hidden","recorded"):
                append(Step(sid,"change",scalar_root,3,old,read_domain=sc.domain,
                            ref_root=scalar_root,ref_incarnation=sc.incarnation,
                            projection=3,scope="scope.scalar",derived_scope="scope.scalar"),
                       record=scalar_gap=="recorded")
            elif scalar_gap=="copy":
                append(Step(sid,"copy",scalar_root,3),record=False)
            elif scalar_gap=="unknown":
                append(Step(sid,"unknown",scalar_root,3,effect="unknown"),record=False)
    return Program(motif,seed,originals,tuple(sorted((k,v) for k,v in
                    ((key,val) for key,val in (initial_state(originals)).items()))),
                   tuple(steps),tuple(checked))


def initial_state(originals: tuple[Original,...]) -> dict[str,object]:
    state={}
    for i,o in enumerate(originals):
        state[root_key(o.name)]=f"rootfact.initial.{i}"
        for f in range(4):
            v=Value("u8",scalar=i+1) if f==3 else Value("Option")
            state[field_key(o.name,f)]=Cell(f"place.{o.name}.{f}",f"inc.{o.name}.{f}",v,
                                             f"fact.initial.{o.name}.{f}")
    return state


def poison(program: Program, kind: str) -> Program:
    """All mutations are independent destructive copies of the fixture tape."""
    steps=list(program.steps); checked=list(program.checked)
    if kind=="missing": checked.pop(1)
    elif kind=="duplicate": checked.insert(1,checked[0])
    elif kind=="reordered": checked[0],checked[1]=checked[1],checked[0]
    elif kind=="wrong_domain": steps[0]=replace(steps[0],read_domain="D.other")
    elif kind=="wrong_ref": steps[0]=replace(steps[0],write_ref=False)
    elif kind=="wrong_projection": steps[0]=replace(steps[0],projection=1 if steps[0].field!=1 else 0)
    elif kind=="scope_leak": steps[0]=replace(steps[0],scope_closed=False)
    elif kind=="wrong_original": steps[0]=replace(steps[0],ref_incarnation="O.retired")
    elif kind=="stale_occurrence":
        k=next((i for i,w in enumerate(checked) if w.old_occurrence is not None),0)
        checked[k]=replace(checked[k],old_occurrence="occ.retired")
    elif kind=="new_occurrence_reuse":
        k=next((i for i,w in enumerate(checked) if w.old_occurrence is not None and w.new_occurrence is not None),
               next(i for i,w in enumerate(checked) if w.new_occurrence is not None))
        checked[k]=replace(checked[k],new_occurrence=checked[k].old_occurrence or "occ.retired")
    elif kind=="sibling_fact": checked[1]=replace(checked[1],sibling_fact="fact.forged")
    elif kind=="owner_custody":
        os=list(program.originals);os[0]=replace(os[0],allocation=os[1].allocation)
        return replace(program,originals=tuple(os))
    elif kind=="stale_pointer":
        k=next(i for i,s in enumerate(steps) if s.value and s.value.pointer_root)
        steps[k]=replace(steps[k],value=replace(steps[k].value,pointer_incarnation="O.retired"))
    elif kind=="ptr_authority":
        k=next(i for i,s in enumerate(steps) if s.value and s.value.pointer_root)
        steps[k]=replace(steps[k],value=replace(steps[k].value,carries_domain=True))
    elif kind=="wrong_branch": checked[1]=replace(checked[1],source_index=0)
    elif kind=="wrong_source": checked[1]=replace(checked[1],sid="forged.source")
    else: raise ValueError(kind)
    return replace(program,steps=tuple(steps),checked=tuple(checked))

POISON_KINDS=("missing","duplicate","reordered","wrong_domain","wrong_ref",
              "wrong_projection","scope_leak","wrong_original","stale_occurrence",
              "new_occurrence_reuse","sibling_fact","owner_custody","wrong_branch",
              "wrong_source","stale_pointer","ptr_authority")
