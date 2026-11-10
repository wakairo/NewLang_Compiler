/* P-owned passive physical method reused from P1295; original file unchanged. */
#ifndef P1299_NATIVE_OBSERVER_H
#define P1299_NATIVE_OBSERVER_H
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static struct {
    size_t quota, attempts, success, frees, domains, finalized, changes;
    size_t binds, calls, root_calls, scalars, selected, read_scope;
    bool read_closed, finished;
    uint8_t *target, *sibling, *working, *working_sibling;
    const uint8_t *reading;
    uint8_t computed, supplied;
    struct {
        nl_node *pointer;
        size_t domain;
        bool ended, raw, final, released, freed;
    } h[2];
} p1299;
static const size_t p1299_symbols[6] = P1299_SYMBOLS;
static const uint8_t p1299_results[3] = {19, 43, 67};
static void unchanged(void)
{
    assert(p1299.binds && *p1299.target == 103 && *p1299.sibling == 227);
}
static size_t h_index(const void *pointer)
{
    for (size_t i = 0; i < p1299.success; ++i)
        if (p1299.h[i].pointer == pointer) {
            assert(!p1299.h[i].freed);
            return i;
        }
    assert(0);
    return 0;
}
static void *p_allocate(size_t n)
{
    unchanged();
    assert(n == sizeof(nl_node));
    ++p1299.attempts;
    if (p1299.success == p1299.quota)
        return NULL;
    void *p = malloc(n);
    assert(p && p1299.success < 2);
    if (p1299.success)
        assert(p != p1299.h[0].pointer);
    p1299.h[p1299.success++].pointer = p;
    return p;
}
static void p_free(void *p)
{
    unchanged();
    const size_t i = h_index(p);
    assert(p1299.h[i].ended && p1299.h[i].raw && p1299.h[i].final &&
           p1299.h[i].released);
    if (!i && p1299.success == 2)
        assert(p1299.h[1].freed);
    p1299.h[i].freed = true;
    ++p1299.frees;
    free(p);
}
static void p_domain(size_t d)
{
    unchanged();
    assert(d && p1299.domains < p1299.success);
    if (p1299.domains)
        assert(d != p1299.h[0].domain);
    p1299.h[p1299.domains++].domain = d;
}
static void p_initialize(const nl_node *n, size_t d)
{
    const size_t i = h_index(n);
    assert(d == p1299.h[i].domain && !n->f0.tag && !n->f0.ptr &&
           n->f1 == (i ? 149 : 239));
}
static void p_root(const nl_node *n, size_t d)
{
    const size_t i = h_index(n);
    assert(!p1299.h[i].ended && d == p1299.h[i].domain);
}
static void p_field(const nl_node *n, const void *f, unsigned write,
                    size_t index, size_t scope)
{
    const size_t i = h_index(n);
    assert(!p1299.h[i].ended);
    if (index == 1) {
        assert(i == 1 && !write && !p1299.reading && f == &n->f1 &&
               n->f1 == 149);
        p1299.reading = f;
        p1299.read_scope = scope;
    } else
        assert(index == 0 && f == &n->f0);
}
static void p_t_bind(size_t frame, uint8_t *first, uint8_t *second)
{
    assert(first != second);
    if (frame == P1299_ROOT_FRAME) {
        assert(!p1299.binds && !p1299.attempts && *first == 103 && *second == 227);
        p1299.target = first;
        p1299.sibling = second;
    } else {
        unchanged();
        assert(p1299.binds == 1 && p1299.success == 2 && p1299.selected == 1 &&
               p1299.read_closed && *first == 67 && *second == 193 &&
               first != p1299.target && first != p1299.sibling &&
               second != p1299.target && second != p1299.sibling &&
               first != &p1299.h[0].pointer->f1 &&
               first != &p1299.h[1].pointer->f1);
        p1299.working = first;
        p1299.working_sibling = second;
    }
    ++p1299.binds;
}
static void p_t_call(uint8_t *target, uint8_t old, uint8_t incoming)
{
    if (target == p1299.target) {
        assert(!p1299.root_calls && old == 103 &&
               incoming == p1299_results[p1299.quota] &&
               incoming == p1299.computed && incoming == p1299.supplied &&
               *target == incoming && *p1299.sibling == 227 &&
               p1299.scalars == 2 && p1299.frees == p1299.quota &&
               p1299.finalized == p1299.quota);
        ++p1299.root_calls;
    } else {
        unchanged();
        assert(p1299.binds == 2 && !p1299.calls && target == p1299.working &&
               old == 67 && incoming == 149 && *target == incoming &&
               *p1299.working_sibling == 193 && *p1299.reading == incoming &&
               p1299.changes == 1);
        ++p1299.calls;
    }
}
static void p_binding(size_t frame, size_t symbol, const void *address)
{
    if (frame != P1299_ROOT_FRAME)
        return;
    for (size_t i = 0; i < 6; ++i) {
        if (symbol != p1299_symbols[i])
            continue;
        const uint8_t actual = *(const uint8_t *)address;
        fprintf(stderr, "P1299-DIAG i=%zu actual=%u root_calls=%zu supplied=%u expected_old=103 expected_current=%u expected_sibling=227\n", i, actual, p1299.root_calls, p1299.supplied, p1299.supplied);
        assert(p1299.scalars == i);
        if (i < 2) {
            assert(!p1299.root_calls && actual == p1299_results[p1299.quota] &&
                   p1299.frees == p1299.quota);
            if (!i)
                p1299.computed = actual;
            else
                p1299.supplied = actual;
        } else {
            assert(p1299.root_calls == 1 && actual == (i == 3   ? p1299.supplied
                                                   : i == 4 ? 227
                                                            : 103));
        }
        ++p1299.scalars;
    }
}
static void p_arm(unsigned tag, const nl_node *n)
{
    unchanged();
    assert(tag == 2 && !p1299.selected && p1299.success == 2 &&
           n == p1299.h[1].pointer && p1299.h[0].pointer->f0.ptr == n);
    ++p1299.selected;
}
static void p_scope_end(size_t scope)
{
    if (p1299.reading && !p1299.read_closed && scope == p1299.read_scope)
        p1299.read_closed = true;
}
static void p_change(const nl_node_option *field, const nl_node_option *old)
{
    unchanged();
    assert(p1299.success == 2 && field == &p1299.h[0].pointer->f0 &&
           p1299.h[0].pointer->f1 == 239 && p1299.h[1].pointer->f1 == 149);
    if (!p1299.changes)
        assert(!old->tag && !old->ptr && field->tag == 1 &&
               field->ptr == p1299.h[1].pointer);
    else
        assert(p1299.changes == 1 && p1299.calls == 1 && old->tag == 1 &&
               old->ptr == p1299.h[1].pointer && !field->tag && !field->ptr);
    ++p1299.changes;
}
static void p_tail(const nl_node *n, const void *allocation, size_t domain)
{
    assert(h_index(n) == 1 && allocation == n && domain == p1299.h[1].domain &&
           !p1299.h[1].ended);
}
static void p_end(const nl_node *n, size_t d)
{
    const size_t i = h_index(n);
    assert(!p1299.h[i].ended && d == p1299.h[i].domain &&
           n->f1 == (i ? 149 : 239) && !n->f0.tag && !n->f0.ptr);
    p1299.h[i].ended = true;
}
static void p_raw(const void *p, size_t n)
{
    const size_t i = h_index(p);
    assert(n == sizeof(nl_node) && p1299.h[i].ended && !p1299.h[i].raw);
    p1299.h[i].raw = true;
}
static void p_finalize(size_t d)
{
    for (size_t i = 0; i < p1299.success; ++i)
        if (p1299.h[i].domain == d) {
            assert(p1299.h[i].ended && p1299.h[i].raw && !p1299.h[i].final);
            p1299.h[i].final = true;
            ++p1299.finalized;
            return;
        }
    assert(0);
}
static void p_release(const void *a, const void *r, size_t n)
{
    const size_t i = h_index(a);
    assert(a == r && n == sizeof(nl_node) && p1299.h[i].raw &&
           p1299.h[i].final && !p1299.h[i].released);
    p1299.h[i].released = true;
}
static void p_finish(void)
{
    const size_t both = p1299.quota == 2;
    assert(p1299.root_calls == 1 && p1299.scalars == 6 &&
           p1299.success == p1299.quota && p1299.frees == p1299.quota &&
           p1299.domains == p1299.quota && p1299.finalized == p1299.quota &&
           p1299.attempts == (p1299.quota ? 2u : 1u) &&
           p1299.binds == 1 + both && p1299.calls == both &&
           p1299.changes == 2 * both && p1299.selected == both &&
           *p1299.target == p1299.computed && *p1299.sibling == 227);
    p1299.finished = true;
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
#define NL_MIXED_COPY_STORE(...) assert(0)
#define malloc p_allocate
#define free p_free
#endif
