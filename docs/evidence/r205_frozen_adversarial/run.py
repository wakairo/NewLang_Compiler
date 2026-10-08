#!/usr/bin/env python3
"""R205 independent frozen CLI evidence runner; never executes emitted C."""
import argparse,csv,hashlib,json,re,subprocess,sys
from pathlib import Path
SHA="aed9e7b7d49de338c570c8c6e89197f2251db134"
BLOB="d62b74f7e4f2607de1db9f4a31093fa14e3429eb"
def invoke(argv,cwd=None,t=30):
    return subprocess.run(list(map(str,argv)),cwd=cwd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=t)
def version_check(root,compiler):
    def checked(cmd,expected,what):
        p=invoke(cmd)
        if p.returncode or p.stdout.decode().strip()!=expected:
            raise RuntimeError(what+": "+repr((p.returncode,p.stdout.decode(errors="replace"),p.stderr.decode(errors="replace"))))
    checked(["git","-C",root,"rev-parse","HEAD"],SHA,"frozen commit")
    checked(["git","-C",root,"hash-object",root/"tests/fixtures/live_tail_handoff.nl"],BLOB,"seed Git blob")
    if "NewLang_v0_spec_Draft17_27.md" not in (root/"docs/reference/CURRENT_SPEC.md").read_text():
        raise RuntimeError("not Draft17.27")
    checked(["llvm-config-23","--version"],"23.1.2","LLVM version")
    if not compiler.is_file(): raise RuntimeError("missing frozen newlangc")
    p=invoke([compiler,"--version"])
    if p.returncode: raise RuntimeError("newlangc --version failed")
    return dict(frozen_compiler_sha=SHA,seed_blob=BLOB,llvm="23.1.2",binary_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),binary_version=p.stdout.decode(errors="replace").strip())
def save(out,rows,meta):
    cols=["id","mode","expected","description","source_sha256","exit","diagnostics","stage","stdout_bytes","stderr_file","stdout_file","interpretation"]
    with (out/"results.csv").open("w",newline="") as f:
        w=csv.DictWriter(f,fieldnames=cols);w.writeheader();w.writerows(rows)
    (out/"results.json").write_text(json.dumps({"provenance":meta,"results":rows},indent=2))
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--frozen-root",required=True,type=Path)
    ap.add_argument("--compiler",required=True,type=Path)
    ap.add_argument("--evidence",required=True,type=Path)
    ap.add_argument("--out",required=True,type=Path)
    a=ap.parse_args()
    root=a.frozen_root.resolve();compiler=a.compiler.resolve();e=a.evidence.resolve();out=a.out.resolve()
    out.mkdir(parents=True,exist_ok=True)
    try: meta=version_check(root,compiler)
    except Exception as exc:
        (out/"BLOCKED.txt").write_text(repr(exc)+"\n")
        print("R205_PREFLIGHT_BLOCKED",repr(exc),flush=True)
        return 3
    entries=list(csv.DictReader((e/"manifest.csv").open()))
    rows=[]
    print("R205_FROZEN_PROVENANCE",json.dumps(meta),flush=True)
    for item in entries:
        name=item["id"];src=e/"corpus"/(name+".nl")
        if not src.is_file():raise RuntimeError("missing source "+name)
        data=src.read_bytes()
        try:p=invoke([compiler,src],t=35)
        except Exception as exc:
            (out/"BLOCKED.txt").write_text(name+" invocation: "+repr(exc))
            save(out,rows,meta);return 3
        stderr=p.stderr.decode("utf-8","replace")
        codes=",".join(dict.fromkeys(re.findall(r"\b[A-Z][A-Z0-9]*(?:-[A-Z0-9]+)+\b",stderr)))
        admitted=p.returncode==0 or (p.returncode==4 and "V1-BACKEND-UNSUPPORTED" in stderr)
        if admitted:stage="SEMANTIC_ADMITTED"
        elif re.search(r"lexical|syntax|parser|source.profile|P193-DEFINITION-PROFILE",stderr,re.I):stage="LEXER_OR_PROFILE_NOT_OWNER_GATE"
        else:stage="REJECTED_STAGE_UNCONFIRMED"
        bad=item["expected"].startswith("REJECT")
        interpretation="SUSPECTED_UNSOUND_STOP" if bad and admitted else ("ADMITTED_REVIEW" if admitted else "REJECTED_REVIEW")
        (out/(name+".stderr.txt")).write_bytes(p.stderr)
        (out/(name+".stdout.bin")).write_bytes(p.stdout)
        row={"id":name,"mode":item["mode"],"expected":item["expected"],"description":item["description"],"source_sha256":hashlib.sha256(data).hexdigest(),"exit":p.returncode,"diagnostics":codes,"stage":stage,"stdout_bytes":len(p.stdout),"stderr_file":name+".stderr.txt","stdout_file":name+".stdout.bin","interpretation":interpretation}
        rows.append(row);save(out,rows,meta)
        print("R205_CASE",name,"exit="+str(p.returncode),"codes="+codes,"stage="+stage,"stdout="+str(len(p.stdout)),flush=True)
        if interpretation=="SUSPECTED_UNSOUND_STOP":
            print("R205_STOP_SUSPECTED_UNSOUND",name,src,flush=True)
            (out/"SUSPECTED_FINDING.txt").write_text("Minimal source: corpus/"+name+".nl\nFrozen SHA: "+SHA+"\nReturn code: "+str(p.returncode)+"\nDiagnostic: "+stderr+"\n")
            return 5
    print("R205_COMPLETE executed="+str(len(rows))+" total="+str(len(entries)),flush=True)
    return 0
if __name__=="__main__":sys.exit(main())
