from pathlib import Path
import hashlib,json,subprocess,sys
p=Path(__file__).resolve().parent
compiler=Path(sys.argv[1]);raw=Path(sys.argv[2]);raw.mkdir(exist_ok=False,parents=True)
oracle=json.loads((p/'oracle.json').read_text())
version=json.loads((p/'oracle-v2.json').read_text())
rows=[]
for name,meta in oracle['cases'].items():
    source=p/version['source_overrides'].get(name,'sources/'+name+'.nl')
    for stage,command in [('check',[compiler,'--check',source]),('emit',[compiler,source])]:
        r=subprocess.run(list(map(str,command)),capture_output=True,timeout=60)
        (raw/(name+'-'+stage+'.out')).write_bytes(r.stdout)
        (raw/(name+'-'+stage+'.err')).write_bytes(r.stderr)
        row=dict(case=name,stage=stage,command=list(map(str,command)),status=r.returncode)
        rows.append(row);(raw/'observations.json').write_text(json.dumps(rows,indent=2)+'\n')
        print(name,stage,r.returncode,flush=True)
        if r.returncode!=meta[stage]:
            (raw/'FIRST.json').write_text(json.dumps(row,indent=2)+'\n')
            raise AssertionError(row)
        if stage=='check':
            assert not r.stdout
            if name in version['expected_diagnostics']:assert version['expected_diagnostics'][name].encode() in r.stderr
        elif meta['category']=='control':assert r.stdout==(p/'evidence'/(name+'.c17')).read_bytes()
        elif r.returncode:assert not r.stdout
print('C245 finite source/emit/control cohort PASS')
