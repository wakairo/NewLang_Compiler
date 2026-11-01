from pathlib import Path
import hashlib,json,subprocess,sys
repo=Path(__file__).resolve().parents[2]
data=repo/'tests/p1079'
cli=Path(sys.argv[1]).resolve()
out=Path(sys.argv[2]);assert not out.exists();out.mkdir(parents=True)
for name,digest in json.loads((data/'source-freeze-v3.json').read_bytes())['files'].items():
 assert hashlib.sha256((repo/name).read_bytes()).hexdigest()==digest,name
records=[]
def run(label,args):
 r=subprocess.run(list(map(str,args)),capture_output=True,timeout=120)
 (out/(label+'.stdout')).write_bytes(r.stdout);(out/(label+'.stderr')).write_bytes(r.stderr)
 records.append({'label':label,'command':list(map(str,args)),'exit':r.returncode,'stdout_SHA256':hashlib.sha256(r.stdout).hexdigest(),'stderr_SHA256':hashlib.sha256(r.stderr).hexdigest()})
 (out/'commands-FIRST.json').write_text(json.dumps(records,indent=2)+'\n')
 assert r.returncode>=0,('unexpected signal STOP',label,r.returncode)
 return r
libs=[cli.parent/('libnewlang_'+n+'.a') for n in ['semantic','parser','checked_c_node','lexer','checked','syntax','source','diagnostic']]
b=run('build-observer',['gcc','-std=c17','-Wall','-Wextra','-Werror','-pedantic-errors','-g','-O0','-fno-pie','-no-pie','-I'+str(repo/'include'),data/'observe.c','-Wl,--start-group',*libs,'-Wl,--end-group','-o',out/'observe'])
assert b.returncode==0 and not b.stderr,('observer compile first RED',b.stderr.decode())
results=[]
for f in sorted((data/'sources-v3').glob('*.nl')):
 check=run(f.stem+'-CHECK',[cli,'--check',f])
 emit=run(f.stem+'-C17',[cli,f])
 rows=[];observer_exit=None
 if check.returncode==0:
  current=run(f.stem+'-current',[out/'observe',f]);observer_exit=current.returncode
  rows=[json.loads(l) for l in current.stdout.splitlines() if l.startswith(b'{')]
  assert current.returncode==0 and not current.stderr,('observer first RED',f.stem,current.stdout,current.stderr)
 row={'source':f.stem,'CHECK':check.returncode,'CHECK_stderr':check.stderr.decode(),'C17':emit.returncode,'C17_bytes':len(emit.stdout),'C17_SHA256':hashlib.sha256(emit.stdout).hexdigest(),'C17_stderr':emit.stderr.decode(),'observer_exit':observer_exit,'observations':rows}
 results.append(row);(out/'results-FIRST.json').write_text(json.dumps(results,indent=2)+'\n')
 print(json.dumps(row),flush=True)

