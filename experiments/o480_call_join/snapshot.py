"""Algorithm A: complete semantic snapshots + actual/formal substitution.

No import from delta.py or concrete.py. Full cells/root-currentness are cloned
at a call; the formal body executes independently in a checked environment.
"""
from dataclasses import replace
from ir import Program,Result,World,Cell,Option,Pointer,world,lookup


def check(p:Program)->Result:
    copied=0;visited=0;states=[];emitted=0
    def result(k,why,arm=''):
        return Result(k,why,tuple(states),emitted,len(states)*10,copied,visited,arm)
    if len(p.originals)!=2 or sorted(r.name for r in p.originals)!=['head','tail']:
        return result('UNSUPPORTED','two original roots')
    for key in ('root','region','incarnation','allocation','domain','owner'):
        if len({getattr(r,key) for r in p.originals})!=2:return result('SEMANTIC_REJECT','original not distinct: '+key)
    if not all(r.live for r in p.originals):return result('SEMANTIC_REJECT','stale original')
    for arm,call in (('None',p.none_action),('Some',p.some_action)):
        before=world(p,arm)
        cells=dict(before.cells);rf=dict(before.root_facts)
        visited+=len(cells)+len(rf)
        if call is not None:
            if call.unknown_effect:return result('PRECISION','unknown body effect',arm)
            if call.mode not in ('call','inline'):return result('UNSUPPORTED','body kind',arm)
            formal_env=None
            if call.mode=='call':
                if set(call.summary_effects)!={'Change','Reset'} or len(call.summary_effects)!=2:
                    return result('PRECISION','known-body summary lost Change/Reset',arm)
                formals={}
                for f,actual in call.mappings:
                    if f in formals:return result('SEMANTIC_REJECT','one formal mapped twice',arm)
                    formals[f]=actual
                # A complete formal frame contains a ref into caller's place,
                # a Copy q value, and exact stability D. No invented owner.
                if formals.get('field')!=call.actual_write or formals.get('next')!=call.actual_next or formals.get('stable')!=call.read_domain:
                    return result('SEMANTIC_REJECT','unproved actual/formal substitution',arm)
                formal_env={'wref_place':formals['field'],'copy_source':formals['next'],'read_domain':formals['stable']}
                if formal_env['wref_place'] not in cells:
                    return result('SEMANTIC_REJECT','formal ref target not a caller place',arm)
            body_field=formal_env['wref_place'] if formal_env else call.actual_write
            body_q=formal_env['copy_source'] if formal_env else call.actual_next
            body_domain=formal_env['read_domain'] if formal_env else call.read_domain
            if body_field!='tail.next' or call.ref_root!='tail' or call.ref_incarnation!='I.tail':
                return result('SEMANTIC_REJECT','formal ref wrong original target',arm)
            if body_domain!='D.tail':return result('SEMANTIC_REJECT','original stability Domain',arm)
            if not call.read_stable or not call.write_ref or call.stale_loan or not call.scope_closed or not call.scope or call.scope!=call.derived_scope:
                return result('SEMANTIC_REJECT','scoped write-ref/exit',arm)
            if not call.call_exit_ok:return result('SEMANTIC_REJECT','named body exit',arm)
            if call.sibling_effect:return result('SEMANTIC_REJECT','unrecorded sibling effect',arm)
            if arm!='Some' or body_q!='q' or call.stale_q:
                return result('SEMANTIC_REJECT','q must be Some branch Copy',arm)
            source=cells['head.next'].value
            if not isinstance(source,Option) or source.pointer is None:
                return result('SEMANTIC_REJECT','no matched q source',arm)
            q=source.pointer
            target=lookup(p.originals,q.root)
            if not target or (target.region,target.incarnation)!=(q.region,q.incarnation) or not q.copy or q.allocation is not None or q.domain is not None:
                return result('SEMANTIC_REJECT','bad copied pointer origin',arm)
            old=cells[body_field]
            if call.expected_old is not None and call.expected_old!=old.value:
                return result('SEMANTIC_REJECT','old value not identical',arm)
            if not call.return_old:return result('SEMANTIC_REJECT','old value lost',arm)
            for dependency in p.surviving:
                if dependency.field==body_field and (dependency.fact==old.fact or dependency.occurrence is not None and dependency.occurrence==old.occurrence):
                    return result('SEMANTIC_REJECT','stale field/occurrence survives',arm)
                if dependency.parent_fact==rf['tail']:
                    return result('SEMANTIC_REJECT','stale containing root fact survives',arm)
            # Full semantic state snapshot; write through formal ref and replay
            # body locally; return old Option in distinct package.
            cells=cells.copy(); rf=rf.copy();copied+=len(cells)+len(rf)
            cells[body_field]=Cell(old.place,old.incarnation,Option(q),'fact.call.'+arm+'.'+call.source_id,'occ.call.'+arm+'.'+call.source_id)
            rf['tail']='rootfact.call.'+arm+'.'+call.source_id
            if old.occurrence is not None and old.occurrence==cells[body_field].occurrence:
                return result('SEMANTIC_REJECT','old occurrence reused',arm)
            emitted+=1
            returned=((call.source_id,old.value),)
        else:returned=()
        states.append(World(before.roots,tuple(sorted(cells.items())),tuple(sorted(rf.items())),returned_old=returned))
    if len(states)>p.cap:return result('PRECISION','snapshot state budget')
    for state in states:
        t=dict(state.cells)['tail.next']
        if p.post_use=='read_some' and t.value.pointer is None:
            return result('PRECISION','not Some on all alternatives')
        if p.post_use=='write' and not all(r.live for r in state.roots):
            return result('SEMANTIC_REJECT','stale write target')
        if p.post_use not in ('copy','write','read_some','none'):
            return result('UNSUPPORTED','post-join use')
        visited+=len(state.cells)+len(state.root_facts)
    return result('ACCEPT','all snapshot alternatives safe')
