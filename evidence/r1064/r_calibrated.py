#!/usr/bin/env python3
"""R1064 C153 bounded CALIBRATED runner, NOT FIRST.
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
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
R0 = ROOT / "evidence/r1022"
CANDIDATE = ROOT / "candidate"
BASE = "f56e5ab3d4bd61ef44e68c9631a7fa78cb21252d"
TARGET = "b49a5e24d9fa56d367e116ce037a067c7e1cca38"
TREE = "5dd16c6582b9157bc7de5ffecb8818cab0b20955"
REF = "refs/heads/r-calibrated/r1016-pr1063"
WORKFLOW = ".github/workflows/r-calibrated-r1016-pr1063.yml"
ZIP = "evidence/r1028/original-first-40849018312-attempt1.zip"
ZIP_HASH = "0d074152c822d1b45a71ccee37a91fcfb1fea2eae2ce8fff8c27a35f3c3daac9"
# C153: machine-check real GITHUB_SHA against HEAD and fixed R parent;
# a commit cannot contain its own fixed SHA.

R1052_ARCHIVE_COMMIT = "7f4691a0b4fd4ea0ea09f16e835930f0899b373a"
R1052_ZIP_PATH = "evidence/r1052/calibrated-pr1048-40984457279-attempt1.zip"
R1052_ZIP_BLOB = "eb680f99bfd30fd4e27a3e5f8c562e811a59a70e"
R1052_ZIP_SHA256 = "b79662261d7bbb23626cf57e9852f13b2f69e5dd192893214feb36bd878a075a"
NEW_SOURCE="evidence/r1064/sources/INTEGRATED-local-return.nl"
NEW_ORACLE="evidence/r1064/oracle/INTEGRATED-local-return-EXPECTATIONS.json"
NEW_SOURCE_BLOB="c3a2a755577cdff6d32dfbae9e9bef47ac9d9df3"
NEW_ORACLE_BLOB="11793e38f9832fb09de7b45e30869803df260967"
ALLOWED = (WORKFLOW, "evidence/r1064/r_calibrated.py",
           "evidence/r1064/README.md", "evidence/r1064/manifest.json",
           NEW_SOURCE, NEW_ORACLE)
V2_ZIP = "evidence/r1035/calibrated-v2-40874742688-attempt1.zip"
V2_SHA256 = "6baa3c9ef24b472d58c936abe9e414c81d8c11978f8f8ff48ec7f3bbbe6c0aa9"
V2_BLOB = "b85e5e794b03d1d623e71d3a5fa6d36cccf18060"
FROZEN_BLOBS = {
    "evidence/r1022/sources/POS-A-register-before-H.nl": "d9a40ea83d7b51b19bd20cd6e562a6d6a8697da4",
    "evidence/r1022/sources/POS-B-H-before-register.nl": "6d1a03a1d2c1f89ae19c413bc3cba553d6aeb97b",
    "evidence/r1022/sources/NEG-1-missing-Some-link.nl": "a89fff7304df7405b113b9d4625b098de36c5d87",
    "evidence/r1022/sources/NEG-2-wrong-tail-original-pointer.nl": "055853059ad8253973c4c4271842a49a10da0fff",
    "evidence/r1022/sources/NEG-3-double-nonCopy-terminal.nl": "db72bc023c9cf03cec6f066117007fed3f8064e8",
    "evidence/r1022/sources/NEG-4-field-loan-escape.nl": "d3a0de975edf014d34c7c2ed145cdcedf08ba354",
    "evidence/r1022/oracle/R1016-FIRST-ORIGINAL-v1.txt": "ddf7dc87bca92cdbf16407814f15e9092bce9e27",
}
OLD_R_BLOBS = {
    ".github/workflows/r-original-first.yml": "81c26afe4906d2513bfb733cf1896c67ef38ae2a",
    ".github/workflows/r-calibrated-r1016-pr1031.yml": "ad11be92715007fc1fb5ead162849cbc64a29462",
    "evidence/r1022/r_runner.py": "c1704f5da79ef9b71afa5379bc1194ff444dae44",
    "evidence/r1032/r_calibrated.py": "b37441250074bce32f36d6a46532506ac4a26323",
    "evidence/r1035/CALIBRATED-v2-ARCHIVE.md": "7f1baee8c14118788737b7bef2b46f8fb924612b",
    ".github/workflows/r-calibrated-r1016-pr1048.yml": "e7b1b49f191a540bb4863b3fbf6cd177c5b5146c",
    "evidence/r1049/r_calibrated.py": "1ecb8cb73f29eb308f6b341919044314d402546e",
    "evidence/r1049/manifest.json": "2bb4502ffc9fc50215bcf3522f6fd7214b6dae22",
    "evidence/r1049/README.md": "fc9a40de0e519a84ea75f0c0cbd0e7ff1cbf2e2f",
    ".github/workflows/r-calibrated-r1016-pr1056.yml": "33255ecf5a5941a5cc92b05a526edbf089010f03",
    ".github/workflows/r-calibrated-r1016-pr1059.yml": "fcc021aeaf17356fea9ad1c76109f4aa09eecf41",
    "evidence/r1060/r_calibrated.py": "8d601093d41ee238ba8aab6980ae58d305edb69d",
    "evidence/r1060/manifest.json": "307e1b6c2cb387770b73ab93b67113680f104958",
    "evidence/r1060/README.md": "d3fe9adc3075ebc045160d63aa6fc1d1d876cdea",
    "evidence/r1057/r_calibrated.py": "cc49080e88768e7163f37e7c70ff871b45c62d47",
    "evidence/r1057/manifest.json": "0fe1308bb0132905066ff93d2858627255118359",
    "evidence/r1057/README.md": "d21625e098844789e557f8d6456065ec6a72888f",
}
spec = importlib.util.spec_from_file_location("r1022_readonly_helpers", R0/"r_runner.py")
rr = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rr)   # no original __main__ / original FIRST call
rr.ROOT = CANDIDATE           # only future exact PR1063 source/build tree
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
        raise RuntimeError("R executor must be the one C145 staging child")
    diff=git("diff","--name-status",BASE,"HEAD","--").splitlines()
    expected={"A\t"+p for p in ALLOWED}
    if len(diff)!=len(ALLOWED) or set(diff)!=expected:
        raise RuntimeError("unreviewed R source diff: "+repr(diff))
    if (git("rev-parse","HEAD:"+ZIP)!="7467df2023c84fe86409f815136df779de728bae"
        or git("rev-parse","HEAD:.github/workflows/r-original-first.yml")!="81c26afe4906d2513bfb733cf1896c67ef38ae2a"
        or git("rev-parse","HEAD:evidence/r1022/r_runner.py")!="c1704f5da79ef9b71afa5379bc1194ff444dae44"):
        raise RuntimeError("FIRST archive/runner/workflow Git blob changed")
    archive=rr.regular(ROOT/ZIP,30000)
    if len(archive)!=27685 or rr.sha(archive)!=ZIP_HASH:
        raise RuntimeError("original FIRST ZIP bytes/hash changed")
    if git("rev-parse","HEAD:"+V2_ZIP)!=V2_BLOB:
        raise RuntimeError("calibrated v2 ZIP Git blob changed")
    prior=rr.regular(ROOT/V2_ZIP,40000)
    if len(prior)!=33200 or rr.sha(prior)!=V2_SHA256:
        raise RuntimeError("calibrated v2 ZIP bytes/hash changed")
    for path, blob in FROZEN_BLOBS.items():
        if git("rev-parse","HEAD:"+path)!=blob:
            raise RuntimeError("frozen original Git blob changed: "+path)
    for path,blob in OLD_R_BLOBS.items():
        if git("rev-parse","HEAD:"+path)!=blob:
            raise RuntimeError("prior R runner/workflow/archive changed: "+path)
    for path, blob in ((NEW_SOURCE,NEW_SOURCE_BLOB),(NEW_ORACLE,NEW_ORACLE_BLOB)):
        if git("rev-parse","HEAD:"+path)!=blob or git("hash-object",path)!=blob:
            raise RuntimeError("R1064 independent freeze blob mismatch: "+path)
        j.write({"pre_producer_frozen":path,"blob":blob,
                 "sha256":rr.sha(rr.regular(ROOT/path,131072))})
    j.write({"r1060_original_run":"41026541892/attempt1",
             "prior_pos_b":"TRUE_C17_RED_IMMUTABLE",
             "original_raw_sha256":"6e68c98864bd4711cff09bf8dfdbab69485bb43a77b07ca266d43f70bd121206"})
    # R1052 is on an independent nontriggering archive branch, not copied or
    # cherry-picked into this R executor. Required fetched Git history only.
    if git("rev-parse",R1052_ARCHIVE_COMMIT+":"+R1052_ZIP_PATH)!=R1052_ZIP_BLOB:
        raise RuntimeError("R1052 permanent raw ZIP Git blob mismatch/unavailable")
    result=subprocess.run(["/usr/bin/git","-C",str(ROOT),"cat-file","blob",R1052_ZIP_BLOB],
                          check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    r1052=result.stdout
    if len(r1052)!=48639 or rr.sha(r1052)!=R1052_ZIP_SHA256:
        raise RuntimeError("R1052 raw ZIP byte integrity mismatch")
    j.write({"r1052_git_archive":R1052_ARCHIVE_COMMIT,
             "r1052_zip_blob":R1052_ZIP_BLOB,
             "r1052_zip_bytes":len(r1052),"r1052_zip_sha256":R1052_ZIP_SHA256,
             "r1051_original_result":"TWO_POSITIVES_C17_RED_NO_NATIVE_UNCHANGED"})
    j.write({"r1057_original_run":"41015924765/attempt1",
             "r1057_original_artifact_id":12959691321,
             "r1057_zip_sha256":"aae11fae2100b39fd8c69a46e6ef07fe378bfde77e40b1b40360c5a462a5a2c0",
             "r1057_pos_b":"TRUE_C17_RED_NO_NATIVE_UNCHANGED"})
    j.write({"custody":"FIRST_AND_CALIBRATED_V2_BYTES_VERIFIED",
             "first_run":"40849018312/attempt1",
             "v1_run":"40869364041/attempt1_RAW_LOST",
             "v2_run":"40874742688/attempt1",
             "v2_zip_sha256":V2_SHA256,
             "v2_zip_blob":V2_BLOB})
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
        raise RuntimeError("candidate is not physical exact PR1063 source/tree")
    j.write({"gate":"PINNED_R_SOURCE_AND_PR1063","executor":pushed,"parent":BASE,
             "target_HEAD":TARGET,"target_tree":TREE,"original_first_zip_sha256":ZIP_HASH,
             "authorization":"ISSUE_1060_C153_BOUNDED","adoption":"NOT_AUTHORIZED"})

def stage(argv,folder,name,j,env=None):
    return rr.stage(argv,folder,name,j,env=env)

def run_observations(folder,work,j,clang,clang_env):
    compiler=CANDIDATE/"build-frontend/newlangc"
    if not compiler.is_file() or compiler.is_symlink():
        raise RuntimeError("PR1063 built compiler unavailable")
    j.write({"candidate_compiler_sha256":rr.sha(rr.regular(compiler,32*1024*1024)),
             "origin":"GCC13 built from PR1063 exact source; observed binary SHA only"})
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
    new_source=ROOT/NEW_SOURCE
    new_hash=rr.sha(rr.regular(new_source,131072))
    j.write({"new_source":"INTEGRATED-local-return",
             "freeze_blob":NEW_SOURCE_BLOB,"oracle_blob":NEW_ORACLE_BLOB,
             "source_sha256":new_hash,"prediction":"PREPRODUCER_NOT_OBSERVED"})
    cases=list(rr.CASES)+[("INTEGRATED-local-return",new_hash)]
    for index,(name,frozen) in enumerate(cases):
        src=new_source if index==6 else R0/"sources"/(name+".nl")
        j.write({"case":name,"phase":"CALIBRATED_BEGIN",
                 "source_sha256":frozen,"positive":index<2 or index==6})
        check=stage([str(compiler),"--check",str(src)],folder,name+"-check",j)
        if (index<2 or index==6) and check["exit"]==0 and built["exit"]==0:
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
        if 2<=index<6:
            with out.open("xb") as f:
                f.write(b"R1064-NEGATIVE-CALIBRATED-NAMED-SENTINEL\n")
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
        raise SystemExit("R1064 C153: dedicated Actions-only calibrated execution required")
    e=os.environ
    temp=Path(e.get("RUNNER_TEMP","/nonexistent"))
    run=e.get("GITHUB_RUN_ID","")
    attempt=e.get("GITHUB_RUN_ATTEMPT","")
    pushed=e.get("GITHUB_SHA","")
    if (not temp.is_dir() or temp.is_symlink() or not run.isdecimal()
        or attempt!="1" or not re.fullmatch("[0-9a-f]{40}",pushed)):
        raise RuntimeError("fresh GitHub Actions identity/temp required")
    folder=temp/("R1064-CALIBRATED-"+run+"-"+attempt+"-"+pushed)
    # C153 workflow creates this unique R1064 raw evidence directory before setup.
    # Do not reuse or overwrite any pre-existing journal or earlier R run.
    if (not folder.is_dir() or folder.is_symlink()
        or stat.S_IMODE(folder.stat().st_mode)!=0o700):
        raise RuntimeError("future workflow preflight raw directory missing/unsafe")
    for name in ("preflight.stdout","preflight.stderr","bootstrap.stdout",
                 "bootstrap.stderr","cmake-configure.stdout","cmake-configure.stderr",
                 "cmake-build.stdout","cmake-build.stderr"):
        path=folder/name
        if not path.is_file() or path.is_symlink():
            raise RuntimeError("future setup raw stage missing: "+name)
    j=Journal(folder)
    try:
        j.write({"phase":"CALIBRATED_OPEN","run":run,"attempt":attempt,
                 "push_SHA":pushed,"ref":e.get("GITHUB_REF"),"FIRST":"NO_REPLAY"})
        gate(j)
        j.write({"original_frozen":"7_OF_7_BYTE_HASH_AND_GIT_BLOB_MATCH"})
        for path in sorted(folder.glob("*")):
            if path.is_file() and path.name!="CALIBRATED-RAW.jsonl":
                st=path.lstat()
                if not stat.S_ISREG(st.st_mode) or st.st_nlink!=1:
                    raise RuntimeError("unsafe setup evidence file: "+path.name)
                with path.open("rb") as stream:
                    digest=hashlib.file_digest(stream,"sha256").hexdigest()
                j.write({"setup_raw":path.name,"bytes":st.st_size,"sha256":digest})
        j.write({"setup_capture":"RAW_ONLY_NOT_VERDICT",
                 "Actions_checkout_logs":"Actions-native; not a separate R raw stream"})
        clang,clang_env=rr.curated_clang(j)
        with tempfile.TemporaryDirectory(prefix="R1064-native-",dir=temp) as scratch:
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
