#!/usr/bin/env python3
"""Track R #230: malicious transformations of ACTUAL frozen emitted C17.
Observer checks wrongness; mutations are never used as NewLang compiler findings.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/"r230"/"results"
BASE=OUT/"generated"/"A00_canonical_accept.c"
REFUSE=OUT/"generated"/"A01_refusal.c"
CC=os.getenv("CC_NATIVE","cc")
OBS=ROOT/"r230"/"observer.c"
HOOKS=ROOT/"r230"/"observer_hooks.h"
HERE=OUT/"destructive"
HERE.mkdir(exist_ok=True,parents=True)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(args, env=None):
    p=subprocess.run([str(x) for x in args],capture_output=True,text=True,
                     errors="replace",env=env,timeout=40)
    return dict(exit=p.returncode,stdout=p.stdout,stderr=p.stderr)


def mutate(source, old, new):
    n=source.count(old)
    if n!=1:
        raise RuntimeError(f"mutant anchor requires count 1; got {n} for {old!r}")
    return source.replace(old,new,1)


def compile_and_run(name,c_path,fail_at=0,sanitize=False):
    exe=HERE/(name+"_native"+("_san" if sanitize else ""))
    cmd=[CC,"-std=c17","-g","-O0","-fno-omit-frame-pointer",
         "-fno-pie","-no-pie",
         '-DNEWLANG_HEAP_OBSERVER="observer_hooks.h"',
         "-I"+str(HOOKS.parent),"-finstrument-functions",
         "-Wl,--wrap=malloc","-Wl,--wrap=calloc",
         "-Wl,--wrap=realloc","-Wl,--wrap=free",
         str(c_path),str(OBS),"-ldl","-o",str(exe)]
    if sanitize:
        cmd[1:1]=["-fsanitize=address,undefined",
                  "-fno-sanitize-recover=all"]
    compilation=run(cmd)
    report=dict(compiler_command=cmd,compilation=compilation,
                executable_sha256=sha(exe) if exe.exists() else None)
    if compilation["exit"]!=0:
        return report
    env=os.environ.copy()
    env["R230_FAIL_ALLOCATION_N"]=str(fail_at)
    env["ASAN_OPTIONS"]="detect_leaks=1:halt_on_error=1"
    execution=run([exe],env)
    log=HERE/(name+("_san" if sanitize else "")+".log")
    log.write_text("exit="+str(execution["exit"])+"\nstdout:\n"+
                   execution["stdout"]+"\nstderr:\n"+execution["stderr"])
    s=execution["stderr"]
    summary=re.findall(r"^R230 SUMMARY .*",s,re.M)
    report["native"]=dict(exit=execution["exit"],
                          observer_summary=summary[-1] if summary else None,
                          hook_failures=re.findall(r"^R230 HOOK_FAIL .*",s,re.M),
                          freed=re.findall(r"^R230 FREE ptr=.*",s,re.M),
                          leaked=re.findall(r"^R230 LEAK .*",s,re.M),
                          duplicates=re.findall(r"^R230 DOUBLE_FREE_SUPPRESSED .*",s,re.M),
                          invalid=re.findall(r"^R230 UNTRACKED_FREE_SUPPRESSED .*",s,re.M),
                          sanitizer_errors=re.findall(r"(?:ERROR: AddressSanitizer|runtime error:).*",s),
                          trace="\n".join(x for x in s.splitlines()
                                          if x.startswith("R230 ") and
                                          not x.startswith(("R230 ENTER ","R230 EXIT "))),
                          log=str(log.relative_to(ROOT)))
    return report


def main():
    original=BASE.read_text()
    refusal=REFUSE.read_text()
    cases={
        "M00_return_wrong_ptr": (original,"nl_live_tail nl_v_59 = {nl_v_56,nl_v_57,nl_v_58};",
             "nl_live_tail nl_v_59 = {NULL,nl_v_57,nl_v_58};",0,"identity"),
        "M01_return_wrong_allocation": (original,
             "nl_live_tail nl_v_59 = {nl_v_56,nl_v_57,nl_v_58};",
             "nl_live_tail nl_v_59 = {nl_v_56,(nl_allocation){NULL},nl_v_58};",
             0,"identity"),
        "M02_return_wrong_domain": (original,
             "nl_live_tail nl_v_59 = {nl_v_56,nl_v_57,nl_v_58};",
             "nl_live_tail nl_v_59 = {nl_v_56,nl_v_57,(nl_domain){999}};",
             0,"identity"),
        "M03_skip_head_link_clear": (original,
             "*nl_v_53 = nl_v_54;",
             "(void)nl_v_54;",0,"link state"),
        "M04_recipient_false_none": (original,
             "nl_custody nl_v_67 = {1,nl_v_68};",
             "nl_custody nl_v_67 = {0,nl_v_68};",0,"tag"),
        "M05_recipient_packet_ptr_changed": (original,
             "nl_live_tail nl_v_68 = nl_b_13_32;",
             "nl_live_tail nl_v_68 = nl_b_13_32; nl_v_68.ptr=NULL;",
             0,"identity"),
        "M06_recipient_packet_allocation_changed": (original,
             "nl_live_tail nl_v_68 = nl_b_13_32;",
             "nl_live_tail nl_v_68 = nl_b_13_32; nl_v_68.allocation.handle=NULL;",
             0,"identity"),
        "M07_extraction_wrong_ptr": (original,
             "nl_live_tail nl_b_14_36 = nl_v_75.packet;",
             "nl_live_tail nl_b_14_36 = nl_v_75.packet; nl_b_14_36.ptr=NULL;",
             0,"identity"),
        "M08_extraction_wrong_allocation": (original,
             "nl_live_tail nl_b_14_36 = nl_v_75.packet;",
             "nl_live_tail nl_b_14_36 = nl_v_75.packet;"
             " nl_b_14_36.allocation.handle=NULL;",0,"identity"),
        "M09_extraction_wrong_domain": (original,
             "nl_live_tail nl_b_14_36 = nl_v_75.packet;",
             "nl_live_tail nl_b_14_36 = nl_v_75.packet;"
             " nl_b_14_36.domain.token=999;",0,"identity"),
        "M10_skip_terminal_free": (original,
             "free(nl_v_89.handle);","(void)nl_v_89.handle;",0,"leak"),
        "M11_duplicate_free_safe_stop": (original,
             "free(nl_v_89.handle);",
             "free(nl_v_89.handle); free(nl_v_89.handle);",
             0,"duplicate"),
        "M12_wrong_extraction_branch": (original,
             "if(nl_v_75.tag == 1) {",
             "if(nl_v_75.tag == 0) {",0,"leak"),
        "M13_skip_second_failure_cleanup": (original,
             "free(nl_v_21.handle);","(void)nl_v_21.handle;",
             2,"leak"),
        "M14_refusal_skip_terminal_free": (refusal,
             "free(nl_v_114.handle);","(void)nl_v_114.handle;",
             0,"leak"),
        "M15_return_stale_displaced_some": (original,
             "nl_custody nl_v_69 = *nl_v_66;",
             "nl_custody nl_v_69 = nl_v_67;",
             0,"nonDiscardable None"),
        "M16_recipient_does_not_store": (original,
             "*nl_v_66 = nl_v_67;",
             "(void)nl_v_67;",0,"custody"),
    }
    out={"frozen_emitted_sha256":sha(BASE),
         "refusal_emitted_sha256":sha(REFUSE),
         "observer_header_sha256":sha(HOOKS),
         "baseline":{}, "mutations":{}}
    for name,body in [
        ("accept",original),("refuse",refusal)]:
        path=HERE/("BASE_"+name+".c")
        path.write_text(body)
        report=compile_and_run("BASE_"+name,path)
        out["baseline"][name]=report
        n=report.get("native",{})
        print("HOOK_BASELINE",name,"compile",report["compilation"]["exit"],
              "exit",n.get("exit"),"errors",n.get("hook_failures"),
              "summary",n.get("observer_summary"),flush=True)
    for name,(src,old,new,fail,expected) in cases.items():
        amended=mutate(src,old,new)
        c_path=HERE/(name+".c")
        c_path.write_text(amended)
        report=compile_and_run(name,c_path,fail)
        report.update(mutant_sha256=sha(c_path),failure_world=fail,
                      expected_observer_signal=expected,
                      original_sha256=sha(REFUSE if src is refusal else BASE))
        out["mutations"][name]=report
        n=report.get("native",{})
        observed=bool(n.get("hook_failures") or n.get("leaked") or
                      n.get("duplicates") or n.get("invalid") or
                      n.get("sanitizer_errors"))
        report["detected"]=observed
        print("MUTANT",name,"compiled",report["compilation"]["exit"],
              "exit",n.get("exit"),"detected",observed,
              "hook_fail",len(n.get("hook_failures",[])),
              "leaks",len(n.get("leaked",[])),
              "duplicates",len(n.get("duplicates",[])),
              "summary",n.get("observer_summary"),flush=True)
        (HERE/"result.json").write_text(json.dumps(out,indent=2))
    print("MUTANT_SUMMARY",json.dumps({
        "baseline_clean":[n for n,r in out["baseline"].items()
            if r.get("compilation",{}).get("exit")==0 and
               r.get("native",{}).get("exit")==0 and
               not r.get("native",{}).get("hook_failures")],
        "mutants_detected":[n for n,r in out["mutations"].items()
                            if r.get("detected")],
        "mutants_undetected":[n for n,r in out["mutations"].items()
                              if not r.get("detected")]
    }),flush=True)

if __name__=="__main__":
    main()
