"""O482 neutral experiment vocabulary: hypotheses, never C-issued authority."""
from __future__ import annotations
from dataclasses import dataclass, replace, asdict
import hashlib, json

FIELDS=('next','prev','child','payload')

@dataclass(frozen=True)
class Original:
    name:str
    region:str
    root:str
    incarnation:str
    allocation:str
    domain:str
    owner:str
    live:bool=True

@dataclass(frozen=True)
class Pointer:
    root:str
    region:str
    incarnation:str
    copy:bool=True
    allocation:None=None
    domain:None=None

@dataclass(frozen=True)
class Option:
    pointer:Pointer|None=None

@dataclass(frozen=True)
class Cell:
    place:str
    incarnation:str
    value:Option|int
    fact:str
    occurrence:str|None=None

@dataclass(frozen=True)
class Dep:
    field:str
    fact:str
    occurrence:str|None=None
    parent_fact:str|None=None

@dataclass(frozen=True)
class Action:
    mode:str='call' # 'call' or 'inline'
    mappings:tuple[tuple[str,str],...]=(('field','tail.next'),('next','q'),('stable','D.tail'))
    actual_write:str='tail.next'
    actual_next:str='q'
    read_domain:str='D.tail'
    ref_root:str='tail'
    ref_incarnation:str='I.tail'
    scope:str='scope.call'
    derived_scope:str='scope.call'
    scope_closed:bool=True
    read_stable:bool=True
    write_ref:bool=True
    return_old:bool=True
    expected_old:Option|None=None
    unknown_effect:bool=False
    summary_effects:tuple[str,...]=('Change','Reset')
    sibling_effect:bool=False
    call_exit_ok:bool=True
    stale_q:bool=False
    stale_loan:bool=False
    source_id:str='Some:replace:0'

@dataclass(frozen=True)
class Program:
    originals:tuple[Original,...]
    some_action:Action|None
    none_action:Action|None=None
    tail_old:Option=Option()
    surviving:tuple[Dep,...]=()
    post_use:str='copy' # copy|write|read_some|none
    cap:int=2
    hypothesis:str='two-normal'
    seed:int=482

@dataclass(frozen=True)
class World:
    roots:tuple[Original,...]
    cells:tuple[tuple[str,Cell],...]
    root_facts:tuple[tuple[str,str],...]
    active_scopes:tuple[str,...]=()
    returned_old:tuple[tuple[str,Option],...]=()

@dataclass(frozen=True)
class Result:
    status:str
    reason:str
    states:tuple[World,...]=()
    records:int=0
    slots:int=0
    clones:int=0
    visits:int=0
    first:str=''


def lookup(originals:tuple[Original,...], name:str)->Original|None:
    return next((o for o in originals if o.name==name),None)


def pointer(originals:tuple[Original,...],name:str)->Pointer:
    o=lookup(originals,name)
    if o is None:raise ValueError(name)
    return Pointer(o.name,o.region,o.incarnation)


def world(p:Program, arm:str)->World:
    d={}
    roots={}
    for r in p.originals:
        roots[r.name]='rootfact.entry.'+r.name
        for f in FIELDS:
            name=r.name+'.'+f
            v=(Option(pointer(p.originals,'tail')) if r.name=='head' and f=='next' and arm=='Some' else Option()) if f!='payload' else (1 if r.name=='head' else 2)
            if r.name=='tail' and f=='next':v=p.tail_old
            d[name]=Cell('place.'+name,'I.field.'+name,v,'fact.entry.'+name,
                         ('occ.guard.head.next' if arm=='Some' else None) if name=='head.next' else ('occ.tail.old' if r.name=='tail' and f=='next' and isinstance(v,Option) and v.pointer is not None else None))
    return World(p.originals,tuple(sorted(d.items())),tuple(sorted(roots.items())))


def digest(p:Program)->str:
    return hashlib.sha256(json.dumps(asdict(p),sort_keys=True,separators=(',',':')).encode()).hexdigest()


def fixture(*,mode='call',old_some=False,**kw)->Program:
    rs=tuple(Original(n,'R.'+n,'O.'+n,'I.'+n,'A.'+n,'D.'+n,'owner.'+n) for n in ('head','tail'))
    old=Option(pointer(rs,'head')) if old_some else Option()
    return Program(rs,Action(mode=mode),tail_old=old,**kw)


def alter(p:Program,**changes)->Program:
    return replace(p,**changes)


def alter_action(p:Program,**changes)->Program:
    return replace(p,some_action=replace(p.some_action,**changes))
