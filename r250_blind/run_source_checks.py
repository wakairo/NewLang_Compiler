#!/usr/bin/env python3
"""R250 black-box CLI runner: invokes only build products; never inspects compiler internals."""
import base64
import hashlib
import json
from pathlib import Path
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
DIR = Path(__file__).resolve().parent
manifest = json.loads((DIR/"manifest.json").read_text(encoding="utf-8"))
binary_paths = []
for path in (ROOT/"build").rglob("*"):
    if path.is_file() and os.access(path,os.X_OK):
        try:
            if path.open("rb").read(4) == b"\x7fELF":
                binary_paths.append(path)
        except OSError:
            pass
binary_paths.sort(key=lambda p: (
    0 if p.name in ("newlangc","newlang","newlang-compiler","newlang_compiler") else 1,
    0 if "newlang" in p.name.lower() else 1, str(p)))
print("EXECUTABLES",json.dumps([str(p.relative_to(ROOT)) for p in binary_paths]),flush=True)
print("ROOT_HEAD",subprocess.check_output(["git","rev-parse","HEAD"],cwd=ROOT,text=True).strip(),flush=True)
print("FROZEN_SHA","d8765a5ae11919d3076cec0d1432973a0f18fdcd",flush=True)

def run(cmd, seconds=60):
    try:
        p=subprocess.run([str(x) for x in cmd],cwd=ROOT,capture_output=True,timeout=seconds)
        return {"exit_status":p.returncode,"stdout_b64":base64.b64encode(p.stdout).decode("ascii"),
                "stderr_b64":base64.b64encode(p.stderr).decode("ascii"),
                "stdout_preview":p.stdout.decode("utf-8","backslashreplace")[:1500],
                "stderr_preview":p.stderr.decode("utf-8","backslashreplace")[:3000],
                "timed_out":False}
    except subprocess.TimeoutExpired as e:
        return {"exit_status":None,"stdout_b64":base64.b64encode(e.stdout or b"").decode("ascii"),
                "stderr_b64":base64.b64encode(e.stderr or b"").decode("ascii"),
                "timed_out":True}

candidates = [p for p in binary_paths if "newlang" in p.name.lower() and "test" not in p.name.lower()]
if not candidates:
    candidates = [p for p in binary_paths if "test" not in str(p).lower()]
reports = {"compiler_candidates":[],"selection":None,"cases":[]}
for binary in candidates[:5]:
    digest=hashlib.sha256(binary.read_bytes()).hexdigest()
    entry={"binary":str(binary.relative_to(ROOT)),"sha256":digest,
           "help_long":run([binary,"--help"],10),
           "help_short":run([binary,"-h"],10)}
    reports["compiler_candidates"].append(entry)
    print("CANDIDATE",entry["binary"],"sha256",digest,flush=True)
    print("HELP_LONG",entry["help_long"].get("stdout_preview"),entry["help_long"].get("stderr_preview"),flush=True)
    print("HELP_SHORT",entry["help_short"].get("stdout_preview"),entry["help_short"].get("stderr_preview"),flush=True)

if not candidates:
    print("ENVIRONMENT_NO_COMPILER_BIN",flush=True)
else:
    binary=candidates[0]
    sample=ROOT/manifest[0]["file"]
    # CLI syntax discovery; no project tests, P reports, or implementation consulted.
    variants=[
        ["check",str(sample)],
        ["--check",str(sample)],
        [str(sample),"--check"],
        [str(sample)],
        ["compile",str(sample)],
        ["--emit-c",str(sample)],
        [str(sample),"-o",str(DIR/"probe.c")],
    ]
    trial=[]
    for argv in variants:
        output=run([binary]+argv,25)
        trial.append({"argv":argv,"result":output})
        print("CLI_TRIAL",json.dumps({"argv":argv,"rc":output.get("exit_status"),
            "stderr":output.get("stderr_preview"),"stdout":output.get("stdout_preview")},ensure_ascii=False),flush=True)
    reports["discovery_trials"]=trial
    # Prefer checker-specific spelling if present; use a known-success actual compile.
    accepted=[t for t in trial if str(sample) in t["argv"] and (
        t["result"].get("exit_status")==0 or
        (t["result"].get("exit_status")==4 and
         "accepted program uses a construct" in t["result"].get("stderr_preview","")))]
    if accepted:
        chosen=accepted[0]["argv"]
    else:
        chosen=variants[0]
    reports["selection"]={"binary":str(binary.relative_to(ROOT)),"sha256":hashlib.sha256(binary.read_bytes()).hexdigest(),
                          "argv_template":chosen}
    for c in manifest:
        path=ROOT/c["file"]
        argv=[str(path) if arg==str(sample) else arg for arg in chosen]
        # Avoid leave-behind: track C outputs in dedicated evidence directory only.
        before=set(str(x) for x in (DIR/"sources").rglob("*.c"))
        result=run([binary]+argv,40)
        after=set(str(x) for x in (DIR/"sources").rglob("*.c"))
        c_outputs=sorted(after-before)
        report={**c,"argv":argv,"exit_status":result["exit_status"],"timed_out":result["timed_out"],
                "stdout_b64":result["stdout_b64"],"stderr_b64":result["stderr_b64"],
                "stdout_preview":result.get("stdout_preview",""),
                "stderr_preview":result.get("stderr_preview",""),
                "c_outputs":c_outputs}
        reports["cases"].append(report)
        print("CASE_RAW",json.dumps({"name":c["name"],"stdout_b64":result["stdout_b64"],"stderr_b64":result["stderr_b64"]}),flush=True)
        print("CASE",json.dumps({"name":c["name"],"rc":result["exit_status"],
                                "stderr":report["stderr_preview"][:600],
                                "stdout":report["stdout_preview"][:300],
                                "c_outputs":c_outputs},ensure_ascii=False),flush=True)
(DIR/"run_results.json").write_text(json.dumps(reports,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
print("RESULTS_SHA256",hashlib.sha256((DIR/"run_results.json").read_bytes()).hexdigest(),flush=True)
