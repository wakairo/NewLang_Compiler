"""Re-census and freeze finite ordinals, then test only changed call paths."""
from pathlib import Path
import argparse,hashlib,json,os,re,subprocess,sys,tempfile

here=Path(__file__).resolve().parent
p=argparse.ArgumentParser()
p.add_argument('binary',type=Path);p.add_argument('destination',type=Path)
p.add_argument('--audit-recorded',choices=['gcc','asan'])
args=p.parse_args()
sha=lambda b:hashlib.sha256(b).hexdigest()
env={**os.environ,'ASAN_OPTIONS':'detect_leaks=1:abort_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
total=0
for short,name in [('03','03-left-final-return'),('05','05-left-discard-normal')]:
    source=here/'sources'/f'{name}.nl'
    if args.audit_recorded:
        plan_dir=args.destination/f'oom-plan-{args.audit_recorded}-{short}-v1'
    else:
        args.destination.mkdir(parents=True,exist_ok=True)
        run=Path(tempfile.mkdtemp(prefix=f'{short}-',dir=args.destination))
        census=run/'census.log'
        with census.open('wb') as f:
            r=subprocess.run([args.binary,source,'census'],stdout=f,stderr=subprocess.STDOUT,env=env,timeout=120)
        assert r.returncode==0,'census failed; injection not started'
        plan_dir=run/'plan'
        r=subprocess.run([sys.executable,here/'select_oom.py',args.binary,census,plan_dir],capture_output=True)
        (run/'selection.out').write_bytes(r.stdout);(run/'selection.err').write_bytes(r.stderr)
        assert r.returncode==0,'selection failed; injection not started'
    plan=json.loads((plan_dir/'pre-injection-plan-v1.json').read_text())
    assert sha(args.binary.read_bytes())==plan['binary_sha256']
    for phase,meta in plan['stages'].items():
        points=plan_dir/f'{phase}.points'
        assert sha(points.read_bytes())==meta['points_sha256']
        expected=[int(x) for x in points.read_text().split()]
        if args.audit_recorded:
            log=args.destination/f'oom-{args.audit_recorded}-{short}-{phase}-FIRST-v1.log'
        else:
            log=run/f'{phase}.log'
            with log.open('wb') as f:
                r=subprocess.run([args.binary,source,phase,points],stdout=f,stderr=subprocess.STDOUT,env=env,timeout=180)
            assert r.returncode==0,(phase,'trial failure; stop')
        text=log.read_text()
        reached=[int(x) for x in re.findall(r'C263 reached stage='+phase+r' ordinal=(\d+) ',text)]
        assert reached==expected,(phase,len(reached),len(expected))
        assert f'C263 selected OOM stage={phase} trials={len(expected)} PASS' in text
        assert not re.search(r'ERROR: (?:AddressSanitizer|LeakSanitizer)|runtime error:',text)
        total+=len(expected)
        print(name,phase,'reached',len(expected),'PASS')
assert total<=1024,'finite cohort budget'
print('C263 focused selected OOM',total,'PASS', 'recorded execution audit' if args.audit_recorded else 'fresh injection')
