from pathlib import Path
import json,subprocess,sys,tempfile

here=Path(__file__).resolve().parent
compiler=Path(sys.argv[1])
oracle=json.loads((here/'oracle-v2.json').read_text())
for name,meta in oracle['profiles'].items():
    if meta['candidate_check']==0:continue
    source=here/'sources'/f'{name}.nl'
    with tempfile.TemporaryDirectory(prefix='p1208-refusal-') as directory:
        target=Path(directory)/'out.c'
        target.write_bytes(b'P1208 named original\n')
        before=target.stat()
        r=subprocess.run([compiler,'--emit-c17-to',target,source],capture_output=True,timeout=60)
        assert r.returncode==3 and not r.stdout,(name,r.returncode,r.stderr)
        after=target.stat()
        assert (after.st_ino,after.st_mtime_ns,after.st_size)==(before.st_ino,before.st_mtime_ns,before.st_size)
        assert target.read_bytes()==b'P1208 named original\n'
        assert list(Path(directory).iterdir())==[target]
        print(name,'CHECK refusal/no-clobber/temporary PASS')
