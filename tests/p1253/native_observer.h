#ifndef P1253_NATIVE_OBSERVER_H
#define P1253_NATIVE_OBSERVER_H
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
} p299;
static const uint8_t p_old[P299_STORES] = P299_OLD_TRACE,
                     p_next[P299_STORES] = P299_NEW_TRACE;
static const size_t p_roles[5] = P299_ROLES;
static const size_t p_caller[4] = P299_CALLER_ROLES;
#define P299_SIBLING (P299_INDEX ? 67 : 181)
static inline size_t p_index(const void *p)
{
    for (size_t i = 0; i < p299.success; ++i)
        if (p299.heap[i].live && p299.heap[i].address == (uintptr_t)p)
            return i;
    assert(0);
    return 0;
}
static inline void *p_allocate(size_t bytes)
{
    assert(bytes == sizeof(nl_node));
    ++p299.attempts;
    if (p299.success == p299.quota)
        return NULL;
    void *p = malloc(bytes);
    assert(p && p299.success < 2);
    p299.heap[p299.success].live = p;
    p299.heap[p299.success].address = (uintptr_t)p;
    ++p299.success;
    return p;
}
static inline void p_free(void *p)
{
    size_t i = p_index(p);
    assert(p299.heap[i].ended && p299.heap[i].raw && p299.heap[i].final &&
           p299.heap[i].released);
    p299.heap[i].live = NULL;
    ++p299.frees;
    free(p);
}
static inline void p_closed(void)
{
    assert(p299.success == p299.quota &&
           p299.attempts == (p299.quota ? 2u : 1u));
    assert(p299.frees == p299.success && p299.domains == p299.success &&
           p299.finalized == p299.success);
    assert(p299.changes == (p299.success == 2 ? 2u : 0u));
    for (size_t i = 0; i < p299.success; ++i)
        assert(!p299.heap[i].live && p299.heap[i].ended && p299.heap[i].raw &&
               p299.heap[i].final && p299.heap[i].released);
}
static inline void p_domain(size_t token)
{
    assert(token && p299.domains < p299.success);
    if (p299.domains)
        assert(token != p299.heap[0].domain);
    p299.heap[p299.domains++].domain = token;
}
static inline void p_initialized(const nl_node *node, size_t domain)
{
    size_t i = p_index(node);
    assert(domain == p299.heap[i].domain);
    assert(node->f0.tag == 0 && node->f0.ptr == NULL &&
           node->f1 == (i ? 227 : 53));
}
static inline void p_end(const nl_node *node, size_t domain)
{
    size_t i = p_index(node);
    assert(domain == p299.heap[i].domain && !p299.heap[i].ended);
    assert(node->f0.tag == 0 && node->f0.ptr == NULL &&
           node->f1 == (i ? 227 : 53));
    p299.heap[i].ended = true;
}
static inline void p_raw(const void *raw, size_t bytes)
{
    size_t i = p_index(raw);
    assert(bytes == sizeof(nl_node) && p299.heap[i].ended && !p299.heap[i].raw);
    p299.heap[i].raw = true;
}
static inline void p_finalize(size_t token)
{
    for (size_t i = 0; i < p299.success; ++i)
        if (p299.heap[i].domain == token) {
            assert(p299.heap[i].ended && p299.heap[i].raw &&
                   !p299.heap[i].final);
            p299.heap[i].final = true;
            ++p299.finalized;
            return;
        }
    assert(0);
}
static inline void p_release(const void *allocation, const void *raw,
                             size_t bytes)
{
    size_t i = p_index(allocation);
    assert(allocation == raw && bytes == sizeof(nl_node));
    assert(p299.heap[i].ended && p299.heap[i].raw && p299.heap[i].final &&
           !p299.heap[i].released);
    p299.heap[i].released = true;
}
static inline void p_change(nl_node_option *field, const nl_node_option *old)
{
    assert(p299.success == 2 && p299.heap[0].live && p299.heap[1].live &&
           field == &p299.heap[0].live->f0);
    if (!p299.changes)
        assert(!old->tag && !old->ptr && field->tag == 1 &&
               field->ptr == p299.heap[1].live);
    else
        assert(p299.changes == 1 && old->tag == 1 &&
               old->ptr == p299.heap[1].live && !field->tag && !field->ptr);
    ++p299.changes;
}
static inline void p_tail(const nl_node *ptr, const void *allocation,
                          size_t token)
{
    assert(p299.success == 2 && p299.heap[1].live == ptr && allocation == ptr &&
           token == p299.heap[1].domain && !p299.heap[1].ended);
}
static inline void p_bind(const uint8_t *a, const uint8_t *b)
{
    assert(!p299.binds && a && b == a + 1 && *a == 67 && *b == 181);
    ++p299.binds;
    p299.field[0] = a;
    p299.field[1] = b;
#if P299_PREHEAP
    assert(p299.attempts == 0);
#else
    p_closed();
#endif
}
static inline void p_phase(size_t phase, const uint8_t *a, const uint8_t *b)
{
    p_closed();
    assert(p299.prefix_roles == (P299_HAS_PREFIX ? 3u : 0u));
    assert(p299.binds == 1 && phase == P299_INITIAL + p299.visits &&
           phase <= P299_LIMIT);
    assert(a == p299.field[0] && b == p299.field[1]);
    if (p299.visits)
        assert(p299.roles == 31);
    assert(*p299.field[P299_INDEX] == p_old[p299.visits] &&
           *p299.field[1 - P299_INDEX] == P299_SIBLING);
    p299.phase = phase;
    ++p299.visits;
    p299.roles = 0;
}
static inline void p_store(uint8_t *field, uint8_t old, uint8_t next)
{
    assert(p299.phase <= P299_LIMIT && p299.stores == p299.visits - 1 &&
           field == p299.field[P299_INDEX]);
    assert(old == p_old[p299.stores] && next == p_next[p299.stores] &&
           *field == next);
    assert(*p299.field[1 - P299_INDEX] == P299_SIBLING);
    ++p299.stores;
}
static inline void p_local(size_t symbol, uintmax_t value)
{
    if (!p299.visits) {
        const size_t prefix[2] = P299_PREFIX_ROLES;
        for (size_t i = 0; i < 2; ++i)
            if (prefix[i] && symbol == prefix[i]) {
                assert(!(p299.prefix_roles & (1u << i)) &&
                       value == P299_POLICY);
                p299.prefix_roles |= 1u << i;
            }
        return;
    }
    if (p299.results) {
        for (size_t i = 0; i < 4; ++i)
            if (symbol == p_caller[i]) {
                assert(!(p299.caller_roles & (1u << i)));
                uintmax_t expected = i < 2    ? P299_OLD
                                     : i == 2 ? P299_TERMINAL
                                              : P299_SIBLING;
                assert(value == expected);
                p299.caller_roles |= 1u << i;
                return;
            }
        return;
    }
    if (p299.phase >= P299_LIMIT)
        return; /* Terminal aliases are checked by store/result. */
    for (size_t i = 0; i < 5; ++i)
        if (symbol == p_roles[i]) {
            assert(!(p299.roles & (1u << i)));
            uintmax_t expected = i == 0 || i == 3 ? p_next[p299.visits - 1]
                                 : i == 4         ? P299_SIBLING
                                                  : p_old[p299.visits - 1];
            assert(value == expected);
            p299.roles |= 1u << i;
            return;
        }
}
static inline void p_result(size_t value)
{
    assert(value == P299_OLD && p299.visits == P299_STORES &&
           p299.stores == P299_STORES && !p299.results);
    ++p299.results;
}
static inline void p_read(const uint8_t *field, uint8_t value)
{
    assert(p299.results == 1 && p299.reads < 2);
    assert(field == p299.field[p299.reads ? 1 - P299_INDEX : P299_INDEX]);
    assert(value == (p299.reads ? P299_SIBLING : P299_TERMINAL));
    ++p299.reads;
}
static inline void p_finish_live(void)
{
    p_closed();
    assert(!p299.finished && p299.binds == 1 && p299.visits == P299_STORES &&
           p299.stores == P299_STORES && p299.results == 1 && p299.reads == 2 &&
           p299.caller_roles == 15);
    assert(*p299.field[P299_INDEX] == P299_TERMINAL &&
           *p299.field[1 - P299_INDEX] == P299_SIBLING);
    printf("P1253 live-original preH=%d index=%d H=%zu attempts=%zu free=%zu "
           "final=%d sibling=%d old_result=%d caller_Copy=%d writes=%zu PASS\n",
           P299_PREHEAP, P299_INDEX, p299.quota, p299.attempts, p299.frees,
           P299_TERMINAL, P299_SIBLING, P299_OLD, P299_OLD, p299.stores);
    p299.finished = true;
}
static inline void p_reset(size_t quota)
{
    assert(quota <= 2);
    memset(&p299, 0, sizeof(p299));
    p299.quota = quota;
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
