#!/usr/bin/env python3
"""R516 precommitted blind passive C17/native harness. NO guest mutation."""
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

BASE = Path(__file__).resolve().parent
OUT = Path.cwd() / "r516-evidence"
OUT.mkdir(exist_ok=True)
COMPILER = Path(os.environ["R516_COMPILER"]).resolve()
EXPECTED = {
    "true": ("c921596f72e64116630676b98d6ce43c529420a4bec26c437d01981cc39cc864", 108),
    "false": ("0dfc413cfc63fa379b6b4c1d20a6beea1421188add4e05b9d1bc660fc0ca1664", 173),
    "reject_arm": ("d9f6c09fdb63ba50e5beaeb26b6ada585cfe67c28883a53c53c7504ce6c0c272", None),
}
LOG = []

def record(stage, cmd, result):
    row = {"stage": stage, "command": [str(x) for x in cmd],
           "exit": result.returncode, "stdout": result.stdout,
           "stderr": result.stderr}
    LOG.append(row)
    (OUT / "results.json").write_text(json.dumps(LOG, indent=2) + "\n")
    print("\nSTAGE", stage, "exit", result.returncode, flush=True)
    print("STDOUT", result.stdout[:6000], "STDERR", result.stderr[:6000], flush=True)
    return result

def run(stage, cmd, timeout=120):
    return record(stage, cmd, subprocess.run([str(x) for x in cmd],
          cwd=Path.cwd(), text=True, capture_output=True, timeout=timeout))

def stop(reason):
    print("R516 STOP:", reason, flush=True)
    raise SystemExit(2)

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

for name, (sha, _new) in EXPECTED.items():
    path = BASE / (name + ".nl")
    if digest(path) != sha:
        stop("PRE-EXECUTION FROZEN SOURCE HASH MISMATCH " + name)
true_text=(BASE / "true.nl").read_text()
false_text=(BASE / "false.nl").read_text()
if true_text.replace("operand_b = usize(29)", "operand_b = usize(30)") != false_text:
    stop("TRUE FALSE differ beyond source operand")
print("R516 VERIFIED FROZEN SOURCE SHA256", EXPECTED, flush=True)
print("COMPILER BINARY SHA256", digest(COMPILER), flush=True)

for name in ("true","false"):
    src=BASE/(name+".nl")
    result=run(name+".CHECK", [COMPILER,"--check",src])
    if result.returncode != 0 or result.stdout or result.stderr:
        stop(name+" positive source/CHECK mismatch; freeze diagnostics, NO tuning")

negative=run("reject_arm.CHECK", [COMPILER,"--check",BASE/"reject_arm.nl"])
if negative.returncode == 0:
    stop("FINDING: invalid unreachable ELSE arm admitted")
if negative.returncode != 3:
    stop("CONTROL INCONCLUSIVE: negative fails outside expected semantic exit 3")
if "256" not in (negative.stderr + negative.stdout) and not re.search("range|literal|u8", negative.stderr, re.I):
    stop("CONTROL INCONCLUSIVE: cannot independently attribute negative to u8 range")
print("R516 negative: rejected at CHECK; inspect diagnostic for parser vs semantic", flush=True)

for name in ("true","false"):
    src=BASE/(name+".nl")
    emitted=run(name+".EMIT", [COMPILER,src])
    if emitted.returncode != 0 or not emitted.stdout.strip():
        stop(name+" emission mismatch / finite backend unsupported")
    cpath=OUT/(name+".c")
    cpath.write_text(emitted.stdout)
    before=digest(cpath)
    print(name,"IMMUTABLE EMITTED C SHA256",before,flush=True)
    for cc in ("gcc","clang"):
        if not shutil.which(cc):
            stop("missing strict C17 compiler "+cc)
        for opt in ("O0","O2"):
            exe=OUT/(name+"-"+cc+"-"+opt)
            flags=["-std=c17","-Wall","-Wextra","-Wpedantic","-Werror","-g",
                   "-"+opt, "-fno-omit-frame-pointer"]
            c=run(name+"."+cc+"."+opt+".COMPILE",[cc,*flags,cpath,"-o",exe])
            if c.returncode != 0:
                stop("strict C17 compile failed "+name+" "+cc+" "+opt)
            result=run(name+"."+cc+"."+opt+".NATIVE",[exe])
            if result.returncode != 0:
                stop("native run failed "+name+" "+cc+" "+opt)
    if shutil.which("clang"):
        exe=OUT/(name+"-ubsan")
        flags=["-std=c17","-Wall","-Wextra","-Wpedantic","-Werror","-g","-O0",
               "-fsanitize=undefined","-fno-sanitize-recover=all"]
        c=run(name+".UBSAN.COMPILE",["clang",*flags,cpath,"-o",exe])
        if c.returncode != 0: stop("UBSan compile failed "+name)
        result=run(name+".UBSAN.NATIVE",[exe])
        if result.returncode != 0: stop("UBSan native failed "+name)
    # Read-only observation of the ORIGINAL unaltered C bytes; not an instrumented guest.
    # Use one final return stop before the C main exits so the original stack root remains live.
    lines=cpath.read_text().splitlines()
    positions=[i+1 for i,line in enumerate(lines) if re.search(r"\breturn\s+0\s*;",line)]
    if not positions: stop("OBSERVER INCONCLUSIVE: no final return 0 line in emitted C")
    if not shutil.which("gdb"): stop("OBSERVER INCONCLUSIVE: gdb missing")
    script=OUT/(name+".gdb")
    script.write_text("set pagination off\nset confirm off\nset print pretty on\n"
        + f"break {cpath}:{positions[-1]}\nrun\ninfo locals\n"
        + "python\nimport gdb\nf=gdb.selected_frame()\n"
        + "for s in f.block():\n"
        + "    if not s.is_variable: continue\n"
        + "    try:\n"
        + "        v=s.value(f)\n"
        + "        print('R516_GUEST_LOCAL',s.name,'type',v.type,'value',v,'address',v.address)\n"
        + "    except Exception as e: print('R516_GUEST_LOCAL_UNAVAILABLE',s.name,str(e))\n"
        + "end\nquit\n")
    rr=run(name+".GDB_PASSIVE",["gdb","-q","-batch","-nx","-x",script,
                                 OUT/(name+"-gcc-O0")])
    (OUT/(name+".gdb.txt")).write_text(rr.stdout+"\n"+rr.stderr)
    if rr.returncode != 0 or "Breakpoint 1," not in rr.stdout or "R516_GUEST_LOCAL" not in rr.stdout:
        stop("OBSERVER INCONCLUSIVE: debugger cannot show guest main at final return")
    if digest(cpath) != before:
        stop("FINDING: emitted C altered during passive observation")
    print("R516_NATIVE_OBSERVATION_REQUIRES_MANUAL_ORIGINAL_ROOT_VALIDATION", name, flush=True)
print("R516 STAGE COMPLETE; NOT AUTOMATIC SAFE: independently inspect original root/field address, former and sibling", flush=True)
