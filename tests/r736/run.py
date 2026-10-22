"""R736 original SHA-gated CLI/public/native runner. No P source oracle import."""
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
"negative_wrong_original.nl":"d1e84b069b52b01c7b74242e7f0c4820ba962a3a61b5ef08a4c3ffd5d38b40d2"
}
def call(args,**kw):
    return subprocess.run([str(x) for x in args],capture_output=True,text=True,**kw)
for name,sha in expected.items():
    data=(root/name).read_bytes()
    assert hashlib.sha256(data).hexdigest()==sha,(name,"historical SHA mismatch")
    assert data.endswith(b"\n") and b"\r" not in data
print("R736 immutable oracle hashes=4/4",flush=True)
with tempfile.TemporaryDirectory(prefix="r736-") as d:
    scratch=Path(d)
    for name in expected:
        path=root/name
        r=call([compiler,"--check",path])
        negative=name.startswith("negative_")
        print("R736 CHECK",name,"exit",r.returncode,"stderr",r.stderr[:700],flush=True)
        if negative:
            assert r.returncode!=0 and not r.stdout,("UNEXPECTED SOURCE ADMISSION",name)
            emitted=call([compiler,path])
            assert emitted.returncode!=0 and not emitted.stdout
            sink=scratch/"named.c"
            sink.write_bytes(b"R736_OLD_CONTENT\n")
            refused=call([compiler,"--emit-c17-to",sink,path])
            assert refused.returncode!=0 and sink.read_bytes()==b"R736_OLD_CONTENT\n"
            print("R736 NEGATIVE fail-closed named-output preserved",name,flush=True)
            continue
        assert r.returncode==0 and not r.stdout and not r.stderr,("SOURCE-CHECK REFUSAL",name,r.stderr)
        observed=call([public,path])
        print("R736 PUBLIC",name,observed.returncode,observed.stdout[:1600],observed.stderr[:700],flush=True)
        assert observed.returncode==0,("SOURCE ISSUER/PUBLIC PROOF REFUSAL",name,observed.stderr)
        generated=call([compiler,path])
        assert generated.returncode==0 and generated.stdout and not generated.stderr,("BACKEND REFUSAL",name,generated.stderr)
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
print("R736 fully checked + native PASS for both frozen positive families and two negatives",flush=True)
