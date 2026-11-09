#ifndef P1291_NATIVE_OBSERVER_H
#define P1291_NATIVE_OBSERVER_H
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static struct {
    size_t quota, attempts, success, frees, domains, finalized, changes;
    size_t scalar_scope, selected, copies, t_binds, writes, arm_copies, outer;
    bool scalar_closed, finished;
    const uint8_t *scalar;
    uint8_t *target, *sibling;
    struct {
        nl_node *pointer;
        size_t domain;
        bool ended, raw, final, released;
    } h[2];
} p1291;
static const size_t p_symbols[6] = P1291_SYMBOLS;
static const size_t p_results[3] = P1291_RESULTS;
static size_t p_index(const void *pointer)
{
    for (size_t i = 0; i < p1291.success; ++i)
        if (p1291.h[i].pointer == pointer)
            return i;
    assert(0);
    return 0;
}
static void *p_allocate(size_t n)
{
    assert(n == sizeof(nl_node));
    ++p1291.attempts;
    if (p1291.success == p1291.quota)
        return NULL;
    void *p = malloc(n);
    assert(p && p1291.success < 2);
    if (p1291.success)
        assert(p != p1291.h[0].pointer);
    p1291.h[p1291.success++].pointer = p;
    return p;
}
static void p_free(void *pointer)
{
    const size_t i = p_index(pointer);
    assert(p1291.h[i].ended && p1291.h[i].raw && p1291.h[i].final &&
           p1291.h[i].released);
    if (!i && p1291.success == 2)
        assert(!p1291.h[1].pointer);
    p1291.h[i].pointer = NULL;
    ++p1291.frees;
    free(pointer);
}
static void p_domain(size_t token)
{
    assert(token && p1291.domains < p1291.success);
    if (p1291.domains)
        assert(token != p1291.h[0].domain);
    p1291.h[p1291.domains++].domain = token;
}
static void p_initialize(const nl_node *n, size_t domain)
{
    const size_t i = p_index(n);
    assert(domain == p1291.h[i].domain && !n->f0.tag && !n->f0.ptr &&
           n->f1 == (i ? P1291_NEW : P1291_FRONT));
}
static void p_field(const nl_node *root, const void *field, unsigned write,
                    size_t index, size_t scope)
{
    assert(!p1291.h[p_index(root)].ended);
    if (index == 1) {
        assert(p_index(root) == 1 && !write && !p1291.scalar &&
               field == &root->f1 && root->f1 == P1291_NEW);
        p1291.scalar = field;
        p1291.scalar_scope = scope;
    } else
        assert(index == 0 && field == &root->f0);
}
static void p_binding(size_t frame, size_t symbol, size_t kind,
                      const void *address)
{
    /* Symbols and scopes are world-local. Never inspect another frame's
     * pointer/owner binding as though an equal numeric symbol meant u8. */
    if (frame == P1291_FRAME) {
        const size_t expected[6] = {P1291_NEW, P1291_NEW, P1291_OLD,
                                    P1291_OLD, P1291_NEW, P1291_SIBLING};
        for (size_t i = 0; i < 6; ++i)
            if (symbol == p_symbols[i]) {
                assert(kind == 4 && p1291.scalar && p1291.scalar_closed &&
                       p1291.copies == i &&
                       *(const uint8_t *)address == expected[i] &&
                       p1291.writes == (i < 2 ? 0u : 1u));
                ++p1291.copies;
            }
    }
    if (frame == P1291_ARM_FRAME && symbol == P1291_ARM_SYMBOL) {
        assert(kind == 4 && p1291.copies == 6 && p1291.writes == 1 &&
               !p1291.arm_copies && !p1291.frees && p1291.changes == 1 &&
               *(const uint8_t *)address == P1291_OLD);
        ++p1291.arm_copies;
    }
    if (frame == P1291_OUTER_FRAME)
        for (size_t i = 0; i < 3; ++i)
            if (symbol == p_results[i]) {
                const size_t expected[3] = {P1291_H0, P1291_H1, P1291_OLD};
                assert(kind == 4 && p1291.outer == i &&
                       p1291.success == p1291.quota &&
                       p1291.frees == p1291.success &&
                       p1291.finalized == p1291.success &&
                       *(const uint8_t *)address == expected[p1291.quota]);
                /* All original H and T carriers have ended here. Observe
                 * only the actual continuation u8, never dangling pointers. */
                ++p1291.outer;
            }
}
static void p_t_bind(uint8_t *first, uint8_t *second)
{
    uint8_t *target = P1291_FIELD ? second : first;
    uint8_t *sibling = P1291_FIELD ? first : second;
    assert(
        p1291.success == 2 && p1291.copies == 2 && p1291.scalar_closed &&
        !p1291.t_binds && *target == P1291_OLD && *sibling == P1291_SIBLING &&
        first != second && first != &p1291.h[0].pointer->f1 &&
        first != &p1291.h[1].pointer->f1 && second != &p1291.h[0].pointer->f1 &&
        second != &p1291.h[1].pointer->f1 && p1291.selected == 1);
    p1291.target = target;
    p1291.sibling = sibling;
    ++p1291.t_binds;
}
static void p_t_call(uint8_t *target, uint8_t old, uint8_t incoming)
{
    assert(p1291.t_binds == 1 && !p1291.writes && target == p1291.target &&
           old == P1291_OLD && incoming == P1291_NEW && *target == incoming &&
           *p1291.sibling == P1291_SIBLING && *p1291.scalar == incoming &&
           p1291.changes == 1);
    ++p1291.writes;
}
static void p_arm(unsigned tag, const nl_node *p)
{
    assert(tag == 2 && !p1291.selected && p1291.success == 2 &&
           p == p1291.h[1].pointer && p1291.h[0].pointer->f0.ptr == p &&
           p1291.changes == 1);
    ++p1291.selected;
}
static void p_scope_end(size_t scope)
{
    if (p1291.scalar && !p1291.scalar_closed && scope == p1291.scalar_scope)
        p1291.scalar_closed = true;
}
static void p_change(const nl_node_option *field, const nl_node_option *old)
{
    assert(p1291.success == 2 && field == &p1291.h[0].pointer->f0 &&
           p1291.h[0].pointer->f1 == P1291_FRONT &&
           p1291.h[1].pointer->f1 == P1291_NEW);
    if (!p1291.changes)
        assert(!old->tag && !old->ptr && field->tag == 1 &&
               field->ptr == p1291.h[1].pointer);
    else
        assert(p1291.writes == 1 && p1291.arm_copies == 1 &&
               p1291.changes == 1 && old->tag == 1 &&
               old->ptr == p1291.h[1].pointer && !field->tag && !field->ptr);
    ++p1291.changes;
}
static void p_tail(const nl_node *n, const void *allocation, size_t domain)
{
    assert(p_index(n) == 1 && allocation == n && domain == p1291.h[1].domain &&
           !p1291.h[1].ended);
}
static void p_end(const nl_node *n, size_t domain)
{
    const size_t i = p_index(n);
    assert(!p1291.h[i].ended && domain == p1291.h[i].domain &&
           n->f1 == (i ? P1291_NEW : P1291_FRONT) && !n->f0.tag && !n->f0.ptr);
    p1291.h[i].ended = true;
}
static void p_raw(const void *p, size_t n)
{
    const size_t i = p_index(p);
    assert(n == sizeof(nl_node) && p1291.h[i].ended && !p1291.h[i].raw);
    p1291.h[i].raw = true;
}
static void p_finalize(size_t domain)
{
    for (size_t i = 0; i < p1291.success; ++i)
        if (p1291.h[i].domain == domain) {
            assert(p1291.h[i].ended && p1291.h[i].raw && !p1291.h[i].final);
            p1291.h[i].final = true;
            ++p1291.finalized;
            return;
        }
    assert(0);
}
static void p_release(const void *a, const void *raw, size_t n)
{
    const size_t i = p_index(a);
    assert(a == raw && n == sizeof(nl_node) && p1291.h[i].raw &&
           p1291.h[i].final && !p1291.h[i].released);
    p1291.h[i].released = true;
}
static void p_finish(void)
{
    const bool both = p1291.quota == 2;
    assert(
        p1291.success == p1291.quota && p1291.frees == p1291.success &&
        p1291.domains == p1291.success && p1291.finalized == p1291.success &&
        p1291.outer == 3 && p1291.attempts == (p1291.quota ? 2u : 1u) &&
        p1291.changes == (both ? 2u : 0u) && p1291.copies == (both ? 6u : 0u) &&
        p1291.t_binds == (both ? 1u : 0u) && p1291.writes == p1291.t_binds &&
        p1291.selected == p1291.t_binds && p1291.arm_copies == p1291.t_binds);
    for (size_t i = 0; i < p1291.success; ++i)
        assert(!p1291.h[i].pointer && p1291.h[i].ended && p1291.h[i].released);
    p1291.finished = true;
}
#define NL_NODE_BIND(f, s, k, p) p_binding(f, s, k, p)
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
#define NL_HEAP_ROOT(...) ((void)0)
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
#define NL_MIXED_COPY_BIND(f, s, a, b) p_t_bind(a, b)
#define NL_MIXED_COPY_CALL(p, o, n) p_t_call(p, o, n)
#define NL_MIXED_COPY_STORE(...) assert(0)
#define malloc p_allocate
#define free p_free
#endif
