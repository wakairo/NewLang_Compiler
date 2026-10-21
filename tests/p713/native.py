"""Actual ordinary CLI C; separate passive reads never supply guest values."""
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
    additions = {}

    def find(pattern, bounds=None):
        lo, hi = bounds if bounds else (0, len(text))
        matches = list(re.finditer(pattern, text[lo:hi]))
        require(len(matches) == 1, pattern)
        return matches[0], lo

    def after(pair, extra):
        match, offset = pair
        position = match.end() + offset
        additions[position] = additions.get(position, '') + extra

    def state(values):
        return ' || '.join(f'*obs_{i} != {v}' for i, v in enumerate(values))

    base, index, conditional = evidence['base'], evidence['index'], evidence['if']
    require(index == oracle['index'], 'source-issued projection')
    require(evidence['parent_before'] != evidence['parent_after'], 'new parent ValueFact')
    ctor = find(rf'(?:const )?nl_type_\d+ nl_local_{base} = \{{[^;]+;')
    after(ctor, ''.join(f'\n    const volatile uint8_t *const obs_{i} = &nl_local_{base}.f{i};'
                       for i in range(2)) + f'\n    if ({state(oracle["initial"])}) exit(91);')
    # Both real branches and the actual computed quantity comparison are mandatory.
    header = find(r'if \(\(nl_less_left_\d+ < nl_less_right_\d+\)\) \{')
    split = text.index('    } else {', header[0].end())
    end = text.index('\n    }\n', split) + len('\n    }')
    bounds = [(header[0].end(), split), (split + len('    } else {'), end)]
    if 'updates' in oracle:
        updates = oracle['updates']
    else:
        updates = [[[index, value, oracle['old']]] if value is not None else []
                   for value in oracle['incoming']]
    for a in range(2):
        position = bounds[a][0]
        additions[position] = additions.get(position, '') + f'\n        obs_arm = {a};'
        events = [e for e in evidence['arms'][a]['events'] if e['change']]
        require(len(events) == oracle['changes'][a] == len(updates[a]), 'all actual Change events')
        current = list(oracle['initial'])
        for e, (field, incoming, old) in zip(events, updates[a]):
            require(current[field] == old and field == index, 'independent old/field oracle')
            ref = f'nl_local_{e["ref"]}'
            capture = find(rf'const uint8_t nl_replace_old_{e["node"]} = \*{ref};', bounds[a])
            store = find(rf'\*{ref} = ([^;]+);', bounds[a])
            if e['incoming_symbol']:
                require(store[0][1] == f'nl_local_{e["incoming_symbol"]}', 'real old Copy dataflow')
            after(capture, f'\n            if ({ref} != obs_{field} || '
                  f'nl_replace_old_{e["node"]} != {old} || {state(current)}) exit(91);')
            current[field] = incoming
            after(store, f'\n            ++obs_changes;\n            if ({state(current)}) exit(91);')
        if not events:
            require(not re.search(r'\*nl_local_\d+ = ', text[slice(*bounds[a])]), 'no fake Change in unchanged arm')
    result = find(rf'const uint8_t nl_local_{evidence["result_binding"]} = nl_if_result_{conditional};')
    after(result, f'\n    if (obs_arm != {oracle["taken"]} || '
          f'obs_changes != {oracle["changes"][oracle["taken"]]} || '
          f'nl_local_{evidence["result_binding"]} != {oracle["old"]} || {state(oracle["post"])}) exit(91);')
    for read in evidence['reads']:
        actual = find(rf'const uint8_t nl_local_{read["symbol"]} = nl_local_{base}.f{read["index"]};')
        after(actual, f'\n    if (nl_local_{read["symbol"]} != {oracle["post"][read["index"]]}) exit(91);')
    for position in sorted(additions, reverse=True):
        text = text[:position] + additions[position] + text[position:]
    return '#include <stdlib.h>\nstatic unsigned obs_arm = 99, obs_changes;\n' + text


def exercise(compiler, probe, cc, sanitizer, directory):
    folder = Path(directory)
    freeze = json.loads((folder / 'freeze.json').read_text())
    extra = json.loads((folder / 'extra-freeze.json').read_text())
    positive = dict(freeze['positive'], **extra['positive'])
    rows = {}
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        for name, oracle in positive.items():
            source = folder / (name + '.nl')
            sha = oracle.get('sha256', extra['sha256'].get(source.name))
            require(hashlib.sha256(source.read_bytes()).hexdigest() == sha, 'frozen source')
            checked = run([compiler, '--check', source])
            emitted = run([compiler, source])
            require(checked.returncode == 0 and not checked.stdout and not checked.stderr, checked)
            require(emitted.returncode == 0 and emitted.stdout and not emitted.stderr, emitted)
            result = run([probe, source])
            require(result.returncode == 0 and not result.stderr, result)
            evidence = json.loads(result.stdout)
            code = emitted.stdout.decode()
            observed = passive(code, evidence, oracle)
            fake = re.sub(r'if \(\(nl_less_left_\d+ < nl_less_right_\d+\)\)', 'if (1)', code)
            try:
                passive(fake, evidence, oracle)
            except AssertionError:
                pass
            else:
                raise AssertionError('constant-condition shortcut accepted')
            index, base = evidence['index'], evidence['base']
            mutants = {
                'wrong_condition': re.sub(r'(nl_less_left_\d+) < (nl_less_right_\d+)', r'\1 >= \2', observed),
            }
            # An unchanged runtime arm executes no ref/store/capture. Do not
            # call unexecuted corruption a rejected native mutation, or mutate
            # the passive observer's own pointer instead of the guest ref.
            if oracle['changes'][oracle['taken']]:
                mutants['wrong_field'] = re.sub(
                    rf'(uint8_t \*const nl_local_\d+ = &nl_local_{base}\.)f{index};',
                    rf'\g<1>f{1-index};', observed)
                mutants['wrong_old'] = re.sub(
                    r'(const uint8_t nl_replace_old_\d+ = )\*nl_local_\d+;', r'\g<1>0;', observed)
            require(all(text != observed for text in mutants.values()), 'ineffective synthetic C mutation')
            for label, text in [('original', code), ('passive', observed), *mutants.items()]:
                c = root / (name + '-' + label + '.c')
                c.write_text(text)
                for opt in ['-O0', '-O2']:
                    exe = root / (name + label + opt)
                    args = [cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', opt, c, '-o', exe]
                    if sanitizer != 'none':
                        args += [f'-fsanitize={sanitizer}', '-fno-sanitize-recover=all',
                                 '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
                    build = run(args)
                    require(build.returncode == 0, build.stderr.decode())
                    guest = run([exe])
                    require(guest.returncode == (91 if label in mutants else 0) and not guest.stdout and not guest.stderr, guest)
            named = root / 'saved.c'
            named.write_bytes(b'OLD\n')
            saved = run([compiler, '--emit-c17-to', named, source])
            require(saved.returncode == 0 and not saved.stdout and named.read_bytes() == emitted.stdout, saved)
            rows[name] = {'check': 0, 'validator': 0, 'C17': 0, 'native': 'original+passive O0/O2 PASS',
                          'C17_sha256': hashlib.sha256(emitted.stdout).hexdigest(), 'owned': evidence, 'synthetic_C_negative': list(mutants)}
        negatives = json.loads((folder / 'evidence/baseline.json').read_text())
        for name, contract in freeze['negative'].items():
            source = folder / (name + '.nl')
            require(hashlib.sha256(source.read_bytes()).hexdigest() == contract['sha256'], 'negative freeze')
            row = negatives[source.name]['check']
            diagnostic = row['diagnostic'].split('error(', 1)[1].split(':', 1)[0]
            for args in [['--check'], []]:
                result = run([compiler] + args + [source])
                require(result.returncode == row['exit'] and not result.stdout and diagnostic.encode() in result.stderr, result)
            rollback = run([Path(compiler).parent / 'p713_gate_test', 'reject', source])
            require(rollback.returncode == 0 and b'rollback=PASS artifact=NULL' in rollback.stdout, rollback)
            named.write_bytes(b'KEEP\n')
            rejected = run([compiler, '--emit-c17-to', named, source])
            require(rejected.returncode == row['exit'] and not rejected.stdout and named.read_bytes() == b'KEEP\n', rejected)
        extras = json.loads((folder / 'evidence/extra-first.json').read_text())
        for name in ['noncopy_availability', 'wrong_field', 'sole_normal']:
            source = folder / (name + '.nl')
            require(hashlib.sha256(source.read_bytes()).hexdigest() == extra['sha256'][source.name], 'supplementary freeze')
            check = run([compiler, '--check', source])
            emit = run([compiler, source])
            require(check.returncode == extras[name]['check'] and not check.stdout, check)
            if name == 'sole_normal':
                require(emit.returncode == 4 and not emit.stdout and b'V1-BACKEND-UNSUPPORTED' in emit.stderr, emit)
            else:
                code = extras[name]['diagnostic'].split('error(', 1)[1].split(':', 1)[0]
                require(emit.returncode == 3 and not emit.stdout and code.encode() in emit.stderr, emit)
    print(json.dumps(rows, sort_keys=True))


if __name__ == '__main__':
    exercise(*sys.argv[1:])
