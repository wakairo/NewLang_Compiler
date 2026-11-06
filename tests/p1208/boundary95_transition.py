"""Prospective supported version of P's immutable historical boundary95.

The original source, refusal oracle, checker and actual prior raw stay exact.
New semantics are not issued here: use actual CHECK/owned/current/C17 APIs.
"""
from pathlib import Path
import hashlib,json,subprocess,sys,tempfile,zipfile
here=Path(__file__).resolve().parent;repo=here.parents[1]
compiler,observer,name=sys.argv[1:]
assert name=='95-mixed-result-binding'
source=repo/'tests/p1193/production'/f'{name}.nl'
assert hashlib.sha256(source.read_bytes()).hexdigest()=='f3b329eefb5e989815dcf8b79eb019df60328577653c9d6797c8803f07da3c57'
old=repo/'tests/p1193/production'
for p,digest in [('boundary.py','2768f2e913f1dc5badbd384f19ed0d037da8873537f0d5118eb59c1a8aeed540'),('boundaries.json','1c9aadd319cd5705fb49c630de181bdc44533821b0920172ea4ad432bf20b882')]:
    assert hashlib.sha256((old/p).read_bytes()).hexdigest()==digest
assert json.loads((old/'boundaries.json').read_text())['cases'][name]=='P14-LOOP-PRECISION'
receipt=json.loads((here/'evidence-receipt-v1.json').read_text())
archive=repo/receipt['archive']
assert hashlib.sha256(archive.read_bytes()).hexdigest()==receipt['sha256']
with zipfile.ZipFile(archive) as z:
    raw=z.read('prior/preserved-95-P1193-prior-LastTest-section.log')
    assert raw.count(b'error(precision)[P14-LOOP-PRECISION]')==3
    assert b'ownership=NOT_ISSUED rollback=YES' in raw
    assert z.read('prior/boundaries.json')==(old/'boundaries.json').read_bytes()
    assert z.read('prior/95-mixed-result-binding.nl')==source.read_bytes()
with tempfile.TemporaryDirectory(prefix='p1208-transition95-') as d:
    target=Path(d)/'out.c';target.write_bytes(b'prospective positive target\n')
    results=[]
    for args in ([compiler,'--check',source],[compiler,source],[compiler,'--emit-c17-to',target,source]):
        r=subprocess.run(list(map(str,args)),capture_output=True,timeout=60)
        assert r.returncode==0,(args,r.returncode,r.stderr)
        results.append(r)
    assert not results[0].stdout and not results[2].stdout
    assert target.read_bytes()==results[1].stdout and list(Path(d).iterdir())==[target]
r=subprocess.run([observer,str(source),'0','61','3','193'],capture_output=True,timeout=60)
assert r.returncode==0,(r.returncode,r.stderr)
assert b'P1193 CHECK stage=full status=0' in r.stdout
assert b'P1193 LOOP stage=full current_validate=0' in r.stdout
assert r.stdout.count(b'call_terminates=0 call_results=1 known=0 value=0')==2
print('C263 boundary95 prior source/oracle/actual refusal preserved; prospective CHECK/owned/current/C17 PASS; native0')
