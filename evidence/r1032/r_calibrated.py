#!/usr/bin/env python3
"""R1032 C132 CALIBRATED runner, NOT FIRST. No execution authorized by C132.
Reuses only R-owned safe capture/curated SDK/public probe helpers. Never invokes
the original FIRST main, gate, observer-run, or any P/T fixture/runner.
"""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import stat
import tempfile

ROOT = Path(__file__).resolve().parents[2]
R0 = ROOT / "evidence/r1022"
CANDIDATE = ROOT / "candidate"
BASE = "9f917c58dfb38c9ff057b82d3a12a9e068b3222f"
TARGET = "a56cd08c23548852504cf95e94eb6c9a56f3dd8f"
TREE = "a2cd1c9a9c66a8686d7e4b99f545b82865ef3877"
REF = "refs/heads/r-calibrated/r1016-pr1031"
WORKFLOW = ".github/workflows/r-calibrated-r1016-pr1031.yml"
ZIP = "evidence/r1028/original-first-40849018312-attempt1.zip"
ZIP_HASH = "0d074152c822d1b45a71ccee37a91fcfb1fea2eae2ce8fff8c27a35f3c3daac9"
ALLOWED = (WORKFLOW, "evidence/r1032/r_calibrated.py",
           "evidence/r1032/README.md", "evidence/r1032/manifest.json")
spec = importlib.util.spec_from_file_location("r1022_readonly_helpers", R0/"r_runner.py")
rr = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rr)   # no original __main__ / original FIRST call
rr.ROOT = CANDIDATE           # only future exact PR1031 source/build tree
# rr.HERE, rr.TARGET, rr.TARGET_TREE remain ORIGINAL, for R1016 7-file inventory.

def git(*args, candidate=False):
    return rr.plain(["/usr/bin/git"] +
                    (["-C", str(CANDIDATE)] if candidate else ["-C",str(ROOT)]) +
                    list(args))

class Journal:
    def __init__(self,folder):
        self.fd=os.open(folder/"CALIBRATED-RAW.jsonl",
            os.O_WRONLY|os.O_CREAT|os.O_EXCL|getattr(os,"O_NOFOLLOW",0),0o600)
    def write(self,x):
        b=(json.dumps(x,sort_keys=True,ensure_ascii=True)+"\n").encode()
        if len(b)>16384 or os.write(self.fd,b)!=len(b):
            raise RuntimeError("CALIBRATED raw journal partial/oversized write")
        os.fsync(self.fd)
    def close(self):
        os.fsync(self.fd)
        os.close(self.fd)

def gate(j):
    e=os.environ
    if (e.get("GITHUB_ACTIONS")!="true" or e.get("GITHUB_EVENT_NAME")!="push"
        or e.get("GITHUB_REPOSITORY")!="wakairo/NewLang_Compiler"
        or e.get("GITHUB_REF")!=REF or e.get("GITHUB_RUN_ATTEMPT")!="1"
        or e.get("GITHUB_WORKFLOW_REF")!="wakairo/NewLang_Compiler/"+WORKFLOW+"@"+REF):
        raise RuntimeError("foreign event, ref, attempt, or workflow")
    pushed=e.get("GITHUB_SHA","")
    if not re.fullmatch("[0-9a-f]{40}",pushed):
        raise RuntimeError("invalid pushed SHA")
    if git("rev-parse","HEAD")!=pushed or git("status","--porcelain","--untracked-files=no"):
        raise RuntimeError("non-exact or dirty R executor")
    if git("rev-parse","HEAD^")!=BASE or len(git("rev-list","--parents","-n1","HEAD").split())!=2:
        raise RuntimeError("R executor must be the one C132 staging child")
    diff=git("diff","--name-status",BASE,"HEAD","--").splitlines()
    if set(diff)!={"A\t"+p for p in ALLOWED}:
        raise RuntimeError("unreviewed R source diff: "+repr(diff))
    if (git("rev-parse","HEAD:"+ZIP)!="7467df2023c84fe86409f815136df779de728bae"
        or git("rev-parse","HEAD:.github/workflows/r-original-first.yml")!="81c26afe4906d2513bfb733cf1896c67ef38ae2a"
        or git("rev-parse","HEAD:evidence/r1022/r_runner.py")!="c1704f5da79ef9b71afa5379bc1194ff444dae44"):
        raise RuntimeError("FIRST archive/runner/workflow Git blob changed")
    archive=rr.regular(ROOT/ZIP,30000)
    if len(archive)!=27685 or rr.sha(archive)!=ZIP_HASH:
        raise RuntimeError("original FIRST ZIP bytes/hash changed")
    if git("rev-parse","0d933749005f0a2d9ed5777627e91732bb568547^")!=rr.TARGET:
        raise RuntimeError("R original publication ancestry changed")
    rr.inventory() # source 6 plus oracle original absolute bytes/SHA256
    for name,_ in rr.CASES:
        p="evidence/r1022/sources/"+name+".nl"
        if git("rev-parse","HEAD:"+p)!=git("rev-parse",rr.ORIGINAL+":"+p):
            raise RuntimeError("original source Git blob drift: "+name)
    p="evidence/r1022/oracle/R1016-FIRST-ORIGINAL-v1.txt"
    if git("rev-parse","HEAD:"+p)!=git("rev-parse",rr.ORIGINAL+":"+p):
        raise RuntimeError("original oracle Git blob drift")
    for p in ALLOWED:
        if git("rev-parse","HEAD:"+p)!=git("hash-object",p):
            raise RuntimeError("new source differs from pushed Git blob: "+p)
        j.write({"source":p,"blob":git("rev-parse","HEAD:"+p),
                 "sha256":rr.sha(rr.regular(ROOT/p,131072))})
    if (git("rev-parse","HEAD",candidate=True)!=TARGET or
        git("rev-parse","HEAD^{tree}",candidate=True)!=TREE or
        git("status","--porcelain","--untracked-files=no",candidate=True)):
        raise RuntimeError("candidate is not physical exact PR1031 source/tree")
    j.write({"gate":"PINNED_R_SOURCE_AND_PR1031","executor":pushed,"parent":BASE,
             "target_HEAD":TARGET,"target_tree":TREE,"original_first_zip_sha256":ZIP_HASH,
             "approval":"EXTERNAL_C_EXACT_COMMIT_REVIEW_NOT_MACHINE_ATTESTED"})

def stage(argv,folder,name,j,env=None):
    return rr.stage(argv,folder,name,j,env=env)

def run_observations(folder,work,j,clang,clang_env):
    compiler=CANDIDATE/"build-frontend/newlangc"
    if not compiler.is_file() or compiler.is_symlink():
        raise RuntimeError("PR1031 built compiler unavailable")
    j.write({"candidate_compiler_sha256":rr.sha(rr.regular(compiler,32*1024*1024)),
             "origin":"GCC13 built from PR1031 exact source; observed binary SHA only"})
    libs=("newlang_semantic","newlang_checked","newlang_parser","newlang_syntax",
          "newlang_lexer","newlang_source","newlang_diagnostic")
    probe=work/"public-current-probe"
    cmd=["/usr/bin/gcc-13","-std=c17","-Wall","-Wextra","-Werror","-pedantic-errors",
         "-I",str(CANDIDATE/"include"),str(R0/"r_probe.c"),"-o",str(probe),
         "-Wl,--start-group"]+[str(CANDIDATE/"build-frontend"/("lib"+v+".a"))
         for v in libs]+["-Wl,--end-group"]
    built=stage(cmd,folder,"public-probe-build",j)
    j.write({"public_probe":"NOT_OBSERVABLE" if built["exit"]!=0 else
             "BUILT_NOT_YET_PUBLIC_VALIDATED"})
    for index,(name,frozen) in enumerate(rr.CASES):
        src=R0/"sources"/(name+".nl")
        j.write({"case":name,"phase":"CALIBRATED_BEGIN",
                 "source_sha256":frozen,"positive":index<2})
        check=stage([str(compiler),"--check",str(src)],folder,name+"-check",j)
        if index<2 and check["exit"]==0 and built["exit"]==0:
            result=stage([str(probe),str(src)],folder,name+"-public-current",j)
            stdout=rr.regular(folder/(name+"-public-current.stdout"),rr.CHILD_FSIZE,folder)
            current=len(re.findall(rb"(?m)^R1023 CURRENT id=\d+ validate=0 parameters=0 events=[1-9]\d*",stdout))
            events=len(re.findall(rb"(?m)^R1023 EVENT id=\d+ event=\d+ GET=1 validate=0",stdout))
            j.write({"case":name,"public_current_probe_exit":result["exit"],
                "actual_source_owned_current_rows":current,"valid_event_rows":events,
                "public_current":"PARTIAL_API_OBSERVED" if result["exit"]==0 and current>=2
                  and events>=2 else "NOT_OBSERVABLE",
                "physical_equivalence":"NOT_IMPLIED_BY_CHECK_OR_PUBLIC_GET"})
        else:
            j.write({"case":name,"public_current":"NOT_OBSERVABLE",
                     "cause":"negative refusal, source CHECK failure, or probe build failure"})
        out=folder/(name+"-named.c")
        if index>=2:
            with out.open("xb") as f:
                f.write(b"R1032-NEGATIVE-CALIBRATED-NAMED-SENTINEL\n")
                f.flush()
                os.fsync(f.fileno())
            before=rr.snapshot(out,folder,name+"-before",j)
            emit=stage([str(compiler),"--emit-c17-to",str(out),str(src)],
                        folder,name+"-negative-emit",j)
            after=rr.snapshot(out,folder,name+"-after",j)
            same=bool(before.get("exists") and after.get("exists") and
                 all(before[k]==after[k] for k in ("bytes","sha256","dev","ino")))
            first=("CHECK" if check["exit"]!=0 else "EMITTER" if emit["exit"]!=0
                   else "UNEXPECTED_ACCEPT")
            j.write({"case":name,"negative_earliest_refusal_layer":first,
                "check_exit":check["exit"],"emitter_exit":emit["exit"],
                "named_output_no_clobber":same,"intended_guard":"REVIEW_RAW_DIAGNOSTIC",
                "temp_file_nonappearance":"NOT_PROVEN",
                "verdict":"RED" if emit["exit"]==0 or not same else
                          "REFUSAL_OBSERVED_NOT_PRECISE_GUARD"})
        elif check["exit"]!=0:
            j.write({"case":name,"C17":"NOT_OBSERVABLE_CHECK_REJECTED",
                     "native":"NOT_OBSERVABLE"})
        else:
            emit=stage([str(compiler),"--emit-c17-to",str(out),str(src)],
                       folder,name+"-positive-emit",j)
            if emit["exit"]!=0 or not out.is_file():
                j.write({"case":name,"C17":"CALIBRATED_RED_OR_UNSUPPORTED",
                         "native":"NOT_OBSERVABLE","original_H_T":"NOT_OBSERVABLE"})
            else:
                generated=rr.regular(out,rr.C_CAP,folder)
                if not generated:
                    raise RuntimeError("empty C17 output")
                j.write({"case":name,"C17":"GENERATED_NOT_PHYSICAL_PASS",
                         "C17_sha256":rr.sha(generated),"bytes":len(generated)})
                for cc,env in (("/usr/bin/gcc-13",None),(clang,clang_env)):
                    for opt in ("0","2"):
                        key=name+"-"+Path(cc).name+"-O"+opt
                        flags=[cc,"-std=c17","-Wall","-Wextra","-Werror",
                               "-pedantic-errors","-O"+opt]
                        exe=work/key
                        b=stage(flags+[str(out),"-o",str(exe)],
                                folder,key+"-clean-build",j,env=env)
                        if b["exit"]!=0:
                            j.write({"case":name,"variant":key,
                                     "native":"NOT_OBSERVABLE_C_BUILD_RED"})
                            continue
                        native=stage([str(exe)],folder,key+"-clean",j)
                        j.write({"case":name,"variant":key,"native_exit":native["exit"],
                                 "clean_result":"EXIT_ONLY_NOT_PHYSICAL_PROOF"})
                        observed=work/(key+"-obs")
                        b=stage(flags+["-I",str(R0),
                            '-DNEWLANG_HEAP_OBSERVER="r_observer.h"',
                            str(out),"-o",str(observed)],
                            folder,key+"-observer-build",j,env=env)
                        if b["exit"]!=0:
                            j.write({"case":name,"variant":key,
                                     "original_H_T":"NOT_OBSERVABLE_OBSERVER_BUILD_RED"})
                            continue
                        for budget in (0,1,2):
                            world=key+"-guest"+str(budget)
                            raw=stage([str(observed)],folder,world,j,env={
                                "PATH":"/usr/bin:/bin","LANG":"C",
                                "R1022_GUEST_SUCCESS_BUDGET":str(budget)})
                            err=rr.regular(folder/(world+".stderr"),rr.CHILD_FSIZE,folder)
                            allocations=len(re.findall(rb"(?m)^R1022_OBS ALLOC_RESULT 0x[0-9a-fA-F]+ ",err))
                            frees=len(re.findall(rb"(?m)^R1022_OBS FREE 0x[0-9a-fA-F]+ ",err))
                            copies=len(re.findall(rb"(?m)^R1022_OBS COPY_PHYSICAL_CURRENT_BYTE ",err))
                            j.write({"case":name,"variant":key,"guest_budget":budget,
                                "native_observer_exit":raw["exit"],"alloc_successes_seen":allocations,
                                "frees_seen":frees,"live_copy_byte_events":copies,
                                "physical_partial":"RAW_CALLBACKS_ONLY" if err else "NOT_OBSERVABLE",
                                "original_H_region_alloc_domain":"NOT_OBSERVABLE",
                                "original_T_old_new_sibling":"NOT_OBSERVABLE",
                                "guest_budget_not_host_OOM":True})
        j.write({"case":name,"host_OOM":"NOT_OBSERVABLE_NO_INJECTION",
                 "ASan_LSan":"NOT_OBSERVABLE_NOT_IMPLEMENTED",
                 "UBSan":"NOT_OBSERVABLE_NOT_IMPLEMENTED",
                 "phase":"CALIBRATED_END"})
    j.write({"phase":"CALIBRATED_CAPTURE_END",
             "verdict":"RAW_ONLY_NOT_A_PASS","FIRST":"ORIGINAL_UNCHANGED",
             "review":"Independent evaluator must read actual stderr/stdout/physical witnesses"})

def main():
    if len(os.sys.argv)!=2 or os.sys.argv[1]!="--execute":
        raise SystemExit("R1032: only future separately C-authorized Actions execution")
    e=os.environ
    temp=Path(e.get("RUNNER_TEMP","/nonexistent"))
    run=e.get("GITHUB_RUN_ID","")
    attempt=e.get("GITHUB_RUN_ATTEMPT","")
    pushed=e.get("GITHUB_SHA","")
    if (not temp.is_dir() or temp.is_symlink() or not run.isdecimal()
        or attempt!="1" or not re.fullmatch("[0-9a-f]{40}",pushed)):
        raise RuntimeError("fresh GitHub Actions identity/temp required")
    folder=temp/("R1032-CALIBRATED-"+run+"-"+attempt+"-"+pushed)
    folder.mkdir(mode=0o700) # exclusive, never overwrites calibrated or FIRST
    j=Journal(folder)
    try:
        j.write({"phase":"CALIBRATED_OPEN","run":run,"attempt":attempt,
                 "push_SHA":pushed,"ref":e.get("GITHUB_REF"),"FIRST":"NO_REPLAY"})
        gate(j)
        j.write({"original_frozen":"7_OF_7_BYTE_HASH_AND_GIT_BLOB_MATCH"})
        clang,clang_env=rr.curated_clang(j)
        with tempfile.TemporaryDirectory(prefix="R1032-native-",dir=temp) as scratch:
            run_observations(folder,Path(scratch),j,clang,clang_env)
    except BaseException as exc:
        j.write({"phase":"CALIBRATED_STOP","error":str(exc)[:1024],
                 "type":type(exc).__name__,"verdict":"RED_OR_INCONCLUSIVE",
                 "FIRST":"UNMODIFIED"})
        raise
    finally:
        j.close() # always() upload separately attempted, not guaranteed
if __name__=="__main__":
    main()
