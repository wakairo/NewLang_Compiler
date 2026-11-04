from pathlib import Path
import hashlib,json,subprocess,sys,tempfile
p=Path(__file__).resolve().parent
compiler,gcc,clang,out=sys.argv[1:]
for name,expected in json.loads((p/'freeze.json').read_bytes()).items():
    assert hashlib.sha256((p/name).read_bytes()).hexdigest()==expected,name
Path(out).mkdir(parents=True,exist_ok=True)
raw=Path(tempfile.mkdtemp(prefix='bounds-',dir=out));rows=[]
def run(command,label):
    r=subprocess.run(list(map(str,command)),capture_output=True,timeout=60)
    (raw/(label+'.out')).write_bytes(r.stdout);(raw/(label+'.err')).write_bytes(r.stderr)
    rows.append(dict(name=label,status=r.returncode,stderr=r.stderr.decode()))
    (raw/'observations.json').write_text(json.dumps(rows,indent=2)+'\n')
    print(label,r.returncode,flush=True);return r
for name,e in json.loads((p/'oracle.json').read_bytes()).items():
    source=p/(name+'.nl')
    r=run([compiler,'--check',source],name+'-check');assert r.returncode==e['check'] and not r.stdout
    sentinel=b'C222 bounds original named bytes\x00\xff\n';c=raw/(name+'.c');c.write_bytes(sentinel);before=c.stat()
    r=run([compiler,'--emit-c17-to',c,source],name+'-emit');assert r.returncode==e['emit'] and not r.stdout
    if e['emit']:
        assert c.read_bytes()==sentinel and (before.st_ino,before.st_mtime_ns)==(c.stat().st_ino,c.stat().st_mtime_ns)
        if name=='05-ending-loan':assert b'LOAN' in r.stderr or b'DEPENDEN' in r.stderr
    else:
        for cc in dict.fromkeys([gcc,clang]):
            for opt in ['-O0','-O2']:
                r=run([cc,'-std=c17','-Wall','-Wextra','-Wpedantic','-Werror',opt,c,'-o',raw/'not-executed'],name+'-'+Path(cc).name+opt)
                assert not r.returncode,r.stderr
        assert e['native'] is False
print('C222 zero-guard/all-H unsafe-first-break/ending dependency/static infinite boundaries PASS; infinite native NOT RUN')
