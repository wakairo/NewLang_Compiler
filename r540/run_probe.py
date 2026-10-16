#!/usr/bin/env python3
"""Independent R540 first-pass: source and oracle hashes fixed before compiler use."""
import hashlib, json, os, pathlib, subprocess, sys, shutil

ROOT=pathlib.Path(__file__).resolve().parent
WORK=ROOT.parent.parent
OUT=WORK/"r540-results"
OUT.mkdir(exist_ok=True)
REPORT={"authority":{"main":"ebb8c4d034a6df2ec6fbfae7e47ebb37650accae",
"compiler":"7f639a3a89d55f44848e361e1458529fe5395345",
"canonical":"Draft17.32"},"steps":[],"verdict":"INCONCLUSIVE"}
def sha(path): return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()
def flush():
    (OUT/"r540_initial_blind.json").write_text(json.dumps(REPORT,indent=2)+"\n")
def stop(reason):
    REPORT["verdict"]="BLIND FINDING / STOP"
    REPORT["stop_reason"]=reason
    flush()
    print("R540 STOP:",reason,flush=True)
    sys.exit(4)
def run(tag,argv,stdout_file=None):
    p=subprocess.run([str(x) for x in argv],cwd=WORK,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=180,text=True,errors="replace")
    (OUT/(tag+".stdout")).write_text(p.stdout)
    (OUT/(tag+".stderr")).write_text(p.stderr)
    item={"name":tag,"cmd":list(map(str,argv)),"returncode":p.returncode,
          "stdout":p.stdout[:800],"stderr":p.stderr[:1200]}
    REPORT["steps"].append(item);flush()
    print("R540",tag,"exit",p.returncode,"stderr:",p.stderr[:300],flush=True)
    if stdout_file is not None and p.returncode==0:
        pathlib.Path(stdout_file).write_text(p.stdout)
    return p
def negative(tag,binary,filename):
    p=run(tag,[binary,"--check",ROOT/"source"/filename])
    if p.returncode==0:stop(tag+" unexpectedly accepted")
    if p.returncode!=3:stop(tag+" refused with non-semantic exit="+str(p.returncode))
    return p
try:
    f=json.loads((ROOT/"frozen_manifest.json").read_text())
    for rel,expected in f["entries"].items():
        got=sha(ROOT/rel)
        if got!=expected:stop("frozen source/oracle SHA mismatch "+rel+" got "+got+" wanted "+expected)
    REPORT["frozen_verified"]=True;flush()
    target=WORK/"compiler"/"r540-build"/"newlangc_p534"
    default=WORK/"compiler"/"r540-build"/"newlangc"
    if not target.exists():stop("fixed opt-in binary missing: "+str(target))
    if not default.exists():stop("default-off binary missing: "+str(default))
    REPORT["binary_sha256"]={"optin":sha(target),"default":sha(default)}
    flush()
    pos=ROOT/"source"/"positive_original.nl"
    p=run("default_off_check",[default,"--check",pos])
    if p.returncode==0:stop("default-off CLI unexpectedly admits Array4")
    p=run("positive_check",[target,"--check",pos])
    if p.returncode!=0:stop("positive source CHECK refused at fixed compiler")
    generated=OUT/"positive_original.c"
    p=run("positive_emit",[target,pos],generated)
    if p.returncode!=0 or not generated.exists() or not generated.stat().st_size:
        stop("positive source C17 emission unavailable")
    REPORT["generated_c_sha256"]=sha(generated);flush()
    for tag,fn in (("arity3","negative_arity3.nl"),("arity5","negative_arity5.nl"),
                   ("wrong_u8_element","negative_u8_element.nl"),
                   ("payloadless_some","negative_payloadless_some.nl"),
                   ("nonleaf_binding","negative_nonleaf_binding.nl")):
        negative(tag,target,fn)
    extra=run("two_roots_check",[target,"--check",ROOT/"source"/"positive_two_roots.nl"])
    REPORT["two_roots_exploratory"]=extra.returncode;flush()
    for cc in ("gcc","clang"):
        if shutil.which(cc) is None:
            REPORT["tool_missing"]=cc;flush()
            continue
        run(cc+"_version",[cc,"--version"])
        for opt in ("O0","O2"):
            binary=OUT/("positive_original_"+cc+"_"+opt)
            args=[cc,"-std=c17","-Wall","-Wextra","-Wpedantic","-Werror",
                  "-g3","-"+opt,generated,"-o",binary]
            p=run("compile_"+cc+"_"+opt,args)
            if p.returncode!=0:stop("strict C17 compiler refusal "+cc+" "+opt)
            p=run("native_"+cc+"_"+opt,[binary])
            if p.returncode!=0:stop("native executable failure "+cc+" "+opt)
    if shutil.which("gdb"):
        selected=OUT/"positive_original_gcc_O0"
        if selected.exists():
            run("gdb_passive",["gdb","--batch","--quiet","-ex","set pagination off",
                "-ex","break main","-ex","run","-ex","info locals",
                "-ex","next","-ex","info locals","-ex","next","-ex","info locals",
                "-ex","next","-ex","info locals","-ex","bt",selected])
    REPORT["verdict"]="LAYER RESULTS CAPTURED; owned/physical must be independently adjudicated"
    REPORT["limits"]="CHECK and C17/native success alone are not proof of public owned root/child issuer or passive physical identity; do not infer these."
    flush()
except subprocess.TimeoutExpired as e: stop("timeout "+str(e))
except Exception as e: stop("harness exception "+repr(e))
