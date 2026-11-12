"""Unmodified C17 raw/passive; each real execution is counted before running."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

parser=argparse.ArgumentParser()
parser.add_argument('compiler');parser.add_argument('cc');parser.add_argument('parent')
parser.add_argument('sanitizer',choices=['none','address','undefined'])
args=parser.parse_args();root=Path(__file__).parent
Path(args.parent).mkdir(parents=True,exist_ok=True)
work=Path(tempfile.mkdtemp(prefix='native-',dir=args.parent));attempts=[];commands=[]
def run(label,command,env=None):
    command=list(map(str,command));guest=label.endswith('.run')
    if guest:
        attempts.append({'label':label,'command':command,'status':None})
        (work/'guest-attempts.json').write_text(json.dumps(attempts,indent=2)+'\n')
    r=subprocess.run(command,capture_output=True,timeout=60,env=env)
    (work/(label+'.stdout')).write_bytes(r.stdout);(work/(label+'.stderr')).write_bytes(r.stderr)
    if guest:
        attempts[-1]['status']=r.returncode
        (work/'guest-attempts.json').write_text(json.dumps(attempts,indent=2)+'\n')
    assert r.returncode==0,(command,r.returncode,r.stderr.decode())
    return r
for profile,name in enumerate(['A-current-copy-call','B-field-zero-alias']):
    source=root/(name+'.nl');target=work/(name+'.c')
    r=run(name+'.emit',[args.compiler,'--emit-c17-to',target,source])
    assert not r.stdout and not r.stderr
    data=target.read_bytes();sha=hashlib.sha256(data).hexdigest()
    helpers=re.findall(rb'static uint8_t nl_fn_\d+\(uint8_t nl_local_\d+\)\n\{\n(.*?)\n\}\n',data,re.S)
    assert len(helpers)==1 and b'for (;;)' not in helpers[0]
    assert data.count(b'NL_LOOP_BIND(&')==1
    index=1-profile
    read=re.search(rb'const uint8_t (nl_local_\d+) = nl_local_\d+\.f'+str(index).encode()+rb';',data)
    assert read and re.search(rb'= nl_fn_\d+\('+read[1]+rb'\);',data)
    assert re.search(rb'NL_CURRENT_CALL_RESULT\(\d+, \d+, nl_local_\d+\);',data)
    for passive in ([False,True] if args.sanitizer=='none' else [True]):
        for level in ([0,2] if args.sanitizer=='none' else [0]):
            label=name+('-passive' if passive else '-raw')+'-O'+str(level)
            binary=work/label;cmd=[args.cc,'-std=c17','-Wall','-Wextra','-Wpedantic','-Werror','-UNDEBUG','-O'+str(level)]
            if passive:cmd+=['-DP1368_PROFILE='+str(profile),'-DNEWLANG_CURRENT_LOOP_OBSERVER="'+str(root/'observer.h')+'"']
            if args.sanitizer!='none':cmd+=['-fsanitize='+args.sanitizer,'-fno-sanitize-recover=all','-fno-omit-frame-pointer']
            cmd += [target,'-o',binary];run(label+'.build',cmd)
            assert target.read_bytes()==data
            env=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
            result=run(label+'.run',[binary],env)
            assert not result.stderr
            assert result.stdout==(b'P1368 current read/call input/result/old/new/sibling same live original PASS\n' if passive else b'')
            commands.append({'label':label,'command':list(map(str,cmd)),'C_SHA256':sha,'passive':passive,'numeric_identity_observed':passive})
            (work/'commands.json').write_text(json.dumps(commands,indent=2)+'\n');print(label,'PASS',flush=True)
print(work)
