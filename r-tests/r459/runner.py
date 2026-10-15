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
def read_oracle(name):
    r=run([OUT/"r459-public-probe",SOURCE/name],tag="public-owned-"+name,expect=0)
    found=re.findall(r"R459_ORACLE source=(initial_some|none_control) anchor_symbol=(\\d+) peer_symbol=(\\d+) snapshot_symbol=(\\d+) copy_nodes=(\\d+) ptr_from_ref=(\\d+)",r.stdout)
    if len(found)!=1 or found[0][0] != name.removesuffix(".nl"):
        raise RuntimeError("R459 INCONCLUSIVE: missing authentic source-issued checked-owned original mapping "+name)
    d=dict(zip(("name","anchor","peer","snapshot","copy_nodes","ptr_from_ref"),found[0]))
    ids=[int(d[k]) for k in ("anchor","peer","snapshot")]
    if min(ids)<=0 or len(set(ids))!=3 or int(d["copy_nodes"])==0 or (name=="initial_some.nl" and int(d["ptr_from_ref"])==0):
        raise RuntimeError("R459 INCONCLUSIVE: incomplete authentic public owned oracle "+name)
    print(f"R459 SOURCE_ISSUED_ORACLE {name}: {d}",flush=True)
    return ids
def emitted(name):
    source=SOURCE/name
    checked=run([COMPILER,"--check",source],tag="positive-check-"+name)
    if checked.returncode!=0:
        raise RuntimeError("R459 FIRST BARRIER: SOURCE/CHECK refused "+name)
    if checked.stdout or checked.stderr:
        raise RuntimeError("R459 STOP: successful --check unexpectedly wrote output")
    oracle=read_oracle(name)
    c=OUT/(name+".c")
    r=subprocess.run([str(COMPILER),str(source)],cwd=ROOT,capture_output=True,timeout=120)
    c.write_bytes(r.stdout)
    (OUT/(name+".emit.err")).write_bytes(r.stderr)
    print(f"R459 C17_EMISSION {name}: rc={r.returncode} length={len(r.stdout)} sha256={sha(c)} stderr={r.stderr[:3000]!r}",flush=True)
    if r.returncode!=0 or not r.stdout:
        raise RuntimeError("R459 COVERAGE FINDING: CHECK/public-owned passed but C17 backend unsupported/refused "+name)
    source_text=r.stdout.decode("utf-8",errors="strict")
    for expected in ("#include NEWLANG_NODE_OBSERVER","NL_NODE_BIND(","NL_NODE_FINISH()"):
        if expected not in source_text:
            raise RuntimeError("R459 INCONCLUSIVE: emitted C missing passive observation interface "+expected)
    return c,oracle
def native(c,control,oracle):
    frozen_sha=sha(c)
    defs=['-DNEWLANG_NODE_OBSERVER="r-tests/r459/observe_bind.h"',
          "-DR459_ANCHOR_SYMBOL="+str(oracle[0]),
          "-DR459_PEER_SYMBOL="+str(oracle[1]),
          "-DR459_SNAPSHOT_SYMBOL="+str(oracle[2]),
          "-DR459_EXPECT_SOME="+str(0 if control else 1)]
    def one(cc,opt,sanitized=False):
        label=c.stem+"-"+cc+("-san" if sanitized else "-O"+opt)
        flags=[cc,"-std=c17","-Wall","-Wextra","-Wpedantic","-Werror","-g","-O"+opt]
        if sanitized: flags+=["-fsanitize=address,undefined","-fno-omit-frame-pointer"]
        plain=OUT/(label+"-plain")
        observed=OUT/(label+"-observed")
        run(flags+[str(c),"-o",plain],tag="compile-plain-"+label,expect=0)
        env=os.environ.copy()
        if sanitized:
            env["ASAN_OPTIONS"]="detect_leaks=1:halt_on_error=1"
            env["UBSAN_OPTIONS"]="halt_on_error=1"
        run([plain],tag="native-plain-"+label,expect=0,env=env)
        run(flags+["-I",ROOT]+defs+[str(c),"-o",observed],tag="compile-passive-"+label,expect=0)
        measured=run([observed],tag="native-passive-"+label,env=env)
        if measured.returncode!=0:
            raise RuntimeError("R459 STOP: native passive refusal; inspect original event facts for mismatch vs missing observer "+label)
        if (measured.stdout.count("R459_OBSERVER_FINAL status=PASS ")!=1 or
            measured.stdout.count("R459_OBSERVE original=anchor ")!=1 or
            measured.stdout.count("R459_OBSERVE original=peer ")!=1 or
            measured.stdout.count("R459_OBSERVE copy=snapshot ")!=1):
            raise RuntimeError("R459 INCONCLUSIVE: missing or duplicated real physical event "+label)
        anchor=re.search(r"original=anchor .*?physical=(0x[0-9a-fA-F]+)",measured.stdout)
        peer=re.search(r"original=peer .*?physical=(0x[0-9a-fA-F]+).*? ptr=((?:0x[0-9a-fA-F]+)|\\(nil\\))",measured.stdout)
        if not anchor or not peer or anchor.group(1).lower()==peer.group(1).lower():
            raise RuntimeError("R459 INCONCLUSIVE: cannot independently read two distinct original addresses "+label)
        if not control and anchor.group(1).lower()!=peer.group(2).lower():
            raise RuntimeError("R459 FINDING: Some ptr mismatches actual original anchor C address "+label)
        if sha(c)!=frozen_sha:
            raise RuntimeError("R459 STOP: emitted guest C bytes changed during passive observation")
        print(f"R459 PHYSICAL_PLAIN_OBSERVED_PASS label={label} C_SHA256={frozen_sha} original_anchor={anchor.group(1)} original_peer={peer.group(1)} peer_ptr={peer.group(2)}",flush=True)
    for cc in ("gcc","clang"):
        for opt in ("0","2"): one(cc,opt)
    for cc in ("gcc","clang"): one(cc,"1",sanitized=True)
    print(f"R459 SAME_C_BYTES_ALL_MODES sha256={frozen_sha}",flush=True)
def main():
    for p in sorted(SOURCE.glob("*.nl")): print(f"R459 FREEZE SOURCE SHA256 {p.name} {sha(p)}",flush=True)
    print("R459 AUTHORITY candidate b69b6f63c6eb8681ebd468e7988f961b0378f23f; baseline 4506112b1dea4a5a63b4ac41ff9e8f8f8388146d",flush=True)
    classify_negative("ref_escape.nl")
    classify_negative("stale_root.nl")
    for name,control in [("none_control.nl",True),("initial_some.nl",False)]:
        c,oracle=emitted(name)
        native(c,control,oracle)
    print("R459 PASS TESTED FINITE BOUNDARY: independent CHECK/public-owned/C17/native/passive O0/O2 gcc+clang+sanitizers",flush=True)
if __name__=="__main__":
    try: main()
    except Exception as e:
        print(f"R459 TERMINAL RUN FAILURE: {e}",file=sys.stderr,flush=True)
        sys.exit(1)
