"""Ordinary CLI boundary only; no R-B native/backend expansion."""
from pathlib import Path
import hashlib,json,subprocess,sys,tempfile
compiler,root=sys.argv[1:];r=Path(root)
f=json.loads((r/'freeze.json').read_text());s=json.loads((r/'supplement-freeze.json').read_text())
external=r.parent/'p778/external_r776'
def run(args):return subprocess.run([str(x) for x in args],capture_output=True)
with tempfile.TemporaryDirectory(prefix='p785-cli-') as tmp:
 target=Path(tmp)/'saved.c'
 for source,digest in [(r/'sources/pressure_arm.nl',f['source_sha256']),(external/'positive_t_interposed.nl',f['external_R_B_sha256'])]:
  assert hashlib.sha256(source.read_bytes()).hexdigest()==digest
  p=run([compiler,'--check',source]);assert p.returncode==0 and not p.stdout and not p.stderr
  p=run([compiler,source]);assert p.returncode==4 and not p.stdout and b'V1-BACKEND-UNSUPPORTED' in p.stderr
  target.write_bytes(b'correct original C')
  named=run([compiler,'--emit-c17-to',target,source]);assert named.returncode==4 and not named.stdout and target.read_bytes()==b'correct original C'
  print(source.name,'CHECK0 / C174 / zero C / named unchanged; B native NOT TESTED')
 a=external/'positive_h_first.nl';assert hashlib.sha256(a.read_bytes()).hexdigest()==f['external_R_A_sha256']
 p=run([compiler,'--check',a]);assert p.returncode==0
 p=run([compiler,a]);assert p.returncode==0 and p.stdout and not p.stderr
 named=run([compiler,'--emit-c17-to',target,a]);assert named.returncode==0 and target.read_bytes()==p.stdout
 print('R-A normal/named C17 byte-identical regression PASS; native remains separate existing suite')
 assert hashlib.sha256((external/'oracle.json').read_bytes()).hexdigest()==f['external_R_oracle_sha256']
 old=json.loads((external/'oracle.json').read_text())['source_sha256']
 negatives=[(external/Path(name).name,digest) for name,digest in old.items() if Path(name).name.startswith('negative_')]
 negatives += [(r/'sources'/name,digest) for name,digest in s['sha256'].items()]
 for source,digest in negatives:
  assert hashlib.sha256(source.read_bytes()).hexdigest()==digest
  checked=run([compiler,'--check',source]);emitted=run([compiler,source]);assert checked.returncode==emitted.returncode==3 and not checked.stdout and not emitted.stdout
  target.write_bytes(b'correct original C');named=run([compiler,'--emit-c17-to',target,source]);assert named.returncode==3 and target.read_bytes()==b'correct original C'
  print(source.name,checked.stderr.decode().strip())
