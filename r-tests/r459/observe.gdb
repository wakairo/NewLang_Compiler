# R459 gdb-only read-only physical observer for generated C17.
# Do not change guest memory, rewrite generated C, or derive an expected oracle.
set pagination off
set confirm off
set print pretty off
set step-mode off
python
import gdb, json, os, sys
want = {"anchor":37,"peer":211}
seen = {}
steps = 0
def fields(v):
    try:
        t=v.type.strip_typedefs()
        if t.code != gdb.TYPE_CODE_STRUCT: return {}
        return {f.name:v[f.name] for f in t.fields() if f.name}
    except Exception: return {}
def walk(v, path, depth):
    if depth>3: return
    fs=fields(v)
    if "next" in fs and "payload" in fs:
        try:
            p=int(fs["payload"])
            address=int(v.address) if v.address is not None else 0
            if address and p in want.values():
                nm = "anchor" if p==37 else "peer"
                # Original lexical mapping is required: no incidental temporary copies.
                if nm in path.lower():
                    flat={}
                    def flatten(x,key,d):
                        if d>3: return
                        flds=fields(x)
                        if flds:
                            for k,y in flds.items(): flatten(y,key+"."+k,d+1)
                        else:
                            try:
                                t=x.type.strip_typedefs()
                                if t.code==gdb.TYPE_CODE_PTR: flat[key]={"ptr":int(x)}
                                elif t.code in (gdb.TYPE_CODE_INT,gdb.TYPE_CODE_BOOL,gdb.TYPE_CODE_ENUM): flat[key]={"integer":int(x)}
                            except Exception: pass
                    flatten(fs["next"],"next",0)
                    seen[nm] = {"symbol":path,"address":address,"payload":p,"next":flat}
        except Exception: pass
    for k,x in fs.items():
        if k not in ("next",):
            walk(x,path+"."+k,depth+1)
def scan():
    fr=gdb.selected_frame()
    while fr:
        try:
            b=fr.block()
            while b:
                for s in b:
                    if not (s.is_variable or s.is_argument): continue
                    try: walk(s.value(fr),s.name,0)
                    except Exception: pass
                if b.is_global: break
                b=b.superblock
        except Exception: pass
        fr=fr.older()
def assess():
    if len(seen)!=2: return None
    a,p=seen["anchor"],seen["peer"]
    if a["address"]==p["address"]: return "REJECT_SAME_PHYSICAL_ORIGINAL"
    # Require a true separate option tag and a pointer stored in the peer link.
    a_next=a["next"]; p_next=p["next"]
    def tag(fs):
        matches=[v["integer"] for k,v in fs.items()
                 if any(h in k.lower() for h in ("tag","has","some","present")) and "integer" in v]
        return matches[0] if len(matches)==1 else None
    def pointers(fs):
        return [v["ptr"] for k,v in fs.items() if "ptr" in v]
    at,pt=tag(a_next),tag(p_next)
    ap,pp=pointers(a_next),pointers(p_next)
    if os.environ.get("R459_CONTROL")=="1":
        if at==0 and pt==0: return "PASS"
    elif at==0 and pt==1 and a["address"] in pp and all(x==0 for x in ap):
        return "PASS"
    if pt is not None and at is not None and pp:
        return "REJECT_PHYSICAL_TOPOLOGY"
    return None
try:
    gdb.execute("start",to_string=True)
    for steps in range(1,2801):
        scan()
        result=assess()
        if result:
            print("R459 GDB_READ_ONLY "+json.dumps({"status":result,"steps":steps,"roots":seen},sort_keys=True),flush=True)
            if result!="PASS": gdb.execute("quit 41")
            gdb.execute("continue",to_string=True)
            break
        try: gdb.execute("step",to_string=True)
        except gdb.error: break
    else: result=None
    if not result:
        print("R459 OBSERVER_UNRESOLVED "+json.dumps({"steps":steps,"observed":seen},sort_keys=True),flush=True)
        gdb.execute("quit 42")
except Exception as e:
    print("R459 OBSERVER_ENVIRONMENT "+repr(e),flush=True)
    gdb.execute("quit 43")
end
quit 0
