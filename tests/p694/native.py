"""Named output equality, native builds and caught file-failure containment."""
import hashlib
import json
import os
from pathlib import Path
import resource
import signal
import subprocess
import sys
import tempfile


def run(args, **kw):
    return subprocess.run(list(map(str, args)), capture_output=True, **kw)


def require(ok, why):
    if not ok:
        raise AssertionError(why)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def exercise(compiler, array_compiler, faults, cc, sanitizer, folder):
    folder = Path(folder)
    root = folder.parents[1]
    freeze = json.loads((folder/'freeze.json').read_text())
    baseline = json.loads((folder/'evidence/baseline.json').read_text())
    for name, sha in freeze['sources'].items():
        require(digest((root/name).read_bytes()) == sha, 'changed frozen source')
    report = {'compiler': run([cc, '--version']).stdout.decode().splitlines()[0],
              'sanitizer': sanitizer, 'native': [], 'failures': []}
    flags = ['-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror']
    if sanitizer != 'none':
        flags += [f'-fsanitize={sanitizer}', '-fno-sanitize-recover=all',
                  '-fno-omit-frame-pointer']
    with tempfile.TemporaryDirectory(prefix='p694-') as temp:
        d = Path(temp)
        output = d/'result with space.c'

        def no_temp():
            require(not list(d.glob('*.tmp.*')), 'leftover temporary')

        def deny(dest, source, status, **kw):
            before = output.read_bytes()
            r = run([compiler, '--emit-c17-to', dest, source], **kw)
            require(r.returncode == status and not r.stdout and r.stderr, r)
            require(output.read_bytes() == before, 'old artifact damaged')
            no_temp()
            report['failures'].append({'destination': str(dest), 'exit': status,
                                       'old_sha256': digest(before),
                                       'diagnostic': r.stderr.decode()})

        for name in freeze['positive']:
            source = root/name
            checked = run([compiler, '--check', source])
            require(checked.returncode == 0 and not checked.stdout and not checked.stderr,
                    checked)
            ordinary = run([compiler, source])
            require(ordinary.returncode == 0 and not ordinary.stderr and
                    digest(ordinary.stdout) == baseline[name]['stdout']['sha256'], ordinary)
            for _ in range(2):
                r = run([compiler, '--emit-c17-to', output, source])
                require(r.returncode == 0 and not r.stdout and not r.stderr and
                        output.read_bytes() == ordinary.stdout, r)
                require(output.stat().st_mode & 0o077 == 0, 'temporary not private')
                no_temp()
            for opt in ('-O0', '-O2'):
                exe = d/'program'
                r = run([cc, *flags, opt, output, '-o', exe])
                require(r.returncode == 0 and not r.stderr, r)
                r = run([exe])
                require(r.returncode == 0 and not r.stdout and not r.stderr, r)
                report['native'].append({'source': name, 'optimization': opt,
                    'named_sha256': digest(ordinary.stdout), 'native_exit': 0})
            r = run([faults, source, d/'fault.c'])
            require(r.returncode == 0, r)
            report.setdefault('fault_controls', []).append(r.stdout.decode())
        # A third, explicitly TEST-ONLY Array parser still shares the same sink.
        array = root/'tests/p534/original.nl'
        ordinary = run([array_compiler, array])
        r = run([array_compiler, '--emit-c17-to', output, array])
        require(ordinary.returncode == 0 and r.returncode == 0 and not r.stdout and
                not r.stderr and output.read_bytes() == ordinary.stdout, r)
        report['test_only_array_byte_equal'] = True
        for name, status in freeze['expectations']['negative'].items():
            deny(output, root/name, status)
        # Source aliases, including aliases spelled through another directory.
        source = d/'input.nl'
        source.write_bytes((root/freeze['positive'][0]).read_bytes())
        before = source.read_bytes()
        deny(source, source, 1)
        alias = d/'symlink.c'
        alias.symlink_to(source)
        deny(alias, source, 1)
        require(alias.is_symlink() and source.read_bytes() == before, 'symlink source changed')
        alias.unlink()
        os.link(source, alias)
        deny(alias, source, 1)
        require(alias.read_bytes() == before and source.read_bytes() == before,
                'hardlinked source changed')
        alias.unlink()
        # Source itself can be a symlink; destination inode must still differ.
        alias.symlink_to(source)
        deny(source, alias, 1)
        alias.unlink()
        alias.symlink_to(d/'missing-target')
        deny(alias, source, 1)
        require(alias.is_symlink() and not (d/'missing-target').exists(), 'dangling link changed')
        alias.unlink()
        sub = d/'sub'
        sub.mkdir()
        deny(sub/'..'/'input.nl', source, 1)
        deny(sub, source, 1)
        deny(d/'absent'/'out.c', source, 1)
        fifo = d/'fifo'
        os.mkfifo(fifo)
        deny(fifo, source, 1)
        fifo.unlink()
        blocked = sub/'blocked.c'
        blocked.write_bytes(output.read_bytes())
        blocked_before = blocked.read_bytes()
        sub.chmod(0o555)
        try:
            deny(blocked, source, 1)
            require(blocked.read_bytes() == blocked_before and
                    not list(sub.glob('*.tmp.*')), 'readonly artifact damaged')
        finally:
            sub.chmod(0o755)
        # Ordinary relative path and a parent symlink resolve consistently.
        linkdir = d/'parent-link'
        linkdir.symlink_to(sub, target_is_directory=True)
        r = run([compiler, '--emit-c17-to', 'parent-link/relative.c', source], cwd=d)
        require(r.returncode == 0 and not r.stdout and
                (sub/'relative.c').read_bytes() == run([compiler, source]).stdout, r)
        require(not list(sub.glob('*.tmp.*')), 'parent temporary leaked')

        def file_limit():
            signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
            resource.setrlimit(resource.RLIMIT_FSIZE, (128, 128))

        # Real kernel write failure: staged regular file reaches RLIMIT_FSIZE.
        deny(output, source, 1, preexec_fn=file_limit)
        # Fresh destination absent on failure; never create a misleading file.
        fresh = d/'fresh.c'
        deny(fresh, root/'tests/p685/wrong_write.nl', 3)
        require(not fresh.exists(), 'failed output became visible')
        with open('/dev/full', 'wb', buffering=0) as full:
            r = subprocess.run([str(compiler), str(source)], stdout=full,
                               stderr=subprocess.PIPE)
        require(r.returncode == 1 and b'V0-C-OUTPUT' in r.stderr, r)
        report['stdout_dev_full_unchanged'] = True
    return report


if __name__ == '__main__':
    print(json.dumps(exercise(*sys.argv[1:]), indent=2))
