#!/usr/bin/env python3
"""R592 independently frozen-source CLI CHECK -> C17 native phase probe."""
import hashlib, pathlib, re, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[3]
CC = ROOT / "build-r592" / "newlangc"
TESTS = [
 ("P01_literal_23", True, "fn main() -> unit {\n    let result = usize(2) + usize(3);\n    unit\n}\n"),
 ("P02_distinct_names", True, "fn main() -> unit {\n    let alpha = usize(37);\n    let beta = usize(59);\n    let answer = alpha + beta;\n    unit\n}\n"),
 ("P03_zero", True, "fn main() -> unit {\n    let a = usize(0);\n    let b = usize(0);\n    let result = a + b;\n    unit\n}\n"),
 ("P04_near_max_64", True, "fn main() -> unit {\n    let large = usize(18446744073709551608);\n    let small = usize(6);\n    let result = large + small;\n    unit\n}\n"),
 ("N01_overflow_64", False, "fn main() -> unit {\n    let first = usize(18446744073709551615);\n    let second = usize(1);\n    let result = first + second;\n    unit\n}\n"),
 ("N02_literal_out_of_range_64", False, "fn main() -> unit {\n    let value = usize(18446744073709551616);\n    let result = value + usize(0);\n    unit\n}\n"),
 ("N03_type_u8_plus_usize", False, "fn main() -> unit {\n    let result = u8(7) + usize(1);\n    unit\n}\n"),
 ("N04_unknown_formal", False, "fn add_one(value: usize) -> usize {\n    let answer = value + usize(1);\n    answer\n}\nfn main() -> unit {\n    unit\n}\n"),
 ("N05_chain", False, "fn main() -> unit {\n    let answer = usize(1) + usize(2) + usize(3);\n    unit\n}\n"),
 ("N06_parenthesized", False, "fn main() -> unit {\n    let answer = (usize(1) + usize(2));\n    unit\n}\n"),
 ("N07_mixed_comparison", False, "fn main() -> unit {\n    let answer = usize(1) + usize(2) == usize(3);\n    unit\n}\n"),
 ("N08_bare_literal", False, "fn main() -> unit {\n    let answer = 2 + usize(3);\n    unit\n}\n"),
 ("E01_existing_equality", True, "fn main() -> unit {\n    let left = usize(13);\n    let same = usize(13);\n    let different = usize(14);\n    let eq_true = left == same;\n    let eq_false = left == different;\n    unit\n}\n")
]
def command(args):
    return subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=50)
def run():
    count=0
    with tempfile.TemporaryDirectory(prefix="r592-") as tmp:
        d=pathlib.Path(tmp)
        for case,expected,src in TESTS:
            p=d/(case+".nl");p.write_text(src)
            digest=hashlib.sha256(src.encode()).hexdigest()
            a=command([str(CC),"--check",str(p)])
            b=command([str(CC),str(p)])
            ok=(a.returncode==0 and b.returncode==0) if expected else (a.returncode!=0 and b.returncode!=0 and b.stdout==b"")
            print("CASE",case,"sha256",digest,"CHECK",a.returncode,"C17",b.returncode,"C_BYTES",len(b.stdout),"phase_stderr",a.stderr.decode(errors="replace")[:450].replace("\n"," | "),"prediction_match",ok,flush=True)
            if not ok:
                print("BLIND_FINDING: original prediction mismatch; STOP; no calibrated repair",flush=True)
                return 1
            if expected:
                c=b.stdout
                (d/(case+".c")).write_bytes(c)
                if b"SIZE_MAX" in c: print("GUARD_SIZE_MAX",case,flush=True)
                if b"CHAR_BIT" in c: print("GUARD_CHAR_BIT",case,flush=True)
                print("C_SHA256",case,hashlib.sha256(c).hexdigest(),flush=True)
                for compiler in ["gcc","clang"]:
                    for opt in ["0","2"]:
                        exe=d/(case+"-"+compiler+opt)
                        comp=command([compiler,"-std=c17","-Wall","-Wextra","-Werror","-O"+opt,str(d/(case+".c")),"-o",str(exe)])
                        if comp.returncode:
                            print("BLIND_FINDING C17",case,compiler,opt,comp.stderr.decode(errors="replace")[:1000],flush=True)
                            return 2
                        native=command([str(exe)])
                        if native.returncode:
                            print("BLIND_FINDING native",case,compiler,opt,native.returncode,native.stderr.decode(errors="replace")[:500],flush=True)
                            return 3
                        count+=1
        print("FIRST_BARRIER_CLI_NATIVE_SUCCESS: cases",len(TESTS),"native_executions",count,"BUT no passive live-carrier observer or public proof validated by this script",flush=True)
    return 0
if __name__=="__main__":
    sys.exit(run())
