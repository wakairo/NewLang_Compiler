"""R740 calibrated replay: original R736 frozen bytes plus new R source. No P oracle."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

compiler, public, cc, sanitizer, root = sys.argv[1:]
root=Path(root)
expected={
"positive_direct_local_return.nl":"676f662afca5254e3399ebb1195d4fe75cefc239180fe8602590a8c01bb320ee",
"positive_custody_local_return.nl":"9f483e577ab20f84bbefa1bc5a21cb3c13f660ead45975a50ebbfc803280c41e",
"negative_move_twice.nl":"91c17d2c615d2b227d390fb83be3786e58fa21ab615e6ce1b628949d3d3c4c9c",
"negative_wrong_original.nl":"d1e84b069b52b01c7b74242e7f0c4820ba962a3a61b5ef08a4c3ffd5d38b40d2",
"calibrated_caller_inline.nl":"89ec73f7a2f145c57cdc1b5499f6035fec609671b760cbad90b8330defeadb80",
"calibrated_ctor_reordered_receiver.nl":"8cb6f92098df394d0b5a00a7164eba9bc0260f5a40b4c7ae188b6f6e95872eec",
"negative_wrong_attach_carrier.nl":"da862504c4d62c02bb526ed43d01208c5eb6654310aa6935e15e76654ffb435b",
"negative_uncleared_original_link.nl":"249a531683d2c189f3dd36b6fe2db84554324427349d6bed33d274d2a68f37bf",
"negative_double_detach_effect.nl":"cccb9fca9c9c29f6be8ed0bf7a539e7d49869262a4397842636ec82ef3fd2c78"
}
def call(args,**kw):
    return subprocess.run([str(x) for x in args],capture_output=True,text=True,**kw)
for name,sha in expected.items():
    data=(root/name).read_bytes()
    assert hashlib.sha256(data).hexdigest()==sha,(name,"historical SHA mismatch")
    assert data.endswith(b"\n") and b"\r" not in data
print("R740 SHA-gate original=4/4 C=1/1 D=1/1 negatives=3/3",flush=True)
with tempfile.TemporaryDirectory(prefix="r736-") as d:
    scratch=Path(d)
    conservative_refusals=[]
    for name in expected:
        path=root/name
        r=call([compiler,"--check",path])
        if name=="calibrated_caller_inline.nl":
            assert r.returncode==3 and not r.stdout and "P208-UNIT-PROFILE" in r.stderr, ("C HISTORICAL PRECISION REFUSAL",r.stderr)
            assert "distinct known terminal receiver" in r.stderr
            emitted=call([compiler,path])
            assert emitted.returncode!=0 and not emitted.stdout
            sink=scratch/"named.c"
            sink.write_bytes(b"R736_OLD_CONTENT\\n")
            refused=call([compiler,"--emit-c17-to",sink,path])
            assert refused.returncode!=0 and sink.read_bytes()==b"R736_OLD_CONTENT\\n"
            print("R740 C preserved profile refusal + no partial C + named sink unchanged",flush=True)
            continue
        if name in ("negative_wrong_attach_carrier.nl","negative_uncleared_original_link.nl","negative_double_detach_effect.nl"):
            assert r.returncode in (0,3) and not r.stdout, ("UNEXPECTED R SECURITY SOURCE STATUS",name,r.returncode,r.stderr)
            emitted=call([compiler,path])
            assert emitted.returncode!=0 and not emitted.stdout,("R SECURITY NEGATIVE EMITTED C / STOP",name,emitted.stderr)
            sink=scratch/"named.c"
            sink.write_bytes(b"R736_OLD_CONTENT\\n")
            refused=call([compiler,"--emit-c17-to",sink,path])
            assert refused.returncode!=0 and sink.read_bytes()==b"R736_OLD_CONTENT\\n"
            print("R740 NEW SECURITY NEGATIVE",name,"CHECK",r.returncode,"EMIT",emitted.returncode,
                  "diagnostic",r.stderr[:350] or emitted.stderr[:350],"zero_C=PASS",flush=True)
            continue
        negative=name.startswith("negative_") or name=="positive_custody_local_return.nl"
        print("R736 CHECK",name,"exit",r.returncode,"stderr",r.stderr[:700],flush=True)
        if negative:
            assert r.returncode==3 and not r.stdout,("UNEXPECTED SOURCE ADMISSION",name)
            if name=="positive_custody_local_return.nl":
                assert "AVS-DECL-PROFILE" in r.stderr,("B historical eligibility changed",r.stderr)
            elif name=="negative_move_twice.nl":
                assert "P193-DEFINITION-CONSUMED" in r.stderr
            elif name=="negative_wrong_original.nl":
                assert "P193-CALL-DOMAIN" in r.stderr
            emitted=call([compiler,path])
            assert emitted.returncode!=0 and not emitted.stdout
            sink=scratch/"named.c"
            sink.write_bytes(b"R736_OLD_CONTENT\n")
            refused=call([compiler,"--emit-c17-to",sink,path])
            assert refused.returncode!=0 and sink.read_bytes()==b"R736_OLD_CONTENT\n"
            print("R740 B/N fail-closed named-output preserved",name,flush=True)
            continue
        if r.returncode!=0:
            print("R736 SOURCE CHECK REFUSAL — separate precision/legality finding",name,r.stderr,flush=True)
            conservative_refusals.append((name,"SOURCE CHECK REFUSAL"))
            continue
        assert not r.stdout and not r.stderr
        observed=call([public,path])
        print("R736 PUBLIC",name,observed.returncode,observed.stdout[:1600],observed.stderr[:700],flush=True)
        assert observed.returncode==0,("SOURCE ISSUER/PUBLIC PROOF REFUSAL",name,observed.stderr)
        generated=call([compiler,path])
        if generated.returncode!=0:
            print("R736 BACKEND REFUSAL — no C/native claim",name,generated.stderr,flush=True)
            assert not generated.stdout, ("partial stdout C",name)
            conservative_refusals.append((name,"C17 BACKEND UNSUPPORTED"))
            continue
        assert generated.stdout and not generated.stderr
        assert 'int main(void)' in generated.stdout
        assert generated.stdout.count('malloc(')==2,("allocation emission sites",name)
        c=scratch/(name+".c")
        c.write_text(generated.stdout,encoding="utf-8")
        target=scratch/"named.c"
        target.write_bytes(b"R736_OLD_CONTENT\n")
        emitted=call([compiler,"--emit-c17-to",target,path])
        assert emitted.returncode==0 and target.read_text()==generated.stdout
        for level in ("-O0","-O2"):
            for observer in (False,True):
                exe=scratch/(name+level+str(observer))
                cmd=[cc,"-std=c17","-Wall","-Wextra","-Wpedantic","-Werror",
                     level,"-fno-builtin-malloc","-fno-builtin-free",str(c),
                     str(root/"platform.c"),"-Wl,--wrap=malloc","-Wl,--wrap=free",
                     "-o",str(exe)]
                if observer:
                    cmd.append('-DNEWLANG_HEAP_OBSERVER="'+str((root/"observer.h").resolve())+'"')
                else:
                    cmd.append("-DNEWLANG_UNOBSERVED")
                if sanitizer!="none":
                    cmd += ["-fsanitize="+sanitizer,"-fno-sanitize-recover=all","-fno-omit-frame-pointer","-fno-pie","-no-pie"]
                built=call(cmd)
                assert built.returncode==0,("STRICT-C17 FAILURE",name,level,observer,built.stderr)
                for failure,expect in (("1",0),("2",1),("",2)):
                    env=os.environ.copy()
                    env.pop("R736_FAIL_ALLOC",None)
                    if failure:env["R736_FAIL_ALLOC"]=failure
                    executed=call([exe],env=env)
                    print("R736 NATIVE",name,level,"observer",observer,"fail",failure or "none",
                          "exit",executed.returncode,executed.stdout.strip()[:300],executed.stderr[:700],flush=True)
                    assert executed.returncode==0 and not executed.stderr,("NATIVE CONTRADICTION",name,level,observer,failure,executed)
                    if observer:
                        m=re.search(r"R736_NATIVE allocations=(\d+) frees=(\d+) linked=(\d+) returned=(\d+) received=(\d+)",executed.stdout)
                        assert m and tuple(map(int,m.groups()))==(expect,expect,int(expect==2),int(expect==2),int(expect==2))
    if conservative_refusals:
        raise AssertionError(("R740 CALIBRATED ADMISSION GAP",conservative_refusals))
print("R740 A + calibrated D source-to-native PASS; C historical precision unsupported; B/N1/N2/N3/N4/N5 rejected",flush=True)
