"""P1107 frozen CLI/current/native oracle; baseline is a separate measurement."""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile

compiler, cc, sanitizer, mode = sys.argv[1:]
data = Path(__file__).parent
oracle = json.loads((data / 'oracle-v1.json').read_bytes())
rows = []
raw = Path(os.environ['P1107_RAW']) if 'P1107_RAW' in os.environ else None
if raw:
    raw.mkdir(parents=True, exist_ok=True)

def run(args, label):
    p = subprocess.run(list(map(str, args)), capture_output=True)
    row = {'label':label, 'command':list(map(str,args)), 'returncode':p.returncode,
           'stdout_sha256':hashlib.sha256(p.stdout).hexdigest(),
           'stderr_sha256':hashlib.sha256(p.stderr).hexdigest(),
           'stderr':p.stderr.decode(errors='replace')}
    rows.append(row)
    if raw:
        prefix = raw / ('%04d-' % len(rows) + label)
        prefix.with_suffix('.out').write_bytes(p.stdout)
        prefix.with_suffix('.err').write_bytes(p.stderr)
        (raw / 'observations.json').write_text(json.dumps(rows,indent=2)+'\n')
    return p

for name, case in oracle['cases'].items():
    source = data / 'sources' / (name+'.nl')
    expected = case[mode]
    check = run([compiler,'--check',source],name+'-check')
    emit = run([compiler,source],name+'-c17')
    print(name,'CHECK',check.returncode,'C17',emit.returncode,emit.stderr.decode().strip(),flush=True)
    assert check.returncode == expected and emit.returncode == expected, (name,expected)
    assert bool(emit.stdout) == (expected == 0)
    with tempfile.TemporaryDirectory(prefix='p1107-') as temp:
        temp = Path(temp)
        target = temp/'named.c'
        sentinel = b'P1107 preserve original named bytes\x00\xff\n'
        target.write_bytes(sentinel)
        st = target.stat()
        named = run([compiler,'--emit-c17-to',target,source],name+'-named')
        assert named.returncode == expected and not named.stdout
        if expected:
            now = target.stat()
            assert target.read_bytes()==sentinel and (now.st_dev,now.st_ino)==(st.st_dev,st.st_ino)
            target.unlink()
            missing = run([compiler,'--emit-c17-to',target,source],name+'-missing')
            assert missing.returncode==expected and not list(temp.iterdir())
        else:
            assert target.read_bytes()==emit.stdout and list(temp.iterdir())==[target]
        if mode=='baseline' or not case['expected']:
            continue
        v = case['expected']
        roots = 2 if 'other' in v else 1
        finals = [v['other'],v['after']] if roots==2 else [v['after'],[0,0]]
        config = {'ROOTS':roots,'SELECTED':v['root'],'INDEX':v['index'],'OLD':v['old'],
                  'NEW':v['new'],'SIBLING':v['sibling'],'HELPER':int(case['helper']),
                  'FINAL_00':finals[0][0],'FINAL_01':finals[0][1],
                  'FINAL_10':finals[1][0],'FINAL_11':finals[1][1]}
        header = temp/'observer.h'
        header.write_text('#include <stdbool.h>\n'+''.join(f'#define EXPECT_{k} {x}\n' for k,x in config.items())+(data/'observer.h').read_text())
        passive = emit.stdout.decode()
        if case['helper']:
            pattern = r'(static uint8_t nl_fn_\d+\(uint8_t \*(nl_local_\d+), uint8_t (nl_local_\d+)\)\n\{\n)(.*?)(\n\})'
            def observe_helper(m):
                body, stores = re.subn(r'(^\s*\*(nl_local_\d+) = [^;]+;)$',lambda n:n[1]+f'\n    p1107_helper_changed({n[2]});',m[4],flags=re.M)
                assert stores==1
                return m[1]+f'    p1107_helper_enter({m[2]}, {m[3]});\n'+body+m[5]
            passive, count = re.subn(pattern,observe_helper,passive,flags=re.S)
            assert count==1
        for variant, content in [('raw',emit.stdout),('passive',passive.encode())]:
            c = temp/(variant+'.c')
            c.write_bytes(content)
            print(name,variant,'C17 sha256',hashlib.sha256(content).hexdigest(),flush=True)
            for opt in ('-O0','-O2'):
                exe = temp/(variant+opt)
                flags = ['-std=c17','-Wall','-Wextra','-Wpedantic','-Werror',opt]
                if 'clang' in Path(cc).name:
                    flags += ['-Wno-error=gcc-install-dir-libstdcxx']
                if variant=='passive':
                    flags += ['-DNEWLANG_LOOP_OBSERVER="'+str(header)+'"']
                if sanitizer!='none':
                    flags += ['-fsanitize='+sanitizer,'-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
                built = run([cc,*flags,c,'-o',exe],name+'-'+variant+opt+'-build')
                assert built.returncode==0, built.stderr
                native = run([exe],name+'-'+variant+opt+'-native')
                assert native.returncode==0 and not native.stderr, (native.returncode,native.stdout,native.stderr)
                if variant=='passive':
                    assert b'PASS' in native.stdout
                print(name,variant,opt,sanitizer,native.stdout.decode().strip(),flush=True)
