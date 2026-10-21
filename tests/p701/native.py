"""Actual normal CLI bytes; separately labelled passive reads never supply data."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def run(args):
    return subprocess.run(list(map(str, args)), capture_output=True)


def require(ok, reason):
    if not ok:
        raise AssertionError(reason)


def passive(text, evidence, oracle):
    additions = {}

    def find(pattern):
        matches = list(re.finditer(pattern, text))
        require(len(matches) == 1, pattern)
        return matches[0]

    def after(match, extra):
        additions[match.end()] = additions.get(match.end(), '') + extra

    def state(prefix, values):
        return ' || '.join(f'*{prefix}_{i} != {v}' for i, v in enumerate(values))

    for label, symbol, initial in [
        ('body', evidence['callee_symbol'], oracle['callee_initial']),
        ('caller', evidence['caller_symbol'],
         [3, 4] if oracle['caller_index'] == 0 else [4, 3]),
    ]:
        root = f'nl_local_{symbol}'
        ctor = find(rf'(?:const )?nl_type_\d+ {root} = \{{[^;]+;')
        extra = ''.join(f'\n    const volatile uint8_t *const obs_{label}_{i} = &{root}.f{i};'
                        for i in range(2))
        extra += f'\n    if ({state("obs_" + label, initial)}) abort();'
        if label == 'body':
            extra += '\n    ++obs_calls;'
        after(ctor, extra)
    current = list(oracle['callee_initial'])
    require(len(evidence['changes']) == len(oracle['updates']), 'all changes')
    for e, (index, new, old) in zip(evidence['changes'], oracle['updates']):
        require(e['index'] == index and current[index] == old, 'field/old oracle')
        require(e['parent_fact'] != e['parent_post'] and
                e['child_fact'] != e['child_post'], 'actual Change/Reset')
        ref = f'nl_local_{e["ref"]}'
        capture = find(rf'const uint8_t nl_replace_old_{e["node"]} = \*{ref};')
        store = find(rf'\*{ref} = ([^;]+);')
        if e['incoming_symbol']:
            require(store[1] == f'nl_local_{e["incoming_symbol"]}', 'old Copy forwarding')
        after(capture, f'\n        if ({ref} != obs_body_{index} || '
              f'nl_replace_old_{e["node"]} != {old} || {state("obs_body", current)}) abort();')
        current[index] = new
        after(store, f'\n        if ({state("obs_body", current)}) abort();')
    call = find(rf'const uint8_t nl_call_{evidence["call"]} = nl_fn_{evidence["function"]}\(\);')
    after(call, f'\n    if (obs_calls != 1 || nl_call_{evidence["call"]} != {oracle["returned"]}) abort();')
    index = oracle['caller_index']
    require(index == evidence['caller_index'], 'caller projection')
    ref = f'nl_local_{evidence["caller_ref"]}'
    old_name = f'nl_replace_old_{evidence["caller_change"]}'
    capture = find(rf'const uint8_t {old_name} = \*{ref};')
    store = find(rf'\*{ref} = nl_local_{evidence["caller_incoming"]};')
    before = [3, 4] if index == 0 else [4, 3]
    final = list(before)
    final[index] = oracle['returned']
    after(capture, f'\n        if ({ref} != obs_caller_{index} || {old_name} != 3 || '
          f'{state("obs_caller", before)}) abort();')
    after(store, f'\n        if ({state("obs_caller", final)}) abort();')
    for position in sorted(additions, reverse=True):
        text = text[:position] + additions[position] + text[position:]
    return '#include <stdlib.h>\nstatic unsigned obs_calls;\n' + text


def exercise(compiler, probe, cc, sanitizer, folder):
    folder = Path(folder)
    frozen = json.loads((folder / 'freeze.json').read_text())
    oracles = json.loads((folder / 'native-oracles.json').read_text())
    rows = {}
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        for name, contract in frozen.items():
            source = folder / (name + '.nl')
            require(hashlib.sha256(source.read_bytes()).hexdigest() == contract['sha256'], 'frozen bytes')
            checked = run([compiler, '--check', source])
            emitted = run([compiler, source])
            require(checked.returncode == 0 and not checked.stdout and not checked.stderr, checked)
            require(emitted.returncode == 0 and not emitted.stderr and emitted.stdout, emitted)
            result = run([probe, source])
            require(result.returncode == 0 and not result.stderr, result)
            evidence = json.loads(result.stdout)
            require(evidence['result'] == contract['returned'] and
                    evidence['callee_root'] != evidence['caller_root'], evidence)
            text = emitted.stdout.decode()
            observed = passive(text, evidence, oracles[name])
            # Equal-value literal substitution is not real call/Copy forwarding.
            fake = text.replace(f'nl_fn_{evidence["function"]}();', str(contract['returned']) + ';')
            try:
                passive(fake, evidence, oracles[name])
            except AssertionError:
                pass
            else:
                raise AssertionError('observer accepted missing actual call')
            for label, code in [('original', text), ('passive', observed)]:
                c = root / (name + '-' + label + '.c')
                c.write_text(code)
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
            rows[name] = {'checked': 0, 'owned_validator': 0, 'emit': 0,
                          'C17_sha256': hashlib.sha256(emitted.stdout).hexdigest(),
                          'native': 'original+passive O0/O2 PASS', 'evidence': evidence}
        for name, contract in json.loads((folder / 'negative-freeze.json').read_text()).items():
            source = folder / (name + '.nl')
            require(hashlib.sha256(source.read_bytes()).hexdigest() == contract['sha256'], 'negative freeze')
            for mode, arguments in [('check', ['--check']), ('emit', [])]:
                result = run([compiler] + arguments + [source])
                require(result.returncode == contract[mode] and not result.stdout, result)
                if result.returncode:
                    require(contract['code'].encode() in result.stderr and
                            ('error(' + contract['category'] + ')').encode() in result.stderr, result)
            named = root / 'saved.c'
            named.write_bytes(b'KEEP\n')
            result = run([compiler, '--emit-c17-to', named, source])
            require(result.returncode == contract['emit'] and not result.stdout and named.read_bytes() == b'KEEP\n', result)
        export = run([compiler, '--experimental-c17-u8-export', 'peak_after_write', folder / 'internal_one.nl'])
        require(export.returncode == 4 and not export.stdout and b'C17-U8-EXPORT-UNSUPPORTED' in export.stderr,
                'foreign contract must remain closed')
    print(json.dumps(rows, sort_keys=True))


if __name__ == '__main__':
    exercise(*sys.argv[1:])
