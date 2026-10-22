"""Frozen actual no-pre-read sources; unchanged generated C vs passive observer.
R inputs, when explicitly supplied, remain external immutable historical cases.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

compiler, probe, public, cc, sanitizer, directory, *external = sys.argv[1:]
root = Path(directory).resolve()
support = root.parent/'integration'
frozen = json.loads((root/'freeze.json').read_text())

def run(args, **kw):
    return subprocess.run(list(map(str,args)),capture_output=True,**kw)

def observe(code):
    binding=re.search(r'nl_live_tail (nl_b_\d+_\d+) = (nl_v_\d+);',code)
    assert binding
    local,packed=binding.groups()
    code=code.replace(binding[0],binding[0]+f'\nP738_BOUND({local},{packed});',1)
    move=re.search(r'nl_live_tail (nl_v_\d+) = '+local+r';\n'+local+r' = \(nl_live_tail\)\{0\};',code)
    assert move
    return code.replace(move[0],move[0]+f'\nP738_MOVED({local},{move[1]});',1)

with tempfile.TemporaryDirectory() as temp:
    tmp=Path(temp)
    cases=[(root/(x['name']+'.nl'),x) for x in frozen['cases']]
    if external:
        assert len(external)==1
        cases.append((Path(external[0]),dict(name='inherited-R-A',sha256=frozen['R_A_sha256'],head_payload=41,tail_payload=73,receiver_read=True,return_read=True)))
    for source,case in cases:
        assert hashlib.sha256(source.read_bytes()).hexdigest()==case['sha256']
        check=run([compiler,'--check',source]); view=run([public,source])
        assert check.returncode==view.returncode==0 and not check.stdout and not check.stderr and not view.stderr,(check,view)
        print(case['name'],view.stdout.decode().strip())
        r=run([probe,source]); assert r.returncode==0,r.stderr
        rows=[(line.split()[0],list(map(int,line.split()[1:]))) for line in r.stdout.decode().splitlines()]
        initial=[v for k,v in rows if k=='INIT']; roots=[v for k,v in rows if k=='ROOT']; fields=[v for k,v in rows if k=='FIELD']; calls=[v for k,v in rows if k=='CALL']
        assert len(initial)==2 and [v[-1] for v in initial]==[case['head_payload'],case['tail_payload']]
        assert len(roots)==2+case['return_read']+case['receiver_read'] and len(fields)==2 and len(calls)==1
        assert all(initial[0][i]!=initial[1][i] for i in range(4))
        definitions=[]
        for prefix,values in zip(('H','T'),initial):
            definitions += [f'-DEXPECT_{prefix}{k}={v}' for k,v in zip(('R','P','I','D','V'),values)]
        definitions += [f'-DEXPECT_S{i}={roots[i][5] if i<len(roots) else 0}' for i in range(6)]
        definitions += [f'-DEXPECT_{k}={v}' for k,v in zip(('PP','PA','PD'),calls[0][1:4])]
        definitions += [f'-DEXPECT_RECEIVER_READ={int(case["receiver_read"])}',f'-DEXPECT_RETURN_READ={int(case["return_read"])}']
        definitions += [f'-DEXPECT_NOMINAL={fields[0][1]}',f'-DEXPECT_CHILD={fields[0][2]}',f'-DEXPECT_CI={fields[0][3]}']
        emitted=run([compiler,source]); assert emitted.returncode==0 and not emitted.stderr
        raw=emitted.stdout;code=raw.decode()
        assert code.count('malloc(24);')==2 and 'NL_HEAP_COPY(nl_v_' not in code and 'NL_NODE_ARM(2,' not in code
        named=tmp/'existing.c';named.write_bytes(b'previous-output')
        n=run([compiler,'--emit-c17-to',named,source]);assert n.returncode==0 and named.read_bytes()==raw
        for opt in ['-O0','-O2']:
            for passive in [False,True]:
                name=case['name']+opt+str(passive);path=tmp/(name+'.c');exe=tmp/name
                path.write_text(observe(code) if passive else code)
                flags=definitions+['-DNEWLANG_HEAP_OBSERVER="'+str(root/'observer.h')+'"'] if passive else ['-DNEWLANG_UNOBSERVED']
                cmd=[cc,'-std=c17','-Wall','-Wextra','-Wpedantic','-Werror','-DNDEBUG',opt,path,support/'two_heap_platform.c','-fno-builtin-malloc','-fno-builtin-free','-Wl,--wrap=malloc','-Wl,--wrap=free','-o',exe,*flags]
                if sanitizer!='none':cmd += [f'-fsanitize={sanitizer}','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
                built=run(cmd);assert built.returncode==0,built.stderr.decode()
                for failure in ['1','2',None]:
                    env=dict(os.environ);env.pop('NEWLANG_TEST_FAIL_CALL',None)
                    if failure:env['NEWLANG_TEST_FAIL_CALL']=failure
                    actual=run([exe],env=env);assert actual.returncode==0 and not actual.stderr,(name,failure,actual.stderr.decode())
                    if passive:
                        output=actual.stdout.decode();lines=output.splitlines();assert len(lines)==2,output
                        m=re.fullmatch(r'OBSERVED head=([0-9a-f]+) tail=([0-9a-f]+) trials=(\d+) free=(\d+) roots=(\d+) changes=(\d+) copy=(\d+) scopes=(\d+) receiver=(\d+) receiver_free=(\d+) donor_free=(\d+)',lines[0]);assert m,output
                        h,t=[int(x,16) for x in m.groups()[:2]];counts=list(map(int,m.groups()[2:]));live=re.fullmatch(r'LIVE_RETURN producer=(\d+) returned=(\d+) received=(\d+) original_tail=([0-9a-f]+)',lines[1]);assert live
                        if failure=='1':assert h==t==0 and counts==[1,0,0,0,0,0,0,0,0]
                        elif failure=='2':assert h and not t and counts==[2,1,0,0,0,0,0,0,1]
                        else:
                            nroots=2+int(case['return_read'])+int(case['receiver_read'])
                            assert h and t and h!=t and counts==[2,2,nroots,2,0,nroots,1,1,1]
                        assert list(map(int,live.groups()[:3]))==([0,0,0] if failure else [1,1,1]) and int(live[4],16)==t
                        print(name,failure or 'success',output.strip())
        print(case['name'],'no-pre-read genuine local/call/return, unchanged+passive O0/O2, original identity+cleanup PASS')
    baseline=json.loads((root/'evidence/P-source-baseline.json').read_text())
    for case in frozen['negatives']:
        path=root/(case['name']+'.nl');assert hashlib.sha256(path.read_bytes()).hexdigest()==case['sha256']
        expected=baseline[case['name']]['check'];code=re.search(r'\[([^]]+)\]',expected['stderr'])[1].encode()
        for args in [[compiler,'--check',path],[compiler,path]]:
            r=run(args);assert r.returncode==expected['exit']==3 and not r.stdout and code in r.stderr,(path,r.stderr)
        named.write_bytes(b'previous-output');r=run([compiler,'--emit-c17-to',named,path]);assert r.returncode==3 and named.read_bytes()==b'previous-output'
        print(case['name'],code.decode(),'refusal unchanged; zero C/named output preserved')
    for name,case in json.loads((root/'profile-freeze.json').read_text()).items():
        path=root/(name+'.nl');assert hashlib.sha256(path.read_bytes()).hexdigest()==case['sha256']
        c=run([compiler,'--check',path]);r=run([compiler,path]);assert c.returncode==0 and r.returncode==4 and not r.stdout and b'V1-BACKEND-UNSUPPORTED' in r.stderr
        named.write_bytes(b'previous-output');r=run([compiler,'--emit-c17-to',named,path]);assert r.returncode==4 and named.read_bytes()==b'previous-output'
        print(name,'CHECK0; finite backend4 preserved; no C/named output overwrite')
