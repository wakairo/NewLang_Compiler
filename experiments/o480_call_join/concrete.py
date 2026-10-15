"""Concrete finite two-world interpreter, authored separately from A/B."""
from __future__ import annotations
from dataclasses import replace
from ir import FIELDS, Program, Action, Result, World, Cell, Option, Dep, world, lookup

def run(p:Program)->Result:
    if len(p.originals)!=2 or {o.name for o in p.originals}!={'head','tail'}:
        return Result('UNSUPPORTED','two distinct lexical Node roots required')
    if len({(o.region,o.root,o.incarnation,o.allocation,o.domain,o.owner) for o in p.originals})!=2 or any(not o.live for o in p.originals):
        return Result('SEMANTIC_REJECT','original root ownership/identity')
    for attr in ('region','root','incarnation','allocation','domain','owner'):
        if len({getattr(o,attr) for o in p.originals})!=2:return Result('SEMANTIC_REJECT','distinct '+attr)
    out=[];steps=0
    for arm,act in (('None',p.none_action),('Some',p.some_action)):
        state=world(p,arm)
        if act:
            if act.unknown_effect:return Result('PRECISION','unknown callee effect',first=arm)
            if act.mode not in ('call','inline'):return Result('UNSUPPORTED','unmodelled call body',first=arm)
            if act.mode=='call':
                m={}
                for k,v in act.mappings:
                    if k in m:return Result('SEMANTIC_REJECT','duplicate formal mapping',first=arm)
                    m[k]=v
                if m.get('field')!=act.actual_write or m.get('next')!=act.actual_next or m.get('stable')!=act.read_domain:
                    return Result('SEMANTIC_REJECT','formal-to-actual substitution',first=arm)
            if act.actual_write!='tail.next' or act.ref_root!='tail' or act.ref_incarnation!='I.tail':
                return Result('SEMANTIC_REJECT','wrong root/place/incarnation',first=arm)
            if not act.read_stable or not act.write_ref or act.stale_loan or not act.scope_closed or not act.scope or act.derived_scope!=act.scope:
                return Result('SEMANTIC_REJECT','stale/nonclosing write-ref',first=arm)
            if act.read_domain!='D.tail':return Result('SEMANTIC_REJECT','original Domain mismatch',first=arm)
            if act.actual_next!='q' or (arm=='None' and act.actual_next=='q') or act.stale_q:
                return Result('SEMANTIC_REJECT','matched Copy q not valid',first=arm)
            if not act.call_exit_ok:return Result('SEMANTIC_REJECT','callee exit failure',first=arm)
            if act.sibling_effect:return Result('SEMANTIC_REJECT','unrecorded sibling modification',first=arm)
            d=dict(state.cells); old=d[act.actual_write]
            if act.expected_old is not None and old.value!=act.expected_old:
                return Result('SEMANTIC_REJECT','old Option return not actual',first=arm)
            if not act.return_old:return Result('SEMANTIC_REJECT','old result dropped by required call contract',first=arm)
            oldptr=old.value.pointer if isinstance(old.value,Option) else None
            for ref in p.surviving:
                if ref.field==act.actual_write and (ref.fact==old.fact or ref.occurrence==old.occurrence and old.occurrence) :
                    return Result('SEMANTIC_REJECT','surviving stale field ref',first=arm)
                if ref.parent_fact==dict(state.root_facts)['tail']:
                    return Result('SEMANTIC_REJECT','surviving stale parent root ref',first=arm)
            new=Option(dict(state.cells)['head.next'].value.pointer)
            if new.pointer is None:return Result('SEMANTIC_REJECT','q is not Some',first=arm)
            t=lookup(p.originals,new.pointer.root)
            if t is None or (t.region,t.incarnation)!=(new.pointer.region,new.pointer.incarnation) or not t.live or new.pointer.allocation or new.pointer.domain:
                return Result('SEMANTIC_REJECT','stale or privileged Copy ptr',first=arm)
            d[act.actual_write]=replace(old,value=new,fact='fact.call.'+arm+'.'+act.source_id,occurrence='occ.call.'+arm+'.'+act.source_id)
            roots=dict(state.root_facts);roots['tail']='rootfact.call.'+arm+'.'+act.source_id
            state=replace(state,cells=tuple(sorted(d.items())),root_facts=tuple(sorted(roots.items())),returned_old=((act.source_id,old.value),))
            # Old memory occurrence is absent from new field; returned *Copy* payload has a distinct carrier.
            assert d[act.actual_write].occurrence!=old.occurrence
            steps+=1
        out.append(state)
    # For each possible incoming normal state, use every alternative.
    for s in out:
        tail=dict(s.cells)['tail.next']
        if p.post_use=='read_some' and tail.value.pointer is None:return Result('PRECISION','not Some in all incoming states',tuple(out),steps)
        if p.post_use=='write':
            if any(not r.live for r in s.roots):return Result('SEMANTIC_REJECT','cannot write ended original',tuple(out),steps)
        if p.post_use not in ('copy','write','read_some','none'):
            return Result('UNSUPPORTED','unknown post-join consumer',tuple(out),steps)
    return Result('ACCEPT','all concrete normal states',tuple(out),steps,len(out)*10,len(out)*10,4*len(out))
