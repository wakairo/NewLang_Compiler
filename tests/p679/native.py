"""P679 actual generated guest; passive checks never initialize/mutate guest state."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def run(args):
    return subprocess.run(args, capture_output=True, text=True, timeout=60)


def require(ok, value):
    if not ok:
        raise SystemExit(str(value))


def observe(compiler, probe, cc, sanitizer, source, manifest, comparison):
    source, manifest = Path(source), Path(manifest)
    frozen = json.loads(manifest.read_text())
    require(hashlib.sha256(source.read_bytes()).hexdigest() == frozen['sha256']['device_settings_swap.nl'], 'changed frozen source')
    checked, emitted = run([compiler, '--check', str(source)]), run([compiler, str(source)])
    require(checked.returncode == emitted.returncode == 0 and not checked.stdout and not checked.stderr and not emitted.stderr, (checked, emitted))
    owned = run([probe, str(source)])
    require(owned.returncode == 0 and not owned.stderr, owned)
    e = json.loads(owned.stdout)
    require(e['old_values'] == [31, 7] and e['write_values'] == [7, 31] and e['final_values'] == [31, 7] and e['source_identifier_forwarding'], e)
    require(hashlib.sha256(Path(comparison).read_bytes()).hexdigest() == frozen['sha256']['comparison.c'], 'changed frozen C comparison')
    text = emitted.stdout
    root = f"nl_local_{e['root_symbol']}"
    refs = [f'nl_local_{s}' for s in e['ref_symbols']]
    olds = [f'nl_local_{s}' for s in e['old_symbols']]
    reads = [f'nl_local_{s}' for s in e['read_symbols']]
    def one(pattern):
        found = list(re.finditer(pattern, text))
        require(len(found) == 1, ('expected one generated operation', pattern, len(found)))
        return found[0]
    ctor = one(rf'nl_type_{e["nominal"]} {root} = \{{[^}}]+\}};')
    saved = one(rf'const uint8_t {reads[0]} = {root}\.f0;')
    pointers, captures, writes, returns, bindings = [], [], [], [], []
    for i, field in enumerate([1, 0]):
        pointers.append(one(rf'uint8_t \*const {refs[i]} = &{root}\.f{field};'))
        # Discover actual C temporary spelling; semantic IDs determine the
        # source binding/ref mapping, never a guessed address or injected value.
        captures.append(one(rf'const uint8_t (\w+) = \*{refs[i]};'))
        input_name = reads[0] if i == 0 else olds[0]
        writes.append(one(rf'\*{refs[i]} = {input_name};'))
        bindings.append(one(rf'const uint8_t {olds[i]} = (\w+);'))
        returns.append(one(rf'{bindings[i][1]} = {captures[i][1]};'))
    positions = [ctor.start(), saved.start()]
    for i in range(2):
        positions += [pointers[i].start(), captures[i].start(), writes[i].start(), returns[i].start(), bindings[i].start()]
    require(positions == sorted(positions), 'lost source order or Copy-old causality')
    after = [one(rf'const uint8_t {reads[1]} = {root}\.f0;'), one(rf'const uint8_t {reads[2]} = {root}\.f1;')]
    require(all(v.start() > bindings[1].start() for v in after), 'post-state read before second write')
    watched = text.replace('#include <stdint.h>', '#include <stdint.h>\n#include <stdlib.h>', 1)
    addresses = (f'\n    const void *const original = &{root};'
                 f'\n    const volatile uint8_t *const port_address = &{root}.f0;'
                 f'\n    const volatile uint8_t *const mode_address = &{root}.f1;')
    def state(port, mode):
        return (f'&{root} != original || &{root}.f0 != port_address || &{root}.f1 != mode_address || '
                f'port_address == mode_address || *port_address != {port} || *mode_address != {mode}')
    watched = watched.replace(ctor[0], ctor[0] + addresses + f'\n    if ({state(7, 31)}) exit(91);', 1)
    watched = watched.replace(saved[0], saved[0] + f'\n    if ({reads[0]} != 7 || {state(7, 31)}) exit(92);', 1)
    for i, (before, current, old, address) in enumerate([((7, 31), (7, 7), 31, 'mode_address'), ((7, 7), (31, 7), 7, 'port_address')]):
        watched = watched.replace(pointers[i][0], pointers[i][0] + f'\n        if ({refs[i]} != {address}) exit(93);', 1)
        watched = watched.replace(captures[i][0], captures[i][0] + f'\n        if ({captures[i][1]} != {old} || {state(*before)}) exit(94);', 1)
        watched = watched.replace(writes[i][0], writes[i][0] + f'\n        if ({state(*current)}) exit(95);', 1)
        watched = watched.replace(bindings[i][0], bindings[i][0] + f'\n    if ({olds[i]} != {old} || {state(*current)}) exit(96);', 1)
    for v, name, expected in zip(after, reads[1:], [31, 7]):
        watched = watched.replace(v[0], v[0] + f'\n    if ({name} != {expected} || {state(31, 7)}) exit(97);', 1)
    results = {}
    with tempfile.TemporaryDirectory(prefix='p679-actual-guest-') as tmp:
        for label, code in [('original', text), ('passive', watched), ('separate_factored_C_comparison', Path(comparison).read_text())]:
            path = Path(tmp) / (label + '.c')
            path.write_text(code)
            results[label] = []
            for opt in ['-O0', '-O2']:
                exe = Path(tmp) / (label + opt)
                flags = [] if sanitizer == 'none' else [f'-fsanitize={sanitizer}', '-fno-sanitize-recover=all']
                built = run([cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', opt, *flags, str(path), '-o', str(exe)])
                require(built.returncode == 0 and not built.stdout, built)
                guest = run([str(exe)])
                require(guest.returncode == 0 and not guest.stdout and not guest.stderr, guest)
                results[label].append(guest.returncode)
    print(json.dumps({'source_sha256': frozen['sha256']['device_settings_swap.nl'], 'c17_sha256': hashlib.sha256(text.encode()).hexdigest(), 'owned': e, 'check_cli': 0, 'c17_cli': 0, 'native': results, 'sanitizer': sanitizer, 'scope': 'TECHNICAL composition subcheckpoint only; no natural NewLang stdout or Product GO'}))


if __name__ == '__main__':
    observe(*sys.argv[1:])
