"""P-owned passive observer derived from the preserved C254 apparatus."""
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
 puts("C256 physical-original/old/current/helper/phase PASS");
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
