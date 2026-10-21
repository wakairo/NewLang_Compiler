"""Unedited ordinary CLI C plus a separate passive physical observer.

The observer supplies no guest values/authority: only actual-address captures,
volatile reads, counters and assertions from the independent pre-execution oracle.
"""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def run(args):
    return subprocess.run(list(map(str, args)), capture_output=True)


def require(ok, why):
    if not ok:
        raise AssertionError(why)


def passive(text, evidence, oracle):
    calls, direct = evidence['calls'], evidence['direct']
    if 'calls' in oracle:
        initial = {calls[0]['source_symbol']: oracle['initial']['a'],
                   calls[1]['source_symbol']: oracle['initial']['b']}
        updates = oracle['calls']
        last = oracle['last_write']
    else:
        require(len(calls) == 1, 'one source call')
        initial = {calls[0]['source_symbol']: oracle['initial'],
                   direct[0]['symbol']: oracle['target_initial']}
        updates = [{'index': oracle['source_index'], 'incoming': oracle['incoming'],
                    'old': oracle['old'], 'post': oracle['post']}]
        last = {'index': oracle['target_index'], 'old': oracle['target_old'],
                'incoming': oracle['old'], 'post': oracle['target_post']}
    require(len(calls) == len(updates) and len(direct) == 1, 'source coverage')
    additions = {}

    def find(pattern):
        matches = list(re.finditer(pattern, text))
        require(len(matches) == 1, pattern)
        return matches[0]

    def after(match, extra):
        additions[match.end()] = additions.get(match.end(), '') + extra

    def state(values):
        return ' || '.join(f'*obs_{symbol}_{i} != {v}'
                           for symbol, pair in values.items() for i, v in enumerate(pair))

    for symbol, pair in initial.items():
        ctor = find(rf'(?:const )?nl_type_\d+ nl_local_{symbol} = \{{[^;]+;')
        extra = ''.join(f'\n    obs_{symbol}_{i} = &nl_local_{symbol}.f{i};' for i in range(2))
        extra += '\n    if (' + ' || '.join(f'*obs_{symbol}_{i} != {v}' for i, v in enumerate(pair)) + ') abort();'
        after(ctor, extra)
    representative = calls[0]
    ref = f'nl_local_{representative["ref_formal"]}'
    value = f'nl_local_{representative["value_formal"]}'
    function = representative['function']
    entry = find(rf'static uint8_t nl_fn_{function}\(uint8_t \*{ref}, uint8_t {value}\)\n\{{')
    entry_checks = []
    capture_checks = []
    store_checks = []
    before = {k: list(v) for k, v in initial.items()}
    change_id = representative['changes'][0]['node']
    for index, (call, step) in enumerate(zip(calls, updates)):
        require(call['function'] == function and len(call['changes']) == 1, 'same source helper')
        require(call['index'] == step['index'], 'original field index')
        require(call['parent_fact'] != call['parent_post'] and
                call['child_fact'] != call['child_post'], 'genuine Change/Reset')
        symbol, field = call['source_symbol'], call['index']
        require(before[symbol][field] == step['old'], 'old oracle')
        entry_checks.append(f'if (obs_calls == {index+1} && ({ref} != obs_{symbol}_{field} || '
                            f'{value} != {step["incoming"]} || {state(before)})) abort();')
        capture_checks.append(f'if (obs_calls == {index+1} && '
                              f'nl_replace_old_{change_id} != {step["old"]}) abort();')
        before[symbol] = list(step['post'])
        store_checks.append(f'if (obs_calls == {index+1} && ({state(before)})) abort();')
        actual = find(rf'const uint8_t nl_call_{call["call"]} = nl_fn_{function}\(nl_local_{call["caller_ref"]}, [^;]+;')
        if call['incoming_symbol']:
            require(actual[0].endswith(f'nl_local_{call["incoming_symbol"]});'), 'actual Copy argument forwarding')
        after(actual, f'\n        if (obs_calls != {index+1} || '
              f'nl_call_{call["call"]} != {step["old"]} || {state(before)}) abort();')
    after(entry, '\n    ++obs_calls;\n    ' + '\n    '.join(entry_checks))
    capture = find(rf'const uint8_t nl_replace_old_{change_id} = \*{ref};')
    store = find(rf'\*{ref} = {value};')
    after(capture, '\n    ' + '\n    '.join(capture_checks))
    after(store, '\n    ' + '\n    '.join(store_checks))
    write = direct[0]
    require(write['index'] == last['index'] and
            before[write['symbol']][write['index']] == last['old'], 'receiver original')
    capture = find(rf'const uint8_t nl_replace_old_{write["node"]} = \*nl_local_{write["ref"]};')
    store = find(rf'\*nl_local_{write["ref"]} = nl_local_{write["incoming_symbol"]};')
    after(capture, f'\n        if (nl_local_{write["ref"]} != obs_{write["symbol"]}_{write["index"]} || '
          f'nl_replace_old_{write["node"]} != {last["old"]}) abort();')
    before[write['symbol']] = list(last['post'])
    after(store, f'\n        if ({state(before)}) abort();')
    # End the observer's borrowed pointers before the real caller locals end.
    # This also keeps strict GCC13 dangling-pointer diagnostics enabled.
    after(find(r'\(void\)0;'), '\n    ' + ' '.join(
        f'obs_{symbol}_{i} = NULL;' for symbol in initial for i in range(2)))
    for position in sorted(additions, reverse=True):
        text = text[:position] + additions[position] + text[position:]
    globals_ = '\n'.join(f'static const volatile uint8_t *obs_{symbol}_{i};'
                         for symbol in initial for i in range(2))
    return '#include <stdlib.h>\n#include <stdint.h>\nstatic unsigned obs_calls;\n' + globals_ + '\n' + text


def exercise(compiler, probe, cc, sanitizer, folder):
    folder = Path(folder)
    frozen = json.loads((folder / 'frozen.json').read_text())
    extra = json.loads((folder / 'additional-freeze.json').read_text())
    sha = dict(frozen['sha256'], **extra['sha256'])
    oracles = dict(frozen['positive'], relay=extra['relay'])
    rows = {}
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        for name, oracle in oracles.items():
            source = folder / (name + '.nl')
            require(hashlib.sha256(source.read_bytes()).hexdigest() == sha[source.name], 'frozen source')
            checked = run([compiler, '--check', source])
            emitted = run([compiler, source])
            require(checked.returncode == 0 and not checked.stdout and not checked.stderr, checked)
            require(emitted.returncode == 0 and emitted.stdout and not emitted.stderr, emitted)
            result = run([probe, source])
            require(result.returncode == 0 and not result.stderr, result)
            evidence = json.loads(result.stdout)
            code = emitted.stdout.decode()
            observed = passive(code, evidence, oracle)
            first = evidence['calls'][0]
            fake = re.sub(rf'nl_fn_{first["function"]}\(nl_local_{first["caller_ref"]}, [^;]+;', '0;', code, count=1)
            try:
                passive(fake, evidence, oracle)
            except AssertionError:
                pass
            else:
                raise AssertionError('missing physical call accepted')
            for label, text in [('original', code), ('passive', observed)]:
                c = root / (name + '-' + label + '.c')
                c.write_text(text)
                for opt in ['-O0', '-O2']:
                    exe = root / (name + '-' + label + opt)
                    args = [cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', opt, c, '-o', exe]
                    if sanitizer != 'none':
                        args += [f'-fsanitize={sanitizer}', '-fno-sanitize-recover=all',
                                 '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
                    build = run(args)
                    require(build.returncode == 0, build.stderr.decode())
                    guest = run([exe])
                    require(guest.returncode == 0 and not guest.stdout and not guest.stderr, guest)
            named = root / 'saved.c'
            named.write_bytes(b'OLD\n')
            saved = run([compiler, '--emit-c17-to', named, source])
            require(saved.returncode == 0 and not saved.stdout and named.read_bytes() == emitted.stdout, saved)
            rows[name] = {'check': 0, 'validator': 0, 'C17': 0,
                          'C17_sha256': hashlib.sha256(emitted.stdout).hexdigest(),
                          'native': 'original+passive O0/O2 PASS', 'owned': evidence}
        negative = json.loads((folder / 'evidence/baseline.json').read_text())
        for name, result in json.loads((folder / 'evidence/additional-first.json').read_text()).items():
            if name == 'relay':
                continue
            negative[name + '.nl'] = {'check': {'exit': result['check'], 'diagnostic': result['diagnostic']}}
        for name, contract in negative.items():
            if contract['check']['exit'] == 0:
                continue
            source = folder / name
            require(hashlib.sha256(source.read_bytes()).hexdigest() == sha[name], 'negative source freeze')
            diagnostic = contract['check']['diagnostic'].split('error(', 1)[1].split(':', 1)[0]
            for arguments in [['--check'], []]:
                result = run([compiler] + arguments + [source])
                require(result.returncode == 3 and not result.stdout and diagnostic.encode() in result.stderr, result)
            named = root / 'saved.c'
            named.write_bytes(b'KEEP\n')
            result = run([compiler, '--emit-c17-to', named, source])
            require(result.returncode == 3 and not result.stdout and named.read_bytes() == b'KEEP\n', result)
    print(json.dumps(rows, sort_keys=True))


if __name__ == '__main__':
    exercise(*sys.argv[1:])
