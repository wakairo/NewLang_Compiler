#ifndef P1280_NATIVE_OBSERVER_H
#define P1280_NATIVE_OBSERVER_H
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static struct {
    size_t quota, attempts, success, frees, domains, finalized, changes, copies;
    size_t scalar_scope;
    bool scalar_closed, finished;
    size_t selected_arms, t_binds, t_writes, alias_copies;
    uint8_t *t_first, *t_sibling;
    const uint8_t *scalar;
    struct {
        nl_node *pointer;
        size_t domain;
        bool ended, raw, final, released;
    } h[2];
} p1280;
static const size_t p_symbols[4] = P1280_SYMBOLS;
static size_t p_index(const void *pointer)
{
    for (size_t i = 0; i < p1280.success; ++i)
        if (p1280.h[i].pointer == pointer)
            return i;
    assert(0);
    return 0;
}
static void *p_allocate(size_t n)
{
    assert(n == sizeof(nl_node));
    ++p1280.attempts;
    if (p1280.success == p1280.quota)
        return NULL;
    void *p = malloc(n);
    assert(p && p1280.success < 2);
    if (p1280.success)
        assert(p != p1280.h[0].pointer);
    p1280.h[p1280.success++].pointer = p;
    return p;
}
static void p_free(void *pointer)
{
    const size_t i = p_index(pointer);
    assert(p1280.h[i].ended && p1280.h[i].raw && p1280.h[i].final &&
           p1280.h[i].released);
    if (i == 0 && p1280.success == 2)
        assert(p1280.h[1].pointer == NULL);
    p1280.h[i].pointer = NULL;
    ++p1280.frees;
    free(pointer);
}
static void p_domain(size_t token)
{
    assert(token && p1280.domains < p1280.success);
    if (p1280.domains)
        assert(token != p1280.h[0].domain);
    p1280.h[p1280.domains++].domain = token;
}
static void p_initialize(const nl_node *n, size_t domain)
{
    const size_t i = p_index(n);
    assert(domain == p1280.h[i].domain && !n->f0.tag && !n->f0.ptr &&
           n->f1 == (i == 0 ? 231 : P1280_BYTE));
}
static void p_field(const nl_node *root, const void *field, unsigned write,
                    size_t index, size_t scope)
{
    assert(!p1280.h[p_index(root)].ended);
    if (index == 1) {
        assert(p_index(root) == 1 && !write && !p1280.scalar &&
               field == &root->f1 && root->f1 == P1280_BYTE);
        p1280.scalar = field;
        p1280.scalar_scope = scope;
    } else {
        assert(index == 0 && field == &root->f0);
    }
}
static void p_binding(size_t symbol, size_t kind, const void *address)
{
    for (size_t i = 0; i < 4; ++i)
        if (symbol == p_symbols[i]) {
            assert(kind == 4 && p1280.scalar && p1280.scalar_closed);
            const size_t actual = *(const uint8_t *)address;
            if (i == 0) {
                assert(!p1280.copies && actual == P1280_BYTE &&
                       actual == *p1280.scalar);
                ++p1280.copies;
            } else {
                assert(p1280.t_writes == 1 && p1280.t_binds == 1 &&
                       actual == (i == 1   ? 31
                                  : i == 2 ? P1280_BYTE
                                           : 127));
                ++p1280.alias_copies;
            }
        }
}
static void p_t_bind(uint8_t *first, uint8_t *second)
{
    assert(p1280.success == 2 && p1280.copies == 1 && p1280.scalar_closed &&
           !p1280.t_binds && *first == 31 && *second == 127 &&
           first != second && first != &p1280.h[0].pointer->f1 &&
           first != &p1280.h[1].pointer->f1 &&
           p1280.selected_arms == P1280_LINKED);
    p1280.t_first = first;
    p1280.t_sibling = second;
    ++p1280.t_binds;
}
static void p_t_call(uint8_t *first, uint8_t old, uint8_t incoming)
{
    assert(p1280.t_binds == 1 && !p1280.t_writes && first == p1280.t_first &&
           old == 31 && incoming == P1280_BYTE && *first == incoming &&
           *p1280.t_sibling == 127 && *p1280.scalar == incoming &&
           p1280.changes == 1);
    ++p1280.t_writes;
}
static inline void p_arm(unsigned tag, const nl_node *p)
{
    assert(P1280_LINKED && tag == 2 && !p1280.selected_arms &&
           p1280.success == 2 && p == p1280.h[1].pointer &&
           p1280.h[0].pointer->f0.ptr == p && p1280.changes == 1);
    ++p1280.selected_arms;
}
static void p_scope_end(size_t scope)
{
    /* Scope IDs belong to their issued worlds. A later outer detach may
     * reuse this number; close only the actual observed scalar-loan interval.
     */
    if (p1280.scalar && !p1280.scalar_closed && scope == p1280.scalar_scope) {
        assert(p1280.scalar && !p1280.scalar_closed);
        p1280.scalar_closed = true;
    }
}
static void p_change(const nl_node_option *field, const nl_node_option *old)
{
    assert(p1280.success == 2 && field == &p1280.h[0].pointer->f0 &&
           p1280.h[0].pointer->f1 == 231 &&
           p1280.h[1].pointer->f1 == P1280_BYTE);
    if (!p1280.changes)
        assert(old->tag == 0 && !old->ptr && field->tag == 1 &&
               field->ptr == p1280.h[1].pointer);
    else
        assert(p1280.t_writes == 1 && p1280.alias_copies == 3 &&
               p1280.changes == 1 && old->tag == 1 &&
               old->ptr == p1280.h[1].pointer && !field->tag && !field->ptr);
    ++p1280.changes;
}
static void p_tail(const nl_node *n, const void *allocation, size_t domain)
{
    assert(p_index(n) == 1 && allocation == n && domain == p1280.h[1].domain &&
           !p1280.h[1].ended);
}
static void p_end(const nl_node *n, size_t domain)
{
    size_t i = p_index(n);
    assert(!p1280.h[i].ended && domain == p1280.h[i].domain &&
           n->f1 == (i == 0 ? 231 : P1280_BYTE) && !n->f0.tag && !n->f0.ptr);
    p1280.h[i].ended = true;
}
static void p_raw(const void *p, size_t n)
{
    size_t i = p_index(p);
    assert(n == sizeof(nl_node) && p1280.h[i].ended && !p1280.h[i].raw);
    p1280.h[i].raw = true;
}
static void p_finalize(size_t domain)
{
    for (size_t i = 0; i < p1280.success; ++i)
        if (p1280.h[i].domain == domain) {
            assert(p1280.h[i].ended && p1280.h[i].raw && !p1280.h[i].final);
            p1280.h[i].final = true;
            ++p1280.finalized;
            return;
        }
    assert(0);
}
static void p_release(const void *a, const void *raw, size_t n)
{
    size_t i = p_index(a);
    assert(a == raw && n == sizeof(nl_node) && p1280.h[i].raw &&
           p1280.h[i].final && !p1280.h[i].released);
    p1280.h[i].released = true;
}
static void p_finish(void)
{
    assert(p1280.success == p1280.quota && p1280.frees == p1280.success &&
           p1280.domains == p1280.success && p1280.finalized == p1280.success &&
           p1280.attempts == (p1280.quota ? 2u : 1u) &&
           p1280.changes == (p1280.quota == 2 ? 2u : 0u) &&
           p1280.copies == (p1280.quota == 2 ? 1u : 0u) &&
           p1280.t_binds == (p1280.quota == 2 ? 1u : 0u) &&
           p1280.t_writes == p1280.t_binds &&
           p1280.alias_copies == (p1280.quota == 2 ? 3u : 0u) &&
           p1280.selected_arms == (p1280.quota == 2 ? P1280_LINKED : 0u));
    for (size_t i = 0; i < p1280.success; ++i)
        assert(!p1280.h[i].pointer && p1280.h[i].ended && p1280.h[i].released);
    p1280.finished = true;
}
#define NL_NODE_BIND(f, s, k, p) p_binding(s, k, p)
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
