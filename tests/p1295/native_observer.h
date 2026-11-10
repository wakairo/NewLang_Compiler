#ifndef P1295_NATIVE_OBSERVER_H
#define P1295_NATIVE_OBSERVER_H
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static struct {
    size_t quota, attempts, success, frees, domains, finalized, changes;
    size_t binds, calls, stores, scalars, selected, read_scope;
    bool read_closed, finished;
    uint8_t *target, *sibling, *working, *working_sibling;
    const uint8_t *reading;
    uint8_t computed, supplied;
    struct {
        nl_node *pointer;
        size_t domain;
        bool ended, raw, final, released, freed;
    } h[2];
} p1295;
static const size_t p1295_symbols[6] = P1295_SYMBOLS;
static const uint8_t p1295_results[3] = {13, 29, 71};
static void unchanged(void)
{
    assert(p1295.binds && *p1295.target == 101 && *p1295.sibling == 213);
}
static size_t h_index(const void *pointer)
{
    for (size_t i = 0; i < p1295.success; ++i)
        if (p1295.h[i].pointer == pointer) {
            assert(!p1295.h[i].freed);
            return i;
        }
    assert(0);
    return 0;
}
static void *p_allocate(size_t n)
{
    unchanged();
    assert(n == sizeof(nl_node));
    ++p1295.attempts;
    if (p1295.success == p1295.quota)
        return NULL;
    void *p = malloc(n);
    assert(p && p1295.success < 2);
    if (p1295.success)
        assert(p != p1295.h[0].pointer);
    p1295.h[p1295.success++].pointer = p;
    return p;
}
static void p_free(void *p)
{
    unchanged();
    const size_t i = h_index(p);
    assert(p1295.h[i].ended && p1295.h[i].raw && p1295.h[i].final &&
           p1295.h[i].released);
    if (!i && p1295.success == 2)
        assert(p1295.h[1].freed);
    p1295.h[i].freed = true;
    ++p1295.frees;
    free(p);
}
static void p_domain(size_t d)
{
    unchanged();
    assert(d && p1295.domains < p1295.success);
    if (p1295.domains)
        assert(d != p1295.h[0].domain);
    p1295.h[p1295.domains++].domain = d;
}
static void p_initialize(const nl_node *n, size_t d)
{
    const size_t i = h_index(n);
    assert(d == p1295.h[i].domain && !n->f0.tag && !n->f0.ptr &&
           n->f1 == (i ? 163 : 233));
}
static void p_root(const nl_node *n, size_t d)
{
    const size_t i = h_index(n);
    assert(!p1295.h[i].ended && d == p1295.h[i].domain);
}
static void p_field(const nl_node *n, const void *f, unsigned write,
                    size_t index, size_t scope)
{
    const size_t i = h_index(n);
    assert(!p1295.h[i].ended);
    if (index == 1) {
        assert(i == 1 && !write && !p1295.reading && f == &n->f1 &&
               n->f1 == 163);
        p1295.reading = f;
        p1295.read_scope = scope;
    } else
        assert(index == 0 && f == &n->f0);
}
static void p_t_bind(size_t frame, uint8_t *first, uint8_t *second)
{
    assert(first != second);
    if (frame == P1295_ROOT_FRAME) {
        assert(!p1295.binds && !p1295.attempts && *first == 213 &&
               *second == 101);
        p1295.target = second;
        p1295.sibling = first;
    } else {
        unchanged();
        assert(p1295.binds == 1 && p1295.success == 2 && p1295.selected == 1 &&
               p1295.read_closed && *first == 179 && *second == 71 &&
               first != p1295.target && first != p1295.sibling &&
               second != p1295.target && second != p1295.sibling &&
               second != &p1295.h[0].pointer->f1 &&
               second != &p1295.h[1].pointer->f1);
        p1295.working = second;
        p1295.working_sibling = first;
    }
    ++p1295.binds;
}
static void p_t_call(uint8_t *target, uint8_t old, uint8_t incoming)
{
    unchanged();
    assert(p1295.binds == 2 && !p1295.calls && target == p1295.working &&
           old == 71 && incoming == 163 && *target == incoming &&
           *p1295.working_sibling == 179 && *p1295.reading == incoming &&
           p1295.changes == 1);
    ++p1295.calls;
}
static void p_t_store(uint8_t *target, uint8_t old, uint8_t incoming)
{
    assert(!p1295.stores && target == p1295.target && old == 101 &&
           incoming == p1295_results[p1295.quota] &&
           incoming == p1295.computed && incoming == p1295.supplied &&
           *target == incoming && *p1295.sibling == 213 && p1295.scalars == 2 &&
           p1295.frees == p1295.quota && p1295.finalized == p1295.quota);
    ++p1295.stores;
}
static void p_binding(size_t frame, size_t symbol, const void *address)
{
    if (frame != P1295_ROOT_FRAME)
        return;
    for (size_t i = 0; i < 6; ++i) {
        if (symbol != p1295_symbols[i])
            continue;
        const uint8_t actual = *(const uint8_t *)address;
        assert(p1295.scalars == i);
        if (i < 2) {
            assert(!p1295.stores && actual == p1295_results[p1295.quota] &&
                   p1295.frees == p1295.quota);
            if (!i)
                p1295.computed = actual;
            else
                p1295.supplied = actual;
        } else {
            assert(p1295.stores == 1 && actual == (i == 3   ? p1295.supplied
                                                   : i == 4 ? 213
                                                            : 101));
        }
        ++p1295.scalars;
    }
}
static void p_arm(unsigned tag, const nl_node *n)
{
    unchanged();
    assert(tag == 2 && !p1295.selected && p1295.success == 2 &&
           n == p1295.h[1].pointer && p1295.h[0].pointer->f0.ptr == n);
    ++p1295.selected;
}
static void p_scope_end(size_t scope)
{
    if (p1295.reading && !p1295.read_closed && scope == p1295.read_scope)
        p1295.read_closed = true;
}
static void p_change(const nl_node_option *field, const nl_node_option *old)
{
    unchanged();
    assert(p1295.success == 2 && field == &p1295.h[0].pointer->f0 &&
           p1295.h[0].pointer->f1 == 233 && p1295.h[1].pointer->f1 == 163);
    if (!p1295.changes)
        assert(!old->tag && !old->ptr && field->tag == 1 &&
               field->ptr == p1295.h[1].pointer);
    else
        assert(p1295.changes == 1 && p1295.calls == 1 && old->tag == 1 &&
               old->ptr == p1295.h[1].pointer && !field->tag && !field->ptr);
    ++p1295.changes;
}
static void p_tail(const nl_node *n, const void *allocation, size_t domain)
{
    assert(h_index(n) == 1 && allocation == n && domain == p1295.h[1].domain &&
           !p1295.h[1].ended);
}
static void p_end(const nl_node *n, size_t d)
{
    const size_t i = h_index(n);
    assert(!p1295.h[i].ended && d == p1295.h[i].domain &&
           n->f1 == (i ? 163 : 233) && !n->f0.tag && !n->f0.ptr);
    p1295.h[i].ended = true;
}
static void p_raw(const void *p, size_t n)
{
    const size_t i = h_index(p);
    assert(n == sizeof(nl_node) && p1295.h[i].ended && !p1295.h[i].raw);
    p1295.h[i].raw = true;
}
static void p_finalize(size_t d)
{
    for (size_t i = 0; i < p1295.success; ++i)
        if (p1295.h[i].domain == d) {
            assert(p1295.h[i].ended && p1295.h[i].raw && !p1295.h[i].final);
            p1295.h[i].final = true;
            ++p1295.finalized;
            return;
        }
    assert(0);
}
static void p_release(const void *a, const void *r, size_t n)
{
    const size_t i = h_index(a);
    assert(a == r && n == sizeof(nl_node) && p1295.h[i].raw &&
           p1295.h[i].final && !p1295.h[i].released);
    p1295.h[i].released = true;
}
static void p_finish(void)
{
    const size_t both = p1295.quota == 2;
    assert(p1295.stores == 1 && p1295.scalars == 6 &&
           p1295.success == p1295.quota && p1295.frees == p1295.quota &&
           p1295.domains == p1295.quota && p1295.finalized == p1295.quota &&
           p1295.attempts == (p1295.quota ? 2u : 1u) &&
           p1295.binds == 1 + both && p1295.calls == both &&
           p1295.changes == 2 * both && p1295.selected == both &&
           *p1295.target == p1295.computed && *p1295.sibling == 213);
    p1295.finished = true;
}
#define NL_NODE_BIND(f, s, k, p) p_binding(f, s, p)
#define NL_NODE_REPLACE(...) ((void)0)
#define NL_NODE_ARM(t, p) p_arm(t, p)
#define NL_NODE_RELOAN(...) ((void)0)
#define NL_NODE_FINISH() ((void)0)
#define NL_HEAP_TRIAL(...) ((void)0)
#define NL_HEAP_ARM(...) ((void)0)
#define NL_HEAP_DOMAIN(d) p_domain(d)
#define NL_HEAP_SLOT(...) ((void)0)
#define NL_HEAP_INITIALIZE(p, d) p_initialize(p, d)
#define NL_HEAP_RELOAN(...) ((void)0)
#define NL_HEAP_ROOT(p, d, ...) p_root(p, d)
#define NL_HEAP_FIELD(p, f, w, n, i, c, v, s) p_field(p, f, w, i, s)
#define NL_HEAP_COPY(...) ((void)0)
#define NL_HEAP_CHANGE(p, o, ...) p_change(p, o)
#define NL_HEAP_SCOPE_END(s) p_scope_end(s)
#define NL_HEAP_END(p, d) p_end(p, d)
#define NL_HEAP_RAW(p, n) p_raw(p, n)
#define NL_HEAP_FINALIZE(d) p_finalize(d)
#define NL_HEAP_RELEASE(a, r, n) p_release(a, r, n)
#define NL_HEAP_FINISH() p_finish()
#define NL_HEAP_HANDOFF(p, a, d, ...) p_tail(p, a, d)
#define NL_HEAP_RECEIVER_ENTER(p, a, d, ...) p_tail(p, a, d)
#define NL_HEAP_RECEIVER_EXIT() ((void)0)
#define NL_HEAP_RETURNED(a, d) assert((a)->handle == NULL && (d)->token == 0)
#define NL_LIVE_SEND(p, a, d, ...) p_tail(p, a, d)
#define NL_LIVE_ENTER(h, p, a, d) p_tail(p, a, d)
#define NL_LIVE_RETURN(v)                                                      \
    p_tail((v).ptr, (v).allocation.handle, (v).domain.token)
#define NL_LIVE_RECEIVE(...) ((void)0)
#define NL_MIXED_COPY_BIND(f, s, a, b) p_t_bind(f, a, b)
#define NL_MIXED_COPY_CALL(p, o, n) p_t_call(p, o, n)
#define NL_MIXED_COPY_STORE(p, o, n) p_t_store(p, o, n)
#define malloc p_allocate
#define free p_free
#endif
