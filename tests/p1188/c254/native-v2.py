"""Source-owned CHECK and C carrier safety precede every C254 guest build."""
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
sha = lambda b: hashlib.sha256(b).hexdigest()
freeze = json.loads((here / 'equipment-freeze-v2.json').read_bytes())
for name, expected in freeze['files'].items():
    assert sha((root / name).read_bytes()) == expected, name
oracle = json.loads((here / 'oracle-v1.json').read_bytes())
extension = json.loads((here / 'oracle-extension-v3.json').read_bytes())
rows = []
accepted = []


def run(command, label, expected=0):
    command = list(map(str, command))
    result = subprocess.run(command, capture_output=True, timeout=120)
    (raw / (label + '.out')).write_bytes(result.stdout)
    (raw / (label + '.err')).write_bytes(result.stderr)
    row = dict(label=label, command=command, status=result.returncode, expected=expected)
    rows.append(row)
    (raw / 'observations.json').write_text(json.dumps(rows, indent=2) + '\n')
    if result.returncode != expected:
        (raw / 'FIRST.json').write_text(json.dumps(row, indent=2) + '\n')
        raise AssertionError(row)
    return result


def observer(meta, view, mapping):
    def array(xs):
        return ','.join(map(str, xs))
    calls = len(meta['incoming'])
    fields = [meta['initial'], meta['peer']]
    if meta['index']:
        fields.reverse()
    cases = []
    for i, (symbol, local) in enumerate(view['locals'].items()):
        expected = meta['locals'][local['name']]
        if expected == 'phase':
            expression = 'headers[phases-1]'
        elif expected == 'old':
            expression = 'olds[phases-1]'
        elif expected == 'incoming':
            expression = 'incoming[phases-1]'
        elif expected == 'peer':
            expression = str(meta['peer'])
        elif expected == 'capture':
            expression = str(meta['captured'])
        else:
            assert isinstance(expected, int)
            expression = str(expected)
        cases.append(f'case {symbol}: assert(value=={expression}); ++locals[{i}]; break;')
    local_fn = (f'''assert(phases && phases<={calls});
 switch(symbol){{{''.join(cases)} default:assert(0);}}'''
                if calls else '(void)symbol;(void)value;assert(0);')
    discard_count = len(mapping['direct_discards'])
    discard = ''
    if discard_count:
        choices = ''.join(f'case {symbol}: assert(value=={value}); break;'
                          for symbol, value in mapping['prefix_values'].items())
        discard = f'''static void capture_discard(size_t symbol,uint8_t value){{
 assert(phases && phases<={calls});switch(symbol){{{choices}default:assert(0);}}++discards;
}}
''' if calls else '''static void capture_discard(size_t symbol,uint8_t value){
 (void)symbol;(void)value;assert(0);
}
'''
    arrays = (f'''static const size_t incoming[]={{ {array(meta['incoming'])} }};
static const size_t olds[]={{ {array(meta['old'])} }};
''' if calls else '')
    helper = f'''static void helper_input(size_t value){{
 assert(calls==returns && calls+1==phases && calls<{calls});
 assert(value==headers[calls] && *original[{meta['index']}]==child[calls]);++calls;
}}
static uint8_t helper_return(uint8_t value){{
 assert(returns+1==calls && value==incoming[returns]);
 assert(*original[{meta['index']}]==child[returns]);++returns;return value;
}}
''' if calls else '''static void helper_input(size_t value){(void)value;assert(0);}
static uint8_t helper_return(uint8_t value){assert(0);return value;}
'''
    return f'''#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
static uint8_t *original[2];
static size_t phases,calls,returns,results,before_reads,after_reads,discards;
static size_t locals[{len(view['locals'])}];
static const size_t headers[]={{ {array(meta['headers'])} }};
static const size_t child[]={{ {array([meta['initial'],*meta['incoming']])} }};
{arrays}
static void finish(void){{
 assert(phases=={len(meta['headers'])} && calls=={calls} && returns==calls);
 assert(results==1 && before_reads=={meta['prefix_field_reads']} && after_reads==2);
 assert(discards=={discard_count*calls});
 for(size_t i=0;i<sizeof locals/sizeof locals[0];++i)assert(locals[i]==calls);
 puts("C254 physical-original/captured-old/current/helper/phase PASS");
}}
static void bind_original(uint8_t *a,uint8_t *b){{
 assert(!original[0] && !original[1] && a!=b && *a=={fields[0]} && *b=={fields[1]});
 original[0]=a;original[1]=b;assert(atexit(finish)==0);
}}
static void phase(size_t value,uint8_t *a,uint8_t *b){{
 assert(a==original[0] && b==original[1] && phases<{len(meta['headers'])});
 assert(value==headers[phases] && *original[{meta['index']}]==child[phases]);
 assert(*original[{1-meta['index']}]=={meta['peer']});++phases;
}}
static void local(size_t symbol,size_t value){{{local_fn}}}
static void result(size_t value){{
 assert(value=={meta['headers'][-1]} && *original[{meta['index']}]=={meta['final']});
 assert(*original[{1-meta['index']}]=={meta['peer']});++results;
}}
static void read_original(uint8_t *p,uint8_t value){{
 assert((p==original[0] || p==original[1]) && value==*p);
 if(!phases){{
  assert(value==(p==original[{meta['index']}]?{meta['initial']}:{meta['peer']}));++before_reads;
 }}else{{
  assert(phases=={len(meta['headers'])} && results==1);
  assert(value==(p==original[{meta['index']}]?{meta['final']}:{meta['peer']}));++after_reads;
 }}
}}
{helper}{discard}
#define NL_LOOP_BIND(a,b) bind_original(a,b)
#define NL_CURRENT_PHASE(n,a,b) phase(n,a,b)
#define NL_CURRENT_LOCAL(s,v) local(s,v)
#define NL_CURRENT_RESULT(n) result(n)
#define NL_LOOP_READ(a,v) read_original(a,v)
'''


# Finish every negative and every structural gate before the first native build.
for name, meta in oracle['cases'].items():
    source = here / 'sources' / (name + '.nl')
    negative = meta['class'] == 'negative-boundary'
    status = extension['target_negative_status'] if negative else 0
    checked = run([compiler, '--check', source], name + '-check', status)
    emitted = run([compiler, source], name + '-emit', status)
    target = raw / (name + '-named.c')
    target.write_bytes(b'C254 original named sentinel\n')
    before = (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns)
    run([compiler, '--emit-c17-to', target, source], name + '-named', status)
    if negative:
        assert extension['target_negative_codes'][name].encode() in checked.stderr
        assert not emitted.stdout
        assert (target.read_bytes(), target.stat().st_ino, target.stat().st_mtime_ns) == before
        continue
    assert target.read_bytes() == emitted.stdout
    if meta['class'] == 'existing-byte-control':
        assert emitted.stdout == (here / 'evidence' / (name + '.c17')).read_bytes()
        continue
    traced = run([probe, source], name + '-owned-roles')
    view = roles(traced.stdout.decode())
    mapping = inspect(emitted.stdout.decode(), view, meta)
    (raw / (name + '-mapping.json')).write_text(json.dumps({'roles': view, 'mapping': mapping}, indent=2) + '\n')
    accepted.append((name, meta, emitted.stdout.decode(), view, mapping))

native_count = 0
for name, meta, text, view, mapping in accepted:
    body = mapping['body']
    for symbol in {u['symbol'] for u in mapping['direct_discards']}:
        needle = '(void)nl_local_' + str(symbol) + ';'
        expected = sum(u['symbol'] == symbol for u in mapping['direct_discards'])
        assert body.count(needle) == expected
        body = body.replace(needle, f'capture_discard({symbol},nl_local_{symbol});')
    observed = text[:mapping['start']] + body + text[mapping['stop']:]
    pattern = r'(static uint8_t nl_fn_' + str(view['helper']) + r'\((?:const )?size_t (nl_local_\d+)\)\n\{\n)(.*?)(^\})'
    def passive(match):
        changed, count = re.subn(r'\breturn ([^;\n]+);', r'return helper_return(\1);', match[3])
        assert count == 1
        return match[1] + '    helper_input(' + match[2] + ');\n' + changed + match[4]
    observed, count = re.subn(pattern, passive, observed, flags=re.S | re.M)
    assert count == 1
    plain = raw / (name + '.c')
    plain.write_text(text)
    instrumented = raw / (name + '-passive.c')
    instrumented.write_text(observed)
    header = raw / (name + '-observer.h')
    header.write_text(observer(meta, view, mapping))
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
                assert (b'C254 physical-original/captured-old/current/helper/phase PASS' in result.stdout) == active
                native_count += 1
                exe.unlink()
    print(name, 'source-owned/structural/native PASS', flush=True)
print('C254 finite source-owned/current/C17/negative/named/controls/native PASS runs', native_count)
print('C254 raw', raw)
