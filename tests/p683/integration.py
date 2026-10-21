"""Unmodified emitted C, genuine owned reads, separate process-status consumer."""
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile


def run(args):
    return subprocess.run(list(map(str, args)), capture_output=True)


def require(ok, detail):
    if not ok:
        raise AssertionError(detail)


def exercise(compiler, probe, cc, sanitizer, folder):
    folder = Path(folder)
    frozen = json.loads((folder / 'freeze.json').read_text())
    extra = json.loads((folder / 'calibrated-freeze.json').read_text())
    frozen['sources'].update(extra['sources'])
    frozen['expectations'].update(extra['expectations'])
    results = {}
    for name, digest in frozen['sources'].items():
        source = folder / name
        require(hashlib.sha256(source.read_bytes()).hexdigest() == digest, name)
        oracle = frozen['expectations'][name]
        checked = run([compiler, '--check', source])
        require(checked.returncode == oracle['check'] and not checked.stdout, checked)
        default = run([compiler, source])
        opt = run([compiler, '--experimental-posix-u8-exit', source])
        expected = oracle.get('experimental_emit', oracle.get('emit'))
        require(opt.returncode == expected, opt)
        require(default.returncode == oracle.get('default_emit', oracle.get('emit')), default)
        require(not default.stdout, default)
        if expected:
            code = 'V1-BACKEND-UNSUPPORTED' if expected == 4 else ('P8-BODY-RESULT' if name == 'wrong_result.nl' else 'P3-TYPE-MISMATCH')
            require(not opt.stdout and code.encode() in opt.stderr, opt)
            results[name] = {'check': checked.returncode, 'default': default.returncode, 'opt_in': opt.returncode, 'c_bytes': 0, 'diagnostic': code}
            continue
        require(not opt.stderr and checked.returncode == 0 and not checked.stderr, opt)
        observed = run([probe, source, oracle['guest_status']])
        require(observed.returncode == 0 and not observed.stderr, observed)
        owned = json.loads(observed.stdout)
        text = opt.stdout.decode()
        require(f'return nl_local_{owned["read_symbols"][1]};' in text, 'return is not real source binding')
        require('return (int)nl_fn_' in text and 'return 31;' not in text and 'return 47;' not in text, 'fabricated status')
        require('printf' not in text and 'exit(' not in text, 'unexpected guest output/runtime')
        statuses = []
        with tempfile.TemporaryDirectory(prefix='p683-guest-') as tmp:
            path = Path(tmp) / 'guest.c'
            path.write_bytes(opt.stdout)  # Exact compiler bytes; never instrument.
            for level in ['-O0', '-O2']:
                exe = Path(tmp) / ('guest' + level)
                flags = [] if sanitizer == 'none' else [f'-fsanitize={sanitizer}', '-fno-sanitize-recover=all']
                built = run([cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', level, *flags, path, '-o', exe])
                require(built.returncode == 0 and not built.stdout and not built.stderr, built)
                guest = run([exe])
                require(guest.returncode >= 0 and guest.returncode == oracle['guest_status'] and not guest.stdout and not guest.stderr, guest)
                statuses.append(guest.returncode)
        results[name] = {'check': 0, 'default': 4, 'opt_in': 0, 'owned': owned, 'c_sha256': hashlib.sha256(opt.stdout).hexdigest(), 'normal_status_O0_O2': statuses, 'guest_stdout_bytes': 0, 'guest_stderr_bytes': 0}
    # Normal unit-entry behavior remains byte-for-byte equivalent at the tail.
    unit_source = folder.parent.parent / 'examples/device_settings_swap.nl'
    unit = run([compiler, unit_source])
    require(unit.returncode == 0 and b'return 0;' in unit.stdout, unit)
    refused = run([compiler, '--experimental-posix-u8-exit', unit_source])
    require(refused.returncode == 4 and not refused.stdout, refused)
    require(run([compiler, '--experimental-posix-u8-exit']).returncode == 2, 'argument error')
    # Independent fair C comparison, not guest state injection.
    with tempfile.TemporaryDirectory(prefix='p683-C-baseline-') as tmp:
        for value in (31, 47):
            path = Path(tmp) / 'comparison.c'
            text = (folder / 'comparison.c17').read_text().replace('INITIAL_MODE', str(value))
            path.write_text(text)
            exe = Path(tmp) / 'comparison'
            r = run([cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', path, '-o', exe])
            require(r.returncode == 0, r)
            r = run([exe]); require(r.returncode == value and not r.stdout and not r.stderr, r)
    # A genuinely signalled child is distinct from normal nonzero status data.
    child = run([sys.executable, '-c', 'import os,signal;os.kill(os.getpid(),signal.SIGTERM)'])
    require(os.name == 'posix' and child.returncode == -signal.SIGTERM, child)
    print(json.dumps({'results': results, 'signal_control': child.returncode, 'sanitizer': sanitizer, 'interpretation': 'nonzero diagnostic data; conventional failure, NOT guest stdout/application success/Product GO'}))


if __name__ == '__main__':
    exercise(*sys.argv[1:])
