"""Actual whole CLI allocation failures and atomic named-output refusal."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

cli, probe, destination = sys.argv[1:]
here = Path(__file__).resolve().parent
source = here / "P-named-callers.nl"
parent = Path(destination)
parent.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix="failure-", dir=parent))
rows = []


def run(args, name, expected=0):
    p = subprocess.run(list(map(str,args)), capture_output=True)
    (out / (name+".stdout")).write_bytes(p.stdout)
    (out / (name+".stderr")).write_bytes(p.stderr)
    rows.append({"name":name,"exit":p.returncode})
    (out / "commands.json").write_text(json.dumps(rows,indent=2)+"\n")
    assert p.returncode == expected, (name,p.returncode,p.stderr,out)
    return p


sentinel = b"P1304 keep this original named output on error\n"
target = out / "preserved.c"


def prepare():
    target.write_bytes(sentinel)
    return target.stat()


def unchanged(before):
    after = target.stat()
    assert target.read_bytes() == sentinel
    assert (before.st_ino,before.st_size,before.st_mtime_ns) == \
        (after.st_ino,after.st_size,after.st_mtime_ns)
    assert sorted(p.name for p in out.iterdir() if ".tmp" in p.name) == []


for name, code in [("N-secondary-value-not-ref.nl","P3-TYPE-MISMATCH"),
                   ("N-primary-ref-escape.nl","P8-EXIT-DEPENDENCY")]:
    p = run([cli,"--check",here/name],name+"-CHECK",3)
    assert code.encode() in p.stderr and not p.stdout
    before = prepare()
    p = run([cli,"--emit-c17-to",target,here/name],name+"-named",3)
    assert code.encode() in p.stderr and not p.stdout
    unchanged(before)
baseline = run([cli,source],"baseline").stdout
p = run([probe,source,target,0],"census")
assert target.read_bytes() == baseline
m = re.search(rb"allocations=(\d+) fired=(\d+) cli-status=(-?\d+)",p.stdout)
assert m and m[2] == b"0" and m[3] == b"0"
count = int(m[1])
assert 0 < count <= 8192
selected = sorted(set(range(1,min(count,8)+1)) |
                  set(range(max(1,count-7),count+1)) | {max(1,count//2)})
assert len(selected) <= 17
plan = {"census":count,"selected":selected,
        "source_sha256":hashlib.sha256(source.read_bytes()).hexdigest(),
        "probe_sha256":hashlib.sha256(Path(probe).read_bytes()).hexdigest()}
(out / "plan.json").write_text(json.dumps(plan,indent=2)+"\n")
(out / "plan.json").chmod(0o444)
for ordinal in selected:
    before = prepare()
    p = run([probe,source,target,ordinal],"oom-"+str(ordinal))
    m = re.search(rb"allocations=(\d+) fired=(\d+) cli-status=(-?\d+)",p.stdout)
    assert m and m[2] == b"1" and int(m[3]) in (1,3)
    unchanged(before)
run([cli,"--emit-c17-to",target,source],"healthy-retry")
assert target.read_bytes() == baseline
print("P1304 named negative2/OOM",len(selected),"of",count,
      "actual fired/output rollback/no-clobber/byte retry PASS",out)
