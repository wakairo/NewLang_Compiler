"""Literal pinned-parent source/current/C17 observations, before production edits."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

cli,cc,directory=sys.argv[1:]
cli=Path(cli).resolve();data=Path(__file__).resolve().parent;repo=data.parents[1]
out=Path(directory);out.mkdir(exist_ok=False,parents=True)
oracle=json.loads((data/'oracle-v1.json').read_bytes());commands=[];results=[]
sha=lambda b:hashlib.sha256(b).hexdigest()
def run(label,args):
    r=subprocess.run(list(map(str,args)),capture_output=True,timeout=60)
    (out/(label+'.stdout')).write_bytes(r.stdout);(out/(label+'.stderr')).write_bytes(r.stderr)
    commands.append(dict(label=label,command=list(map(str,args)),exit=r.returncode,stdout_SHA256=sha(r.stdout),stderr_SHA256=sha(r.stderr)))
    (out/'commands-FIRST.json').write_text(json.dumps(commands,indent=2)+'\n')
    assert r.returncode>=0,(label,r.returncode)
    return r
def ok(r):assert r.returncode==0 and not r.stderr,(r.returncode,r.stdout,r.stderr)
assert sha(cli.read_bytes())==oracle['parent_compiler_SHA256']
libs=[cli.parent/('libnewlang_'+n+'.a') for n in ['semantic','parser','lexer','checked','syntax','source','diagnostic']]
flags=['-std=c17','-Wall','-Wextra','-Werror','-pedantic-errors','-g','-O0','-fno-pie','-no-pie','-I'+str(repo/'include')]
ok(run('build-public-probe',[cc,*flags,data/'probe.c','-Wl,--start-group',*libs,'-Wl,--end-group','-o',out/'probe']))
for name,expected in list(oracle['positive'].items())+list(oracle['negative'].items()):
    positive=name in oracle['positive'];source=data/'sources'/(name+'.nl')
    check=run(name+'-CHECK',[cli,'--check',source]);want=0 if positive else expected['CHECK']
    assert check.returncode==want and not check.stdout,(name,check.returncode,check.stderr)
    public=None
    if check.returncode==0:
        p=run(name+'-public',[out/'probe',source]);ok(p)
        public=[json.loads(x) for x in p.stdout.decode().splitlines() if x.startswith('{')]
        if positive:
            currents=[r for r in public if r['P1055']=='current'];origins=[r for r in public if r['P1055']=='origin']
            assert len(currents)==len(origins)==2 and all(r['GET_body'] and r['GET_parameters'] and r['body']==r['parameters']==0 for r in currents)
            assert all(r['event_count']==1 and r['returned_event']==0 and r['old']==r['returned'] and r['old_forward_source'] for r in origins)
    target=out/'sentinel.c';sentinel=b'P1055 preserve parent output\n';target.write_bytes(sentinel);before=target.stat()
    emitted=run(name+'-C17',[cli,'--emit-c17-to',target,source]);status=expected['parent_C17'] if positive else expected['C17']
    assert emitted.returncode==status and not emitted.stdout,(name,emitted.returncode,emitted.stderr)
    after=target.stat()
    if status:
        assert target.read_bytes()==sentinel and (before.st_dev,before.st_ino)==(after.st_dev,after.st_ino) and not list(out.glob('*.tmp*'))
    else:assert target.read_bytes() and not emitted.stderr
    result=dict(source=name,source_SHA256=sha(source.read_bytes()),CHECK=check.returncode,public=public,C17=emitted.returncode,
                diagnostic=emitted.stderr.decode(),C_SHA256=sha(target.read_bytes()) if not status else None,
                first_private_runtime_predicate='UNKNOWN; uninstrumented fixed binary',native='NOT TESTED')
    results.append(result);(out/'results-FIRST.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps(result),flush=True)
print('P1055 pinned-parent first results preserved; no Production edit/native yet',flush=True)
