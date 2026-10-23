#!/usr/bin/env python3
"""R #780 independent calibrated native gate; uses R #776 frozen bytes.
No P observer, oracle, emitted C or compiled native artifact reused.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

which, cli, cc, sanitizer, root = sys.argv[1:]
root = Path(root)
freeze = root / "tests/r776/frozen"
folder = root / "tests/r780"
manifest = json.loads((folder/"frozen/manifest.json").read_text())
old_oracle = json.loads((freeze/"oracle.json").read_text())
cases = {
 "original": (freeze/"positive_h_first.nl",manifest["frozen_r_a_sha256"],
              manifest["primary_oracle"]),
 "variant": (folder/"frozen/positive_index_swapped.nl",manifest["variant_sha256"],
             manifest["variant_oracle"]),
}
def req(ok, desc):
    if not ok:
        raise AssertionError(desc)
def cmd(args, **kwargs):
    return subprocess.run([str(x) for x in args],capture_output=True,timeout=60,**kwargs)
def checked(p, desc):
    req(p.returncode==0,desc+" rc="+str(p.returncode)+
         " stderr="+p.stderr.decode(errors="replace")[-1600:])
def hashok(path,digest):
    req(hashlib.sha256(path.read_bytes()).hexdigest()==digest,
        "immutable source SHA mismatch "+str(path))
def validate_freeze():
    hashok(freeze/"oracle.json",manifest["frozen_original_oracle_sha256"])
    for rel, expected in old_oracle["source_sha256"].items():
        hashok(root/rel, expected)
    for name,(path,digest,_) in cases.items():
        hashok(path,digest)
    print("R780 original R776 bytes and new R variant SHA verified",flush=True)
validate_freeze()
path,digest,oracle=cases[which]
with tempfile.TemporaryDirectory(prefix="r780-native-") as td:
    temp=Path(td)
    c=cmd([cli,"--check",path])
    checked(c,"source-check")
    req(not c.stderr and not c.stdout,"CHECK not silent")
    out=cmd([cli,path])
    checked(out,"actual C17 emit")
    req(not out.stderr and len(out.stdout)>500 and b"int main(void)" in out.stdout,
        "missing complete C17")
    guest=temp/"from_real_source.c"
    guest.write_bytes(out.stdout)
    named=temp/"persisted.c"
    named.write_bytes(b"R780-NAMED-PRIOR-VALID-DATA")
    named_run=cmd([cli,"--emit-c17-to",named,path])
    checked(named_run,"named emit")
    req(not named_run.stdout and not named_run.stderr and named.read_bytes()==out.stdout,
        "named successful output differs from stdout")
    print("R780 C17",which,"source_sha256",digest,
          "c17_sha256",hashlib.sha256(out.stdout).hexdigest(),
          "c17_bytes",len(out.stdout),"named=IDENTICAL",flush=True)
    # Frozen values are host-side assertion parameters ONLY, not guest initializers.
    initial0=oracle["T_panel_initial"]; initial1=oracle["T_reserve_initial"]
    final0=oracle["T_panel_final"]; final1=oracle["T_reserve_final"]
    h=oracle["H_payloads"]
    defs={
      "HEAD_PAYLOAD":h[0],"TAIL_PAYLOAD":h[1],
      "B00":initial0[0],"B01":initial0[1],"B10":initial1[0],"B11":initial1[1],
      "A00":final0[0],"A01":final0[1],"A10":final1[0],"A11":final1[1],
      "OLD0":oracle["T_panel_old"],"OLD1":oracle["T_reserve_old"],
      "NEW0":final0[oracle["T_panel_index"]],
      "NEW1":final1[oracle["T_reserve_index"]],
      "IDX0":oracle["T_panel_index"],"IDX1":oracle["T_reserve_index"]
    }
    strict=["-std=c17","-pedantic-errors","-Wall","-Wextra","-Werror"]
    flags=[] if sanitizer=="none" else ["-fsanitize="+sanitizer,"-fno-omit-frame-pointer",
                                       "-fno-sanitize-recover=all"]
    for opt in ("-O0","-O2"):
        plain=temp/("plain"+opt)
        build=cmd([cc,*strict,opt,*flags,guest,"-o",plain])
        checked(build,"STRICT C17 "+str(cc)+" "+opt)
        normal=cmd([plain],env=os.environ.copy())
        checked(normal,"plain native "+opt)
        req(not normal.stdout and not normal.stderr,"unexpected plain output")
        observed=temp/("observed"+opt)
        args=[cc,*strict,opt,*flags,guest,
            root/"tests/integration/two_heap_platform.c",
            "-Wl,--wrap=malloc","-Wl,--wrap=free","-o",observed,
            '-DNEWLANG_NODE_OBSERVER="'+str(folder/"node_observer.h")+'"',
            '-DNEWLANG_HEAP_OBSERVER="'+str(folder/"runtime_observer.h")+'"',
            *["-D"+k+"="+str(v) for k,v in defs.items()]]
        built=cmd(args)
        checked(built,"passive observer strict C17 "+opt)
        for fail_at,trial,alloc in ((0,2,2),(1,1,0),(2,2,1)):
            env=dict(os.environ,NEWLANG_TEST_FAIL_CALL=str(fail_at),
                     ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                     UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
            observed_run=cmd([observed],env=env)
            checked(observed_run,which+" "+opt+" fail-at="+str(fail_at))
            msg=observed_run.stdout.decode(errors="replace")
            req("R780_NATIVE PASS" in msg and "trials="+str(trial) in msg and
                "allocated="+str(alloc)+" freed="+str(alloc) in msg,
                "physical observer incomplete "+msg)
            req(not observed_run.stderr,"sanitizer/observer stderr "+observed_run.stderr.decode())
            print(json.dumps({"source":which,"compiler":str(cc),"opt":opt,
                 "sanitizer":sanitizer,"fail_at":fail_at,
                 "actual_native":msg.strip()}),flush=True)
print("R780 per-source native PASS (bounded worlds only)",flush=True)
