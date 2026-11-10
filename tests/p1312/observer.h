#ifndef P1312_PASSIVE_OBSERVER_H
#define P1312_PASSIVE_OBSERVER_H
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

unsigned p1312_world(void);
unsigned p1312_attempts(void);
unsigned p1312_successes(void);
unsigned p1312_frees(void);

static struct {
    const nl_node *node;
    size_t domain;
    bool ended, raw, finalized, released;
} p1312_heap[2];
static size_t p1312_trials, p1312_domains, p1312_graph_changes, p1312_payloads;
static size_t p1312_root_scopes[8], p1312_borrowed;
static size_t p1312_receivers, p1312_producers;
static uint8_t *p1312_t0, *p1312_t1;
static size_t p1312_binds, p1312_phases, p1312_stores, p1312_results,
    p1312_reads;
static uint8_t p1312_locals[16];
static size_t p1312_local_count;

static size_t p1312_heap_index(const void *p)
{
    for (size_t i = 0; i < p1312_successes(); ++i)
        if ((const void *)p1312_heap[i].node == p)
            return i;
    assert(!"unknown actual original H pointer");
    return 0;
}
static uint8_t p1312_outcome(void)
{
    static const uint8_t expected[] = {27, 83, 212};
    return expected[p1312_world()];
}
static void p1312_trial(void *p, size_t size, size_t align)
{
    assert(p1312_trials < 2 && size == 24 && align == 8);
    ++p1312_trials;
    assert(p1312_trials == p1312_attempts());
    if (p) {
        assert((uintptr_t)p % align == 0);
        p1312_heap[p1312_successes() - 1].node = p;
    }
}
static void p1312_initialize(const nl_node *p, size_t domain)
{
    size_t i = p1312_heap_index(p);
    assert(domain && !p1312_heap[i].domain && p->f0.tag == 0 && !p->f0.ptr);
    assert(p->f1 == (i ? 212 : 19));
    p1312_heap[i].domain = domain;
}
static void p1312_root(const nl_node *p, size_t domain, size_t scope)
{
    size_t i = p1312_heap_index(p);
    assert(!p1312_heap[i].ended && domain == p1312_heap[i].domain);
    assert(scope && scope < 8 && !p1312_root_scopes[scope]);
    p1312_root_scopes[scope] = 1;
    ++p1312_borrowed;
}
static void p1312_field(const nl_node *p, const void *field, size_t index)
{
    size_t i = p1312_heap_index(p);
    assert(!p1312_heap[i].ended && index < 2);
    if (!index)
        assert(field == (const void *)&p->f0);
    else {
        assert(i == 1 && field == (const void *)&p->f1 && p->f1 == 212);
        ++p1312_payloads;
    }
}
static void p1312_change(const nl_node_option *field, const nl_node_option *old,
                         size_t change)
{
    assert(p1312_world() == 2 && field == &p1312_heap[0].node->f0);
    if (change == 1) {
        assert(p1312_graph_changes == 0 && old->tag == 0 && !old->ptr);
        assert(field->tag == 1 && field->ptr == p1312_heap[1].node);
    } else {
        assert(change == 2 && p1312_graph_changes == 1 && old->tag == 1);
        assert(old->ptr == p1312_heap[1].node && field->tag == 0 &&
               !field->ptr);
    }
    ++p1312_graph_changes;
}
static void p1312_scope_end(size_t scope)
{
    assert(scope && scope < 8 && p1312_root_scopes[scope] && p1312_borrowed);
    p1312_root_scopes[scope] = 0;
    --p1312_borrowed;
}
static void p1312_end(const nl_node *p, size_t domain)
{
    size_t i = p1312_heap_index(p);
    assert(!p1312_heap[i].ended && domain == p1312_heap[i].domain);
    assert(!p1312_borrowed && !p->f0.tag && !p->f0.ptr);
    if (i)
        assert(!p1312_heap[0].node->f0.tag && !p1312_heap[0].node->f0.ptr);
    p1312_heap[i].ended = true;
}
static void p1312_raw(const void *p, size_t size)
{
    size_t i = p1312_heap_index(p);
    assert(size == 24 && p1312_heap[i].ended && !p1312_heap[i].raw);
    p1312_heap[i].raw = true;
}
static void p1312_finalize(size_t domain)
{
    size_t i = 0;
    while (i < p1312_successes() && p1312_heap[i].domain != domain)
        ++i;
    assert(i < p1312_successes() && p1312_heap[i].raw &&
           !p1312_heap[i].finalized);
    p1312_heap[i].finalized = true;
}
static void p1312_release(const void *allocation, const void *storage,
                          size_t size)
{
    size_t i = p1312_heap_index(allocation);
    assert(allocation == storage && size == 24 && p1312_heap[i].finalized);
    assert(!p1312_heap[i].released);
    p1312_heap[i].released = true;
}
static void p1312_owner(const nl_node *p, const void *allocation, size_t domain)
{
    assert(p1312_world() == 2 && p == p1312_heap[1].node && allocation == p);
    assert(domain == p1312_heap[1].domain && !p1312_heap[1].ended);
}
static void p1312_bind(uint8_t *first, uint8_t *second)
{
    assert(!p1312_binds++ && first != second && *first == 41 && *second == 167);
    p1312_t0 = first;
    p1312_t1 = second;
}
static void p1312_phase(size_t phase, uint8_t *first, uint8_t *second)
{
    static const uint8_t expected[] = {41, 49, 117};
    assert(p1312_phases < 3 && phase == p1312_phases && first == p1312_t0 &&
           second == p1312_t1);
    assert(*first == expected[p1312_phases] && *second == 167);
    assert(p1312_frees() == p1312_world() && !p1312_borrowed);
    ++p1312_phases;
}
static void p1312_store(uint8_t *p, uint8_t old, uint8_t value)
{
    static const uint8_t previous[] = {41, 49, 117};
    assert(p1312_stores < 3 && p == p1312_t0 && *p == value &&
           *p1312_t1 == 167);
    assert(old == previous[p1312_stores]);
    assert(value == (p1312_stores == 0   ? 49
                     : p1312_stores == 1 ? 117
                                         : p1312_outcome()));
    ++p1312_stores;
}
static void p1312_local(uint8_t value)
{
    assert(p1312_local_count < 16);
    p1312_locals[p1312_local_count++] = value;
}
static void p1312_result(uint8_t value)
{
    assert(!p1312_results++ && value == 117 && p1312_stores == 3);
}
static void p1312_read(const uint8_t *p, uint8_t value)
{
    assert(p1312_reads < 2 && p == (p1312_reads ? p1312_t1 : p1312_t0));
    assert(*p == value && value == (p1312_reads ? 167 : p1312_outcome()));
    ++p1312_reads;
}
static void p1312_finish(void)
{
    unsigned world = p1312_world();
    uint8_t outcome = p1312_outcome();
    const uint8_t expected[] = {outcome, outcome, 49,      41,  117, 49,
                                117,     117,     outcome, 167, 117};
    assert(p1312_trials == (world ? 2u : 1u) && p1312_domains == world);
    assert(p1312_binds == 1 && p1312_phases == 3 && p1312_stores == 3 &&
           p1312_results == 1 && p1312_reads == 2);
    assert(p1312_local_count == sizeof(expected) && !p1312_borrowed);
    for (size_t i = 0; i < sizeof(expected); ++i)
        assert(p1312_locals[i] == expected[i]);
    assert(p1312_successes() == world && p1312_frees() == world);
    for (size_t i = 0; i < world; ++i)
        assert(p1312_heap[i].released);
    assert(p1312_graph_changes == (world == 2 ? 2u : 0u));
    assert(p1312_payloads == (world == 2 ? 1u : 0u));
    assert(p1312_receivers == (world == 2 ? 1u : 0u) &&
           p1312_producers == p1312_receivers);
    assert(*p1312_t0 == outcome && *p1312_t1 == 167);
    printf("PASSIVE H%u outcome=%u terminal-old=117 current=%u sibling=167 "
           "phases=3 backedges=2 same-T=1 frees=%u\n",
           world, (unsigned)outcome, (unsigned)*p1312_t0, p1312_frees());
}

#define NL_NODE_BIND(...) ((void)0)
#define NL_NODE_REPLACE(...) ((void)0)
#define NL_NODE_ARM(...) ((void)0)
#define NL_NODE_RELOAN(...) ((void)0)
#define NL_NODE_FINISH() ((void)0)
#define NL_HEAP_TRIAL(p, n, a) p1312_trial(p, n, a)
#define NL_HEAP_ARM(...) ((void)0)
#define NL_HEAP_DOMAIN(...) ((void)++p1312_domains)
#define NL_HEAP_SLOT(...) ((void)0)
#define NL_HEAP_INITIALIZE(p, d) p1312_initialize(p, d)
#define NL_HEAP_RELOAN(...) ((void)0)
#define NL_HEAP_ROOT(p, d, w, r, i, s) p1312_root(p, d, s)
#define NL_HEAP_FIELD(p, f, w, pi, i, ci, cf, s) p1312_field(p, f, i)
#define NL_HEAP_COPY(...) ((void)0)
#define NL_HEAP_CHANGE(f, o, n) p1312_change(f, o, n)
#define NL_HEAP_SCOPE_END(s) p1312_scope_end(s)
#define NL_HEAP_END(p, d) p1312_end(p, d)
#define NL_HEAP_RAW(p, n) p1312_raw(p, n)
#define NL_HEAP_FINALIZE(d) p1312_finalize(d)
#define NL_HEAP_RELEASE(a, p, n) p1312_release(a, p, n)
#define NL_HEAP_FINISH() p1312_finish()
#define NL_HEAP_HANDOFF(p, a, d, ...) p1312_owner(p, a, d)
#define NL_HEAP_RECEIVER_ENTER(p, a, d, ...)                                   \
    (p1312_owner(p, a, d), (void)++p1312_receivers)
#define NL_HEAP_RECEIVER_EXIT() ((void)0)
#define NL_HEAP_RETURNED(...) ((void)0)
#define NL_LIVE_SEND(p, a, d, ...) p1312_owner(p, a, d)
#define NL_LIVE_ENTER(e, p, a, d)                                              \
    (p1312_owner(p, a, d), (void)++p1312_producers)
#define NL_LIVE_RETURN(...) ((void)0)
#define NL_LIVE_RECEIVE(...) ((void)0)
#define NL_CURRENT_PHASE(n, a, b) p1312_phase(n, a, b)
#define NL_CURRENT_LOCAL(n, v) p1312_local(v)
#define NL_CURRENT_RESULT(v) p1312_result(v)
#define NL_MIXED_COPY_BIND(f, s, a, b) p1312_bind(a, b)
#define NL_MIXED_COPY_STORE(p, o, n) p1312_store(p, o, n)
#define NL_LOOP_READ(p, v) p1312_read(p, v)
#endif
