"""Strict, unedited raw/passive C17; each guest attempt is recorded first."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

p = argparse.ArgumentParser()
p.add_argument('compiler'); p.add_argument('cc'); p.add_argument('parent')
p.add_argument('sanitizer', choices=['none', 'address', 'undefined'])
p.add_argument('--completed-raw', help='local restart only: exact earlier raw successes')
args = p.parse_args()
root = Path(__file__).resolve().parent
oracle = json.loads((root / 'oracle.json').read_text())
Path(args.parent).mkdir(parents=True, exist_ok=True)
work = Path(tempfile.mkdtemp(prefix='native-', dir=args.parent))
attempts, commands = [], []
previous = Path(args.completed_raw) if args.completed_raw else None
previous_commands = {}
if previous:
    assert args.sanitizer == 'none'
    identity = json.loads((previous.parent.parent / 'gcc-native-plan.json').read_text())
    assert identity['ELF_SHA256'] == hashlib.sha256(Path(args.compiler).read_bytes()).hexdigest()
    previous_commands = {x['label']: x for x in json.loads((previous / 'commands.json').read_text())}
    previous_attempts = {x['label']: x for x in json.loads((previous / 'guest-attempts.json').read_text())}


def run(label, command, env=None):
    command = list(map(str, command))
    guest = label.endswith('.run')
    if guest:
        attempts.append({'label': label, 'command': command, 'status': None})
        (work / 'guest-attempts.json').write_text(json.dumps(attempts, indent=2) + '\n')
    r = subprocess.run(command, capture_output=True, timeout=60, env=env)
    (work / (label + '.stdout')).write_bytes(r.stdout)
    (work / (label + '.stderr')).write_bytes(r.stderr)
    if guest:
        attempts[-1]['status'] = r.returncode
        (work / 'guest-attempts.json').write_text(json.dumps(attempts, indent=2) + '\n')
    assert r.returncode == 0, (command, r.returncode, r.stderr.decode())
    return r


def aliases(data):
    # C symbols are resolved independently within each real function body.
    return dict(re.findall(rb'\b(nl_(?:local|loan_result)_\d+) = '
                           rb'(nl_(?:local|loan_result|replace_old)_\d+);', data))


def origin(symbol, assignments):
    seen = set()
    while symbol in assignments:
        assert symbol not in seen
        seen.add(symbol)
        symbol = assignments[symbol]
    return symbol


for profile, name in enumerate(['L-straight-local-old', 'B-field-zero-local-alias']):
    source = root / (name + '.nl')
    assert hashlib.sha256(source.read_bytes()).hexdigest() == oracle['source'][source.name]['SHA256']
    target = work / (name + '.c')
    r = run(name + '.emit', [args.compiler, '--emit-c17-to', target, source])
    assert not r.stdout and not r.stderr
    data = target.read_bytes()
    sha = hashlib.sha256(data).hexdigest()
    helpers = re.findall(rb'static uint8_t (nl_fn_\d+)\(uint8_t (nl_local_\d+)\)\n'
                         rb'\{\n(.*?)\n\}\n', data, re.S)
    assert len(helpers) == 1
    function, formal, body = helpers[0]
    assert b'for (;;)' not in body and data.count(b'for (;;)') == 1
    spec = oracle['L' if profile == 0 else 'B']
    index = str(spec['field_index']).encode()
    assignment = aliases(body)
    aggregate = re.search(rb'nl_type_\d+ (nl_local_\d+) = \{ ([^}]+)\};', body)
    assert aggregate
    incoming = re.search(rb'\.f' + index + rb' = (nl_local_\d+),', aggregate[2])
    assert incoming and origin(incoming[1], assignment) == formal
    loan = re.search(rb'uint8_t \*const (nl_local_\d+) = &' + aggregate[1] + rb'\.f' + index + rb';', body)
    assert loan
    old = re.search(rb'const uint8_t (nl_replace_old_\d+) = \*' + loan[1] + rb';', body)
    assert old and re.search(rb'\*' + loan[1] + rb' = ' + str(spec['callee_new']).encode() + rb';', body)
    returned = re.search(rb'    return (nl_local_\d+);', body)
    assert returned and origin(returned[1], assignment) == old[1]
    # The same source field's runtime read feeds the actual, and the returned
    # local Copy (not the new callee field) feeds the caller's real store.
    caller = re.search(rb'static void nl_fn_\d+\(void\)\n\{\n(.*?)\n\}\n', data, re.S)
    assert caller
    caller = caller[1]
    assignments = aliases(caller)
    read = re.search(rb'const uint8_t (nl_local_\d+) = (nl_local_\d+)\.f' + index + rb';', caller)
    call = re.search(rb'const uint8_t (nl_local_\d+) = ' + function + rb'\((nl_local_\d+)\);', caller)
    assert read and call and origin(call[2], assignments) == read[1]
    writer = re.search(rb'uint8_t \*const (nl_local_\d+) = &' + read[2] + rb'\.f' + index + rb';', caller)
    assert writer
    store = re.search(rb'\*' + writer[1] + rb' = (nl_local_\d+);', caller)
    assert store and origin(store[1], assignments) == call[1]
    assert data.count(b'NL_LOOP_BIND(&') == data.count(b'NL_CURRENT_CALLEE_BIND(&') == 1
    assert body.index(b'NL_CURRENT_CALLEE_END(') < body.index(b'    return ')
    for passive in ([False, True] if args.sanitizer == 'none' else [True]):
        for level in ([0, 2] if args.sanitizer == 'none' else [0]):
            label = name + ('-passive' if passive else '-raw') + '-O' + str(level)
            binary = work / label
            cmd = [args.cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-UNDEBUG', '-O' + str(level)]
            if passive:
                cmd += ['-DP1372_PROFILE=' + str(profile),
                        '-DNEWLANG_CURRENT_LOOP_OBSERVER="' + str(root / 'observer.h') + '"']
            if args.sanitizer != 'none':
                cmd += ['-fsanitize=' + args.sanitizer, '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
            cmd += [target, '-o', binary]
            completed = previous_commands.get(label)
            if completed and not passive:
                attempt = previous_attempts[label + '.run']
                assert completed['passive'] is False and completed['C_SHA256'] == sha
                assert completed['command'][:-3] == list(map(str, cmd))[:-3]
                assert attempt['status'] == 0 and Path(attempt['command'][0]).is_file()
                assert (previous / (label + '.run.stdout')).read_bytes() == b''
                assert (previous / (label + '.run.stderr')).read_bytes() == b''
                assert (previous / target.name).read_bytes() == data
                commands.append(dict(completed, reused_completed_raw=str(previous),
                                     original_attempt=attempt))
                (work / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
                print(label, 'REUSED exact prior raw SUCCESS; not rerun', flush=True)
                continue
            run(label + '.build', cmd)
            assert target.read_bytes() == data
            env = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
            result = run(label + '.run', [binary], env)
            expected = (b'P1372 live caller/local distinct; input/old/Copy/current/sibling '
                        b'and local ending PASS\n') if passive else b''
            assert not result.stderr and result.stdout == expected
            commands.append({'label': label, 'command': list(map(str, cmd)),
                             'C_SHA256': sha, 'passive': passive,
                             'numeric_identity_observed': passive,
                             'caller_local_separation_while_live': passive,
                             'local_pointer_cleared_before_return': passive})
            (work / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
            print(label, 'PASS', flush=True)
print(work, flush=True)
