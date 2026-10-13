#!/usr/bin/env python3
"""R406 independently authored BLIND C17/native test. Never use P fixtures."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

compiler, cc, sanitizer = sys.argv[1:]
PROGRAMS = {
 "single": ("fn copper(seed:u8)->u8 { seed }\nfn main()->unit { let observed = copper(u8(255)); return unit; }\n",[255]),
 "two_calls": ("fn marsh(y:u8)->u8 { y }\nfn main()->unit { let beginning = marsh(u8(0)); let ending = marsh(u8(173)); return unit; }\n",[0,173]),
 "forwarded": ("fn rivulet(k:u8)->u8 { k }\nfn main()->unit { let first = rivulet(u8(007)); let next = rivulet(first); return unit; }\n",[7,7]),
}
NEGATIVES = {
 "wrong_type": "fn garnet(r:u8)->u8 { r }\nfn main()->unit { let n=garnet(unit); return unit; }\n",
 "wrong_arity": "fn jasper(r:u8)->u8 { r }\nfn main()->unit { let n=jasper(); return unit; }\n",
 "out_of_range": "fn pebble(r:u8)->u8 { r }\nfn main()->unit { let n=pebble(u8(256)); return unit; }\n",
 "repeat_mismatch": "fn cloud(r:u8)->u8 { r }\nfn main()->unit { let one=cloud(u8(3)); let two=cloud(unit); return unit; }\n",
 "richer_body": "fn alder(r:u8)->u8 { r + u8(1) }\nfn main()->unit { let n=alder(u8(2)); return unit; }\n",
}
FUNCTION = re.compile(r"(?m)^(static uint8_t (?P<fn>nl_fn_\d+)\(uint8_t (?P<arg>nl_local_\d+)\)\s*\n\{)")
CALL = re.compile(r"(?m)^(?P<indent>[ \t]*)const uint8_t (?P<dst>nl_call_\d+) = (?P<fn>nl_fn_\d+)\((?P<actual>[^;\n]*)\);[ \t]*$")
RETURN = re.compile(r"\breturn (nl_local_\d+);")
transcript=[]

def note(s):
 print(s,flush=True)
 transcript.append(s)

def digest(b):
 return hashlib.sha256(b).hexdigest()

def run(args,cwd):
 return subprocess.run(args,cwd=cwd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=45)

def describe(label,source):
 b=source.encode()
 note("SOURCE "+label+" sha256="+digest(b)+" bytes="+str(len(b))+" text="+repr(source))
 return b

def compile_c(path,out,cwd):
 cmd=[cc,"-std=c17","-Wall","-Wextra","-Wpedantic","-Werror"]
 if sanitizer in ("address","undefined"):
  cmd+=["-fsanitize="+sanitizer,"-fno-omit-frame-pointer"]
 cmd +=[str(path),"-o",str(out)]
 r=run(cmd,cwd)
 note("STRICT_C17 "+repr(cmd)+" exit="+str(r.returncode)+" stderr="+repr(r.stderr.decode(errors="replace")[:2000]))
 assert r.returncode==0,"strict C17 compile failed "+path.name

def execute(exe,cwd):
 r=run([str(exe)],cwd)
 out=r.stdout.decode(errors="replace")
 note("NATIVE "+exe.name+" exit="+str(r.returncode)+" stdout="+repr(out)+" stderr="+repr(r.stderr.decode(errors="replace")[:2000]))
 assert r.returncode==0,"native process failed "+exe.name
 return out

def observer(raw):
 c=raw.decode("utf-8")
 defs=list(FUNCTION.finditer(c))
 calls=list(CALL.finditer(c))
 note("EMITTED_FUNCTIONS "+repr([(m.group("fn"),m.group("arg")) for m in defs]))
 note("EMITTED_CALLS "+repr([(m.group("fn"),m.group("actual"),m.group("dst")) for m in calls]))
 assert len(defs)==1,"emitted u8 function count !=1"
 assert calls,"emitted typed calls not observable"
 def on_function(m):
  return m.group(1)+'\n    fprintf(stdout, "R406_IN %u\\n", (unsigned)'+m.group("arg")+');'
 c,n=FUNCTION.subn(on_function,c)
 assert n==1
 def on_call(m):
  return m.group(0)+"\n"+m.group("indent")+'fprintf(stdout, "R406_OUT %u\\n", (unsigned)'+m.group("dst")+');'
 c,n=CALL.subn(on_call,c)
 assert n==len(calls)
 return "#include <stdio.h>\n"+c,len(calls)

def trace(output):
 a=[]
 for s in output.splitlines():
  m=re.fullmatch(r"R406_(IN|OUT) (\d+)",s)
  assert m,"unexpected observer line "+repr(s)
  a.append((m.group(1),int(m.group(2))))
 return a

def expected_trace(values):
 return [(k,v) for v in values for k in ("IN","OUT")]

def summary():
 path=os.getenv("GITHUB_STEP_SUMMARY")
 if path:
  with open(path,"a",encoding="utf-8") as f:
   f.write("\n### R406 fresh BLIND native transcript\n\n")
   f.write("Tested candidate: bb6d8b58447b7d61f5fe2243e430a9aa1ef4d8f2\n\n")
   f.write("~~~text\n"+"\n".join(transcript).replace("~~~","___")+"\n~~~\n")

def main():
 note("R406 COMPILER="+compiler+" CC="+cc+" SANITIZER="+sanitizer)
 with tempfile.TemporaryDirectory(prefix="r406-native-") as temp:
  d=Path(temp)
  for name,(source,values) in PROGRAMS.items():
   p=d/(name+".nl")
   p.write_bytes(describe(name,source))
   r=run([compiler,str(p)],d)
   note("SOURCE_CHECK "+name+" exit="+str(r.returncode)+" c17_length="+str(len(r.stdout))+" stderr="+repr(r.stderr.decode(errors="replace")))
   assert r.returncode==0 and r.stdout,"positive SOURCE/CHECK failed "+name
   note("C17_BYTES "+name+" sha256="+digest(r.stdout)+" size="+str(len(r.stdout)))
   note("C17_TEXT "+name+" "+repr(r.stdout.decode("utf-8")[:8000]))
   original=d/(name+".c")
   original.write_bytes(r.stdout)
   compile_c(original,d/(name+"_raw"),d)
   plain=execute(d/(name+"_raw"),d)
   note("PLAIN_GUEST "+name+" stdout="+repr(plain)+" (no value ABI inferred)")
   annotated,n=observer(r.stdout)
   assert n==len(values),"wrong emitted typed call count "+name
   c=d/(name+"_observed.c")
   c.write_text(annotated)
   compile_c(c,d/(name+"_observed"),d)
   seen=trace(execute(d/(name+"_observed"),d))
   note("OBSERVED "+name+" "+repr(seen)+" expected="+repr(expected_trace(values)))
   assert seen==expected_trace(values),"actual/formal/result/receiver mismatch "+name
   if name=="single":
    mutated,count=RETURN.subn("return 0;",annotated,count=1)
    assert count==1,"no typed emitted return for controlled freeze"
    p=d/"frozen.c"
    p.write_text(mutated)
    compile_c(p,d/"frozen",d)
    poisoned=trace(execute(d/"frozen",d))
    note("MUTATED_RETURN "+repr(poisoned)+" baseline="+repr(seen))
    assert poisoned==[("IN",255),("OUT",0)] and poisoned!=seen,"failed to detect frozen return"
  for name,source in NEGATIVES.items():
   p=d/("negative_"+name+".nl")
   p.write_bytes(describe("negative_"+name,source))
   r=run([compiler,str(p)],d)
   err=r.stderr.decode(errors="replace")
   note("NEGATIVE "+name+" exit="+str(r.returncode)+" c17_bytes="+str(len(r.stdout))+" diagnostic="+repr(err[:2000]))
   assert r.returncode!=0 and not r.stdout and err.strip(),"negative admission/partial output "+name
  note("R406 PASS BOUNDED: strict C17 native values, freeze detection, negative no partial C")
 summary()

if __name__=="__main__":
 try: main()
 except Exception as e:
  note("R406 FAILURE "+repr(e))
  summary()
  raise
