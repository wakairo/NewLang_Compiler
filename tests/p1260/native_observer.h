#ifndef P1260_NATIVE_OBSERVER_H
#define P1260_NATIVE_OBSERVER_H
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static struct {
    size_t quota, attempts, success, frees, domains, finalized, changes;
    size_t visits, phase, stores, reads, results, binds;
    unsigned roles, caller_roles, prefix_roles;
    bool finished;
    const uint8_t *field[2];
    struct {
        nl_node *live;
        uintptr_t address;
        size_t domain;
        bool ended, raw, final, released;
    } heap[2];
} p314;
static const uint8_t p_initial[2] = P314_INITIAL, p_final[2] = P314_FINAL;
static const uint8_t p_old[] = P314_OLD, p_next[] = P314_NEW;
static const size_t p_fields[] = P314_FIELDS;
static const size_t p_symbols[] = P314_SYMBOLS;
static const uintmax_t p_values[] = P314_VALUES;
static uint8_t p_shadow[2];
static unsigned p_seen;
static inline size_t p_index(const void *p)
{
    for (size_t i = 0; i < p314.success; ++i)
        if (p314.heap[i].live && p314.heap[i].address == (uintptr_t)p)
            return i;
    assert(0);
    return 0;
}
static inline void *p_allocate(size_t bytes)
{
    assert(bytes == sizeof(nl_node));
    ++p314.attempts;
    if (p314.success == p314.quota)
        return NULL;
    void *p = malloc(bytes);
    assert(p && p314.success < 2);
    p314.heap[p314.success].live = p;
    p314.heap[p314.success].address = (uintptr_t)p;
    ++p314.success;
    return p;
}
static inline void p_free(void *p)
{
    size_t i = p_index(p);
    assert(p314.heap[i].ended && p314.heap[i].raw && p314.heap[i].final &&
           p314.heap[i].released);
    p314.heap[i].live = NULL;
    ++p314.frees;
    free(p);
}
static inline void p_closed(void)
{
    assert(p314.success == p314.quota &&
           p314.attempts == (p314.quota ? 2u : 1u));
    assert(p314.frees == p314.success && p314.domains == p314.success &&
           p314.finalized == p314.success);
    assert(p314.changes == (p314.success == 2 ? 2u : 0u));
    for (size_t i = 0; i < p314.success; ++i)
        assert(!p314.heap[i].live && p314.heap[i].ended && p314.heap[i].raw &&
               p314.heap[i].final && p314.heap[i].released);
}
static inline void p_domain(size_t token)
{
    assert(token && p314.domains < p314.success);
    if (p314.domains)
        assert(token != p314.heap[0].domain);
    p314.heap[p314.domains++].domain = token;
}
static inline void p_initialized(const nl_node *node, size_t domain)
{
    size_t i = p_index(node);
    assert(domain == p314.heap[i].domain);
    assert(node->f0.tag == 0 && node->f0.ptr == NULL &&
           node->f1 == (i ? 203 : 72));
}
static inline void p_end(const nl_node *node, size_t domain)
{
    size_t i = p_index(node);
    assert(domain == p314.heap[i].domain && !p314.heap[i].ended);
    assert(node->f0.tag == 0 && node->f0.ptr == NULL &&
           node->f1 == (i ? 203 : 72));
    p314.heap[i].ended = true;
}
static inline void p_raw(const void *raw, size_t bytes)
{
    size_t i = p_index(raw);
    assert(bytes == sizeof(nl_node) && p314.heap[i].ended && !p314.heap[i].raw);
    p314.heap[i].raw = true;
}
static inline void p_finalize(size_t token)
{
    for (size_t i = 0; i < p314.success; ++i)
        if (p314.heap[i].domain == token) {
            assert(p314.heap[i].ended && p314.heap[i].raw &&
                   !p314.heap[i].final);
            p314.heap[i].final = true;
            ++p314.finalized;
            return;
        }
    assert(0);
}
static inline void p_release(const void *allocation, const void *raw,
                             size_t bytes)
{
    size_t i = p_index(allocation);
    assert(allocation == raw && bytes == sizeof(nl_node));
    assert(p314.heap[i].ended && p314.heap[i].raw && p314.heap[i].final &&
           !p314.heap[i].released);
    p314.heap[i].released = true;
}
static inline void p_change(nl_node_option *field, const nl_node_option *old)
{
    assert(p314.success == 2 && p314.heap[0].live && p314.heap[1].live &&
           field == &p314.heap[0].live->f0);
    if (!p314.changes)
        assert(!old->tag && !old->ptr && field->tag == 1 &&
               field->ptr == p314.heap[1].live);
    else
        assert(p314.changes == 1 && old->tag == 1 &&
               old->ptr == p314.heap[1].live && !field->tag && !field->ptr);
    ++p314.changes;
}
static inline void p_tail(const nl_node *ptr, const void *allocation,
                          size_t token)
{
    assert(p314.success == 2 && p314.heap[1].live == ptr && allocation == ptr &&
           token == p314.heap[1].domain && !p314.heap[1].ended);
}
static inline void p_bind(const uint8_t *a, const uint8_t *b)
{
    assert(!p314.binds && a && b == a + 1 && *a == p_initial[0] &&
           *b == p_initial[1]);
    assert(!p314.attempts);
    ++p314.binds;
    p314.field[0] = a;
    p314.field[1] = b;
    memcpy(p_shadow, p_initial, sizeof p_shadow);
}
static inline void p_phase(size_t phase, const uint8_t *a, const uint8_t *b)
{
    p_closed();
    assert(p314.binds == 1 && phase == p314.visits && phase < P314_LOOP_STORES);
    assert(p314.stores == p314.visits && a == p314.field[0] &&
           b == p314.field[1]);
    assert(*a == p_shadow[0] && *b == p_shadow[1]);
    p314.phase = phase;
    ++p314.visits;
}
static inline void p_store(uint8_t *field, uint8_t old, uint8_t next)
{
    assert(p314.stores < P314_STORES);
    const size_t k = p314.stores, index = p_fields[k];
    assert(index < 2 && field == p314.field[index] && old == p_old[k] &&
           next == p_next[k]);
    assert(old == p_shadow[index] && *field == next);
    p_shadow[index] = next;
    assert(*p314.field[0] == p_shadow[0] && *p314.field[1] == p_shadow[1]);
    if (k < P314_LOOP_STORES)
        assert(!p314.results && k == p314.visits - 1);
    else
        assert(p314.results == 1 && p314.visits == P314_LOOP_STORES);
    ++p314.stores;
}
static inline void p_local(size_t symbol, uintmax_t value)
{
    /* Only root locals before the first loop visit or after its result.
     * Numeric arm-local symbols never inherit a root role. */
    if (p314.visits && !p314.results)
        return;
    for (size_t i = 0; i < sizeof p_symbols / sizeof *p_symbols; ++i)
        if (symbol == p_symbols[i]) {
            assert(!(p_seen & (1u << i)) && value == p_values[i]);
            p_seen |= 1u << i;
            return;
        }
}
static inline void p_result(size_t value)
{
    assert(!p314.results && value == P314_BREAK &&
           p314.stores == P314_LOOP_STORES && p314.visits == P314_LOOP_STORES);
    ++p314.results;
}
static inline void p_read(const uint8_t *field, uint8_t value)
{
    assert(p314.results == 1 && p314.reads < 2 && p314.stores == P314_STORES);
    size_t index = p314.reads ? 1 - P314_INDEX : P314_INDEX;
    assert(field == p314.field[index] && value == p_final[index]);
    ++p314.reads;
}
static inline void p_finish_live(void)
{
    p_closed();
    assert(!p314.finished && p314.binds == 1 &&
           p314.visits == P314_LOOP_STORES && p314.stores == P314_STORES &&
           p314.results == 1 && p314.reads == 2);
    assert(p_seen == ((1u << (sizeof p_symbols / sizeof *p_symbols)) - 1));
    assert(*p314.field[0] == p_final[0] && *p314.field[1] == p_final[1]);
    printf("P1260 live-original H=%zu attempts=%zu free=%zu writes=%zu "
           "final=[%u,%u] old=%u PASS\n",
           p314.quota, p314.attempts, p314.frees, p314.stores,
           (unsigned)*p314.field[0], (unsigned)*p314.field[1],
           (unsigned)P314_BREAK);
    p314.finished = true;
}
static inline void p_reset(size_t quota)
{
    assert(quota <= 2);
    memset(&p314, 0, sizeof p314);
    p314.quota = quota;
    p_seen = 0;
}
#define malloc(n) p_allocate(n)
#define free(p) p_free(p)
#define NL_NODE_BIND(...) ((void)0)
#define NL_NODE_REPLACE(...) ((void)0)
#define NL_NODE_ARM(...) ((void)0)
#define NL_NODE_RELOAN(...) ((void)0)
#define NL_NODE_FINISH() ((void)0)
#define NL_HEAP_TRIAL(...) ((void)0)
#define NL_HEAP_ARM(...) ((void)0)
#define NL_HEAP_DOMAIN(t) p_domain(t)
#define NL_HEAP_SLOT(...) ((void)0)
#define NL_HEAP_INITIALIZE(p, d) p_initialized(p, d)
#define NL_HEAP_RELOAN(...) ((void)0)
#define NL_HEAP_ROOT(...) ((void)0)
#define NL_HEAP_FIELD(...) ((void)0)
#define NL_HEAP_COPY(...) ((void)0)
#define NL_HEAP_CHANGE(p, o, ...) p_change(p, o)
#define NL_HEAP_SCOPE_END(...) ((void)0)
#define NL_HEAP_END(p, d) p_end(p, d)
#define NL_HEAP_RAW(p, n) p_raw(p, n)
#define NL_HEAP_FINALIZE(t) p_finalize(t)
#define NL_HEAP_RELEASE(a, r, n) p_release(a, r, n)
#define NL_HEAP_FINISH() p_finish_live()
#define NL_HEAP_HANDOFF(p, a, d, ...) p_tail(p, a, d)
#define NL_HEAP_RECEIVER_ENTER(p, a, d, ...) p_tail(p, a, d)
#define NL_HEAP_RECEIVER_EXIT() ((void)0)
#define NL_HEAP_RETURNED(a, d) assert((a)->handle == NULL && (d)->token == 0)
#define NL_LIVE_SEND(p, a, d, ...) p_tail(p, a, d)
#define NL_LIVE_ENTER(h, p, a, d) p_tail(p, a, d)
#define NL_LIVE_RETURN(v)                                                      \
    p_tail((v).ptr, (v).allocation.handle, (v).domain.token)
#define NL_LIVE_RECEIVE(...) ((void)0)
#define NL_CURRENT_PHASE(p, a, b) p_phase(p, a, b)
#define NL_CURRENT_LOCAL(s, v) p_local(s, (uintmax_t)(v))
#define NL_CURRENT_RESULT(v) p_result(v)
#define NL_MIXED_COPY_BIND(f, s, a, b) p_bind(a, b)
#define NL_MIXED_COPY_STORE(p, o, n) p_store(p, o, n)
#define NL_LOOP_READ(p, v) p_read(p, v)
#endif
