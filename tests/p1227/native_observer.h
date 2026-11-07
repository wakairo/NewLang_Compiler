#ifndef P1227_NATIVE_OBSERVER_H
#define P1227_NATIVE_OBSERVER_H
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static struct {
    size_t quota, attempts, success, frees, domains, finalized, changes;
    size_t visits, phase, stores, reads, results, binds;
    unsigned roles;
    bool finished;
    const uint8_t *field[2];
    struct {
        nl_node *live;
        uintptr_t address;
        size_t domain;
        bool ended, raw, final, released;
    } heap[2];
} p280;
static const uint8_t p_old[3] = {31, 53, 149}, p_next[3] = {53, 149, 149};
static const size_t p_roles[5] = P280_ROLES;
static inline size_t p_index(const void *p)
{
    for (size_t i = 0; i < p280.success; ++i)
        if (p280.heap[i].live && p280.heap[i].address == (uintptr_t)p)
            return i;
    assert(0);
    return 0;
}
static inline void *p_allocate(size_t bytes)
{
    assert(bytes == sizeof(nl_node));
    ++p280.attempts;
    if (p280.success == p280.quota)
        return NULL;
    void *p = malloc(bytes);
    assert(p && p280.success < 2);
    p280.heap[p280.success].live = p;
    p280.heap[p280.success].address = (uintptr_t)p;
    ++p280.success;
    return p;
}
static inline void p_free(void *p)
{
    size_t i = p_index(p);
    assert(p280.heap[i].ended && p280.heap[i].raw && p280.heap[i].final &&
           p280.heap[i].released);
    p280.heap[i].live = NULL;
    ++p280.frees;
    free(p);
}
static inline void p_closed(void)
{
    assert(p280.success == p280.quota &&
           p280.attempts == (p280.quota ? 2u : 1u));
    assert(p280.frees == p280.success && p280.domains == p280.success &&
           p280.finalized == p280.success);
    assert(p280.changes == (p280.success == 2 ? 2u : 0u));
    for (size_t i = 0; i < p280.success; ++i)
        assert(!p280.heap[i].live && p280.heap[i].ended && p280.heap[i].raw &&
               p280.heap[i].final && p280.heap[i].released);
}
static inline void p_domain(size_t token)
{
    assert(token && p280.domains < p280.success);
    if (p280.domains)
        assert(token != p280.heap[0].domain);
    p280.heap[p280.domains++].domain = token;
}
static inline void p_initialized(const nl_node *node, size_t domain)
{
    size_t i = p_index(node);
    assert(domain == p280.heap[i].domain);
    assert(node->f0.tag == 0 && node->f0.ptr == NULL &&
           node->f1 == (i ? 167 : 41));
}
static inline void p_end(const nl_node *node, size_t domain)
{
    size_t i = p_index(node);
    assert(domain == p280.heap[i].domain && !p280.heap[i].ended);
    assert(node->f0.tag == 0 && node->f0.ptr == NULL &&
           node->f1 == (i ? 167 : 41));
    p280.heap[i].ended = true;
}
static inline void p_raw(const void *raw, size_t bytes)
{
    size_t i = p_index(raw);
    assert(bytes == sizeof(nl_node) && p280.heap[i].ended && !p280.heap[i].raw);
    p280.heap[i].raw = true;
}
static inline void p_finalize(size_t token)
{
    for (size_t i = 0; i < p280.success; ++i)
        if (p280.heap[i].domain == token) {
            assert(p280.heap[i].ended && p280.heap[i].raw &&
                   !p280.heap[i].final);
            p280.heap[i].final = true;
            ++p280.finalized;
            return;
        }
    assert(0);
}
static inline void p_release(const void *allocation, const void *raw,
                             size_t bytes)
{
    size_t i = p_index(allocation);
    assert(allocation == raw && bytes == sizeof(nl_node));
    assert(p280.heap[i].ended && p280.heap[i].raw && p280.heap[i].final &&
           !p280.heap[i].released);
    p280.heap[i].released = true;
}
static inline void p_change(nl_node_option *field, const nl_node_option *old)
{
    assert(p280.success == 2 && p280.heap[0].live && p280.heap[1].live &&
           field == &p280.heap[0].live->f0);
    if (!p280.changes)
        assert(!old->tag && !old->ptr && field->tag == 1 &&
               field->ptr == p280.heap[1].live);
    else
        assert(p280.changes == 1 && old->tag == 1 &&
               old->ptr == p280.heap[1].live && !field->tag && !field->ptr);
    ++p280.changes;
}
static inline void p_tail(const nl_node *ptr, const void *allocation,
                          size_t token)
{
    assert(p280.success == 2 && p280.heap[1].live == ptr && allocation == ptr &&
           token == p280.heap[1].domain && !p280.heap[1].ended);
}
static inline void p_bind(const uint8_t *a, const uint8_t *b)
{
    assert(!p280.binds && a && b == a + 1 && *a == 31 && *b == 203);
    ++p280.binds;
    p280.field[0] = a;
    p280.field[1] = b;
#if P280_PREHEAP
    assert(p280.attempts == 0);
#else
    p_closed();
#endif
}
static inline void p_phase(size_t phase, const uint8_t *a, const uint8_t *b)
{
    p_closed();
    assert(p280.binds == 1 && phase == p280.visits && phase <= 3);
    assert(a == p280.field[0] && b == p280.field[1]);
    if (phase)
        assert(p280.roles == 31);
    assert(*a == (phase < 3 ? p_old[phase] : 149) && *b == 203);
    p280.phase = phase;
    ++p280.visits;
    p280.roles = 0;
}
static inline void p_store(uint8_t *field, uint8_t old, uint8_t next)
{
    assert(p280.phase < 3 && p280.stores == p280.phase &&
           field == p280.field[0]);
    assert(old == p_old[p280.phase] && next == p_next[p280.phase] &&
           *field == next && *p280.field[1] == 203);
    ++p280.stores;
}
static inline void p_local(size_t symbol, uintmax_t value)
{
    for (size_t i = 0; i < 5; ++i)
        if (symbol == p_roles[i]) {
            assert(p280.phase < 3 && !(p280.roles & (1u << i)));
            uintmax_t expected = i == 0 || i == 3 ? p_next[p280.phase]
                                 : i == 4         ? 203
                                                  : p_old[p280.phase];
            assert(value == expected);
            p280.roles |= 1u << i;
            return;
        }
}
static inline void p_result(size_t value)
{
    assert(value == 3 && p280.visits == 4 && p280.stores == 3 && !p280.results);
    ++p280.results;
}
static inline void p_read(const uint8_t *field, uint8_t value)
{
    assert(p280.results == 1 && p280.reads < 2 &&
           field == p280.field[p280.reads]);
    assert(value == (p280.reads ? 203 : 149));
    ++p280.reads;
}
static inline void p_finish_live(void)
{
    /* Called inside generated main, while its original stack T is LIVE. */
    p_closed();
    assert(!p280.finished && p280.binds == 1 && p280.visits == 4 &&
           p280.stores == 3 && p280.results == 1 && p280.reads == 2);
    assert(*p280.field[0] == 149 && *p280.field[1] == 203);
    printf("P1227 live-original preH=%d H=%zu attempts=%zu free=%zu final=149 "
           "sibling=203 result=3 writes=3 PASS\n",
           P280_PREHEAP, p280.quota, p280.attempts, p280.frees);
    p280.finished = true;
}
static inline void p_reset(size_t quota)
{
    assert(quota <= 2);
    memset(&p280, 0, sizeof(p280));
    p280.quota = quota;
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
