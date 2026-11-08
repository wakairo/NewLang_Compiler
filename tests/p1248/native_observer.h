#ifndef P1248_NATIVE_OBSERVER_H
#define P1248_NATIVE_OBSERVER_H
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static struct {
    size_t quota, attempts, success, frees, domains, finalized, changes;
    size_t visits, phase, stores, reads, results, binds;
    unsigned roles, caller_roles;
    bool finished;
    const uint8_t *field[2];
    struct {
        nl_node *live;
        uintptr_t address;
        size_t domain;
        bool ended, raw, final, released;
    } heap[2];
} p296;
static const uint8_t p_old[P296_LIMIT + 1] = P296_OLD_TRACE,
                                        p_next[P296_LIMIT + 1] = P296_NEW_TRACE;
static const size_t p_roles[5] = P296_ROLES;
static const size_t p_caller[4] = P296_CALLER_ROLES;
#define P296_SIBLING (P296_INDEX ? 29 : 173)
static inline size_t p_index(const void *p)
{
    for (size_t i = 0; i < p296.success; ++i)
        if (p296.heap[i].live && p296.heap[i].address == (uintptr_t)p)
            return i;
    assert(0);
    return 0;
}
static inline void *p_allocate(size_t bytes)
{
    assert(bytes == sizeof(nl_node));
    ++p296.attempts;
    if (p296.success == p296.quota)
        return NULL;
    void *p = malloc(bytes);
    assert(p && p296.success < 2);
    p296.heap[p296.success].live = p;
    p296.heap[p296.success].address = (uintptr_t)p;
    ++p296.success;
    return p;
}
static inline void p_free(void *p)
{
    size_t i = p_index(p);
    assert(p296.heap[i].ended && p296.heap[i].raw && p296.heap[i].final &&
           p296.heap[i].released);
    p296.heap[i].live = NULL;
    ++p296.frees;
    free(p);
}
static inline void p_closed(void)
{
    assert(p296.success == p296.quota &&
           p296.attempts == (p296.quota ? 2u : 1u));
    assert(p296.frees == p296.success && p296.domains == p296.success &&
           p296.finalized == p296.success);
    assert(p296.changes == (p296.success == 2 ? 2u : 0u));
    for (size_t i = 0; i < p296.success; ++i)
        assert(!p296.heap[i].live && p296.heap[i].ended && p296.heap[i].raw &&
               p296.heap[i].final && p296.heap[i].released);
}
static inline void p_domain(size_t token)
{
    assert(token && p296.domains < p296.success);
    if (p296.domains)
        assert(token != p296.heap[0].domain);
    p296.heap[p296.domains++].domain = token;
}
static inline void p_initialized(const nl_node *node, size_t domain)
{
    size_t i = p_index(node);
    assert(domain == p296.heap[i].domain);
    assert(node->f0.tag == 0 && node->f0.ptr == NULL &&
           node->f1 == (i ? 191 : 47));
}
static inline void p_end(const nl_node *node, size_t domain)
{
    size_t i = p_index(node);
    assert(domain == p296.heap[i].domain && !p296.heap[i].ended);
    assert(node->f0.tag == 0 && node->f0.ptr == NULL &&
           node->f1 == (i ? 191 : 47));
    p296.heap[i].ended = true;
}
static inline void p_raw(const void *raw, size_t bytes)
{
    size_t i = p_index(raw);
    assert(bytes == sizeof(nl_node) && p296.heap[i].ended && !p296.heap[i].raw);
    p296.heap[i].raw = true;
}
static inline void p_finalize(size_t token)
{
    for (size_t i = 0; i < p296.success; ++i)
        if (p296.heap[i].domain == token) {
            assert(p296.heap[i].ended && p296.heap[i].raw &&
                   !p296.heap[i].final);
            p296.heap[i].final = true;
            ++p296.finalized;
            return;
        }
    assert(0);
}
static inline void p_release(const void *allocation, const void *raw,
                             size_t bytes)
{
    size_t i = p_index(allocation);
    assert(allocation == raw && bytes == sizeof(nl_node));
    assert(p296.heap[i].ended && p296.heap[i].raw && p296.heap[i].final &&
           !p296.heap[i].released);
    p296.heap[i].released = true;
}
static inline void p_change(nl_node_option *field, const nl_node_option *old)
{
    assert(p296.success == 2 && p296.heap[0].live && p296.heap[1].live &&
           field == &p296.heap[0].live->f0);
    if (!p296.changes)
        assert(!old->tag && !old->ptr && field->tag == 1 &&
               field->ptr == p296.heap[1].live);
    else
        assert(p296.changes == 1 && old->tag == 1 &&
               old->ptr == p296.heap[1].live && !field->tag && !field->ptr);
    ++p296.changes;
}
static inline void p_tail(const nl_node *ptr, const void *allocation,
                          size_t token)
{
    assert(p296.success == 2 && p296.heap[1].live == ptr && allocation == ptr &&
           token == p296.heap[1].domain && !p296.heap[1].ended);
}
static inline void p_bind(const uint8_t *a, const uint8_t *b)
{
    assert(!p296.binds && a && b == a + 1 && *a == 29 && *b == 173);
    ++p296.binds;
    p296.field[0] = a;
    p296.field[1] = b;
#if P296_PREHEAP
    assert(p296.attempts == 0);
#else
    p_closed();
#endif
}
static inline void p_phase(size_t phase, const uint8_t *a, const uint8_t *b)
{
    p_closed();
    assert(p296.binds == 1 && phase == p296.visits && phase <= P296_LIMIT);
    assert(a == p296.field[0] && b == p296.field[1]);
    if (phase)
        assert(p296.roles == 31);
    assert(*p296.field[P296_INDEX] == p_old[phase] &&
           *p296.field[1 - P296_INDEX] == P296_SIBLING);
    p296.phase = phase;
    ++p296.visits;
    p296.roles = 0;
}
static inline void p_store(uint8_t *field, uint8_t old, uint8_t next)
{
    assert(p296.phase <= P296_LIMIT && p296.stores == p296.phase &&
           field == p296.field[P296_INDEX]);
    assert(old == p_old[p296.phase] && next == p_next[p296.phase] &&
           *field == next);
    assert(*p296.field[1 - P296_INDEX] == P296_SIBLING);
    ++p296.stores;
}
static inline void p_local(size_t symbol, uintmax_t value)
{
    if (p296.results) {
        for (size_t i = 0; i < 4; ++i)
            if (symbol == p_caller[i]) {
                assert(!(p296.caller_roles & (1u << i)));
                uintmax_t expected = i < 2    ? P296_OLD
                                     : i == 2 ? P296_TERMINAL
                                              : P296_SIBLING;
                assert(value == expected);
                p296.caller_roles |= 1u << i;
                return;
            }
        return;
    }
    if (p296.phase >= P296_LIMIT)
        return; /* Terminal aliases are checked by store/result. */
    for (size_t i = 0; i < 5; ++i)
        if (symbol == p_roles[i]) {
            assert(!(p296.roles & (1u << i)));
            uintmax_t expected = i == 0 || i == 3 ? p_next[p296.phase]
                                 : i == 4         ? P296_SIBLING
                                                  : p_old[p296.phase];
            assert(value == expected);
            p296.roles |= 1u << i;
            return;
        }
}
static inline void p_result(size_t value)
{
    assert(value == P296_OLD && p296.visits == P296_LIMIT + 1 &&
           p296.stores == P296_LIMIT + 1 && !p296.results);
    ++p296.results;
}
static inline void p_read(const uint8_t *field, uint8_t value)
{
    assert(p296.results == 1 && p296.reads < 2);
    assert(field == p296.field[p296.reads ? 1 - P296_INDEX : P296_INDEX]);
    assert(value == (p296.reads ? P296_SIBLING : P296_TERMINAL));
    ++p296.reads;
}
static inline void p_finish_live(void)
{
    p_closed();
    assert(!p296.finished && p296.binds == 1 && p296.visits == P296_LIMIT + 1 &&
           p296.stores == P296_LIMIT + 1 && p296.results == 1 &&
           p296.reads == 2 && p296.caller_roles == 15);
    assert(*p296.field[P296_INDEX] == P296_TERMINAL &&
           *p296.field[1 - P296_INDEX] == P296_SIBLING);
    printf("P1248 live-original preH=%d index=%d H=%zu attempts=%zu free=%zu "
           "final=%d sibling=%d old_result=%d caller_Copy=%d writes=%zu PASS\n",
           P296_PREHEAP, P296_INDEX, p296.quota, p296.attempts, p296.frees,
           P296_TERMINAL, P296_SIBLING, P296_OLD, P296_OLD, p296.stores);
    p296.finished = true;
}
static inline void p_reset(size_t quota)
{
    assert(quota <= 2);
    memset(&p296, 0, sizeof(p296));
    p296.quota = quota;
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
