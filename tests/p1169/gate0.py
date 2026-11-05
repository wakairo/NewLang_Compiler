from pathlib import Path
import hashlib,json,subprocess
root=Path(__file__).resolve().parent
f=json.loads((root/'freeze.json').read_text())
for name,sha in f['files'].items():assert hashlib.sha256((root/name).read_bytes()).hexdigest()==sha,name
exe=Path('/tmp/p1169-baseline')
assert hashlib.sha256(exe.read_bytes()).hexdigest()==f['baseline_sha256']
raw=Path('/tmp/p1169-raw/gate0-v1');raw.mkdir(exist_ok=False)
rows=[]
oracle=json.loads((root/'oracle.json').read_text())
for p in sorted((root/'sources').glob('*.nl')):
    r=subprocess.run([exe,'--check',p],capture_output=True,timeout=60)
    (raw/(p.stem+'.out')).write_bytes(r.stdout)
    (raw/(p.stem+'.err')).write_bytes(r.stderr)
    row={'case':p.stem,'command':[str(exe),'--check',str(p)],'status':r.returncode,'stdout_sha256':hashlib.sha256(r.stdout).hexdigest(),'stderr_sha256':hashlib.sha256(r.stderr).hexdigest()}
    rows.append(row);(raw/'observations.json').write_text(json.dumps(rows,indent=2)+'\n')
    if r.returncode and not (raw/'FIRST.json').exists():(raw/'FIRST.json').write_text(json.dumps(row,indent=2)+'\n')
    print(p.stem,r.returncode,r.stderr.decode().strip(),flush=True)
    expected=oracle['cases'][p.stem]['category']=='control'
    if (r.returncode==0)!=expected:raise SystemExit('Unexpected baseline CHECK: downstream STOP')
    if expected:
        c=subprocess.run([exe,p],capture_output=True,timeout=60)
        (raw/(p.stem+'.c17')).write_bytes(c.stdout)
        (raw/(p.stem+'-emit.err')).write_bytes(c.stderr)
        assert c.returncode==0 and not c.stderr
