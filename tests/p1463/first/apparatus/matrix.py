import hashlib
import json
import os
from pathlib import Path
import subprocess

p = Path('/workspace/p1463-c1025')
w = Path('/tmp/p1463-c1025/guests')
w.mkdir(exist_ok=True)
clang = '/workspace/p1236-c291/sdk/.deps/toolchains/ubuntu-24.04-x86_64-7c9921be7919a10d/newlang-llvm-23.1.3-linux-x64/bin/clang'
plans = []
for compiler, cc in [('gcc','cc'),('clang',clang)]:
    for opt in [0,2]:
        for world, fault in [(0,1),(1,2),(2,0)]:
            if (compiler,opt,world)==('gcc',2,2):
                continue  # same successor guest already completed under ledger
            plans.append(('A',compiler,cc,opt,world,fault,'none',True))
for compiler, cc, opt in [('gcc','cc',0),('clang',clang,0),('clang',clang,2)]:
    plans.append(('A',compiler,cc,opt,2,0,'none',False))
for shape in ['B','C']:
    for compiler, cc in [('gcc','cc'),('clang',clang)]:
        plans.append((shape,compiler,cc,2,2,0,'none',True))
for world,fault in [(0,1),(1,2),(2,0)]:
    plans.append(('A','clang',clang,1,world,fault,'address',True))
plans.append(('A','clang',clang,1,2,0,'undefined',True))

manifest=[]
for shape,compiler,cc,opt,world,fault,san,passive in plans:
    label=f'c1-{shape}-{"passive" if passive else "raw"}-{compiler}-O{opt}-H{world}-{san}'
    out=w/label
    cmd=[cc,'-std=c17',f'-O{opt}','-Wall','-Wextra','-Wpedantic','-Werror']
    if san!='none':
        cmd += [f'-fsanitize={san}','-fno-omit-frame-pointer','-fno-sanitize-recover=all']
    if passive:
        h=p/'apparatus/passive.h'
        cmd += [f'-DP1463_FAIL_AT={fault}',f'-DP1463_FIELD_INDEX={1 if shape=="B" else 0}',
                f'-DP1463_BOOL_TRUE={1 if shape=="C" else 0}',f'-DNEWLANG_NODE_OBSERVER="{h}"',f'-DNEWLANG_HEAP_OBSERVER="{h}"']
    cmd += [str(p/'outputs'/f'{shape}.c1.c')]
    if passive:
        cmd += [str(p/'apparatus/guest_wrap.c'),'-Wl,--wrap=malloc','-Wl,--wrap=free']
    cmd += ['-o',str(out)]
    q=subprocess.run(cmd,capture_output=True)
    (w/f'{label}.compile.json').write_text(json.dumps({'command':cmd,'exit':q.returncode,'stdout':q.stdout.decode(errors='replace'),'stderr':q.stderr.decode(errors='replace')},indent=2)+'\n')
    if q.returncode:
        print(q.stderr.decode(errors='replace'))
        raise SystemExit(f'compile STOP {label}')
    manifest.append({'label':label,'command':cmd,'elf_sha256':hashlib.sha256(out.read_bytes()).hexdigest(),'run':[str(out)],'sanitizer':san})
freeze=p/'apparatus/native-matrix-freeze.json'
if freeze.exists():
    raise SystemExit('freeze exists; do not overwrite')
freeze.write_text(json.dumps(manifest,indent=2)+'\n')
print('COMPILED_ONLY',len(manifest),'freeze_sha256',hashlib.sha256(freeze.read_bytes()).hexdigest())
