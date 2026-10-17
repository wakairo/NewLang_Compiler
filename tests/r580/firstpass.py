"""R580 first-pass executor. Frozen independent oracle is read, not revised.
No candidate patch, P report, or test output is used to construct expectations.
All generated C is byte-for-byte unchanged between plain and passive builds.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

def cmd(argv, cwd=None):
    p = subprocess.run([str(a) for a in argv], cwd=cwd, text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=90)
    return {"args": [str(a) for a in argv], "rc": p.returncode,
            "stdout": p.stdout, "stderr": p.stderr}

def checked(result, stage):
    if result["rc"] or result["stderr"]:
        raise RuntimeError(f"{stage} failed: {result!r}")

def generate_observer(rows):
    binds = [r for r in rows if r[0] == "B"]
    writes = [r for r in rows if r[0] == "W"]
    assert [r[1] for r in binds] == ["c","cptr","b","bptr","a","old_a","old_b"], binds
    assert [int(r[3]) for r in binds] == [1,2,1,2,1,3,3], binds
    c, cp, b, bp, a, oa, ob = [int(r[2]) for r in binds]
    assert [int(binds[i][4]) for i in (0,2,4)] == [33,22,11], binds
    assert int(binds[1][4]) == c and int(binds[3][4]) == b
    assert len(writes) == 2, writes
    assert [list(map(int, w[1:])) for w in writes] == [
        [a,0,0,1,b], [b,0,0,1,c]], writes
    assert all([int(r[5]),int(r[6])] == [0,0] for r in binds if r[1] in ("old_a","old_b"))
    macros = "\n".join(f"#define ROLE_{k} {v}" for k,v in {
        "C":c,"CP":cp,"B":b,"BP":bp,"A":a,"OA":oa,"OB":ob}.items())
    return macros + r"""
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static const nl_node *r_c,*r_b,*r_a;
static const nl_node *const *r_cp,*r_bp;
static const nl_node_option *r_oa,*r_ob;
static size_t r_step;
static void r_require(int good) {
    if(!good) { fputs("R580 passive original topology failed\n",stderr); abort(); }
}
static void r_opt(const nl_node_option *v, int tag,const nl_node *p) {
    r_require(v && v->tag==(unsigned)tag && v->ptr==p);
}
static void r_bind(size_t f,size_t s,size_t k,const void *addr) {
    r_require(addr && f==0);
    switch(r_step) {
    case 0: r_require(s==ROLE_C&&k==1);r_c=addr;
      r_require(r_c->f1==33);r_opt(&r_c->f0,0,NULL);break;
    case 1: r_require(s==ROLE_CP&&k==2);r_cp=addr;
      r_require(*r_cp==r_c);break;
    case 2: r_require(s==ROLE_B&&k==1);r_b=addr;
      r_require(r_b!=r_c&&r_b->f1==22);r_opt(&r_b->f0,0,NULL);break;
    case 3: r_require(s==ROLE_BP&&k==2);r_bp=addr;
      r_require(*r_bp==r_b);break;
    case 4: r_require(s==ROLE_A&&k==1);r_a=addr;
      r_require(r_a!=r_c&&r_a!=r_b&&r_a->f1==11);
      r_opt(&r_a->f0,0,NULL);break;
    case 6: r_require(s==ROLE_OA&&k==3);r_oa=addr;
      r_require(r_oa!=&r_a->f0);r_opt(r_oa,0,NULL);break;
    case 8: r_require(s==ROLE_OB&&k==3);r_ob=addr;
      r_require(r_ob!=&r_b->f0&&r_ob!=r_oa);r_opt(r_ob,0,NULL);break;
    default:r_require(0);
    }
    ++r_step;
}
static void r_replace(const nl_node *root,const nl_node_option *old) {
    r_require(old != &root->f0);
    if(r_step==5) {
      r_require(root==r_a);
      r_opt(old,0,NULL);r_opt(&r_a->f0,1,r_b);r_opt(&r_b->f0,0,NULL);
    } else if(r_step==7) {
      r_require(root==r_b);
      r_opt(old,0,NULL);r_opt(&r_a->f0,1,r_b);r_opt(&r_b->f0,1,r_c);
    } else r_require(0);
    ++r_step;
}
static void r_finish(void) {
    r_require(r_step==9&&r_a&&r_b&&r_c&&r_cp&&r_bp&&r_oa&&r_ob);
    r_require(r_a!=r_b&&r_b!=r_c&&r_c!=r_a);
    r_require(*r_cp==r_c&&*r_bp==r_b);
    r_require(r_a->f1==11&&r_b->f1==22&&r_c->f1==33);
    r_opt(&r_a->f0,1,r_b);r_opt(&r_b->f0,1,r_c);r_opt(&r_c->f0,0,NULL);
    r_opt(r_oa,0,NULL);r_opt(r_ob,0,NULL);
    puts("R580 PASS original lexical 3-root physical identity, two ordered writes");
}
#define NL_NODE_BIND(f,s,k,p) r_bind(f,s,k,p)
#define NL_NODE_REPLACE(r,o) r_replace(r,o)
#define NL_NODE_FINISH() r_finish()
"""

def mutate(case, src):
    if case == "fourth_root":
        key = "    let old_a=loan_write(a@next)"
        assert src.count(key)==1
        return src.replace(key, "    let d=Node{next:Option<ptr<Node>>::None,payload:u8(44)};\n"+key)
    if case == "third_change":
        key = "    unit\n}"
        assert src.count(key)==1
        return src.replace(key, "    let old_c=loan_write(c@next){|w|replace(w,Option<ptr<Node>>::Some(bptr))};\n"+key)
    if case == "type_mismatch":
        key = "replace(w,Option<ptr<Node>>::Some(cptr))"
        assert src.count(key)==1
        return src.replace(key,"replace(w,Option<u8>::None)")
    if case == "ptr_escalation":
        key = "loan_write(b@next)"
        assert src.count(key)==1
        return src.replace(key,"loan_write_ptr(bptr)")
    if case == "unregistered_fragment":
        return "main()\n"
    raise ValueError(case)

def main():
    compiler, manifest_probe, rprobe, build = map(Path, sys.argv[1:5])
    oracle = json.loads((Path(__file__).parent/"frozen_oracle.json").read_text())
    assert oracle["phase"]=="FROZEN PRE-CANDIDATE BLIND ORACLE"
    source = oracle["positive_source"]
    evidence = {"fixed": oracle["fixed_candidate"],
                "source_sha256": hashlib.sha256(source.encode()).hexdigest(),
                "phases": {}, "negative": [], "native": []}
    with tempfile.TemporaryDirectory(prefix="r580-independent-") as t:
        out=Path(t)
        p=out/"original.nl";p.write_text(source)
        for label, argv in [
          ("CHECK", [compiler, "--check", p]),
          ("registered_public_manifest", [manifest_probe,p]),
          ("independent_public_inspector", [rprobe,p]),
          ("C17", [compiler,p])]:
            res=cmd(argv)
            evidence["phases"][label]={"rc":res["rc"],"stderr":res["stderr"],
                "bytes":len(res["stdout"].encode()),
                "sha256":hashlib.sha256(res["stdout"].encode()).hexdigest()}
            if label=="C17" and res["rc"]==0:
                emitted=res["stdout"]
            elif label=="registered_public_manifest" and res["rc"]==0:
                manifest=res["stdout"]
                evidence["manifest"]=manifest
            elif label=="independent_public_inspector" and res["rc"]==0:
                evidence["public_inspector_output"]=res["stdout"]
            if res["rc"] or res["stderr"] or (label=="CHECK" and res["stdout"]):
                print(json.dumps(evidence,indent=2),flush=True)
                raise RuntimeError(f"FIRST BLIND BARRIER: {label}: {res!r}")
        rows=[line.split() for line in manifest.splitlines()]
        header=out/"passive.h";header.write_text(generate_observer(rows))
        evidence["C17_sha256"]=hashlib.sha256(emitted.encode()).hexdigest()
        for cc in ("gcc","clang"):
            for opt in ("-O0","-O2"):
                for passive in (False,True):
                    name=f"{cc}_{opt}_{'passive' if passive else 'plain'}"
                    cfile=out/(name+".c");cfile.write_text(emitted)
                    exe=out/name
                    args=[cc,"-std=c17","-Wall","-Wextra","-Wpedantic",
                          "-Werror",opt]
                    if passive:args.append(f'-DNEWLANG_NODE_OBSERVER="{header}"')
                    result=cmd(args+[cfile,"-o",exe])
                    checked(result,"C17 strict "+name)
                    executed=cmd([exe]);checked(executed,"native "+name)
                    if passive:
                        assert "R580 PASS original lexical 3-root physical identity" in executed["stdout"]
                    else:assert not executed["stdout"]
                    evidence["native"].append({"cc":cc,"opt":opt,
                        "passive":passive,"exit":executed["rc"],
                        "stdout":executed["stdout"]})
        for negative in oracle["negative_cases"]:
            name=negative["name"];path=out/(name+".nl")
            path.write_text(mutate(name,source))
            check=cmd([compiler,"--check",path])
            emit=cmd([compiler,path])
            record={"case":name,"sha256":hashlib.sha256(path.read_bytes()).hexdigest(),
                    "check_rc":check["rc"],"check_diag":check["stderr"],
                    "emit_rc":emit["rc"],"C_bytes":len(emit["stdout"].encode()),
                    "emit_diag":emit["stderr"]}
            evidence["negative"].append(record)
            if name in ("type_mismatch","ptr_escalation","unregistered_fragment"):
                assert check["rc"] != 0 and emit["rc"] != 0 and not emit["stdout"],record
            else:
                assert not (check["rc"] == 0 and emit["rc"] == 0),(
                    "OUTSIDE-BOUNDED-ISSUER SOURCE/C17 ACCEPTED",record)
        print(json.dumps(evidence,indent=2),flush=True)
        print("R580 BLIND restricted source/public-inspector/C17/native FINITE PASS; "
              "candidate-specific validator/forgeries/OOM NOT ESTABLISHED",flush=True)

if __name__=="__main__":
    main()
