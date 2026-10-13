#!/usr/bin/env python3
"""R406 independent exploratory harness. No P fixtures or golden data."""
import hashlib, pathlib, subprocess, sys, tempfile
compiler, cc, sanitizer = sys.argv[1:]
programs = {
    "single": """fn copper(seed:u8)->u8 { seed }
fn main()->unit { let observed = copper(u8(255)); return unit; }
""",
    "two_calls": """fn marsh(y:u8)->u8 { y }
fn main()->unit { let beginning = marsh(u8(0)); let ending = marsh(u8(173)); return unit; }
""",
    "forwarded": """fn rivulet(k:u8)->u8 { k }
fn main()->unit { let first = rivulet(u8(007)); let next = rivulet(first); return unit; }
""",
}
def run(args, cwd):
    r=subprocess.run(args, cwd=cwd, text=True, capture_output=True, timeout=25)
    print("COMMAND", repr(args), "EXIT", r.returncode)
    print("STDOUT", repr(r.stdout[:3500]))
    print("STDERR", repr(r.stderr[:3500]))
    return r
with tempfile.TemporaryDirectory(prefix="r406-blind-") as t:
    p=pathlib.Path(t)
    for label, content in programs.items():
        q=p/(label+".nl")
        q.write_text(content)
        print("R406 SOURCE",label,"sha256",hashlib.sha256(content.encode()).hexdigest(),"text",repr(content))
    run([compiler,"--help"],p)
    source=p/"single.nl"
    guesses=[
        [compiler,str(source)],
        [compiler,"--emit-c",str(source)],
        [compiler,"--emit-checked-c",str(source)],
        [compiler,"--emit-c17",str(source)],
        [compiler,"--emit-checked-c",str(source),"-o",str(p/"probe.c")],
        [compiler,"--emit-c",str(source),"-o",str(p/"probe.c")],
    ]
    for cmd in guesses:
        run(cmd,p)
        print("FILES",[(a.name,a.stat().st_size) for a in sorted(p.iterdir())])
        for f in p.glob("*.c"):
            print("C17",f.name,f.read_text(errors="replace")[:10000])
    print("R406 EXPLORATION ONLY; native/negative not yet observed")
    sys.exit(1)
