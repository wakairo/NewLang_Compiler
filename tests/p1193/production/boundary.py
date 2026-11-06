"""Named-output preservation at the actual source boundary; no guest."""
from pathlib import Path
import hashlib,json,subprocess,sys,tempfile
here=Path(__file__).resolve().parent;repo=here.parents[2]
compiler,observer,name=sys.argv[1:]
freeze=json.loads((here/'validation-freeze-v1.json').read_text())
for path,digest in freeze['files'].items():assert hashlib.sha256((repo/path).read_bytes()).hexdigest()==digest,path
source=here/(name+'.nl') if name.startswith('95-') else here.parent/'c256/sources'/(name+'.nl')
expected=json.loads((here/'boundaries.json').read_text())['cases'][name]
with tempfile.TemporaryDirectory(prefix='p1193-boundary-') as d:
 target=Path(d)/'out.c';target.write_bytes(b'C256 old named output\n');before=(target.read_bytes(),target.stat().st_ino,target.stat().st_mtime_ns)
 for args in [[compiler,'--check',source],[compiler,source],[compiler,'--emit-c17-to',target,source]]:
  r=subprocess.run(list(map(str,args)),capture_output=True);print(r.stderr.decode(),end='')
  assert r.returncode==3 and not r.stdout and expected in r.stderr.decode()
 assert before==(target.read_bytes(),target.stat().st_ino,target.stat().st_mtime_ns)
 if name.startswith('95-'):
  r=subprocess.run([observer,str(source),'0','61','3','193'],capture_output=True)
  print(r.stdout.decode(),end='');assert r.returncode==0
  assert r.stdout.count(b'call_terminates=0 call_results=1 known=0 value=0')==2
  assert b'ownership=NOT_ISSUED rollback=YES' in r.stdout
print('C256 actual boundary and named no-clobber PASS',name)
