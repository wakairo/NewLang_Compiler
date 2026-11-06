"""Rebind real proof and C17 artifacts after the normal-result Copy repair.
No new guest is executed: phase1 already used its forty invocations.
"""
from pathlib import Path
import hashlib,json,subprocess,sys
p=Path(__file__).resolve().parent;repo=p.parents[2]
cc,baseline,roles,census,challenge,destination=map(Path,sys.argv[1:])
dest=destination;dest.mkdir(exist_ok=False,parents=True)
f=json.loads((p/'post-repair-freeze-v1.json').read_text())
for name,digest in f['files'].items():assert hashlib.sha256((repo/name).read_bytes()).hexdigest()==digest
old=json.loads((p/'old-oracle-v1.json').read_text());rows=[]
def run(args,label,status=0):
 r=subprocess.run(list(map(str,args)),capture_output=True,timeout=180)
 (dest/(label+'.out')).write_bytes(r.stdout);(dest/(label+'.err')).write_bytes(r.stderr)
 rows.append(dict(label=label,status=r.returncode,expected=status,args=list(map(str,args))))
 (dest/'observations.json').write_text(json.dumps(rows,indent=2)+'\n')
 assert r.returncode==status,rows[-1]
 return r
for name,m in old['cases'].items():
 source=p/'sources'/(name+'.nl');invalid=m['classification']=='invalid';status=3 if invalid else 0
 run([cc,'--check',source],name+'-check',status)
 c=run([cc,source],name+'-emit',status)
 target=dest/(name+'.named.c');target.write_bytes(b'C256 final sentinel\n');before=(target.read_bytes(),target.stat().st_ino,target.stat().st_mtime_ns)
 run([cc,'--emit-c17-to',target,source],name+'-named',status)
 if invalid:
  assert not c.stdout and before==(target.read_bytes(),target.stat().st_ino,target.stat().st_mtime_ns)
 else:
  assert c.stdout==target.read_bytes()
  executed=Path('/workspace/p1193-c256-checkpoint/native-v3')/(name+'.c')
  assert c.stdout==executed.read_bytes(),('native artifact differs',name)
  run([roles,source],name+'-owned-current')
  run([census,source],name+'-fresh-census')
for name in ['05-all-two','06-eq-two']:
 source=repo/'tests/p1188/sources'/(name+'.nl')
 run([baseline,'--check',source],name+'-baseline-check')
 before=run([baseline,source],name+'-baseline-emit')
 run([cc,'--check',source],name+'-check')
 after=run([cc,source],name+'-emit');assert before.stdout==after.stdout
 run([roles,source],name+'-owned-current')
run([challenge,p/'sources'],'same-twelve-challenges')
print('C256 post-repair all7 CHECK/C17/current, original two-Copy LT/EQ byte controls, same12 authority challenges and fresh5 census PASS; native artifact identity5/5; additional native0')
