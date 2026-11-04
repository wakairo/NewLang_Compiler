from pathlib import Path
import json,hashlib,zipfile
root=Path("/tmp/p1146-input-v1")
(root/"sources").mkdir(parents=True,exist_ok=True)
cases=[
 dict(name="01-add-many-first",nominal="MarkPair",fields=["heat","spare"],values=[72,203],write_index=0,start=1,limit=10,step=4,direction="add",threshold=14,tags=[119,47],trace=[1,5,9,13],result=13,tag=119),
 dict(name="02-add-zero-second",nominal="FlagPair",fields=["keep","flag"],values=[57,83],write_index=1,start=10,limit=10,step=4,direction="add",threshold=10,tags=[29,231],trace=[10],result=10,tag=231),
 dict(name="03-sub-many-second",nominal="SignalPair",fields=["steady","signal"],values=[91,156],write_index=1,start=22,limit=7,step=6,direction="sub",threshold=5,tags=[41,218],trace=[22,16,10,4],result=4,tag=41),
 dict(name="04-add-one-first",nominal="TonePair",fields=["tone","stable"],values=[17,65],write_index=0,start=8,limit=10,step=4,direction="add",threshold=12,tags=[36,214],trace=[8,12],result=12,tag=214),
]
def loop(e):
    update="+" if e["direction"]=="add" else "-"
    cont=f"            continue(cursor {update} usize({e['step']}));\n"
    brk="            break cursor;\n"
    yes,no=(cont,brk) if e["direction"]=="add" else (brk,cont)
    return (f"    let amount = loop (cursor = usize({e['start']})) {{\n"
            f"        if (cursor < usize({e['limit']})) {{\n"+yes+
            "        } else {\n"+no+"        }\n    };\n")
def helper(e):
    a,b=e["tags"]
    return (f"fn choose_{e['nominal']}(quantity: usize) -> u8 {{\n"
            f"    if (quantity < usize({e['threshold']})) {{ u8({a}) }} else {{ u8({b}) }}\n"
            "}\n")
def source(e):
    a,b=e["fields"];x,y=e["values"];field=e["fields"][e["write_index"]];other=e["fields"][1-e["write_index"]]
    return (f"struct {e['nominal']} {{ {a}: u8, {b}: u8 }}\n"+helper(e)+
            "fn main() -> unit {\n"+
            f"    let original = {e['nominal']}{{{a}: u8({x}), {b}: u8({y})}};\n"+loop(e)+
            "    let argument = amount;\n"+
            f"    let incoming = choose_{e['nominal']}(argument);\n"+
            f"    let old = loan_write(original@{field}) {{ |writer| replace(writer, incoming) }};\n"+
            f"    let updated = original@{field};\n"+
            f"    let sibling = original@{other};\n"+
            "    let old_again = old;\n    return unit;\n}\n")
expect={}
for e in cases:
    (root/"sources"/(e["name"]+".nl")).write_text(source(e))
    expect[e["name"]]={**e,"old":e["values"][e["write_index"]],"updated":e["tag"],"sibling":e["values"][1-e["write_index"]],
       "identity":"same original root and field address before loop, at store and after read; sibling distinct and unchanged",
       "result_dependency":"free Unknown usize, ordinary Copy; predicate current real actual/formal/result; scoped old u8 Copy free"}
base=cases[0];text=source(base)
controls={
 "80-scalar-control":helper(base)+"fn main() -> unit {\n"+loop(base)+"    let argument = amount;\n    let incoming = choose_MarkPair(argument);\n    return unit;\n}\n",
 "81-literal-write-control":"struct MarkPair { heat: u8, spare: u8 }\nfn main() -> unit {\n    let original = MarkPair{heat: u8(72), spare: u8(203)};\n    let old = loan_write(original@heat) { |writer| replace(writer, u8(119)) };\n    let updated = original@heat;\n    let sibling = original@spare;\n    let old_again = old;\n    return unit;\n}\n"
}
negatives={
 "90-overflow":text.replace("cursor + usize(4)","cursor + usize(18446744073709551615)"),
 "91-read-mode":text.replace("loan_write(original@heat)","loan_read(original@heat)"),
 "92-ending-reference":text.replace("replace(writer, incoming)","writer"),
 "93-incoming-type":text.replace("replace(writer, incoming)","replace(writer, argument)")
}
for name,data in {**controls,**negatives}.items():(root/"sources"/(name+".nl")).write_text(data)
oracle={"authority":"C226 / fixed main7e492ec / canonical Draft17.34","positive_cases":expect,
 "controls":{name:{"check":0,"emit":0} for name in controls},
 "negative_cases":{name:{"acceptance":"reject CHECK or backend before publication; preserve named output; classify actual first layer, do not overclaim later layer"} for name in negatives},
 "gate0_prediction":{"composite":"CHECK/source-issued may pass, first backend failure predicted at predicate_call aggregate exclusion; earlier actual failure is FIRST","main_unchanged":True},
 "native":{"compilers":["GCC13","Clang23"],"opts":["O0","O2"],"versions":["raw unchanged C","read-only passive observation"],"expected_exit":0},
 "scope":"one original two-u8 T alive from before one guarded scalar loop; actual loop result Copy/pure predicate then original scoped replace/read; no T-changing C, new issuer, bridge or semantic contract"}
(root/"oracle.json").write_text(json.dumps(oracle,indent=2)+"\n")
print("Prepared",len(cases),"positive,",len(controls),"controls,",len(negatives),"negative complete sources, not executed")
