"""G1: source-owned Pair calls and independently observed actual C17 state.

Observation inserts reads/logging and one callee-entry counter in a COPY of
emitted C. Neither actual arguments, callee writes/returns, nor guest calls are
replaced. All expected values and symbol mappings come from the checked source.
"""
from pathlib import Path
import hashlib
import json
import os
import re
import sys
import tempfile
from checked_c_v0_test import run


def ok(result, label):
    if result.returncode or result.stderr:
        raise SystemExit(f"{label}: {result.returncode}\n{result.stdout}{result.stderr}")


def observe(code, rows):
    records = [r for r in rows if r['kind'] == 'record']
    calls = [r for r in rows if r['kind'] == 'call']
    bindings = [r for r in rows if r['kind'] == 'binding']
    if len(records) != 2 or len(calls) != 2 or calls[0]['function'] != calls[1]['function']:
        raise SystemExit('not two real instances using one checked helper')
    function = calls[0]['function']
    pattern = rf'(?m)^(static uint8_t nl_fn_{function}\(uint8_t \*nl_local_\d+, uint8_t nl_local_\d+\)\n\{{)'
    if len(re.findall(pattern, code)) != 1:
        raise SystemExit('missing single parameterized callee definition')
    result = re.sub(pattern, lambda m: m[1] + '\n    ++g1_calls;', code)
    snapshot = ('    g1_snapshot(%d, ' + ', '.join(
        f'nl_local_{r["symbol"]}.f{j}' for r in records for j in range(2)) + ');\n')
    initial_end = f'    (void)nl_local_{records[1]["symbol"]};\n'
    if initial_end not in result:
        raise SystemExit('no source-issued record carrier')
    result = result.replace(initial_end, initial_end + snapshot % 0, 1)
    expected = [{'kind': 'snapshot', 'step': 0,
                 'values': [v for r in records for v in r['initial']]}]
    state = expected[0]['values'].copy()
    old_bindings = {b['name']: b for b in bindings}
    for step, call in enumerate(calls, 1):
        record_index = next(i for i, r in enumerate(records) if r['symbol'] == call['record'])
        state[2 * record_index + call['index']] = call['new']
        old = old_bindings['old_a' if step == 1 else 'old_b']
        line = f'    (void)nl_local_{old["symbol"]};\n'
        result = result.replace(line, line + snapshot % step, 1)
        expected.append({'kind': 'snapshot', 'step': step, 'values': state.copy()})
    for b in bindings:
        line = f'    (void)nl_local_{b["symbol"]};\n'
        log = (f'    printf("{{\\"kind\\":\\"binding\\",\\"name\\":\\"{b["name"]}\\",'
               f'\\"value\\":%u}}\\n", (unsigned)nl_local_{b["symbol"]});\n')
        if line not in result:
            raise SystemExit('missing actual caller binding')
        result = result.replace(line, line + log, 1)
        expected.append({'kind': 'binding', 'name': b['name'], 'value': b['value']})
    last = bindings[-1]
    line = f'    (void)nl_local_{last["symbol"]};\n'
    result = result.replace(line, line + '    printf("{\\"kind\\":\\"calls\\",\\"value\\":%u}\\n", g1_calls);\n', 1)
    expected.append({'kind': 'calls', 'value': 2})
    prefix = '''#include <stdio.h>
static unsigned g1_calls;
static void g1_snapshot(unsigned step, unsigned a, unsigned am, unsigned b, unsigned bm) {
    printf("{\\"kind\\":\\"snapshot\\",\\"step\\":%u,\\"values\\":[%u,%u,%u,%u]}\\n",step,a,am,b,bm);
}
'''
    # Compare categories independently: instrumentation insertion order differs
    # from semantic node order, but each snapshot step / binding is unique.
    return prefix + result, expected


def equivalent(actual, expected):
    def canonical(rows):
        return sorted(json.dumps(r, sort_keys=True) for r in rows)
    return canonical(actual) == canonical(expected)


def main():
    compiler, probe, cc, sanitizer, fixture = sys.argv[1:]
    source = Path(fixture).read_text()
    if hashlib.sha256(source.encode()).hexdigest() != '4033da51c6b2434b07b5f118ce07c3950ff66cd7b6cbb807573447810323c22f':
        raise SystemExit('p382 witness changed')
    flags = ['-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O0']
    if sanitizer != 'none':
        flags += ['-fsanitize=' + sanitizer, '-fno-sanitize-recover=all',
                  '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
    cases = {'original': source,
             'different': source.replace('u8(7)', 'u8(80)').replace('u8(9)', 'u8(90)')
                                .replace('u8(1)', 'u8(3)').replace('u8(2)', 'u8(4)')
                                .replace('u8(11)', 'u8(123)').replace('u8(13)', 'u8(124)'),
             'actual_locals': source.replace(' let old_a=', ' let next_a=u8(22);let next_b=u8(33);\n let old_a=')
                                    .replace('update_state(w,u8(11))', 'update_state(w,next_a)')
                                    .replace('update_state(w,u8(13))', 'update_state(w,next_b)'),
             'renamed': source.replace('update_state', 'refresh'),
             # Same source-issued old Copy result from A becomes the real value
             # written to independent B. This is a causal state transfer,
             # not the original two literal-new updates with discarded old.
             'causal_old_to_b': source.replace(
                 'update_state(w,u8(13))', 'update_state(w,old_a)'),
             'causal_old_to_b_changed': source.replace(
                 'u8(7)', 'u8(80)').replace('u8(11)', 'u8(123)').replace(
                 'update_state(w,u8(13))', 'update_state(w,old_a)')}
    report = {'sanitizer': sanitizer, 'positive': [], 'negative': [], 'mutants': []}
    with tempfile.TemporaryDirectory(prefix='newlang-g1-') as directory:
        root = Path(directory)

        def native(name, code):
            path, exe = root / (name + '.c'), root / name
            path.write_text(code)
            built = run([cc] + flags + [str(path), '-o', str(exe)])
            ok(built, name + ' strict C17 build')
            executed = run([str(exe)])
            ok(executed, name + ' native')
            return [json.loads(r) for r in executed.stdout.splitlines()]

        for name, text in cases.items():
            path = root / (name + '.nl')
            path.write_text(text)
            checked = run([probe, str(path)])
            ok(checked, name + ' public source-owned probe')
            rows = [json.loads(r) for r in checked.stdout.splitlines()]
            if rows[0] != {'kind': 'status', 'parse': 0, 'register': 0, 'check': 0, 'owned': True}:
                raise SystemExit('not a genuine source-owned artifact')
            if name in ('causal_old_to_b', 'causal_old_to_b_changed'):
                # Independently pre-registered application-level oracle:
                # the value loaded/returned by first source call controls the
                # second source call's actual B state, not just an observer.
                initial_a, new_a = ((7, 11) if name == 'causal_old_to_b'
                                    else (80, 123))
                expected_values = {'old_a': initial_a, 'old_b': 9,
                                   'after_a': new_a, 'marker_a': 1,
                                   'after_b': initial_a, 'marker_b': 2}
                records = [r for r in rows if r['kind'] == 'record']
                calls = [r for r in rows if r['kind'] == 'call']
                bindings = {r['name']: r['value'] for r in rows
                            if r['kind'] == 'binding'}
                if (len(records) != 2 or len(calls) != 2 or
                        records[0]['initial'] != [initial_a, 1] or
                        records[1]['initial'] != [9, 2] or
                        calls[0]['old'] != initial_a or
                        calls[0]['new'] != new_a or
                        calls[1]['old'] != 9 or
                        calls[1]['new'] != calls[0]['old'] or
                        bindings != expected_values):
                    raise SystemExit(
                        f'causal Copy-old source-owned dataflow mismatch: {rows}')
            emitted = run([compiler, str(path)])
            ok(emitted, name + ' emission')
            repeated = run([compiler, str(path)])
            ok(repeated, name + ' repeated emission')
            if emitted.stdout != repeated.stdout:
                raise SystemExit('nondeterministic C17')
            if native(name + '-pristine', emitted.stdout):
                raise SystemExit('unexpected pristine output')
            observed_code, expected = observe(emitted.stdout, rows)
            observations = native(name + '-observed', observed_code)
            if not equivalent(observations, expected):
                raise SystemExit(f'caller/callee state mismatch: {observations} != {expected}')
            report['positive'].append({'name': name, 'source': text,
                'source_sha256': hashlib.sha256(text.encode()).hexdigest(),
                'c_sha256': hashlib.sha256(emitted.stdout.encode()).hexdigest(),
                'checked': rows, 'native': observations,
                'compiler_exit': 0, 'pristine_exit': 0, 'observer_exit': 0})
            if name == 'original':
                call = next(r for r in rows if r['kind'] == 'call')
                wrong_field = emitted.stdout.replace(
                    f'&nl_local_{call["record"]}.f{call["index"]};',
                    f'&nl_local_{call["record"]}.f{1-call["index"]};', 1)
                missed = re.sub(rf'(nl_loan_result_{call["scope"]} = )nl_fn_\d+\([^;]+\);',
                                r'\g<1>0;', emitted.stdout, count=1)
                formal_next = call['formal_symbols'][1]
                wrong_return = re.sub(r'return (nl_replace_old_\d+);',
                    rf'(void)\1; return nl_local_{formal_next};', emitted.stdout, count=1)
                for label, mutant in [('wrong_field', wrong_field), ('missed_call', missed),
                                      ('wrong_return', wrong_return)]:
                    if mutant == emitted.stdout:
                        raise SystemExit('no controlled corruption produced')
                    mutant_code, _ = observe(mutant, rows)
                    actual = native(label, mutant_code)
                    if equivalent(actual, expected):
                        raise SystemExit('observer accepted corrupted output: ' + label)
                    report['mutants'].append({'name': label, 'native_exit': 0,
                                              'observer_comparison': 'REJECT', 'native': actual})
        declaration = 'struct Pair{left:u8,right:u8}\n'
        helper = 'fn update_state(field:ref<write,u8>,next:u8)->u8{replace(field,next)}\n'
        rejects = {
            'escape': (declaration + 'fn main()->unit{let p=Pair{left:u8(7),right:u8(9)};'
                       'let bad=loan_write(p@left){|w|w};unit}', 3, 'P8-EXIT-DEPENDENCY'),
            'range': (source.replace('u8(11)', 'u8(256)'), 3, 'V1-U8-LITERAL-RANGE'),
            'return_type': (source.replace('replace(field,next)', 'unit'), 3, 'P8-BODY-RESULT'),
            'read_actual': (declaration + helper + 'fn main()->unit{let x=u8(7);'
                            'loan_read(x){|r|update_state(r,u8(11))};unit}', 3, 'P3-TYPE-MISMATCH'),
            'no_authority': (declaration + helper + 'fn main()->unit{let p=Pair{left:u8(7),right:u8(9)};'
                              'update_state(p@left,u8(11));unit}', 3, 'P3-TYPE-MISMATCH'),
            'arbitrary_record': (source.replace('Pair', 'State'), 3, 'FIELD-PROFILE'),
            'richer_body': (source.replace('{replace(field,next)}', '{let old=replace(field,next);old}'),
                            4, 'V1-BACKEND-UNSUPPORTED'),
            'scalar_parameter': ('fn identity(x:u8)->u8{x}fn main()->unit{identity(u8(7));unit}',
                                 4, 'V1-BACKEND-UNSUPPORTED')}
        for name, (text, status, diagnostic) in rejects.items():
            path = root / (name + '.nl')
            path.write_text(text)
            result = run([compiler, str(path)])
            if result.returncode != status or result.stdout or diagnostic not in result.stderr:
                raise SystemExit(f'{name} rejection mismatch: {result}')
            report['negative'].append({'name': name, 'source': text, 'exit': status,
                'diagnostic': diagnostic, 'stderr': result.stderr.replace(str(root), '<fixture-dir>'), 'c_bytes': 0})
    if destination := os.environ.get('NEWLANG_G1_EVIDENCE'):
        Path(destination).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
