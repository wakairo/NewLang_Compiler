#!/usr/bin/env python3
"""R592 NON-BLIND calibrated passive native observation. Frozen P02 source unchanged.
This is not part of the original BLIND verdict; C emitter inspected AFTER lock
of first-pass verdict blob 48947839a1c9f791821209d8e22667c7cf936aaa.
"""
import hashlib
import pathlib
import re
import subprocess
import tempfile
ROOT=pathlib.Path(__file__).resolve().parents[3]
CC=ROOT/"build-r592"/"newlangc"
SRC=("fn main() -> unit {\n"
     "    let alpha = usize(37);\n"
     "    let beta = usize(59);\n"
     "    let answer = alpha + beta;\n"
     "    unit\n"
     "}\n")
EXPECTED_SRC="1f9152f55345f928c512783714a65a2470c0ada4248e80f17374a71a561f77ea"
EXPECTED_C="406ea83df7efff6cb7f48be976ec92d14e11414c9f96ce63a38d666e270425ab"
assert hashlib.sha256(SRC.encode()).hexdigest()==EXPECTED_SRC, "frozen original changed"
def run(args):
    return subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=50)
with tempfile.TemporaryDirectory(prefix="r592-calibrated-") as folder:
    tmp=pathlib.Path(folder)
    source=tmp/"frozen-p02.nl"
    source.write_text(SRC)
    checked=run([str(CC),"--check",str(source)])
    assert checked.returncode==0 and checked.stdout==b"" and checked.stderr==b"","checked original"
    emitted=run([str(CC),str(source)])
    assert emitted.returncode==0 and emitted.stderr==b"","C17 original emission"
    c=emitted.stdout.decode()
    digest=hashlib.sha256(emitted.stdout).hexdigest()
    print("CALIBRATED_C17_SHA256",digest,flush=True)
    assert digest==EXPECTED_C,"C17 byte identity drift relative to BLIND run"
    assert "SIZE_MAX" in c and "CHAR_BIT" in c,"target/model guard absent"
    pattern=r"(?m)^([ \t]*)const size_t nl_local_(\d+) = \(nl_local_(\d+) \+ nl_local_(\d+)\);$"
    matches=list(re.finditer(pattern,c))
    assert len(matches)==1, "expected exactly one guest C17 live addition"
    m=matches[0]
    indent,dst,lhs,rhs=m.groups()
    assert len({dst,lhs,rhs})==3,"guest result not fresh carrier"
    for index,value in [(lhs,37),(rhs,59)]:
        text="const size_t nl_local_"+index+" = ((size_t)UINTMAX_C("+str(value)+"));"
        assert text in c, "original guest operand not independently staged: "+text
    inject=(f"\n{indent}/* R592 observer: READ ONLY while all three originals are live */\n"
        f"{indent}if (nl_local_{lhs} != (size_t)37 || nl_local_{rhs} != (size_t)59 || "
        f"nl_local_{dst} != (size_t)96 || &nl_local_{lhs} == &nl_local_{rhs} || "
        f"&nl_local_{lhs} == &nl_local_{dst} || &nl_local_{rhs} == &nl_local_{dst}) {{\n"
        f"{indent}    fputs(\"R592_BAD_LIVE_CARRIERS\\n\", stderr); abort();\n"
        f"{indent}}}\n"
        f"{indent}fprintf(stderr,\"R592_OBSERVED l=%zu r=%zu sum=%zu laddr=%p raddr=%p sumaddr=%p\\n\","
        f"nl_local_{lhs},nl_local_{rhs},nl_local_{dst},"
        f"(void*)&nl_local_{lhs},(void*)&nl_local_{rhs},(void*)&nl_local_{dst});\n")
    instrumented="#include <stdio.h>\n#include <stdlib.h>\n"+c[:m.end()]+inject+c[m.end():]
    altered=re.sub(re.escape(m.group(0)),f"{indent}const size_t nl_local_{dst} = (nl_local_{lhs} + nl_local_{lhs});",c,count=1)
    assert altered!=c,"negative mutation not applied"
    broken="#include <stdio.h>\n#include <stdlib.h>\n"+altered[:altered.find(m.group(0))] if False else None
    # Rebuild mutated C with the EXACT SAME passive observer and changed guest expression.
    broken=instrumented.replace(m.group(0),f"{indent}const size_t nl_local_{dst} = (nl_local_{lhs} + nl_local_{lhs});",1)
    assert broken!=instrumented
    for label,contents in [("original",instrumented),("negative",broken)]:
        path=tmp/(label+".c")
        path.write_text(contents)
        for compiler in ("gcc","clang"):
            for opt in ("0","2"):
                exe=tmp/(label+"-"+compiler+opt)
                built=run([compiler,"-std=c17","-Wall","-Wextra","-Werror","-O"+opt,str(path),"-o",str(exe)])
                assert built.returncode==0,(label,compiler,opt,built.stderr.decode(errors="replace"))
                finished=run([str(exe)])
                if label=="original":
                    assert finished.returncode==0,(compiler,opt,finished.stderr)
                    assert finished.stderr.count(b"R592_OBSERVED l=37 r=59 sum=96")==1,"guest was not observed exactly once"
                    assert b"R592_BAD_LIVE_CARRIERS" not in finished.stderr
                    print("CALIBRATED_PASSIVE_NATIVE_PASS",compiler,"O"+opt,finished.stderr.decode().strip(),flush=True)
                else:
                    assert finished.returncode!=0 and b"R592_BAD_LIVE_CARRIERS" in finished.stderr,"intentionally corrupted guest not detected"
                    print("CALIBRATED_CORRUPT_GUEST_DETECTED",compiler,"O"+opt,flush=True)
    print("CALIBRATED_RESULT live original operands=37,59 and actual original result=96; 4/4 compilers; independent mutated guest 4/4 rejected",flush=True)
