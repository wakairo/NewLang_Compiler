"""P1117 finite CLI/native observation, with immutable initial expectations."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

compiler, probe, raw_arg, mode, gcc, clang, *extra = sys.argv[1:]
sanitizer = extra[0] if extra else 'none'
data = Path(__file__).resolve().parent
raw = Path(raw_arg).resolve()
raw.mkdir(parents=True, exist_ok=True)
freeze = json.loads((data/'freeze-v8.json').read_bytes())
for name, digest in freeze['files'].items():
    assert hashlib.sha256((data/name).read_bytes()).hexdigest() == digest
oracle = json.loads((data/'oracle.json').read_bytes())
extension = json.loads((data/'oracle-extension-v8.json').read_bytes())
assert not (oracle['cases'].keys() & extension.keys())
oracle['cases'].update(extension)
rows = []
def run(args, label):
    args = list(map(str,args))
    p = subprocess.run(args,capture_output=True,timeout=90)
    prefix = raw / ('%03d-' % (len(rows)+1)+label)
    prefix.with_suffix('.out').write_bytes(p.stdout)
    prefix.with_suffix('.err').write_bytes(p.stderr)
    rows.append({'label':label,'command':args,'returncode':p.returncode,
                 'stdout_sha256':hashlib.sha256(p.stdout).hexdigest(),
                 'stderr_sha256':hashlib.sha256(p.stderr).hexdigest()})
    (raw/'observations.json').write_text(json.dumps(rows,indent=2)+'\n')
    print(label,p.returncode,p.stderr.decode(errors='replace').strip(),flush=True)
    return p

for name, case in oracle['cases'].items():
    source = data/'sources'/(name+'.nl')
    check = run([compiler,'--check',source],name+'-check')
    emitted = run([compiler,source],name+'-c17')
    expected = case[mode]
    assert check.returncode == expected['check'] and emitted.returncode == expected['emit']
    expected = expected['emit']
    assert bool(emitted.stdout) == (expected == 0)
    output = raw/(name+'-named.c')
    sentinel = b'P1117 preserved named bytes\x00\xff\n'
    output.write_bytes(sentinel)
    before = output.stat()
    named = run([compiler,'--emit-c17-to',output,source],name+'-named')
    assert named.returncode == expected
    if expected:
        assert not named.stdout and output.read_bytes() == sentinel
        assert (before.st_ino,before.st_mtime_ns) == (output.stat().st_ino,output.stat().st_mtime_ns)
    else:
        assert output.read_bytes() == emitted.stdout
    observed = run([probe,source],name+'-owned')
    assert observed.returncode == 0
    if mode == 'candidate' and case.get('runtime'):
        assert b'bool_known=0 node_known=0' in observed.stdout
        assert b'actual checked consumer=1' in observed.stdout
    if expected:
        continue
    for variant in ['raw','passive']:
        if variant == 'passive' and not case.get('runtime'):
            continue
        text = emitted.stdout.decode()
        if variant == 'passive':
            args, results = zip(*case['runtime'])
            header = '#include <assert.h>\n#include <stdio.h>\n#include <stdlib.h>\n'
            header += 'static const size_t p1117_args[] = {'+','.join(map(str,args))+'};\n'
            header += 'static const unsigned p1117_results[] = {'+','.join(map(str,results))+'};\n'
            header += '''static size_t p1117_entered, p1117_returned;
static void p1117_finish(void) { assert(p1117_entered == sizeof(p1117_args)/sizeof(*p1117_args)); assert(p1117_returned == p1117_entered); puts("P1117 runtime true/false results PASS"); }
static void p1117_input(size_t arg) { assert(p1117_entered == p1117_returned); assert(p1117_entered < sizeof(p1117_args)/sizeof(*p1117_args)); assert(arg == p1117_args[p1117_entered]); if (!p1117_entered) assert(atexit(p1117_finish)==0); ++p1117_entered; }
static uint8_t p1117_output(uint8_t value) { assert(p1117_returned+1 == p1117_entered); assert(value == p1117_results[p1117_returned]); printf("P1117 actual quantity=%zu result=%u\\n",p1117_args[p1117_returned],(unsigned)value); ++p1117_returned; return value; }
'''
            pattern = r'(static uint8_t nl_fn_\d+\((?:const )?size_t (nl_local_\d+)\)\n\{\n)(.*?)(^\})'
            def observe(m):
                body, returns = re.subn(r'\breturn ([^;\n]+);',r'return p1117_output(\1);',m[3])
                assert returns == 1
                return m[1]+f'    p1117_input({m[2]});\n'+body+m[4]
            text, count = re.subn(pattern,observe,text,flags=re.S|re.M)
            assert count == 1
            text = text.replace('#include <stdint.h>','#include <stdint.h>\n#include <stddef.h>\n'+header,1)
        c = raw/(name+'-'+variant+'.c')
        c.write_text(text)
        for cc in dict.fromkeys([gcc,clang]):
            for opt in ['-O0','-O2']:
                label = name+'-'+Path(cc).name+'-'+variant+opt
                exe = raw/label
                flags = ['-std=c17','-Wall','-Wextra','-Wpedantic','-Werror',opt]
                if sanitizer != 'none':
                    flags += ['-fsanitize='+sanitizer,'-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
                built = run([cc,*flags,c,'-o',exe],label+'-build')
                assert built.returncode == 0
                native = run([exe],label+'-native')
                assert native.returncode == 0 and not native.stderr
                if variant == 'passive':
                    assert b'runtime true/false results PASS' in native.stdout
                exe.unlink()
print('P1117',mode,'finite observation completed; no LOOP claim',flush=True)
