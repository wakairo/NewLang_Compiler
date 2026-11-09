#!/usr/bin/env python3
"""R1240 independent C289 V1 ATTACK; run only in dedicated GitHub Actions.
Do not change frozen source, oracle, manifest, or predicted semantic values.
This is a bounded empirical test, not a proof of source-owned authority.
"""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys
import tarfile
import time

ARCHIVE_SHA = "e59f22067f192decb2a6c6fb8f7a8d2112608d3a3a82d52a36d96686bbd4a807"
MANIFEST_SHA = "34ed05d2589d37f0933b319ef2561907e7830d60485f014919452a083af2ae00"
ORACLE_SHA = "183db6732a7d63e5e32d4933938335096bafc4de2b95d85a33e3d6d0dec149ad"
# Exact C289 INITIAL source bytes and classification. No producer fixtures.
CASES = {
    "P01_meter_T_first_terminal.nl": ("POSITIVE", "b50522d3ef313aff1a7e7ae4189d7a489d06e86cf8c7d5c2195ef41033c86f9d", 5520),
    "P02_orchard_inner_break.nl": ("POSITIVE", "9d3f9f3332efedee3fb9c7a6643a04808d93aa201a701a056828f4f95bef38be", 5720),
    "N01_equal_value_foreign_root_tripwire.nl": ("VALID_CONTROL", "94ef49b1136dbce726925481f3ee3838a7e115f7bc5692035f8a8becc519dd49", 817),
    "N02_postcurrent_not_old_tripwire.nl": ("VALID_CONTROL", "b2f9f7a7cd3ec1cfa538ceff8407abd59336844cfce59620ee522952934cf420", 707),
    "N03_break_crosses_loan_boundary_reject.nl": ("INVALID_REJECT", "ffc4d4a74faff0e0cb6dbd6b4665eec4c03b64c938b6a1150f404cb5ab483d95", 539),
    "N04_loan_escape_into_break_reject.nl": ("INVALID_REJECT", "3da2c6eec8c7faadee69602c266a7ed8c4f20db6b0d39eb66b097341305d6c70", 522),
    "N05_sibling_intentional_change_tripwire.nl": ("VALID_CONTROL", "3d8b24069f237fd652a54e0b4f6ac07eaa197fdd9b10bed8d963b12f566ba0bd", 783),
    "N06_missing_head_deallocate_reject.nl": ("INVALID_REJECT", "335d03e2bc8d3647b978dc5767efbb22e99f3aca347d5ff0e43a2f7a08716aed", 5462),
    "N07_wrong_break_static_type_reject.nl": ("INVALID_REJECT", "3206ac2dcfdee7c9ea907fd340989669170e06c7074676907cf7d151ee358aec", 824),
}
BASE = "21577cbbedf2e915c779e24269eb1bf7a19f2857"
STAGES = ("SOURCE", "CHECK", "owned", "C17", "native_O0", "native_O2", "ASan_UBSan", "OOM", "physical_old_current")

def digest(data):
    return hashlib.sha256(data).hexdigest()

def stage_record(case_dir, label, argv, *, env=None, timeout=90):
    started = time.monotonic()
    record = {"label": label, "argv": [str(x) for x in argv], "timeout_seconds": timeout}
    try:
        cp = subprocess.run(argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, timeout=timeout, env=env, check=False)
        out, err = cp.stdout, cp.stderr
        record.update(exit_code=cp.returncode, status="EXIT", duration_seconds=round(time.monotonic()-started, 3))
    except subprocess.TimeoutExpired as e:
        out, err = e.stdout or b"", e.stderr or b""
        record.update(exit_code=None, status="TIMEOUT", duration_seconds=round(time.monotonic()-started, 3))
    except OSError as e:
        out, err = b"", str(e).encode("utf-8")
        record.update(exit_code=None, status="OSERROR", duration_seconds=round(time.monotonic()-started, 3))
    # Raw bytes preserved independently of the summary; never filter diagnostics.
    (case_dir / (label + ".stdout.raw")).write_bytes(out)
    (case_dir / (label + ".stderr.raw")).write_bytes(err)
    (case_dir / (label + ".command.json")).write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    record["stdout_bytes"] = len(out)
    record["stderr_bytes"] = len(err)
    return record

def load_original(archive, data_dir):
    buf = archive.read_bytes()
    if len(buf) != 6569 or digest(buf) != ARCHIVE_SHA:
        raise ValueError("CUSTODY FAILURE: C289 original archive digest/size mismatch")
    contents = {}
    with tarfile.open(fileobj=io.BytesIO(buf), mode="r:gz") as tf:
        for m in tf.getmembers():
            if m.isdir():
                continue
            if not m.isfile() or m.issym() or m.islnk() or m.size > 100_000:
                raise ValueError("CUSTODY FAILURE: unsafe tar member")
            name = PurePosixPath(m.name).name
            if name in contents:
                raise ValueError("CUSTODY FAILURE: duplicated member " + name)
            contents[name] = tf.extractfile(m).read()
    if set(contents) != set(CASES) | {"manifest.json", "oracle.json", "SHA256SUMS"}:
        raise ValueError("CUSTODY FAILURE: missing/unexpected archive members " + str(sorted(contents)))
    for name, (_, expected, length) in CASES.items():
        raw = contents[name]
        if len(raw) != length or digest(raw) != expected:
            raise ValueError("CUSTODY FAILURE: source " + name)
        raw.decode("utf-8", errors="strict")
    for name, expected in [("manifest.json", MANIFEST_SHA), ("oracle.json", ORACLE_SHA)]:
        if digest(contents[name]) != expected:
            raise ValueError("CUSTODY FAILURE: " + name + " SHA")
        json.loads(contents[name].decode("utf-8"))
    data_dir.mkdir(parents=True, exist_ok=True)
    for name in CASES:
        (data_dir / name).write_bytes(contents[name])
    return contents

# Observation-only external allocation recorder. It does NOT mint a NewLang
# owner/root/proof or change emitted C. All outputs are explicit partial evidence.
WRAP_C = r"""
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static long requests = 0, successes = 0, frees = 0, live = 0;
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void __real_free(void *);
static long fail_at(void) {
    const char *e = getenv("R1240_FAIL_AT");
    if (!e || !*e) return 0;
    char *end = NULL;
    long v = strtol(e, &end, 10);
    return end && !*end && v > 0 ? v : 0;
}
void *__wrap_malloc(size_t n) {
    requests++;
    if (requests == fail_at()) return NULL;
    void *p = __real_malloc(n);
    if (p) { successes++; live++; }
    return p;
}
void *__wrap_calloc(size_t n, size_t s) {
    requests++;
    if (requests == fail_at()) return NULL;
    void *p = __real_calloc(n,s);
    if (p) { successes++; live++; }
    return p;
}
void __wrap_free(void *p) {
    if (p) { frees++; live--; }
    __real_free(p);
}
__attribute__((destructor)) static void r1240_report(void) {
    fprintf(stderr, "R1240_ALLOC requests=%ld successes=%ld frees=%ld live=%ld fail_at=%ld\n",
            requests, successes, frees, live, fail_at());
}
"""

def run_case(name, kind, compiler, srcdir, outdir):
    cid = name[:3]
    d = outdir / cid
    d.mkdir(parents=True, exist_ok=True)
    result = {"id":cid, "original_file":name, "expected_category":kind, "base":BASE,
              "sha256":CASES[name][1], "layers":{k:"NOT_RUN" for k in STAGES},
              "commands":[], "finding":[], "stop":False}
    result["layers"]["SOURCE"] = "FROZEN_SHA_MATCH_NOT_CHECK_ACCEPTANCE"
    source = srcdir / name
    def step(tag, argv, env=None, timeout=90):
        row = stage_record(d,tag,[str(x) for x in argv],env=env,timeout=timeout)
        result["commands"].append(row)
        print(f"R1240 {cid} {tag} status={row['status']} rc={row['exit_code']} stdout={row['stdout_bytes']} stderr={row['stderr_bytes']}",flush=True)
        return row
    check=step("check",[compiler,"--check",source])
    if check["status"]!="EXIT" or check["exit_code"] is None or check["exit_code"]<0:
        result["layers"]["CHECK"]="CRASH_TIMEOUT_OR_INFRA"
        result["finding"].append("STOP: CHECK crash/timeout/infra, not a source-valid semantic verdict")
        result["stop"]=True
        return result
    if check["exit_code"]!=0:
        result["layers"]["CHECK"]="REJECTED_DIAGNOSTIC_UNCLASSIFIED"
        result["layers"]["owned"]="NOT_RUN_CHECK_REJECTED"
        if kind=="INVALID_REJECT":
            result["finding"].append("Expected-invalid rejected; diagnostic/source-law reason must be audited from raw")
        else:
            result["finding"].append("Expected source-valid rejected: syntax, precision or unsupported; classify from raw, NOT native PASS")
        return result
    result["layers"]["CHECK"]="ACCEPTED"
    # --check is the production semantic-check path; internal owned proof is
    # not a separately observed authenticated public proof in this runner.
    result["layers"]["owned"]="INTERNAL_CHECK_PATH_REACHED_PUBLIC_PROOF_NOT_INDEPENDENTLY_OBSERVED"
    if kind=="INVALID_REJECT":
        result["finding"].append("STOP: SOURCE-INVALID case unexpectedly accepted by production --check")
        result["stop"]=True
        return result
    cfile=d/"source.c"
    emit=step("emit_c17",[compiler,"--emit-c17-to",cfile,source])
    if emit["status"]!="EXIT" or emit["exit_code"]!=0 or not cfile.is_file():
        result["layers"]["C17"]="EMIT_UNSUPPORTED_OR_ERROR_NOT_RUN"
        result["finding"].append("CHECK accepted but C17 emission not reached; inspect raw")
        return result
    ctext=cfile.read_bytes()
    result["layers"]["C17"]="EMITTED_SHA256_"+digest(ctext)
    gcc="gcc-13"
    binaries={}
    for mode, flags in [("O0",["-O0"]),("O2",["-O2"]),("ASan_UBSan",["-O0","-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-sanitize-recover=all"])]:
        exe=d/("native_"+mode)
        build=step("compile_"+mode,[gcc,"-std=c17","-Wall","-Wextra","-Wpedantic","-Werror",*flags,cfile,"-o",exe],timeout=120)
        layer="ASan_UBSan" if mode=="ASan_UBSan" else "native_"+mode
        if build["status"]!="EXIT" or build["exit_code"]!=0:
            result["layers"][layer]="C17_COMPILE_FAILED_NOT_RUN"
            result["finding"].append("Native compiler failed "+mode+", review raw; no native PASS")
            continue
        binaries[mode]=exe
        env=os.environ.copy()
        if mode=="ASan_UBSan":
            env.update(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        actual=step("run_"+mode,[exe],env=env,timeout=45)
        if actual["status"]=="EXIT" and actual["exit_code"]==0:
            result["layers"][layer]="EXECUTED_EXIT0_SMOKE_ONLY_NOT_SEMANTIC_ORACLE"
        else:
            result["layers"][layer]="RUNTIME_RED"
            result["finding"].append("STOP: native/sanitizer RED "+mode+"; inspect raw")
            result["stop"]=True
            break
    # A C main() exit0 does not expose original physical T identity, typed
    # BREAK old result, current/sibling/loan or H domain receipts. Do not forge it.
    result["layers"]["physical_old_current"]="NOT_VERIFIED_NO_SOURCE_OWNED_PHYSICAL_OBSERVER"
    if kind=="POSITIVE" and not result["stop"] and "O0" in binaries:
        wrap=d/"alloc_wrap.c"
        wrap.write_text(WRAP_C,encoding="utf-8")
        exe=d/"oom_observer"
        o=step("compile_oom_wrapper",[gcc,"-std=c17","-O0","-Wall","-Wextra","-Wpedantic","-Werror",
                                     cfile,wrap,"-Wl,--wrap=malloc","-Wl,--wrap=calloc",
                                     "-Wl,--wrap=free","-o",exe],timeout=120)
        if o["status"]=="EXIT" and o["exit_code"]==0:
            metrics=[]
            for fail in (0,1,2):
                env=os.environ.copy()
                env["R1240_FAIL_AT"]=str(fail)
                rr=step("oom_fail_at_"+str(fail),[exe],env=env,timeout=45)
                raw=(d/("oom_fail_at_"+str(fail)+".stderr.raw")).read_text(errors="replace")
                m=re.search(r"R1240_ALLOC requests=(\d+) successes=(\d+) frees=(\d+) live=(-?\d+) fail_at=(\d+)",raw)
                if m:
                    vals=list(map(int,m.groups()))
                    metrics.append({"fail_at":fail,"requests":vals[0],"successes":vals[1],"frees":vals[2],"live":vals[3],"exit":rr["exit_code"]})
                else:
                    result["finding"].append("OOM wrapper did not print metrics; cannot assert cleanup")
                if rr["status"]!="EXIT" or rr["exit_code"]!=0:
                    result["finding"].append("STOP: OOM world runtime RED fail_at="+str(fail))
                    result["stop"]=True
                    break
            (d/"oom_metrics.json").write_text(json.dumps(metrics,indent=2)+"\n",encoding="utf-8")
            if not metrics:
                result["layers"]["OOM"]="NOT_VERIFIED_NO_WRAPPER_METRICS"
            elif any(x["live"]!=0 for x in metrics):
                result["layers"]["OOM"]="WRAPPED_ALLOC_BALANCE_RED_NOT_NORMS_PROOF"
                result["finding"].append("STOP: possible cleanup/ownership leak in OOM wrapper metrics")
                result["stop"]=True
            elif max(x["requests"] for x in metrics)<2:
                result["layers"]["OOM"]="NOT_VERIFIED_SECOND_ALLOCATION_WORLD_NOT_REACHED"
            else:
                result["layers"]["OOM"]="WRAPPED_ALLOC_BALANCE_OBSERVED_NOT_H_IDENTITY_PROOF"
        else:
            result["layers"]["OOM"]="NOT_RUN_OBSERVER_LINK_FAILED"
            result["finding"].append("OOM observer link unsupported, do not mark H cleanup verified")
    return result

def main():
    a=argparse.ArgumentParser()
    a.add_argument("--compiler",required=True,type=Path)
    a.add_argument("--archive",required=True,type=Path)
    a.add_argument("--out",required=True,type=Path)
    args=a.parse_args()
    out=args.out.resolve()
    out.mkdir(parents=True,exist_ok=True)
    summary={"track":"R1240 C303/C309", "baseline":BASE, "archive_sha256":ARCHIVE_SHA,
             "execution":"GitHub Actions only","cases":[], "overall":"NOT_RUN",
             "meaning":"Native exit0 is smoke, not source-owned original/result proof"}
    try:
        raw=load_original(args.archive.resolve(),out/"sources")
        (out/"custody.json").write_text(json.dumps({"archive_sha256":ARCHIVE_SHA,
            "manifest_sha256":MANIFEST_SHA,"oracle_sha256":ORACLE_SHA,
            "sources":{k:{"sha256":CASES[k][1],"bytes":CASES[k][2]} for k in CASES},
            "manifest_top_level_keys":sorted(json.loads(raw["manifest.json"]).keys()),
            "oracle_top_level_keys":sorted(json.loads(raw["oracle.json"]).keys())},
            indent=2)+"\n",encoding="utf-8")
        compiler=args.compiler.resolve()
        if not compiler.is_file():
            raise ValueError("compiler missing")
        summary["overall"]="ATTEMPTED"
        for name,(kind,_,_) in CASES.items():
            item=run_case(name,kind,compiler,out/"sources",out)
            summary["cases"].append(item)
            (out/"summary.json").write_text(json.dumps(summary,indent=2)+"\n",encoding="utf-8")
            if item["stop"]:
                summary["overall"]="STOP_FIRST_UNEXPECTED_RED"
                break
        else:
            summary["overall"]="OBSERVED_WITH_UNVERIFIED_PHYSICAL_LAYERS"
    except Exception as e:
        summary["overall"]="STOP_CUSTODY_OR_HARNESS"
        summary["error"]=repr(e)
        print("R1240 ERROR "+repr(e),file=sys.stderr,flush=True)
    finally:
        completed={x["id"] for x in summary["cases"]}
        for name,(_,_,_) in CASES.items():
            if name[:3] not in completed:
                summary["cases"].append({"id":name[:3],"layers":{k:"NOT_RUN_AFTER_STOP" for k in STAGES}})
        (out/"summary.json").write_text(json.dumps(summary,indent=2)+"\n",encoding="utf-8")
        print("R1240_FINAL_SUMMARY "+json.dumps({"overall":summary["overall"],
            "cases":[{"id":x["id"],"layers":x["layers"],"finding":x.get("finding",[])} for x in summary["cases"]]},sort_keys=True),flush=True)
    if summary["overall"] != "OBSERVED_WITH_UNVERIFIED_PHYSICAL_LAYERS":
        return 1
    for item in summary["cases"]:
        if item.get("expected_category") in ("POSITIVE", "VALID_CONTROL"):
            l = item["layers"]
            if (l.get("CHECK") != "ACCEPTED"
                    or not l.get("C17", "").startswith("EMITTED_SHA256_")
                    or l.get("native_O0") != "EXECUTED_EXIT0_SMOKE_ONLY_NOT_SEMANTIC_ORACLE"
                    or l.get("native_O2") != "EXECUTED_EXIT0_SMOKE_ONLY_NOT_SEMANTIC_ORACLE"):
                print("R1240 INCONCLUSIVE/UNEXPECTED: source-valid case failed required execution layers; do not report green", file=sys.stderr)
                return 2
    return 0

if __name__=="__main__":
    sys.exit(main())
