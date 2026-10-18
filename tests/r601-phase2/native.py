"""R601-PHASE2 fixed-independent oracle: original C17, passive real carriers, mutants."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

def run(*args):
    return subprocess.run(list(map(str,args)),capture_output=True,text=True)

def require(condition,message):
    if not condition:
        raise SystemExit("R601 FAIL: "+message)

def native(cc,opt,code,directory,label,expected_exit):
    path=directory/(label+"-"+cc+"-"+opt[1:]+".c")
    exe=path.with_suffix("")
    path.write_text(code)
    p=run(cc,"-std=c17","-Wall","-Wextra","-Wpedantic","-Werror",opt,path,"-o",exe)
    require(p.returncode==0, "compile "+str(path)+" "+p.stderr)
    p=run(exe)
    require(p.returncode==expected_exit and not p.stdout and not p.stderr,
            "native "+label+"/"+cc+"/"+opt+" observed="+str(p.returncode)+" stderr="+p.stderr)

def main():
    baseline,candidate,root,out=sys.argv[1:]
    root=Path(root)
    out=Path(out)
    out.mkdir(parents=True,exist_ok=True)
    oracle=json.loads((root/"oracle.json").read_text())
    cases=oracle["cases"]
    for snap,compiler,selected in (
        ("baseline",Path(baseline),["add_a","add_b","add_zero"]),
        ("candidate",Path(candidate),["add_a","add_b","add_zero","sub_a","sub_b","sub_zero","sequence"])):
        for name in selected:
            source=root/(name+".nl")
            checked=run(compiler,"--check",source)
            emitted=run(compiler,source)
            require(checked.returncode==0 and emitted.returncode==0 and
                    not checked.stdout and not checked.stderr and not emitted.stderr,
                    snap+"/"+name+" SOURCE/CHECK/C17 "+str(checked)+" "+str(emitted))
            code=emitted.stdout
            require("#include <stddef.h>" in code and
                    "CHAR_BIT == 8" in code and "SIZE_MAX == UINTMAX_C(" in code,
                    snap+"/"+name+" target guards")
            digest=hashlib.sha256(code.encode()).hexdigest()
            (out/(snap+"-"+name+".original.c17.txt")).write_text(code)
            decls=list(re.finditer(r"(?m)^    const size_t (nl_local_\d+) = (.+);$",code))
            entry=cases[name]
            if name=="sequence":
                expectations=entry["answers"]
                operations=entry["ops"]
            else:
                expectations=[*entry["numbers"],entry["answer"]]
                operations=[entry["op"]]
            require(len(decls)==len(expectations),snap+"/"+name+" original live declaration count "+
                    str(len(decls))+" expected "+str(len(expectations)))
            if name=="sequence":
                require(all(" "+op+" " in decls[i][2] for i,op in enumerate(operations)),
                        "source operator sequence changed")
            else:
                require(" "+operations[0]+" " in decls[-1][2],name+" actual source operator absent")
            staged=code.replace("#include <stddef.h>",
                                "#include <stddef.h>\n#include <stdlib.h>",1)
            for match,value in zip(decls,expectations):
                original=match[0]
                add=original+"\n    if ("+match[1]+" != ((size_t)UINTMAX_C("+str(value)+"))) exit(91);"
                require(staged.count(original)==1,"ambiguous carrier")
                staged=staged.replace(original,add,1)
            for cc in ("gcc","clang"):
                for opt in ("-O0","-O2"):
                    native(cc,opt,code,out,snap+"-"+name+"-plain",0)
                    native(cc,opt,staged,out,snap+"-"+name+"-observed",0)
            # Mutations change only original guest arithmetic; observers remain identical.
            index=1 if name=="sequence" else len(decls)-1
            match=decls[index]
            original_line=match[0]
            number=expectations[index]
            corrupted=original_line.replace(" = "+match[2]+";",
                  " = ((size_t)UINTMAX_C("+str(0 if number else 1)+"));",1)
            require(corrupted!=original_line and original_line in staged,"mutant insertion")
            wrong=staged.replace(original_line,corrupted,1)
            op=operations[index] if name=="sequence" else operations[0]
            swapped_op="-" if op=="+" else "+"
            changed=original_line.replace(" "+op+" "," "+swapped_op+" ",1)
            require(changed!=original_line,"operator mutation")
            wrong_operator=staged.replace(original_line,changed,1)
            for cc in ("gcc","clang"):
                for opt in ("-O0","-O2"):
                    native(cc,opt,wrong,out,snap+"-"+name+"-wrong-value",91)
                    if name not in ("add_zero",):
                        native(cc,opt,wrong_operator,out,snap+"-"+name+"-wrong-op",91)
            print(json.dumps(dict(snapshot=snap,source=name,phase="SOURCE/CHECK/C17 PASS",
                original_c_sha256=digest,live_carriers=expectations,operators=operations,
                passive="GCC/Clang O0/O2 PASS",wrong_value="REJECT",
                wrong_op="NOT_DISTINGUISHABLE" if name=="add_zero" else "REJECT"),sort_keys=True),flush=True)
    under=Path(root/"underflow.nl")
    checked=run(candidate,"--check",under)
    emitted=run(candidate,under)
    require(checked.returncode!=0 and emitted.returncode!=0 and
            not checked.stdout and not emitted.stdout and
            cases["underflow"]["diagnostic"] in checked.stderr and
            cases["underflow"]["diagnostic"] in emitted.stderr,
            "underflow must refuse without C")
    print("UNDERFLOW/C17-OUTPUT SENTINEL: semantic CHECK refusal, C bytes=0; no stream transactional OOM claimed",flush=True)

if __name__=="__main__":
    main()
