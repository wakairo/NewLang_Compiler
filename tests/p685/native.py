"""Ordinary emitted C is untouched; a separate passive copy observes carriers."""
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


def passive(text, e, oracle):
    def one(pattern):
        found = list(re.finditer(pattern, text))
        require(len(found) == 1, pattern)
        return found[0]
    root = f'nl_local_{e["root_symbol"]}'
    incoming = f'nl_local_{e["incoming_symbol"]}'
    ref = f'nl_local_{e["ref_symbol"]}'
    old = f'nl_local_{e["old_symbol"]}'
    call = f'nl_call_{e["call"]}'
    idx = e['field_index']
    ctor = one(rf'nl_type_\d+ {root} = \{{[^;]+;')
    actual_call = one(rf'const uint8_t {call} = nl_fn_{e["function"]}\(\);')
    receiver = one(rf'const uint8_t {incoming} = {call};')
    pointer = one(rf'uint8_t \*const {ref} = &{root}\.f{idx};')
    capture = one(rf'const uint8_t (\w+) = \*{ref};')
    store = one(rf'\*{ref} = {incoming};')
    prior = one(rf'const uint8_t {old} = (\w+);')
    forwarded = one(rf'{prior[1]} = {capture[1]};')
    reads = [one(rf'const uint8_t nl_local_{sym} = {root}\.f{field};')
             for sym, field in zip(e['read_symbols'], [idx, 1-idx])]
    chain = [ctor, actual_call, receiver, pointer, capture, store, forwarded, prior, *reads]
    require([x.start() for x in chain] == sorted(x.start() for x in chain), 'lost actual call/write order')
    observed = text.replace('#include <stdint.h>', '#include <stdint.h>\n#include <stdlib.h>', 1)
    addresses = (f'\n    const void *const original = &{root};'
                 f'\n    const volatile uint8_t *const field0 = &{root}.f0;'
                 f'\n    const volatile uint8_t *const field1 = &{root}.f1;')
    def state(values):
        return (f'&{root} != original || &{root}.f0 != field0 || &{root}.f1 != field1 || '
                f'field0 == field1 || *field0 != {values[0]} || *field1 != {values[1]}')
    initial, final = oracle['initial'], oracle['final']
    for match, extra in [
        (ctor, addresses + f'\n    if ({state(initial)}) exit(91);'),
        (actual_call, f'\n    if ({call} != {oracle["value"]} || {state(initial)}) exit(92);'),
        (receiver, f'\n    if ({incoming} != {call} || {state(initial)}) exit(93);'),
        (pointer, f'\n        if ({ref} != field{idx}) exit(94);'),
        (capture, f'\n        if ({capture[1]} != {oracle["old"]} || {state(initial)}) exit(95);'),
        (store, f'\n        if ({state(final)}) exit(96);'),
        (prior, f'\n    if ({old} != {oracle["old"]} || {state(final)}) exit(97);'),
    ]:
        observed = observed.replace(match[0], match[0] + extra, 1)
    for match, sym, expected in zip(reads, e['read_symbols'], [oracle['value'], oracle['sibling']]):
        observed = observed.replace(match[0], match[0] + f'\n    if (nl_local_{sym} != {expected} || {state(final)}) exit(98);', 1)
    return observed


def exercise(compiler, probe, cc, sanitizer, folder):
    folder = Path(folder)
    frozen = json.loads((folder / 'freeze.json').read_text())
    result = {}
    for name, digest in frozen['sources'].items():
        source = folder / name
        require(hashlib.sha256(source.read_bytes()).hexdigest() == digest, name)
        want = frozen['expectations'][name]
        check = run([compiler, '--check', source])
        emit = run([compiler, source])
        require(check.returncode == want['check'] and not check.stdout, check)
        require(emit.returncode == want['candidate_emit'], emit)
        if emit.returncode:
            diagnostic = ('P8-BODY-RESULT' if name == 'wrong_return.nl' else
                          'P3-TYPE-MISMATCH' if name == 'wrong_write.nl' else 'V1-BACKEND-UNSUPPORTED')
            require(not emit.stdout and diagnostic.encode() in emit.stderr, emit)
            result[name] = {'check': check.returncode, 'emit': emit.returncode, 'c_bytes': 0, 'diagnostic': diagnostic}
            continue
        require(not emit.stderr and not check.stderr, emit)
        oracle = frozen['positive'][name]
        owned = run([probe, source, oracle['value'], oracle['old'], oracle['sibling'], oracle['field_index']])
        require(owned.returncode == 0 and not owned.stderr, owned)
        e = json.loads(owned.stdout)
        text = emit.stdout.decode()
        observed = passive(text, e, oracle)
        # Reject equal-valued textual substitutions: provenance is not equality.
        for mutant in [text.replace(f'nl_fn_{e["function"]}();', str(oracle['value'])+';', 1),
                       text.replace(f'*nl_local_{e["ref_symbol"]} = nl_local_{e["incoming_symbol"]};',
                                    f'*nl_local_{e["ref_symbol"]} = {oracle["value"]};', 1)]:
            try:
                passive(mutant, e, oracle)
            except AssertionError:
                pass
            else:
                raise AssertionError('passive dataflow control admitted a literal substitute')
        native = {}
        with tempfile.TemporaryDirectory(prefix='p685-native-') as tmp:
            for kind, code in [('original', text), ('passive', observed)]:
                c = Path(tmp) / (kind + '.c'); c.write_text(code)
                native[kind] = []
                for opt in ['-O0', '-O2']:
                    exe = Path(tmp) / (kind + opt)
                    flags = [] if sanitizer == 'none' else [f'-fsanitize={sanitizer}', '-fno-sanitize-recover=all']
                    built = run([cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', opt, *flags, c, '-o', exe])
                    require(built.returncode == 0 and not built.stdout and not built.stderr, built)
                    guest = run([exe])
                    require(guest.returncode == 0 and not guest.stdout and not guest.stderr, guest)
                    native[kind].append(guest.returncode)
        result[name] = {'check': 0, 'emit': 0, 'owned': e, 'c_sha256': hashlib.sha256(emit.stdout).hexdigest(), 'native': native}
    print(json.dumps({'results': result, 'sanitizer': sanitizer, 'scope': 'finite ordinary zero-formal u8 return into field write; no guest output/status API or Product GO'}))


if __name__ == '__main__':
    exercise(*sys.argv[1:])
