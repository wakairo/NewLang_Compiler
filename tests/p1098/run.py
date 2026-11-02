"""Independent P1098 oracle: CLI, named preservation and passive native identity."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import sys
import tempfile

compiler, cc, sanitizer, data = sys.argv[1:]
data = Path(data)
oracle = json.loads((data / 'oracle-v1.json').read_bytes())
first = json.loads((data / 'evidence/fixed-main-FIRST.json').read_bytes())
control_sha = next(r['stdout_sha256'] for r in first if r['case'] == '06-outside-control' and r['mode'] == 'c17')
def run(cmd):
    p = subprocess.run(list(map(str, cmd)), capture_output=True)
    return p
for name, case in oracle['cases'].items():
    src = data / 'sources' / (name + '.nl')
    check = run([compiler, '--check', src])
    assert check.returncode == case['check'], (name, 'CHECK', check.returncode, check.stderr)
    emit = run([compiler, src])
    assert emit.returncode == case['candidate_c17'], (name, 'C17', emit.returncode, emit.stderr)
    if emit.returncode:
        assert not emit.stdout
    else:
        assert emit.stdout and not emit.stderr
    if name == '06-outside-control':
        assert hashlib.sha256(emit.stdout).hexdigest() == control_sha
    with tempfile.TemporaryDirectory(prefix='p1098-') as tmp:
        tmp = Path(tmp)
        target = tmp / 'named.c'
        sentinel = b'P1098 unchanged named output on failure\x00\xff\n'
        target.write_bytes(sentinel)
        inode = target.stat().st_ino
        named = run([compiler, '--emit-c17-to', target, src])
        assert named.returncode == case['candidate_c17'] and not named.stdout
        if named.returncode:
            assert target.read_bytes() == sentinel and target.stat().st_ino == inode
            target.unlink()
            missing = run([compiler, '--emit-c17-to', target, src])
            assert missing.returncode == named.returncode and not target.exists()
            assert not list(tmp.iterdir())
        else:
            assert target.read_bytes() == emit.stdout and list(tmp.iterdir()) == [target]
        if name[:2] in {'01', '02', '03', '04', '05'}:
            expected = case['expected']
            roots = 2 if 'other' in expected else 1
            finals = [expected['other'], expected['after']] if roots == 2 else [expected['after'], [0, 0]]
            config = {'ROOTS': roots, 'SELECTED': expected['root'],
                      'INDEX': expected['index'], 'OLD': expected['old'],
                      'NEW': expected['new'], 'SIBLING': expected['sibling'],
                      'FINAL_00': finals[0][0], 'FINAL_01': finals[0][1],
                      'FINAL_10': finals[1][0], 'FINAL_11': finals[1][1]}
            header = tmp / 'observer.h'
            header.write_text('#include <stdbool.h>\n' + ''.join(f'#define EXPECT_{k} {v}\n' for k, v in config.items()) + (data / 'observer.h').read_text())
            generated = tmp / 'generated.c'
            generated.write_bytes(emit.stdout)
            for opt in ('-O0', '-O2'):
                exe = tmp / ('native' + opt)
                flags = ['-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', opt,
                         '-DNEWLANG_LOOP_OBSERVER="' + str(header) + '"']
                if 'clang' in Path(cc).name:
                    flags += ['-Wno-error=gcc-install-dir-libstdcxx']
                if sanitizer != 'none':
                    flags += ['-fsanitize=' + sanitizer, '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
                build = run([cc, *flags, generated, '-o', exe])
                assert build.returncode == 0, (name, opt, build.stderr)
                native = run([exe])
                assert native.returncode == 0 and not native.stderr and b'PASS' in native.stdout, (name, opt, native.returncode, native.stdout, native.stderr)
                print('P1098 native', name, cc, opt, sanitizer, native.stdout.decode().strip(), flush=True)
    print('P1098 CLI', name, 'CHECK', check.returncode, 'C17', emit.returncode, 'named no-clobber, no-temp, outside-control bytes', flush=True)
