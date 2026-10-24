"""Frozen P-owned source/public proof and existing C17/native, no R fixtures."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile

compiler, probe, cc, sanitizer, directory, mode = sys.argv[1:]
root = Path(directory)
frozen = json.loads((root / 'freeze.json').read_text())

def run(args):
    return subprocess.run(list(map(str, args)), capture_output=True)

def require(process):
    assert process.returncode == 0, (process.args, process.returncode,
                                     process.stdout, process.stderr)
    return process

def passive(text, v, o):
    base = v['base']
    receiver = v['direct'][0]['base']
    index = o['field_index']
    mirror = [68, 214] if index == 0 else [31, 208]
    target = [o['sibling'], o['sibling']]
    target[index] = o['old']
    initial = {base: target, receiver: mirror}
    final = {key: value[:] for key, value in initial.items()}
    final[base][index] = o['runtime_field']
    final[receiver][1 - index] = o['runtime_field']
    edits = {}
    def one(pattern):
        found = list(re.finditer(pattern, text))
        assert len(found) == 1, pattern
        return found[0]
    def after(match, extra):
        edits[match.end()] = edits.get(match.end(), '') + extra
    def states(values):
        return ' || '.join(f'*watch_{b}_{i} != {value}'
                           for b, values in values.items()
                           for i, value in enumerate(values))
    for b, values in initial.items():
        ctor = one(rf'(?:const )?nl_type_\d+ nl_local_{b} = \{{[^;]+;')
        after(ctor, ''.join(f'\n    watch_{b}_{i}=&nl_local_{b}.f{i};'
                            for i in range(2)) + '\n    if (' +
              ' || '.join(f'*watch_{b}_{i}!={value}'
                          for i, value in enumerate(values)) + ') exit(91);')
    ref, incoming, function = v['ref_formal'], v['value_formal'], v['function']
    entry = one(rf'static void nl_fn_{function}\(uint8_t \*nl_local_{ref}, uint8_t nl_local_{incoming}\)\n\{{')
    after(entry, f'\n    ++watch_calls; if (nl_local_{ref} != watch_{base}_{index} || nl_local_{incoming}!={o["incoming"]} || {states(initial)}) exit(91);')
    actual = one(rf'nl_fn_{function}\(nl_local_{v["caller_ref"]}, [^;]+;')
    after(actual, f'\n    ++watch_continued; if (watch_calls!=1 || *watch_{base}_{index}!={o["runtime_field"]} || *watch_{base}_{1-index}!={o["sibling"]}) exit(91);')
    event = v['streams'][v['normal']][0]
    capture = one(rf'const uint8_t nl_replace_old_{event["node"]} = \*nl_local_{ref};')
    after(capture, f'\n        if (nl_replace_old_{event["node"]}!={o["old"]} || nl_local_{ref}!=watch_{base}_{index}) exit(91);')
    store = re.search(rf'\*nl_local_{ref} = [^;]+;', text[capture.end():])
    assert store
    location = capture.end() + store.end()
    edits[location] = f'\n        ++watch_stores; if (*watch_{base}_{index}!={o["incoming"]} || *watch_{base}_{1-index}!={o["sibling"]}) exit(91);'
    for read in v['reads']:
        binding = one(rf'const uint8_t nl_local_{read["symbol"]} = [^;]+;')
        b, i = read['base'], read['index']
        value = initial[b][i] if binding.start() < actual.start() else final[b][i]
        # Receiver read happens after the caller's direct write.
        after(binding, f'\n    if (nl_local_{read["symbol"]}!={value}) exit(91);')
    selected = (o['truth'] and o['normal_arm'] == 0) or (not o['truth'] and o['normal_arm'] == 1)
    end = one(r'    return 0;\n}')
    edits[end.start()] = f'    if (watch_calls!=1 || watch_continued!=1 || watch_stores!={int(selected)} || {states(final)}) return 91;\n'
    for position, addition in sorted(edits.items(), reverse=True):
        text = text[:position] + addition + text[position:]
    declarations = '#include <stdlib.h>\nstatic unsigned watch_calls,watch_stores,watch_continued;\n'
    declarations += ''.join(f'static const volatile uint8_t *watch_{b}_{i};\n'
                            for b in initial for i in range(2))
    return text.replace('typedef struct', declarations + '\ntypedef struct', 1)

with tempfile.TemporaryDirectory(prefix='p866-') as temporary:
    tmp = Path(temporary)
    for name, o in frozen['cases'].items():
        source = root / 'sources' / name
        assert hashlib.sha256(source.read_bytes()).hexdigest() == frozen['source_sha256'][name]
        checked = require(run([compiler, '--check', source]))
        assert not checked.stdout and not checked.stderr
        view = json.loads(require(run([probe, source])).stdout)
        assert view['unit'] == view['parameters'] == 0 and view['postcall_unknown']
        assert view['normal'] == o['normal_arm'] and view['index'] == o['field_index']
        for stream in range(3):
            events = view['streams'][stream]
            expected = 1 if stream == 2 or stream == o['normal_arm'] else 0
            assert len(events) == expected
            for event in events:
                assert event['old'] == o['old'] and event['incoming'] == o['incoming']
        if mode == 'public':
            print(name, 'CHECK0 unit0 parameters0 dedicated/events0 old/new/sibling and conservative-unknown verified')
            continue
        generated = require(run([compiler, source]))
        assert generated.stdout and not generated.stderr
        text = generated.stdout.decode()
        observed = passive(text, view, o)
        saved = tmp / 'saved.c'
        saved.write_bytes(b'old correct C')
        named = require(run([compiler, '--emit-c17-to', saved, source]))
        assert not named.stdout and saved.read_bytes() == generated.stdout
        for optimization in ['-O0', '-O2']:
            flags = [cc, '-std=c17', '-Wall', '-Wextra', '-Werror', '-pedantic-errors', optimization]
            if sanitizer != 'none':
                flags += ['-fsanitize=' + sanitizer, '-fno-omit-frame-pointer', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
            for label, contents in [('unchanged', text), ('passive', observed)]:
                c = tmp / (label + '.c')
                exe = tmp / label
                c.write_text(contents)
                require(run([*flags, c, '-o', exe]))
                executed = require(run([exe]))
                assert not executed.stderr
            print(name, optimization, 'strict unchanged/passive native0', 'field', o['runtime_field'], 'sibling', o['sibling'])
    if mode != 'public':
        for name, diagnostic in frozen['negative'].items():
            source = root / 'sources' / name
            assert hashlib.sha256(source.read_bytes()).hexdigest() == frozen['source_sha256'][name]
            for args in [['--check'], []]:
                rejected = run([compiler, *args, source])
                assert rejected.returncode == 3 and not rejected.stdout
                assert ('error(semantic)[' + diagnostic.split()[-1] + ']').encode() in rejected.stderr
            saved.write_bytes(b'old correct C')
            rejected = run([compiler, '--emit-c17-to', saved, source])
            assert rejected.returncode == 3 and not rejected.stdout
            assert saved.read_bytes() == b'old correct C'
            assert not list(tmp.glob('.saved.c*'))
            print(name, diagnostic, 'CLI3 zero C sentinel preserved')
