"""P878 independently frozen unit-loan binding and statement controls."""
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
    mirror = o['receiver']
    target = o['original'][:]
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
    def function_body(name):
        definition = one(rf'\b(?:static\s+)?(?:void|int)\s+{re.escape(name)}\(void\)\s*\{{')
        opening = definition.end() - 1
        depth = 1
        # Ignore comments/literals when matching actual C lexical braces.
        tokens = re.finditer(r'''/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|[{}]''',
                             text[opening + 1:], re.S)
        for token in tokens:
            if token.group() == '{':
                depth += 1
            elif token.group() == '}':
                depth -= 1
                if depth == 0:
                    return opening, opening + 1 + token.start()
        raise AssertionError('unclosed owning C function')
    main_open, main_close = function_body('main')
    entries = list(re.finditer(r'\b(nl_fn_\d+)\(\);',
                               text[main_open + 1:main_close]))
    assert len(entries) == 1
    owner_open, owner_close = function_body(entries[0].group(1))
    # These frozen source entry bodies have no terminating edge. Callee
    # RETURN belongs to another function, never to this owning scope.
    assert not re.search(r'\breturn\b', text[owner_open + 1:owner_close])
    edits[owner_open + 1] = '\n    watch_calls=watch_stores=watch_continued=watch_checked=0;'
    for b, values in initial.items():
        ctor = one(rf'(?:const )?nl_type_\d+ nl_local_{b} = \{{[^;]+;')
        assert owner_open < ctor.start() < ctor.end() < owner_close
        after(ctor, ''.join(f'\n    watch_{b}_{i}=&nl_local_{b}.f{i};'
                            for i in range(2)) + '\n    if (' +
              ' || '.join(f'*watch_{b}_{i}!={value}'
                          for i, value in enumerate(values)) + ') exit(91);')
    ref, incoming, function = v['ref_formal'], v['value_formal'], v['function']
    entry = one(rf'static void nl_fn_{function}\(uint8_t \*nl_local_{ref}, uint8_t nl_local_{incoming}\)\n\{{')
    after(entry, f'\n    ++watch_calls; if (nl_local_{ref} != watch_{base}_{index} || nl_local_{incoming}!={o["incoming"]} || {states(initial)}) exit(91);')
    actual = one(rf'nl_fn_{function}\(nl_local_{v["caller_ref"]}, [^;]+;')
    assert owner_open < actual.start() < actual.end() < owner_close
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
    # Dereference originals only before the owning block ends. Clear every
    # address before return; wrapper main retains only scalar observations.
    edits[owner_close] = f'    if (watch_calls!=1 || watch_continued!=1 || watch_stores!={int(selected)} || {states(final)}) exit(91);\n    watch_checked=1;\n'
    edits[owner_close] += ''.join(f'    watch_{b}_{i}=NULL;\n'
                                  for b in initial for i in range(2))
    end = one(r'    return 0;\n}')
    assert main_open < end.start() < main_close
    edits[end.start()] = f'    if (watch_checked!=1 || watch_calls!=1 || watch_continued!=1 || watch_stores!={int(selected)}) return 91;\n'
    for position, addition in sorted(edits.items(), reverse=True):
        text = text[:position] + addition + text[position:]
    declarations = '#include <stdlib.h>\nstatic unsigned watch_calls,watch_stores,watch_continued,watch_checked;\n'
    declarations += ''.join(f'static const volatile uint8_t *watch_{b}_{i};\n'
                            for b in initial for i in range(2))
    return text.replace('typedef struct', declarations + '\ntypedef struct', 1)

with tempfile.TemporaryDirectory(prefix='p878-') as temporary:
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
        assert f'nl_loan_result_{view["scope"]}' not in text
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
        for name, expected in {**frozen['negative'], **frozen['profile']}.items():
            source = root / 'sources' / name
            assert hashlib.sha256(source.read_bytes()).hexdigest() == frozen['source_sha256'][name]
            for args in [['--check'], []]:
                rejected = run([compiler, *args, source])
                code = expected.get('check_exit' if args else 'emit_exit', expected.get('exit'))
                assert rejected.returncode == code and not rejected.stdout, (name, args, rejected)
                if code:
                    assert expected['diagnostic'].encode() in rejected.stderr
            saved.write_bytes(b'old correct C')
            rejected = run([compiler, '--emit-c17-to', saved, source])
            code = expected.get('emit_exit', expected.get('exit'))
            assert rejected.returncode == code and not rejected.stdout
            assert saved.read_bytes() == b'old correct C'
            assert not list(tmp.glob('.saved.c*'))
            print(name, 'CLI', code, expected['diagnostic'], 'C0 sentinel preserved no temporary leftovers')
