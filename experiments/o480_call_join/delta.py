"""Algorithm B: local Change/Reset ledger, mapped effect atoms and dependencies.

Independent verifier; does not import snapshot or concrete. Store two local
updates (fixed-field and parent-root currentness) and returned-old package.
Reconstruct complete worlds ONLY for optional independent result comparison;
this diagnostic materialization is counted as visits, not as a stored delta.
"""
from dataclasses import dataclass,replace
from ir import Program,World,Result,Option,Cell,world,lookup

@dataclass(frozen=True)
class Change:
    sid:str
    place:str
    read_domain:str
    scope:str
    old:Option
    old_fact:str
    old_occurrence:str|None
    new:Option
    new_fact:str
    new_occurrence:str
    parent_old:str
    parent_new:str
    dependencies:tuple[str,...]
    effects:tuple[str,str]=('Change','Reset')


def check(p:Program)->Result:
    updates=[];worlds=[];walked=0
    def finish(status,rule,arm=''):
        return Result(status,rule,tuple(worlds),len(updates),len(updates)*2,0,walked,arm)
    if len(p.originals)!=2 or {o.name for o in p.originals}!={'head','tail'}:
        return finish('UNSUPPORTED','not bounded two roots')
    if not all(x.live for x in p.originals):return finish('SEMANTIC_REJECT','dead original root')
    for prop in ('region','root','incarnation','allocation','domain','owner'):
        if len({getattr(x,prop) for x in p.originals})!=2:
            return finish('SEMANTIC_REJECT','duplicate '+prop)
    for arm,act in (('None',p.none_action),('Some',p.some_action)):
        base=world(p,arm)
        env={k:v for k,v in base.cells}
        parentfacts={k:v for k,v in base.root_facts}
        local=[];returned=()
        if act:
            if act.unknown_effect:return finish('PRECISION','unknown called effect',arm)
            if act.mode not in ('inline','call'):return finish('UNSUPPORTED','unknown operation',arm)
            if act.mode=='call':
                if len(act.summary_effects)!=2 or 'Change' not in act.summary_effects or 'Reset' not in act.summary_effects:
                    return finish('PRECISION','summary atom omission',arm)
                seen=set(); mapping={}
                for symbolic,location in act.mappings:
                    if symbolic in seen:return finish('SEMANTIC_REJECT','duplicate formal binding',arm)
                    seen.add(symbolic);mapping[symbolic]=location
                if any(mapping.get(formal)!=actual for formal,actual in (('field',act.actual_write),('next',act.actual_next),('stable',act.read_domain))):
                    return finish('SEMANTIC_REJECT','mismatched argument map',arm)
            if act.actual_write!='tail.next' or act.ref_root!='tail' or act.ref_incarnation!='I.tail':
                return finish('SEMANTIC_REJECT','wrong actual referent',arm)
            if act.read_domain!='D.tail' or not act.read_stable or not act.write_ref or act.stale_loan:
                return finish('SEMANTIC_REJECT','no proper original D/ref grant',arm)
            if not act.scope or act.scope!=act.derived_scope or not act.scope_closed:
                return finish('SEMANTIC_REJECT','loan escapes call',arm)
            if not act.call_exit_ok:return finish('SEMANTIC_REJECT','call body incomplete exit',arm)
            if act.sibling_effect:return finish('SEMANTIC_REJECT','extra unaccounted sibling effect',arm)
            if act.actual_next!='q' or act.stale_q or arm!='Some':
                return finish('SEMANTIC_REJECT','q source mismatch',arm)
            q=env['head.next'].value
            if not isinstance(q,Option) or q.pointer is None:return finish('SEMANTIC_REJECT','Some binder missing',arm)
            ref=q.pointer;src=lookup(p.originals,ref.root)
            if src is None or not src.live or (ref.region,ref.incarnation)!=(src.region,src.incarnation) or not ref.copy or ref.allocation is not None or ref.domain is not None:
                return finish('SEMANTIC_REJECT','stale/privileged copied ref',arm)
            before=env[act.actual_write]
            if act.expected_old is not None and act.expected_old!=before.value:
                return finish('SEMANTIC_REJECT','returned-old mismatch',arm)
            if not act.return_old:return finish('SEMANTIC_REJECT','old Option not transferred',arm)
            changed_place=act.actual_write; enclosing='tail'
            # dependency proof against future *current* facts, not pointer bytes.
            for package in p.surviving:
                if package.field==changed_place and (package.fact==before.fact or package.occurrence is not None and package.occurrence==before.occurrence):
                    return finish('SEMANTIC_REJECT','Change/Reset conflicts surviving package',arm)
                if package.parent_fact==parentfacts[enclosing]:
                    return finish('SEMANTIC_REJECT','root Change conflicts surviving package',arm)
            record=Change(act.source_id,changed_place,act.read_domain,act.scope,before.value,before.fact,before.occurrence,
                          Option(ref),'fact.call.'+arm+'.'+act.source_id,'occ.call.'+arm+'.'+act.source_id,
                          parentfacts[enclosing],'rootfact.call.'+arm+'.'+act.source_id,
                          ('Copy:q','Original:D.tail','Parent:tail','Field:tail.next'))
            if record.new_occurrence==record.old_occurrence:return finish('SEMANTIC_REJECT','old occurrence not ended',arm)
            local.append(record)
            updates.append(record)
            returned=((act.source_id,record.old),)
        # Post-join read requires each NORMAL branch to establish Some. Whole
        # Change may be safe across both alternatives even when facts differ.
        if p.post_use=='read_some':
            after_value=local[-1].new if local else env['tail.next'].value
            if after_value.pointer is None:return finish('PRECISION','conditional None after join',arm)
        if p.post_use=='write' and not all(o.live for o in p.originals):
            return finish('SEMANTIC_REJECT','no live original for later write',arm)
        if p.post_use not in ('copy','write','read_some','none'):
            return finish('UNSUPPORTED','unknown continuation',arm)
        walked+=len(env)+len(parentfacts)
        # NOTE: diagnostic reconstruction only, not saved in algorithm ledger.
        for d in local:
            prior=env[d.place]
            env[d.place]=replace(prior,value=d.new,fact=d.new_fact,occurrence=d.new_occurrence)
            parentfacts['tail']=d.parent_new
        walked+=len(local)*2
        worlds.append(World(base.roots,tuple(sorted(env.items())),tuple(sorted(parentfacts.items())),returned_old=returned))
    if len(worlds)>p.cap:return finish('PRECISION','bounded alternative budget')
    return finish('ACCEPT','delta effects and dependencies cover both normals')
