"""C250a safe mapping gate precedes every repaired guest native compilation."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile
from mapping import roles, inspect

here = Path(__file__).resolve().parent
root = here.parents[2]
compiler, probe, gcc, clang, sanitizer, destination = sys.argv[1:]
base = Path(destination)
base.mkdir(parents=True, exist_ok=True)
raw = Path(tempfile.mkdtemp(prefix='run-', dir=base))
freeze = json.loads((here / 'equipment-freeze-v2.json').read_bytes())
for name, expected in freeze['files'].items():
    assert hashlib.sha256((root / name).read_bytes()).hexdigest() == expected, name
oracle = json.loads((here / 'oracle-v1.json').read_bytes())
rows = []


def run(command, label):
    command = list(map(str, command))
    result = subprocess.run(command, capture_output=True, timeout=120)
    (raw / (label + '.out')).write_bytes(result.stdout)
    (raw / (label + '.err')).write_bytes(result.stderr)
    row = dict(label=label, command=command, status=result.returncode)
    rows.append(row)
    (raw / 'observations.json').write_text(json.dumps(rows, indent=2) + '\n')
    if result.returncode:
        (raw / 'FIRST.json').write_text(json.dumps(row, indent=2) + '\n')
        raise AssertionError(row)
    return result


def observer(meta, view, discard_count):
    def array(values):
        return ','.join(str(v) for v in values) or '0'
    incoming = meta['incoming']
    initial = meta['initial']
    fields = [initial, meta['sibling']]
    if meta['index']:
        fields.reverse()
    expected = {'old': 'olds[phases-1]', 'old_copy': 'olds[phases-1]',
                'incoming': 'incoming[phases-1]', 'during': 'incoming[phases-1]',
                'beside': str(meta['sibling'])}
    cases = []
    for i, (name, local) in enumerate(view['locals'].items()):
        value = 'headers[phases-1]' if local['quantity'] else expected[name]
        cases.append(f"case {local['symbol']}: assert(value=={value}); ++locals[{i}]; break;")
    helper_calls = len(incoming) if view['helper'] else 0
    return f'''#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
static uint8_t *original[2];
static size_t phases,calls,returns,results,reads,discards,locals[{len(cases)}];
static const size_t headers[]={{ {array(meta['headers'])} }};
static const size_t incoming[]={{ {array(incoming)} }};
static const size_t olds[]={{ {array([initial,*incoming[:-1]])} }};
static const size_t child[]={{ {array([initial,*incoming])} }};
static void finish(void){{
 assert(phases=={len(meta['headers'])} && calls=={helper_calls} && returns==calls);
 assert(results==1 && reads==2 && discards=={discard_count*len(incoming)});
 for(size_t i=0;i<sizeof locals/sizeof locals[0];++i)assert(locals[i]=={len(incoming)});
 puts("C250a phase/alias discard physical-original/current PASS");
}}
static void bind_original(uint8_t *a,uint8_t *b){{
 assert(!original[0] && !original[1] && a!=b && *a=={fields[0]} && *b=={fields[1]});
 original[0]=a;original[1]=b;assert(atexit(finish)==0);
}}
static void phase(size_t value,uint8_t *a,uint8_t *b){{
 assert(a==original[0] && b==original[1] && phases<{len(meta['headers'])});
 assert(value==headers[phases] && *original[{meta['index']}]==child[phases]);
 assert(*original[{1-meta['index']}]=={meta['sibling']});++phases;
}}
static void local(size_t symbol,size_t value){{
 assert(phases && phases<={len(incoming)});
 switch(symbol){{ {''.join(cases)} default:assert(0); }}
}}
static void discard(size_t value){{
 assert(phases && phases<={len(incoming)} && value==headers[phases-1]);++discards;
}}
static void result(size_t value){{
 assert(value=={meta['headers'][-1]} && *original[{meta['index']}]=={incoming[-1]});
 assert(*original[{1-meta['index']}]=={meta['sibling']});++results;
}}
static void read_original(uint8_t *p,uint8_t value){{
 assert((p==original[0] || p==original[1]) && value==*p);
 assert(value==(p==original[{meta['index']}]?{incoming[-1]}:{meta['sibling']}));++reads;
}}
#define NL_LOOP_BIND(a,b) bind_original(a,b)
#define NL_CURRENT_PHASE(n,a,b) phase(n,a,b)
#define NL_CURRENT_LOCAL(s,v) local(s,v)
#define NL_CURRENT_RESULT(n) result(n)
#define NL_LOOP_READ(a,v) read_original(a,v)
''' + (f'''
static void helper_input(size_t value){{
 assert(calls==returns && calls+1==phases && calls<{helper_calls});
 assert(value==headers[calls] && *original[{meta['index']}]==child[calls]);++calls;
}}
static uint8_t helper_return(uint8_t value){{
 assert(returns+1==calls && value==incoming[returns]);
 assert(*original[{meta['index']}]==child[returns]);++returns;return value;
}}
''' if view['helper'] else '')


native_runs = 0
for name, meta in oracle['cases'].items():
    source = here / 'sources' / (name + '.nl')
    run([compiler, '--check', source], name + '-check')
    emitted = run([compiler, source], name + '-emit')
    issued = run([probe, source], name + '-issued-roles')
    view = roles(issued.stdout.decode())
    # This gate is mandatory and precedes even writing native build commands.
    mapping = inspect(emitted.stdout.decode(), view, meta['expected_discard_statements'], True)
    assert mapping['bad_phase_reads'] == meta['target_bad_phase_reads']
    (raw / (name + '-safe-mapping.json')).write_text(json.dumps({'roles': view, 'mapping': mapping}, indent=2) + '\n')
    target = raw / (name + '-named.c')
    target.write_bytes(b'C250a original named sentinel\n')
    run([compiler, '--emit-c17-to', target, source], name + '-named')
    assert target.read_bytes() == emitted.stdout
    if meta['category'] == 'control':
        assert emitted.stdout == (here / 'evidence' / (name + '.c17')).read_bytes()
        continue
    # Authenticate every phase/alias discard token, including binding-suppression
    # tokens, then observe its actual value without replacing guest computation.
    carriers = mapping['carriers']
    pattern = r'\(void\)(' + '|'.join(map(re.escape, carriers)) + r');'
    body, observed_discards = re.subn(pattern, r'discard(\1);', mapping['body'])
    assert observed_discards == sum(carriers.values())
    text = emitted.stdout.decode()
    observed = text[:mapping['start']] + body + text[mapping['stop']:]
    if view['helper']:
        helper_pattern = r'(static uint8_t nl_fn_\d+\((?:const )?size_t (nl_local_\d+)\)\n\{\n)(.*?)(^\})'
        def passive(match):
            body, count = re.subn(r'\breturn ([^;\n]+);', r'return helper_return(\1);', match[3])
            assert count == 1
            return match[1] + '    helper_input(' + match[2] + ');\n' + body + match[4]
        observed, count = re.subn(helper_pattern, passive, observed, flags=re.S | re.M)
        assert count == 1
    header = raw / (name + '-observer.h')
    header.write_text(observer(meta, view, observed_discards))
    plain = raw / (name + '.c')
    plain.write_bytes(emitted.stdout)
    instrumented = raw / (name + '-passive.c')
    instrumented.write_text(observed)
    for tag, cc in [('gcc', gcc)] + ([] if gcc == clang else [('clang', clang)]):
        for opt in ['O0', 'O2']:
            for active in [False, True]:
                label = name + '-' + tag + '-' + opt + ('-passive' if active else '-raw')
                exe = raw / label
                flags = ['-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-UNDEBUG', '-' + opt]
                if active:
                    flags += ['-include', header]
                if sanitizer != 'none':
                    flags += ['-fsanitize=' + sanitizer, '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
                run([cc, *flags, instrumented if active else plain, '-o', exe], label + '-build')
                result = run([exe], label + '-native')
                assert not result.stderr
                assert (b'C250a phase/alias discard' in result.stdout) == active
                native_runs += 1
                exe.unlink()
    print(name, 'safe mapping/native PASS', flush=True)
print('C250a corrected mapping/control/native PASS runs', native_runs)
print('C250a raw', raw)
