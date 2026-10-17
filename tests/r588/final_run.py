"""R588 FINAL NON-BLIND bounded calibration: same R583 runner with only observer declarator corrected; R580/R583 immutable.
The original R580 sample is observation-only; the separately committed new
source is tested against the candidate public validator without host fixtures.
Fail at first genuine unsupported/incorrect stage for the calibrated source.
"""
import hashlib, json, pathlib, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[2]
SOURCE = ROOT/"tests/r583/calibrated.nl"
ORIGINAL = ROOT/"tests/r580/frozen_oracle.json"
def run(*args, expect=None, timeout=90, **kw):
    p = subprocess.run(list(map(str,args)), capture_output=True,
                       text=True, timeout=timeout, **kw)
    print("COMMAND", " ".join(map(str,args)), "rc", p.returncode,
          "stdout_sha", hashlib.sha256(p.stdout.encode()).hexdigest(),
          "outbytes", len(p.stdout.encode()), flush=True)
    if p.stderr: print("STDERR", p.stderr[:8000], flush=True)
    if expect is not None and p.returncode != expect:
        raise RuntimeError(f"expected rc={expect}; got {p.returncode} for {args}")
    return p
def observer(rows):
    binds=[r for r in rows if r[0]=="B"]
    writes=[r for r in rows if r[0]=="W"]
    expected=["c","cptr","a","b","bptr","old_a","old_b"]
    assert [r[1] for r in binds]==expected,binds
    assert [int(r[3]) for r in binds]==[1,2,1,1,2,3,3],binds
    b={r[1]:int(r[2]) for r in binds}
    assert [int(r[4]) for r in binds if r[1] in ("c","a","b")]==[33,11,22]
    assert int(binds[1][4])==b["c"] and int(binds[4][4])==b["b"]
    assert [list(map(int,r[1:])) for r in writes]==[
        [b["a"],0,0,1,b["b"]], [b["b"],0,0,1,b["c"]]],writes
    assert all([int(r[5]),int(r[6])]==[0,0] for r in binds[-2:])
    macros="\n".join(f"#define R_{key.upper()} {n}" for key,n in b.items())
    return macros+ r"""
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static const nl_node *c_root,*a_root,*b_root;
static const nl_node *const *c_ptr;
static const nl_node *const *b_ptr;
static const nl_node_option *old_a,*old_b;
static size_t step;
static void req(int x){if(!x){fputs("R583 ORIGINAL PHYSICAL FAIL\n",stderr);abort();}}
static void opt(const nl_node_option *v,unsigned t,const nl_node *p)
    {req(v && v->tag==t && v->ptr==p);}
static void bind_original(size_t f,size_t s,size_t k,const void *addr) {
  req(!f && addr);
  switch(step){
    case 0: req(s==R_C && k==1);c_root=addr;
      req(c_root->f1==33);opt(&c_root->f0,0,NULL);break;
    case 1: req(s==R_CPTR && k==2);c_ptr=addr;req(*c_ptr==c_root);break;
    case 2: req(s==R_A && k==1);a_root=addr;
      req(a_root!=c_root && a_root->f1==11);opt(&a_root->f0,0,NULL);break;
    case 3: req(s==R_B && k==1);b_root=addr;
      req(b_root!=c_root && b_root!=a_root && b_root->f1==22);
      opt(&b_root->f0,0,NULL);break;
    case 4: req(s==R_BPTR && k==2);b_ptr=addr;req(*b_ptr==b_root);break;
    case 6: req(s==R_OLD_A && k==3);old_a=addr;
      req(old_a!=&a_root->f0);opt(old_a,0,NULL);break;
    case 8: req(s==R_OLD_B && k==3);old_b=addr;
      req(old_b!=&b_root->f0 && old_b!=old_a);opt(old_b,0,NULL);break;
    default:req(0);
  }++step;
}
static void replace_original(const nl_node *root,const nl_node_option *old){
  if(step==5){
    req(root==a_root && old!=&root->f0);
    opt(old,0,NULL);opt(&a_root->f0,1,b_root);opt(&b_root->f0,0,NULL);
  }else if(step==7){
    req(root==b_root && old!=&root->f0);
    opt(old,0,NULL);opt(&a_root->f0,1,b_root);opt(&b_root->f0,1,c_root);
  }else req(0);
  req(c_root->f1==33 && a_root->f1==11 && b_root->f1==22);
  ++step;
}
static void finish_original(void){
  req(step==9 && c_ptr && b_ptr && old_a && old_b);
  req(c_root!=a_root && a_root!=b_root && c_root!=b_root);
  req(*c_ptr==c_root && *b_ptr==b_root);
  opt(&c_root->f0,0,NULL);opt(&a_root->f0,1,b_root);
  opt(&b_root->f0,1,c_root);opt(old_a,0,NULL);opt(old_b,0,NULL);
  printf("R588 CALIBRATED ORIGINAL THREE LEXICAL NODES NATIVE PASS\n");
}
#define NL_NODE_BIND(f,s,k,p) bind_original(f,s,k,p)
#define NL_NODE_REPLACE(r,o) replace_original(r,o)
#define NL_NODE_FINISH() finish_original()
"""
def mutate(name,src):
    if name=="fourth_root":
        anchor="    let old_a=loan_write(a@next)"
        assert src.count(anchor)==1
        return src.replace(anchor,
             "    let d=Node{next:Option<ptr<Node>>::None,payload:u8(44)};\n"+anchor)
    if name=="third_change":
        anchor="    unit\n}"
        assert src.count(anchor)==1
        return src.replace(anchor,
            "    let old_c=loan_write(c@next){|w|replace(w,Option<ptr<Node>>::Some(bptr))};\n"+anchor)
    if name=="type_mismatch":
        anchor="replace(w,Option<ptr<Node>>::Some(cptr))"
        assert src.count(anchor)==1
        return src.replace(anchor,"replace(w,Option<u8>::None)")
    if name=="ptr_escalation":
        anchor="loan_write(b@next)"
        assert src.count(anchor)==1
        return src.replace(anchor,"loan_write_ptr(bptr)")
    if name=="unregistered_fragment":return "main()\n"
    raise ValueError(name)
def main():
    compiler,manifest_probe,probe=map(pathlib.Path,sys.argv[1:4])
    frozen=json.loads(ORIGINAL.read_text())
    assert frozen["fixed_candidate"]=="87fc878b52763858f69678a8350d8c1b66a061bb"
    original=frozen["positive_source"]
    assert hashlib.sha256(original.encode()).hexdigest()==(
        "a748ba8cd9f2819d8a473543bd79158c590bb42aefc89ee2bbefdfbf2146386f")
    calibrated=SOURCE.read_text()
    print("FROZEN ORIGINAL R580",hashlib.sha256(original.encode()).hexdigest(),
          "CALIBRATED SEPARATE",hashlib.sha256(calibrated.encode()).hexdigest(),
          flush=True)
    with tempfile.TemporaryDirectory(prefix="r583-") as d:
        p=pathlib.Path(d)
        old=p/"original_r580.nl";old.write_text(original)
        r=run(compiler,"--check",old,expect=0)
        assert not r.stdout and not r.stderr
        prior=run(probe,"observe",old,expect=0)
        assert prior.stdout.count("LOAN ordinal=")==2,prior.stdout
        print("ORIGINAL FLAG + PUBLIC OBSERVATION\n"+prior.stdout,flush=True)
        flags=[line for line in prior.stdout.splitlines() if line.startswith("LOAN")]
        assert all("from_ptr=0 nonescape=1 conflicting_access=0 implicit_local=1" in line
                   for line in flags),flags
        q=p/"calibrated.nl";q.write_text(calibrated)
        r=run(compiler,"--check",q,expect=0)
        assert not r.stdout and not r.stderr
        registered=run(manifest_probe,q,expect=0)
        assert not registered.stderr
        rows=[line.split() for line in registered.stdout.splitlines()]
        header=p/"passive.h";header.write_text(observer(rows))
        proof=run(probe,"certify",q,expect=0)
        assert "R583 REGISTERED CALIBRATED PUBLIC VALID" in proof.stdout
        print("CALIBRATED PROOF\n"+proof.stdout,flush=True)
        emitted=run(compiler,q,expect=0)
        assert not emitted.stderr and emitted.stdout.startswith("/* Bounded Checked-C")
        csha=hashlib.sha256(emitted.stdout.encode()).hexdigest()
        print("EMITTED STRICT C17 sha256",csha,"bytes",len(emitted.stdout.encode()),
              flush=True)
        for cc in ("gcc","clang"):
            for opt in ("-O0","-O2"):
                for passive in (False,True):
                    name=cc+opt+("_passive" if passive else "_plain")
                    c=p/(name+".c");c.write_text(emitted.stdout)
                    exe=p/name
                    args=[cc,"-std=c17","-Wall","-Wextra","-Wpedantic",
                          "-Werror",opt]
                    if passive:
                        args.append(f'-DNEWLANG_NODE_OBSERVER="{header}"')
                    build=run(*args,c,"-o",exe,expect=0)
                    assert not build.stderr
                    out=run(exe,expect=0)
                    assert not out.stderr
                    if passive:
                        assert out.stdout==(
                            "R588 CALIBRATED ORIGINAL THREE LEXICAL NODES NATIVE PASS\n")
                    else:assert not out.stdout
        for name in ("type_mismatch","ptr_escalation",
                     "unregistered_fragment","fourth_root","third_change"):
            q=p/(name+".nl");q.write_text(mutate(name,calibrated))
            check=run(compiler,"--check",q)
            emit=run(compiler,q)
            assert emit.returncode and not emit.stdout,(name,emit)
            if name in ("type_mismatch","ptr_escalation","unregistered_fragment"):
                assert check.returncode,(name,check)
            print("FAIL-CLOSED NEGATIVE",name,"CHECK",check.returncode,
                  "C17",emit.returncode,"C_bytes",len(emit.stdout),
                  flush=True)
        print("R583 CALIBRATED SOURCE/PUBLIC/STRICT-C17/NATIVE/NEGATIVES FINITE PASS",
              flush=True)
if __name__=="__main__": main()
