from pathlib import Path
import hashlib,json,re,subprocess,sys,tempfile
compiler,probe=sys.argv[1:]
r=Path(__file__).resolve().parent
freeze=json.loads((r/'freeze.json').read_text())
cases=[]
for folder,manifest in [('sources','freeze.json'),('controls','controls-freeze.json')]:
 for name,sha in json.loads((r/manifest).read_text())['sha256'].items():
  s=r/folder/name;assert hashlib.sha256(s.read_bytes()).hexdigest()==sha;cases.append(s)
expected={
 'limits_h_first.nl':[(0,32,91,168)],
 'tuning_t_first.nl':[(1,204,75,17),(0,6,204,199)],
}
codes={
 'negative_wrong_domain':'P3-DOMAIN-MISMATCH',
 'negative_wrong_allocation':'P4-ALLOCATION-MISMATCH',
 'negative_consumed_domain':'P3-USE-AFTER-CONSUME',
 'negative_escaped_ref':'P3-UNKNOWN-BINDING',
 'negative_copy_ref':'P3-TYPE-MISMATCH',
 'negative_wrong_self':'REC-SELF-TARGET',
}
def run(args):return subprocess.run(list(map(str,args)),capture_output=True)
with tempfile.TemporaryDirectory(prefix='p761-') as d:
 d=Path(d)
 for source in cases:
  negative=source.stem.startswith('negative_')
  check=run([compiler,'--check',source]);emit=run([compiler,source])
  assert check.returncode==(3 if negative else 0),(source,check.stderr)
  assert emit.returncode==(3 if negative else 4) and not check.stdout and not emit.stdout,(source,emit.stderr)
  if negative:
   # The ended-root diagnostic is measured, never claimed to be a layout or Copy error.
   if source.stem in codes:assert ('['+codes[source.stem]+']').encode() in check.stderr
  else:
   assert not check.stderr and b'V1-BACKEND-UNSUPPORTED' in emit.stderr
  for name,exists in [('kept.c',True),('absent.c',False)]:
   target=d/name;sentinel=b'previous output remains correct\n'
   if exists:target.write_bytes(sentinel)
   p=run([compiler,'--emit-c17-to',target,source]);assert p.returncode==emit.returncode and not p.stdout
   assert target.read_bytes()==sentinel if exists else not target.exists()
   assert not list(d.glob(name+'.*'))
  if source.name in expected:
   p=run([probe,'valid',source]);assert p.returncode==0,p.stderr
   actual=[tuple(map(int,m)) for m in re.findall(r'index=(\d+).*old=(\d+) new=(\d+) sibling=(\d+)',p.stdout.decode())]
   assert actual==expected[source.name],(source,actual)
   assert b'distinct_original_consumers=1 source_graph=PASS' in p.stdout
   print(source.name,p.stdout.decode().strip())
  print(source.name,'CHECK',check.returncode,'C17',emit.returncode,'no partial C/named preservation PASS')
print('PUBLIC T COPY CERTIFICATE ONLY; mixed C17/native remains unsupported')
