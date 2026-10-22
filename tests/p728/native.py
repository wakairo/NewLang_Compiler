"""Genuine source and public receipts; unchanged C plus a passive native observer."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def run(args):
    return subprocess.run(list(map(str, args)), capture_output=True)


def require(ok, message):
    if not ok:
        raise AssertionError(message)


def passive(text, v, o):
    base, index, direct = v['base'], v['index'], v['direct'][0]
    receiver, other = direct['base'], direct['index']
    target = [o['sibling'], o['sibling']]
    target[index] = o['original']
    archive = list(o['archive'])
    expected = list(target)
    expected[index] = o['current']
    final = list(archive)
    final[other] = o['archive_new']
    initial = {base: target, receiver: archive}
    additions = {}

    def one(pattern):
        matches = list(re.finditer(pattern, text))
        require(len(matches) == 1, pattern)
        return matches[0]

    def after(match, extra):
        additions[match.end()] = additions.get(match.end(), '') + extra

    def state(values):
        return ' || '.join(f'*observer_{b}_{i} != {value}'
                           for b, pair in values.items()
                           for i, value in enumerate(pair))

    for b, pair in initial.items():
        ctor = one(rf'(?:const )?nl_type_\d+ nl_local_{b} = \{{[^;]+;')
        after(ctor, ''.join(f'\n    observer_{b}_{i} = &nl_local_{b}.f{i};'
                            for i in range(2)) + '\n    if (' +
              ' || '.join(f'*observer_{b}_{i} != {x}' for i, x in enumerate(pair)) + ') exit(91);')
    ref, value, function = v['ref_formal'], v['value_formal'], v['function']
    entry = one(rf'static void nl_fn_{function}\(uint8_t \*nl_local_{ref}, uint8_t nl_local_{value}\)\n\{{')
    after(entry, f'\n    ++observer_calls;\n    if (observer_phase != 1 || nl_local_{ref} != observer_{base}_{index} || {state(initial)}) exit(91);')
    actual = one(rf'nl_fn_{function}\(nl_local_{v["caller_ref"]}, [^;]+;')
    additions[actual.start()] = 'observer_phase=1;\n        '
    after(actual, f'\n        if (observer_calls != 1 || observer_phase != {2 if o["terminal"] else 3} || {state({base:expected, receiver:archive})}) exit(91);\n        observer_phase=4;')
    header = one(r'if \(\(nl_less_left_\d+ < nl_less_right_\d+\)\) \{')
    split = text.index('    } else {', header.end())
    end = text.index('\n    }\n', split) + len('\n    }')
    bounds = [(header.end(), split), (split + len('    } else {'), end)]
    for arm, (lo, hi) in enumerate(bounds):
        additions[lo] = f'\n        observer_arm={arm};'
        for event in v['streams'][arm]:
            require(event['old'] == o['original'], 'source-issued old Copy origin')
            captures = list(re.finditer(rf'const uint8_t nl_replace_old_{event["node"]} = \*nl_local_{ref};', text[lo:hi]))
            require(len(captures) == 1, 'source world + arm-qualified old Copy capture')
            capture_end = lo + captures[0].end()
            additions[capture_end] = f'\n        if (nl_replace_old_{event["node"]} != {o["original"]} || nl_local_{ref} != observer_{base}_{index}) exit(91);'
            store = re.search(rf'\*nl_local_{ref} = [^;]+;', text[capture_end:hi])
            require(store is not None, 'physical field store')
            position = capture_end + store.end()
            post = list(target)
            post[index] = event['incoming']
            additions[position] = f'\n        ++observer_stores;\n        if ({state({base:post,receiver:archive})}) exit(91);'
        returns = list(re.finditer(r'\breturn;', text[lo:hi]))
        if arm != v['normal']:
            require(len(returns) == 1, 'real callee RETURN')
            pos = lo + returns[0].start()
            additions[pos] = 'observer_phase=2;\n        '
        else:
            require(not returns, 'normal continuation is not RETURN')
    additions[end] = f'\n    if (observer_phase != 1 || observer_arm != {v["normal"]}) exit(91);\n    observer_phase=3;'
    for read in v['reads']:
        b, i, symbol = read['base'], read['index'], read['symbol']
        expected_read = (target[i] if read['pre'] else expected[i]) if b == base else final[i]
        m = one(rf'const uint8_t nl_local_{symbol} = nl_local_{b}.f{i};')
        phase = 0 if read['pre'] else (5 if b == receiver else 4)
        after(m, f'\n    if (observer_phase != {phase} || nl_local_{symbol} != {expected_read}) exit(91);')
    dr, node = direct['ref'], direct['node']
    old = one(rf'const uint8_t nl_replace_old_{node} = \*nl_local_{dr};')
    after(old, f'\n        if (observer_phase != 4 || nl_replace_old_{node} != {o["archive_old"]} || nl_local_{dr} != observer_{receiver}_{other}) exit(91);')
    store = re.search(rf'\*nl_local_{dr} = [^;]+;', text[old.end():])
    require(store is not None, 'caller continuation store')
    additions[old.end()+store.end()] = f'\n        if ({state({base:expected,receiver:final})}) exit(91);\n        observer_phase=5;'
    # Clear observer pointers while their caller locals are still live.
    caller_start = min(m.start() for m in re.finditer(r'nl_type_\d+ nl_local_\d+ =', text))
    caller_end = text.index('\n}\n', caller_start)
    additions[caller_end] = '\n    if (observer_phase != 5) exit(91);\n    observer_phase=6;'+''.join(f'\n    observer_{b}_{i}=0;' for b in initial for i in range(2))
    for pos in sorted(additions, reverse=True):
        text = text[:pos] + additions[pos] + text[pos:]
    declarations = '#include <stdlib.h>\nstatic unsigned observer_phase, observer_calls, observer_stores, observer_arm;\n'
    declarations += ''.join(f'static const volatile uint8_t *observer_{b}_{i};\n' for b in initial for i in range(2))
    text = text.replace('typedef struct', declarations+'\ntypedef struct', 1)
    return text.replace('    return 0;\n}', f'    if (observer_phase != 6 || observer_calls != 1 || observer_stores != {len(v['streams'][1-v['normal'] if o['terminal'] else v['normal']])}) return 91;\n    return 0;\n}}')


def main():
    compiler, probe, cc, sanitizer, root = sys.argv[1:]
    root = Path(root)
    frozen = json.loads((root/'freeze.json').read_text())
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        for o in frozen['cases'] + json.loads((root/'inherited-oracle.json').read_text()):
            source = root/(o['name']+'.nl')
            require(hashlib.sha256(source.read_bytes()).hexdigest() == o['sha256'], 'source freeze')
            checked = run([compiler, '--check', source])
            require(checked.returncode == 0 and not checked.stdout and not checked.stderr, checked.stderr)
            generated = run([compiler, source])
            require(generated.returncode == 0 and generated.stdout and not generated.stderr, generated.stderr)
            receipt = run([probe, source])
            require(receipt.returncode == 0 and not receipt.stderr, receipt.stderr)
            v = json.loads(receipt.stdout)
            text = generated.stdout.decode()
            observed = passive(text, v, o)
            for level in ['-O0', '-O2']:
                flags = [cc, '-std=c17', '-Wall', '-Wextra', '-Werror', '-pedantic-errors', level]
                if sanitizer not in ('', 'none'):
                    flags += [f'-fsanitize={sanitizer}', '-fno-omit-frame-pointer', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
                for label, data in [('unchanged', text), ('passive', observed)]:
                    c = tmp/(label+'.c'); exe = tmp/label
                    c.write_text(data)
                    built = run(flags+[c, '-o', exe])
                    require(built.returncode == 0, built.stderr)
                    executed = run([exe])
                    require(executed.returncode == 0 and not executed.stderr, executed.stderr)
                # Separate synthetic C perturbations are never source success evidence.
                variants = [('caller-skipped', observed.replace('observer_phase=4;', 'return;', 1))]
                if o['terminal']:
                    variants.append(('missing-callee-return', observed.replace('observer_phase=2;\n        return;', 'observer_phase=2;', 1)))
                if v['streams'][1-v['normal'] if o['terminal'] else v['normal']]:
                    variants.append(('wrong-store', re.sub(rf'\*nl_local_{v["ref_formal"]} = [^;]+;', f'*nl_local_{v["ref_formal"]} = 0;', observed)))
                for label, data in variants:
                    c = tmp/(label+'.c'); exe = tmp/label
                    c.write_text(data)
                    built = run(flags+[c, '-o', exe])
                    if built.returncode:
                        require(b'-Wunreachable-code' in built.stderr or b'unused' in built.stderr or b'dangling-pointer' in built.stderr, built.stderr)
                        print(o['name'],level,label,'strict compile refusal; native NOT RUN')
                    else:
                        executed = run([exe])
                        require(executed.returncode == 91 and not executed.stderr, (label,executed))
                named = tmp/'output.c'
                named.write_bytes(b'preserved-output')
                written = run([compiler, '--emit-c17-to', named, source])
                require(written.returncode == 0 and named.read_bytes() == generated.stdout, 'named output equality')
            print(o['name'], 'CHECK=0 public=0 C17=0 native O0/O2 unchanged/passive=0 caller continuation=PASS')
        for o in json.loads((root/'negative-expectations.json').read_text()):
            source = root/o['source']
            require(hashlib.sha256(source.read_bytes()).hexdigest() == o['sha256'], 'negative freeze')
            for mode, args in [('check',[compiler,'--check',source]),('emit',[compiler,source])]:
                rejected = run(args)
                require(rejected.returncode==o[mode] and not rejected.stdout and (not rejected.stderr if o[mode]==0 else o['diagnostic'].encode() in rejected.stderr), rejected.stderr)
            if o['check'] == 0:
                public = run([probe, 'reject', source])
                require(public.returncode == 0 and not public.stderr, public.stderr)
            named = tmp/'rejected.c'
            named.write_bytes(b'prior-valid-artifact')
            rejected = run([compiler,'--emit-c17-to',named,source])
            require(rejected.returncode==o['emit'] and named.read_bytes()==b'prior-valid-artifact', 'preserved named output')
            print(o['source'], o.get('classification','semantic'), f"CHECK{o['check']} CLI{o['emit']} zero C preserved output", o['diagnostic'])

if __name__ == '__main__':
    main()
