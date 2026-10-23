#!/usr/bin/env python3
"""R #780 byte-frozen independent negative family + R-B fail-closed.
Reports first CLI rejection phase/code without relabeling source ineligibility.
"""
import hashlib
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path
cli, root = sys.argv[1:]
root=Path(root)
frozen=root/"tests/r776/frozen"
manifest=json.loads((root/"tests/r780/frozen/manifest.json").read_text())
oracle=json.loads((frozen/"oracle.json").read_text())
def insist(flag,why):
    if not flag:raise AssertionError(why)
def cmd(*args):
    return subprocess.run([str(x) for x in args],capture_output=True,timeout=30)
def diag(p):
    s=p.stderr.decode(errors="replace")
    pattern=re.search(r"error\(([^)]*)\)\[([^\]]*)\]",s)
    return (pattern.group(1),pattern.group(2)) if pattern else ("UNCLASSIFIED",s[:240])
insist(hashlib.sha256((frozen/"oracle.json").read_bytes()).hexdigest()==
       manifest["frozen_original_oracle_sha256"],"immutable oracle mismatch")
for rel,digest in oracle["source_sha256"].items():
    insist(hashlib.sha256((root/rel).read_bytes()).hexdigest()==digest,
           "immutable R776 source "+rel)
with tempfile.TemporaryDirectory(prefix="r780-gates-") as td:
    td=Path(td)
    negative=(
      "negative_wrong_allocation.nl",
      "negative_stale_link.nl",
      "negative_unknown_field.nl",
      "negative_read_to_write.nl",
      "negative_two_t_nominals.nl",
      "negative_double_consume.nl",
    )
    stages=[]
    for name in negative:
        src=frozen/name
        c=cmd(cli,"--check",src)
        out=cmd(cli,src)
        named=td/(name+".c")
        marker=b"R780-PRESERVE-OLD-C17-BYTES"
        named.write_bytes(marker)
        saved=cmd(cli,"--emit-c17-to",named,src)
        insist(c.returncode!=0 and out.returncode!=0 and saved.returncode!=0 and
               not c.stdout and not out.stdout and not saved.stdout and
               named.read_bytes()==marker,"unsafe negative admission/dirty output: "+name)
        insist(c.stderr and out.stderr and saved.stderr,
               "missing diagnostics for "+name)
        first=diag(c);stages.append((name,first))
        print(json.dumps({"case":name,"first_check_rc":c.returncode,
          "first_layer":first[0],"first_code":first[1],
          "emit_rc":out.returncode,"emit_layer_code":diag(out),
          "named_rc":saved.returncode,"named_sentinel":"UNCHANGED"}),flush=True)
    b=frozen/"positive_t_interposed.nl"
    c=cmd(cli,"--check",b)
    insist(c.returncode==0 and not c.stdout and not c.stderr,"original R-B CHECK")
    out=cmd(cli,b)
    marker=b"R780-B-MUST-REMAIN-UNCHANGED"
    named=td/"B-existing.c"
    named.write_bytes(marker)
    saved=cmd(cli,"--emit-c17-to",named,b)
    insist(out.returncode==4 and saved.returncode==4 and
       not out.stdout and not saved.stdout and named.read_bytes()==marker and
       "V1-BACKEND-UNSUPPORTED" in out.stderr.decode(errors="replace"),
       "R-B mistakenly accepted or named target changed")
    print("R780_B CHECK0, emitter CLI4, zero stdout C, unchanged named sentinel; "
          "public Copy precision 3 independently verified by r780_public_provenance.unit",
          flush=True)
print("R780 negative families=6; actual first-stage diagnostic printed; "
      "valid B fail-closed, native B NOT TESTED",flush=True)
