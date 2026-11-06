"""Finite P-owned C260 native cohort, after real proof and carrier checks."""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess
import sys

here = Path(__file__).resolve().parents[2] / 'p1203'
repo = here.parents[1]
compiler, probe, cc, sanitizer, destination = sys.argv[1:]
raw = Path(destination)
raw.mkdir(parents=True, exist_ok=False)
for name, digest in json.loads((here / 'source-freeze-v1.json').read_text())['files'].items():
    assert hashlib.sha256((here / name).read_bytes()).hexdigest() == digest
oracle = json.loads((here / 'oracle-v1.json').read_text())
rows = []
native_count = 0

def run(command, label, expected=0):
    command = list(map(str, command))
    result = subprocess.run(command, capture_output=True, timeout=120,
        env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=1:abort_on_error=1',
             'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'})
    (raw / (label + '.out')).write_bytes(result.stdout)
    (raw / (label + '.err')).write_bytes(result.stderr)
    row = dict(label=label, command=command, status=result.returncode, expected=expected)
    rows.append(row)
    (raw / 'observations.json').write_text(json.dumps(rows, indent=2) + '\n')
    if result.returncode != expected:
        (raw / 'FIRST.json').write_text(json.dumps(row, indent=2) + '\n')
        raise AssertionError(row)
    return result

accepted = []
for name in ('04-second-field',):
    meta = oracle['cases'][name]
    source = here / 'sources' / (name + '.nl')
    run([compiler, '--check', source], name + '-check')
    proof = run([probe, source], name + '-roles').stdout.decode()
    text = run([compiler, source], name + '-c17').stdout.decode()
    match = re.search(r'C254 MAP loop=(\d+) phase=(\d+) result=(\d+) base=(\d+) index=(\d+) helper=(\d+)', proof)
    assert match
    loop, phase, result, base, index, helper = map(int, match.groups())
    assert phase == result and index == meta['index']
    locals_by_symbol = {int(s): n for s, n in re.findall(r'C254 LOCAL symbol=(\d+) name=(\w+)', proof)}
    assert set(locals_by_symbol.values()) == set(meta['locals'])
    by_name = {n: s for s, n in locals_by_symbol.items()}
    runtime_phase = f'nl_loop_phase_{loop}'
    result_expression = runtime_phase if name == '80-phase-control' else f'(size_t)UINTMAX_C({meta["exit"]})'
    result_assignment = f'nl_local_{result} = {result_expression};'
    assert result_assignment in text
    start = text.index('for (;;) {')
    stop = text.index(result_assignment, start)
    body = text[start:stop]
    assert not re.search(r'\bnl_local_' + str(result) + r'\b', body), 'caller result used before initialization'
    incoming = f'nl_local_{by_name["incoming"]}'
    assert f'const uint8_t {incoming} = nl_fn_{helper}({runtime_phase});' in body
    assert len(re.findall(r'\*nl_local_\d+ = ' + incoming + ';', body)) == 1
    assert f'nl_loop_carry_{loop} = {runtime_phase} + (size_t)UINTMAX_C(2);' in body
    signature = rf'(static uint8_t nl_fn_{helper}\(size_t (nl_local_\d+)\)\n\{{\n)(.*?)(^\}})'
    fn = re.search(signature, text, re.S | re.M)
    assert fn
    initialized = {fn[2]}
    for line in fn[3].splitlines():
        declaration = re.match(r'\s*const (?:size_t|uint8_t) (nl_\w+) = ([^;]+);', line)
        expression = declaration[2] if declaration else line
        used = set(re.findall(r'\bnl_(?:local|less_left|less_right)_\d+\b', expression))
        assert used <= initialized, ('helper carrier', line, used, initialized)
        if declaration:
            initialized.add(declaration[1])
    assert fn[3].count('return ') == 1
    if name == '80-phase-control':
        assert hashlib.sha256(text.encode()).hexdigest() == '92d1d6ddc1025f90adfffaef340e200ec4ea23373f9232461fba0a5e7892c6fb'
    calls = len(meta['incoming'])
    array = lambda values: ','.join(map(str, values or [0]))
    local_cases = []
    for symbol, local in locals_by_symbol.items():
        kind = meta['locals'][local]
        value = {'incoming': 'incoming[phases-1]', 'old': 'olds[phases-1]', 'peer': str(meta['peer'])}[kind]
        local_cases.append(f'case {symbol}: assert(value=={value}); break;')
    fields = [meta['initial'], meta['peer']]
    if index:
        fields.reverse()
    final = meta['incoming'][-1] if calls else meta['initial']
    header_text = f'''#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
static uint8_t *original[2];
static size_t phases,results,reads,local_count;
static const size_t headers[]={{{array(meta['trace'])}}};
static const size_t incoming[]={{{array(meta['incoming'])}}};
static const size_t olds[]={{{array(meta['old'])}}};
static const size_t child[]={{{array([meta['initial'], *meta['incoming']])}}};
static void finish(void){{
 assert(phases=={len(meta['trace'])} && results==1 && reads==2);
 assert(local_count=={calls * len(local_cases)});
 puts("P1203 physical-original/old/current/sibling/actual-Break PASS");
}}
static void bind(uint8_t *a,uint8_t *b){{
 assert(!original[0] && a!=b && *a=={fields[0]} && *b=={fields[1]});
 original[0]=a;original[1]=b;assert(atexit(finish)==0);
}}
static void phase(size_t value,uint8_t *a,uint8_t *b){{
 assert(a==original[0] && b==original[1] && phases<{len(meta['trace'])});
 assert(value==headers[phases] && *original[{index}]==child[phases]);
 assert(*original[{1-index}]=={meta['peer']});++phases;
}}
static void local(size_t symbol,size_t value){{
 assert(phases && phases<={calls});
 switch(symbol){{{''.join(local_cases)} default:assert(0);}}++local_count;
}}
static void result(size_t value){{
 assert(phases=={len(meta['trace'])} && value==(size_t)UINTMAX_C({meta['exit']}));
 assert(*original[{index}]=={final} && *original[{1-index}]=={meta['peer']});++results;
}}
static void read_original(uint8_t *p,uint8_t value){{
 assert(results==1 && (p==original[0] || p==original[1]) && value==*p);++reads;
}}
#define NL_LOOP_BIND(a,b) bind(a,b)
#define NL_CURRENT_PHASE(v,a,b) phase(v,a,b)
#define NL_CURRENT_LOCAL(s,v) local(s,v)
#define NL_CURRENT_RESULT(v) result(v)
#define NL_LOOP_READ(p,v) read_original(p,v)
'''
    header = raw / (name + '.h')
    header.write_text(header_text)
    generated = raw / (name + '.c')
    generated.write_text(text)
    accepted.append((name, generated, header))
print('C266 selected literal CHECK/current/roles/C17/carrier gates PASS; native0', flush=True)
for name, generated, header in accepted:
    for optimization in ['O0', 'O2']:
        for passive in [False, True]:
            label = name + '-' + optimization + ('-passive' if passive else '-raw')
            exe = raw / label
            flags = ['-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-UNDEBUG', '-' + optimization]
            if passive:
                flags += ['-include', header]
            if sanitizer != 'none':
                applied = 'address,undefined' if sanitizer == 'address' else sanitizer
                flags += ['-g', '-fsanitize=' + applied, '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
            run([cc, *flags, generated, '-o', exe], label + '-build')
            response = run([exe], label + '-native')
            native_count += 1
            (raw / 'native-count.json').write_text(json.dumps({'native': native_count, 'cap': 4}) + '\n')
            assert not response.stderr
            assert bool(response.stdout) == passive
            exe.unlink()
assert native_count == 4
print('C266 literal strict raw/passive native PASS', native_count)
