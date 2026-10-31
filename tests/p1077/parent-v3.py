from pathlib import Path
import hashlib,json,subprocess,sys
repo=Path(__file__).resolve().parents[2]
cli=Path(sys.argv[1]).resolve()
out=Path(sys.argv[2]);assert not out.exists();out.mkdir(parents=True)
data=repo/'tests/p1077'
freeze=json.loads((data/'source-freeze-v2.json').read_text())
for p,h in freeze['files'].items():assert hashlib.sha256((repo/p).read_bytes()).hexdigest()==h
flags=['-std=c17','-Wall','-Wextra','-Werror','-pedantic-errors','-O0','-g','-fno-pie','-no-pie']
libs=[cli.parent/('libnewlang_'+n+'.a') for n in ['semantic','parser','checked_c_node','lexer','checked','syntax','source','diagnostic']]
commands=[]
def run(label,args):
 r=subprocess.run(list(map(str,args)),capture_output=True,timeout=120)
 (out/(label+'.stdout')).write_bytes(r.stdout);(out/(label+'.stderr')).write_bytes(r.stderr)
 commands.append({'label':label,'command':list(map(str,args)),'exit':r.returncode,'stdout_SHA256':hashlib.sha256(r.stdout).hexdigest(),'stderr_SHA256':hashlib.sha256(r.stderr).hexdigest()})
 (out/'commands-FIRST.json').write_text(json.dumps(commands,indent=2)+'\n')
 assert r.returncode>=0,('unexpected signal STOP',label,r.returncode)
 return r
b=run('build-observe',['gcc',*flags,'-I'+str(repo/'include'),data/'observe-v2.c','-Wl,--start-group',*libs,'-Wl,--end-group','-o',out/'observe'])
assert b.returncode==0 and not b.stderr,('observer build RED',b.stderr.decode())
results=[]
for source in sorted((data/'sources-v2').glob('*.nl')):
 name=source.stem
 c=run(name+'-CHECK',[cli,'--check',source])
 v=run(name+'-C17',[cli,source])
 rows=[]
 if c.returncode==0 and not name.startswith('negative_'):
  r=run(name+'-current',[out/'observe',source])
  assert r.returncode==0 and not r.stderr,('observer/current RED',name,r.stdout,r.stderr)
  rows=[json.loads(l) for l in r.stdout.splitlines() if l.startswith(b'{')]
 results.append({'source':name,'CHECK':c.returncode,'C17':v.returncode,'C_bytes':len(v.stdout),'current_rows':rows,'CHECK_stderr':c.stderr.decode(),'C17_stderr':v.stderr.decode()})
 (out/'results-FIRST.json').write_text(json.dumps(results,indent=2)+'\n')
 print(json.dumps(results[-1]),flush=True)

