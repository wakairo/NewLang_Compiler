"""Unchanged Pair baseline only; no novel nominal admission is claimed."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def run(args):
    return subprocess.run(args, text=True, capture_output=True, timeout=90)


def require(ok, detail):
    if not ok:
        raise SystemExit(str(detail))


def observe(compiler, probe, cc, sanitizer):
    directory = Path(__file__).resolve().parent
    source = directory / 'pair_control.nl'
    oracle = json.loads((directory / 'freeze.json').read_text())['cases'][source.name]
    require(hashlib.sha256(source.read_bytes()).hexdigest() == oracle['sha256'], 'changed source')
    checked, emitted = run([compiler, '--check', str(source)]), run([compiler, str(source)])
    require(checked.returncode == emitted.returncode == 0 and not checked.stdout and
            not checked.stderr and not emitted.stderr and emitted.stdout, (checked, emitted))
    public = run([probe, str(source)])
    require(public.returncode == 0 and not public.stderr, public)
    data = json.loads(public.stdout)
    require(data['values'] == [oracle['old_result'], oracle['new_value'], oracle['initial'][1]], data)
    text = emitted.stdout
    root, ref, old = (f"nl_local_{data[key]}" for key in ['root_symbol', 'ref_symbol', 'old_symbol'])
    ctor = re.search(rf'nl_type_\d+ {root} = \{{[^}}]+\}};', text)
    capture = re.search(rf'const uint8_t (nl_replace_old_\d+) = \*{ref};', text)
    pointer = f'uint8_t *const {ref} = &{root}.f0;'
    write = f"*{ref} = {oracle['new_value']};"
    returned = re.search(rf'const uint8_t {old} = nl_loan_result_\d+;', text)
    require(ctor and capture and returned and pointer in text and write in text, 'missing actual write')
    watched = text.replace('#include <stdint.h>', '#include <stdint.h>\n#include <stdlib.h>', 1)
    addresses = (f'\n    const void *const original = &{root};'
                 f'\n    const volatile uint8_t *const first = &{root}.f0;'
                 f'\n    const volatile uint8_t *const second = &{root}.f1;')
    def state(value):
        return (f'&{root} != original || &{root}.f0 != first || &{root}.f1 != second || '
                f'first == second || *first != {value} || *second != {oracle["initial"][1]}')
    watched = watched.replace(ctor[0], ctor[0] + addresses +
                              f'\n    if ({state(oracle["initial"][0])}) exit(91);', 1)
    watched = watched.replace(pointer, pointer + f'\n        if ({ref} != first) exit(91);', 1)
    watched = watched.replace(capture[0], capture[0] +
                              f'\n        if ({capture[1]} != {oracle["old_result"]} || '
                              f'{state(oracle["initial"][0])}) exit(91);', 1)
    watched = watched.replace(write, write + f'\n        if ({state(oracle["new_value"])}) exit(91);', 1)
    watched = watched.replace(returned[0], returned[0] +
                              f'\n    if ({old} != {oracle["old_result"]}) exit(91);', 1)
    results = {}
    with tempfile.TemporaryDirectory(prefix='p670-pair-baseline-') as tmp:
        folder = Path(tmp)
        for label, code in [('plain', text), ('passive', watched)]:
            path = folder / (label + '.c')
            path.write_text(code)
            results[label] = []
            for opt in ['-O0', '-O2']:
                exe = folder / (label + opt)
                flags = [] if sanitizer == 'none' else [f'-fsanitize={sanitizer}', '-fno-sanitize-recover=all']
                built = run([cc, '-std=c17', '-pedantic-errors', '-Wall', '-Wextra', '-Werror',
                             opt, *flags, str(path), '-o', str(exe)])
                require(built.returncode == 0 and not built.stdout, built)
                guest = run([str(exe)])
                require(guest.returncode == 0 and not guest.stdout and not guest.stderr, guest)
                results[label].append(guest.returncode)
    print(json.dumps({'source': source.name, 'source_sha256': oracle['sha256'], 'check': 0,
                      'c17': 0, 'c17_sha256': hashlib.sha256(text.encode()).hexdigest(),
                      'public_observation': data, 'native': results, 'sanitizer': sanitizer,
                      'scope': 'adopted Pair control only; novel types NOT REACHED'}))


if __name__ == '__main__':
    observe(*sys.argv[1:])
