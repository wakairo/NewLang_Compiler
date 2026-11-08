#ifndef P1236_NATIVE_OBSERVER_H
#define P1236_NATIVE_OBSERVER_H
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
} p288;
static const uint8_t p_old[3] = {P288_INDEX ? 223 : 37, 79, 157},
                     p_next[3] = {79, 157, P288_TERMINAL};
static const size_t p_roles[5] = P288_ROLES;
static const size_t p_caller[4] = P288_CALLER_ROLES;
#define P288_SIBLING (P288_INDEX ? 37 : 223)
static inline size_t p_index(const void *p)
{
    for (size_t i = 0; i < p288.success; ++i)
        if (p288.heap[i].live && p288.heap[i].address == (uintptr_t)p)
            return i;
    assert(0);
    return 0;
}
static inline void *p_allocate(size_t bytes)
{
    assert(bytes == sizeof(nl_node));
    ++p288.attempts;
    if (p288.success == p288.quota)
        return NULL;
    void *p = malloc(bytes);
    assert(p && p288.success < 2);
    p288.heap[p288.success].live = p;
    p288.heap[p288.success].address = (uintptr_t)p;
    ++p288.success;
    return p;
}
static inline void p_free(void *p)
{
    size_t i = p_index(p);
    assert(p288.heap[i].ended && p288.heap[i].raw && p288.heap[i].final &&
           p288.heap[i].released);
    p288.heap[i].live = NULL;
    ++p288.frees;
    free(p);
}
static inline void p_closed(void)
{
    assert(p288.success == p288.quota &&
           p288.attempts == (p288.quota ? 2u : 1u));
    assert(p288.frees == p288.success && p288.domains == p288.success &&
           p288.finalized == p288.success);
    assert(p288.changes == (p288.success == 2 ? 2u : 0u));
    for (size_t i = 0; i < p288.success; ++i)
        assert(!p288.heap[i].live && p288.heap[i].ended && p288.heap[i].raw &&
               p288.heap[i].final && p288.heap[i].released);
}
static inline void p_domain(size_t token)
{
    assert(token && p288.domains < p288.success);
    if (p288.domains)
        assert(token != p288.heap[0].domain);
    p288.heap[p288.domains++].domain = token;
}
static inline void p_initialized(const nl_node *node, size_t domain)
{
    size_t i = p_index(node);
    assert(domain == p288.heap[i].domain);
    assert(node->f0.tag == 0 && node->f0.ptr == NULL &&
           node->f1 == (i ? 181 : 43));
}
static inline void p_end(const nl_node *node, size_t domain)
{
    size_t i = p_index(node);
    assert(domain == p288.heap[i].domain && !p288.heap[i].ended);
    assert(node->f0.tag == 0 && node->f0.ptr == NULL &&
           node->f1 == (i ? 181 : 43));
    p288.heap[i].ended = true;
}
static inline void p_raw(const void *raw, size_t bytes)
{
    size_t i = p_index(raw);
    assert(bytes == sizeof(nl_node) && p288.heap[i].ended && !p288.heap[i].raw);
    p288.heap[i].raw = true;
}
static inline void p_finalize(size_t token)
{
    for (size_t i = 0; i < p288.success; ++i)
        if (p288.heap[i].domain == token) {
            assert(p288.heap[i].ended && p288.heap[i].raw &&
                   !p288.heap[i].final);
            p288.heap[i].final = true;
            ++p288.finalized;
            return;
        }
    assert(0);
}
static inline void p_release(const void *allocation, const void *raw,
                             size_t bytes)
{
    size_t i = p_index(allocation);
    assert(allocation == raw && bytes == sizeof(nl_node));
    assert(p288.heap[i].ended && p288.heap[i].raw && p288.heap[i].final &&
           !p288.heap[i].released);
    p288.heap[i].released = true;
}
static inline void p_change(nl_node_option *field, const nl_node_option *old)
{
    assert(p288.success == 2 && p288.heap[0].live && p288.heap[1].live &&
           field == &p288.heap[0].live->f0);
    if (!p288.changes)
        assert(!old->tag && !old->ptr && field->tag == 1 &&
               field->ptr == p288.heap[1].live);
    else
        assert(p288.changes == 1 && old->tag == 1 &&
               old->ptr == p288.heap[1].live && !field->tag && !field->ptr);
    ++p288.changes;
}
static inline void p_tail(const nl_node *ptr, const void *allocation,
                          size_t token)
{
    assert(p288.success == 2 && p288.heap[1].live == ptr && allocation == ptr &&
           token == p288.heap[1].domain && !p288.heap[1].ended);
}
static inline void p_bind(const uint8_t *a, const uint8_t *b)
{
    assert(!p288.binds && a && b == a + 1 && *a == 37 && *b == 223);
    ++p288.binds;
    p288.field[0] = a;
    p288.field[1] = b;
#if P288_PREHEAP
    assert(p288.attempts == 0);
#else
    p_closed();
#endif
}
static inline void p_phase(size_t phase, const uint8_t *a, const uint8_t *b)
{
    p_closed();
    assert(p288.binds == 1 && phase == p288.visits && phase <= 2);
    assert(a == p288.field[0] && b == p288.field[1]);
    if (phase)
        assert(p288.roles == 31);
    assert(*p288.field[P288_INDEX] == p_old[phase] &&
           *p288.field[1 - P288_INDEX] == P288_SIBLING);
    p288.phase = phase;
    ++p288.visits;
    p288.roles = 0;
}
static inline void p_store(uint8_t *field, uint8_t old, uint8_t next)
{
    assert(p288.phase <= 2 && p288.stores == p288.phase &&
           field == p288.field[P288_INDEX]);
    assert(old == p_old[p288.phase] && next == p_next[p288.phase] &&
           *field == next);
    assert(*p288.field[1 - P288_INDEX] == P288_SIBLING);
    ++p288.stores;
}
static inline void p_local(size_t symbol, uintmax_t value)
{
    if (p288.results) {
        for (size_t i = 0; i < 4; ++i)
            if (symbol == p_caller[i]) {
                assert(!(p288.caller_roles & (1u << i)));
                uintmax_t expected = i < 2    ? 157
                                     : i == 2 ? P288_TERMINAL
                                              : P288_SIBLING;
                assert(value == expected);
                p288.caller_roles |= 1u << i;
                return;
            }
        return;
    }
    if (p288.phase >= 2)
        return; /* Terminal aliases are checked by store/result. */
    for (size_t i = 0; i < 5; ++i)
        if (symbol == p_roles[i]) {
            assert(!(p288.roles & (1u << i)));
            uintmax_t expected = i == 0 || i == 3 ? p_next[p288.phase]
                                 : i == 4         ? P288_SIBLING
                                                  : p_old[p288.phase];
            assert(value == expected);
            p288.roles |= 1u << i;
            return;
        }
}
static inline void p_result(size_t value)
{
    assert(value == 157 && p288.visits == 3 && p288.stores == 3 &&
           !p288.results);
    ++p288.results;
}
static inline void p_read(const uint8_t *field, uint8_t value)
{
    assert(p288.results == 1 && p288.reads < 2);
    assert(field == p288.field[p288.reads ? 1 - P288_INDEX : P288_INDEX]);
    assert(value == (p288.reads ? P288_SIBLING : P288_TERMINAL));
    ++p288.reads;
}
static inline void p_finish_live(void)
{
    p_closed();
    assert(!p288.finished && p288.binds == 1 && p288.visits == 3 &&
           p288.stores == 3 && p288.results == 1 && p288.reads == 2 &&
           p288.caller_roles == 15);
    assert(*p288.field[P288_INDEX] == P288_TERMINAL &&
           *p288.field[1 - P288_INDEX] == P288_SIBLING);
    printf("P1236 live-original preH=%d index=%d H=%zu attempts=%zu free=%zu "
           "final=%d sibling=%d old_result=157 caller_Copy=157 writes=3 PASS\n",
           P288_PREHEAP, P288_INDEX, p288.quota, p288.attempts, p288.frees,
           P288_TERMINAL, P288_SIBLING);
    p288.finished = true;
}
static inline void p_reset(size_t quota)
{
    assert(quota <= 2);
    memset(&p288, 0, sizeof(p288));
    p288.quota = quota;
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
