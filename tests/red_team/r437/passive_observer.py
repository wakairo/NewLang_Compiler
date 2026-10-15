#!/usr/bin/env python3
"""Pre-registered best-effort, passive C17 observer, discovering ONLY R-emitted C AST.
Never reads Production fixtures, internals or previously generated C."""
import json
import pathlib
import re
import subprocess
import sys
def walk(x):
    if isinstance(x,dict):
        yield x
        for child in x.values():yield from walk(child)
    elif isinstance(x,list):
        for y in x:yield from walk(y)
def descendants(x,kind):
    return [y for y in walk(x) if isinstance(y,dict) and y.get("kind")==kind]
def name_type(x):
    return (x.get("type") or {}).get("qualType","")
def loc(x):
    return (x.get("range") or {}).get("begin",{}).get("offset")
def record_of_next(records,t):
    for record in records:
        n=record.get("name") or ""
        if n and re.search(r"\b"+re.escape(n)+r"\b",t):return record
    return None
def go(inp,out,case):
    text=pathlib.Path(inp).read_text()
    ast=subprocess.run(["clang","-std=c17","-Xclang","-ast-dump=json","-fsyntax-only",str(inp)],capture_output=True,text=True)
    if ast.returncode:return "no-clang-ast:"+ast.stderr[:300]
    nodes=list(walk(json.loads(ast.stdout)))
    records=[x for x in nodes if isinstance(x,dict) and x.get("kind")=="RecordDecl" and x.get("completeDefinition") and descendants(x,"FieldDecl")]
    node_records=[x for x in records if {"next","payload"}.issubset({f.get("name") for f in descendants(x,"FieldDecl")})]
    if len(node_records)!=1:return "node-record-not-uniquely-publicly-identifiable:"+str(len(node_records))
    node=node_records[0]
    aliases={node.get("name") or ""}
    for td in [x for x in nodes if isinstance(x,dict) and x.get("kind")=="TypedefDecl"]:
        if any(n.get("id")==node.get("id") for n in descendants(td,"RecordType")):
            aliases.add(td.get("name",""))
        # AST desugared record type can also reveal a direct typedef.
        if node.get("name") and node.get("name") in name_type(td):aliases.add(td.get("name",""))
    aliases.discard("")
    functions=[x for x in nodes if isinstance(x,dict) and x.get("kind")=="FunctionDecl" and descendants(x,"CompoundStmt")]
    selected=None
    for fn in functions:
        bodies=[x for x in fn.get("inner",[]) if x.get("kind")=="CompoundStmt"]
        if not bodies:continue
        body=bodies[-1]
        decls=[]
        for stmt in body.get("inner",[]):
            if stmt.get("kind")!="DeclStmt":continue
            for var in stmt.get("inner",[]):
                if var.get("kind")=="VarDecl" and any(re.search(r"\b"+re.escape(n)+r"\b",name_type(var)) for n in aliases):
                    decls.append(var)
        if len(decls)>=2:
            selected=(fn,body,decls)
            break
    if not selected:return "two-physical-root-locals-not-in-same-C-scope"
    fn,body,vars=selected
    left,right=vars[0]["name"],vars[1]["name"]
    if not re.fullmatch("[a-zA-Z_][a-zA-Z0-9_]*",left) or not re.fullmatch("[a-zA-Z_][a-zA-Z0-9_]*",right):
        return "unsafe-carrier-identifiers"
    # The R source is always in peer-then-anchor order; check payload values at runtime,
    # not by trusting generated variable spelling/order.
    anchor_value,peer_value=(31,79) if case=="none" else (47,83)
    expected_some=1 if case=="none" else 0
    fields=[f for f in descendants(node,"FieldDecl") if f.get("name")=="next"]
    next_type=name_type(fields[0]) if fields else ""
    option=record_of_next(records,next_type)
    # No guessed C layout or numeric pointer offset.
    tagfield=None;ptrfield=None
    if option:
        for f in descendants(option,"FieldDecl"):
            typ=name_type(f);n=f.get("name") or ""
            if re.search(r"(^|_)(tag|some|present|has|valid)(_|$)",n,re.I) or n in ("kind","is_some"):
                tagfield=n
            if "*" in typ and n:ptrfield=n
    def emit_check(anchor,peer):
        test=(f'\nif ((const void *)&{anchor} == (const void *)&{peer}) abort();\n'
              f'if ((unsigned){anchor}.payload != {anchor_value}U || (unsigned){peer}.payload != {peer_value}U) abort();\n')
        if tagfield:
            test+=f'if ((int){peer}.next.{tagfield} != 0 || (int){anchor}.next.{tagfield} != {expected_some}) abort();\n'
        if ptrfield and expected_some:
            test+=f'if ((const void *){anchor}.next.{ptrfield} != (const void *)&{peer}) abort();\n'
        test+=(f'fprintf(stderr,"R437_NATIVE_OBSERVER_OK case={case} anchor=%p peer=%p payload_anchor=%u payload_peer=%u tag_access={bool(tagfield)} ptr_access={bool(ptrfield)}\\n",'
               f'(void *)&{anchor},(void *)&{peer},(unsigned){anchor}.payload,(unsigned){peer}.payload);\n')
        return test
    # Choose the last direct return (or the enclosing function closing brace).
    returns=[x for x in body.get("inner",[]) if x.get("kind")=="ReturnStmt" and loc(x) is not None]
    offset=loc(returns[-1]) if returns else (body.get("range") or {}).get("end",{}).get("offset")
    if offset is None:return "no-scope-safe-observation-point"
    probe=(f'\nif ((unsigned){left}.payload == {peer_value}U && (unsigned){right}.payload == {anchor_value}U) {{\n'
           +emit_check(right,left)
           +f'}} else if ((unsigned){right}.payload == {peer_value}U && (unsigned){left}.payload == {anchor_value}U) {{\n'
           +emit_check(left,right)
           +'} else abort();\n')
    patched='#include <stdio.h>\n#include <stdlib.h>\n'+text[:offset]+probe+text[offset:]
    pathlib.Path(out).write_text(patched)
    return "instrumented function="+str(fn.get("name"))+" roots="+left+","+right+" tag="+str(tagfield)+" ptr="+str(ptrfield)+" new_file="+str(out)
if __name__=="__main__":
    result=go(sys.argv[1],sys.argv[2],sys.argv[3])
    print("R437 PASSIVE OBSERVER",result,flush=True)
    sys.exit(0 if result.startswith("instrumented ") else 2)
