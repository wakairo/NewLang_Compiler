"""Source-first finite P1208 CHECK/owned/current/C17 gates; guest execution 0."""
from pathlib import Path
import hashlib,json,subprocess,sys

compiler,roles,census,dest=sys.argv[1:]
here=Path(__file__).resolve().parent
raw=Path(dest);raw.mkdir(parents=True,exist_ok=False)
for version in (1,2):
    freeze=json.loads((here/f'source-freeze-v{version}.json').read_text())
    for name,digest in freeze['files'].items():
        assert hashlib.sha256((here/name).read_bytes()).hexdigest()==digest,name
oracle=json.loads((here/'oracle-v2.json').read_text())
rows=[]
def run(args,label,ok):
    r=subprocess.run(list(map(str,args)),capture_output=True,timeout=120)
    (raw/(label+'.out')).write_bytes(r.stdout)
    (raw/(label+'.err')).write_bytes(r.stderr)
    rows.append({'label':label,'args':list(map(str,args)),'status':r.returncode,
        'stdout_bytes':len(r.stdout),'stdout_sha256':hashlib.sha256(r.stdout).hexdigest(),
        'stderr_sha256':hashlib.sha256(r.stderr).hexdigest()})
    (raw/'observations.json').write_text(json.dumps(rows,indent=2)+'\n')
    assert (r.returncode==0)==ok,rows[-1]
    if not ok:assert not r.stdout,rows[-1]
    return r

for name,meta in oracle['profiles'].items():
    source=here/'sources'/f'{name}.nl'
    positive=meta['candidate_check']==0
    run([compiler,'--check',source],name+'-check',positive)
    c=run([compiler,source],name+'-c17',positive)
    if not positive:
        print(name,'refused',flush=True)
        continue
    if meta['preserve_C17_bytes']:
        old=Path('/tmp/p1208-records/gate0')/(name+'-c17.out')
        assert c.stdout==old.read_bytes(),(name,'old C17 bytes changed')
    proof=run([roles,source],name+'-owned-current',True)
    assert b'public-body=YES private-current=YES canonical-root=YES' in proof.stdout
    measured=run([census,source],name+'-census',True)
    assert f'private_return_records={meta["return_records"]}'.encode() in measured.stdout
    assert b'stage=destroyed' in measured.stdout
    print(name,'CHECK/owned/current/C17/census PASS',flush=True)
print('P1208 first source-authenticated gates PASS; generated native0',flush=True)
