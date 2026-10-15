#!/usr/bin/env python3
"""One-shot independently authored R459 real CLI / public-owned / C17/native gate."""
import hashlib, os, pathlib, re, subprocess, sys
ROOT=pathlib.Path(__file__).resolve().parents[2]
SELF=pathlib.Path(__file__).resolve().parent
OUT=ROOT/"r459-output"
OUT.mkdir(exist_ok=True)
COMPILER=ROOT/"build-r459"/"newlangc"
SOURCE=SELF/"source"
def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()
def run(args, *, tag, expect=None, env=None, timeout=120):
    r=subprocess.run([str(a) for a in args],cwd=ROOT,capture_output=True,text=True,env=env,timeout=timeout)
    print(f"R459 {tag}: exit={r.returncode} args={[str(a) for a in args]}",flush=True)
    if r.stdout: print(f"R459 {tag} stdout:\n{r.stdout[:10000]}",flush=True)
    if r.stderr: print(f"R459 {tag} stderr:\n{r.stderr[:10000]}",flush=True)
    if expect is not None and r.returncode != expect:
        raise RuntimeError(f"R459 STOP {tag}: expected exit {expect}, observed {r.returncode}")
    return r
def classify_negative(name):
    p=SOURCE/name
    r=run([COMPILER,"--check",p],tag="negative-"+name)
    # 3 semantic/parser refusal; 0 is a safety contradiction, 1/2 environmental/CLI.
    if r.returncode==0: raise RuntimeError("R459 FINDING: unsafe or unsupported spelling ACCEPTED: "+name)
    if r.returncode!=3: raise RuntimeError("R459 INCONCLUSIVE: negative CLI/environment barrier "+name)
    if not re.search(r"(SCOPE|ESCAP|DEPEND|LIVENESS|STALE|ROOT|REF|LOAN|SEMANTIC|UNSUPPORTED|SYNTAX)",r.stderr,re.I):
        raise RuntimeError("R459 INCONCLUSIVE: rejected with unclassified diagnostic "+name)
    print("R459 NEGATIVE_REJECTED "+name+" class=source/checker-or-syntax (exact diagnostic above)",flush=True)
def emitted(name):
    p=SOURCE/name
    checked=run([COMPILER,"--check",p],tag="positive-check-"+name)
    if checked.returncode!=0:
        raise RuntimeError("R459 FIRST BARRIER: SOURCE/CHECK refused "+name)
    if checked.stdout or checked.stderr:
        raise RuntimeError("R459 STOP: successful --check unexpectedly wrote output")
    if name=="initial_some.nl":
        run([OUT/"r459-public-probe",p],tag="public-owned",expect=0)
    c=OUT/(name+".c")
    r=subprocess.run([str(COMPILER),str(p)],cwd=ROOT,capture_output=True,timeout=120)
    c.write_bytes(r.stdout)
    (OUT/(name+".emit.err")).write_bytes(r.stderr)
    print(f"R459 C17_EMISSION {name}: rc={r.returncode} length={len(r.stdout)} sha256={sha(c)} stderr={r.stderr[:3000]!r}",flush=True)
    if r.returncode!=0 or not r.stdout:
        raise RuntimeError("R459 FIRST BARRIER: BACKEND-UNSUPPORTED or emit failure "+name)
    return c
def native(c,control):
    for cc in ("gcc","clang"):
        for opt in ("0","2"):
            exe=OUT/(c.stem+"-"+cc+"-O"+opt)
            run([cc,"-std=c17","-Wall","-Wextra","-Wpedantic","-Werror","-g",
                 "-O"+opt,str(c),"-o",exe],tag="compile-"+exe.name,expect=0)
            run([exe],tag="native-"+exe.name,expect=0)
            if cc=="gcc" and opt=="0":
                env=os.environ.copy()
                if control: env["R459_CONTROL"]="1"
                g=run(["gdb","--batch","--quiet","-x",SELF/"observe.gdb","--args",exe],
                      tag="physical-"+exe.name,env=env,timeout=90)
                if g.returncode!=0 or "R459 GDB_READ_ONLY" not in g.stdout or '"status": "PASS"' not in g.stdout:
                    raise RuntimeError("R459 STOP native physical observer absent/mismatched "+exe.name)
    for cc in ("gcc","clang"):
        exe=OUT/(c.stem+"-"+cc+"-san")
        run([cc,"-std=c17","-Wall","-Wextra","-Wpedantic","-Werror","-O1","-g",
            "-fsanitize=address,undefined","-fno-omit-frame-pointer",str(c),"-o",exe],
            tag="compile-sanitized-"+cc,expect=0)
        env=os.environ.copy();env["ASAN_OPTIONS"]="detect_leaks=1:halt_on_error=1"
        env["UBSAN_OPTIONS"]="halt_on_error=1"
        run([exe],tag="native-sanitized-"+cc,expect=0,env=env)
def main():
    for p in sorted(SOURCE.glob("*.nl")): print(f"R459 FREEZE SOURCE SHA256 {p.name} {sha(p)}",flush=True)
    print("R459 AUTHORITY candidate b69b6f63c6eb8681ebd468e7988f961b0378f23f; baseline 4506112b1dea4a5a63b4ac41ff9e8f8f8388146d",flush=True)
    classify_negative("ref_escape.nl")
    classify_negative("stale_root.nl")
    for name,control in [("none_control.nl",True),("initial_some.nl",False)]:
        native(emitted(name),control)
    print("R459 PASS TESTED FINITE BOUNDARY: independent CHECK/owned/C17/native/gdb O0/O2 gcc+clang+sanitizers",flush=True)
if __name__=="__main__":
    try: main()
    except Exception as e:
        print(f"R459 TERMINAL RUN FAILURE: {e}",file=sys.stderr,flush=True)
        sys.exit(1)
