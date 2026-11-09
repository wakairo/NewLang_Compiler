#ifndef P1272_NATIVE_OBSERVER_H
#define P1272_NATIVE_OBSERVER_H
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static struct {
    size_t quota, attempts, success, frees, domains, finalized, changes, copies;
    size_t scalar_scope;
    bool scalar_closed, finished;
    const uint8_t *scalar;
    struct {
        nl_node *pointer;
        size_t domain;
        bool ended, raw, final, released;
    } h[2];
} p1272;
static const size_t p_symbols[3] = P1272_SYMBOLS;
static size_t p_index(const void *pointer)
{
    for (size_t i = 0; i < p1272.success; ++i)
        if (p1272.h[i].pointer == pointer)
            return i;
    assert(0);
    return 0;
}
static void *p_allocate(size_t n)
{
    assert(n == sizeof(nl_node));
    ++p1272.attempts;
    if (p1272.success == p1272.quota)
        return NULL;
    void *p = malloc(n);
    assert(p && p1272.success < 2);
    if (p1272.success)
        assert(p != p1272.h[0].pointer);
    p1272.h[p1272.success++].pointer = p;
    return p;
}
static void p_free(void *pointer)
{
    const size_t i = p_index(pointer);
    assert(p1272.h[i].ended && p1272.h[i].raw && p1272.h[i].final &&
           p1272.h[i].released);
    if (i == 0 && p1272.success == 2)
        assert(p1272.h[1].pointer == NULL);
    p1272.h[i].pointer = NULL;
    ++p1272.frees;
    free(pointer);
}
static void p_domain(size_t token)
{
    assert(token && p1272.domains < p1272.success);
    if (p1272.domains)
        assert(token != p1272.h[0].domain);
    p1272.h[p1272.domains++].domain = token;
}
static void p_initialize(const nl_node *n, size_t domain)
{
    const size_t i = p_index(n);
    assert(domain == p1272.h[i].domain && !n->f0.tag && !n->f0.ptr &&
           n->f1 == P1272_BYTE);
    if (i == 1)
        assert(p1272.scalar == &p1272.h[0].pointer->f1 && p1272.scalar_closed &&
               p1272.copies == 3 && n->f1 == *p1272.scalar);
}
static void p_field(const nl_node *root, const void *field, unsigned write,
                    size_t index, size_t scope)
{
    assert(p_index(root) == 0 && !p1272.h[0].ended);
    if (index == 1) {
        assert(!write && !p1272.scalar && field == &root->f1 &&
               root->f1 == P1272_BYTE);
        p1272.scalar = field;
        p1272.scalar_scope = scope;
    } else {
        assert(index == 0 && field == &root->f0);
    }
}
static void p_binding(size_t symbol, size_t kind, const void *address)
{
    for (size_t i = 0; i < 3; ++i)
        if (symbol == p_symbols[i]) {
            assert(kind == 4 && p1272.scalar && p1272.scalar_closed &&
                   p1272.copies == i &&
                   *(const uint8_t *)address == *p1272.scalar &&
                   *(const uint8_t *)address == P1272_BYTE);
            ++p1272.copies;
        }
}
static void p_scope_end(size_t scope)
{
    if (scope == p1272.scalar_scope) {
        assert(p1272.scalar && !p1272.scalar_closed);
        p1272.scalar_closed = true;
    }
}
static void p_change(const nl_node_option *field, const nl_node_option *old)
{
    assert(p1272.success == 2 && field == &p1272.h[0].pointer->f0 &&
           p1272.h[0].pointer->f1 == P1272_BYTE &&
           p1272.h[1].pointer->f1 == P1272_BYTE);
    if (!p1272.changes)
        assert(old->tag == 0 && !old->ptr && field->tag == 1 &&
               field->ptr == p1272.h[1].pointer);
    else
        assert(p1272.changes == 1 && old->tag == 1 &&
               old->ptr == p1272.h[1].pointer && !field->tag && !field->ptr);
    ++p1272.changes;
}
static void p_tail(const nl_node *n, const void *allocation, size_t domain)
{
    assert(p_index(n) == 1 && allocation == n && domain == p1272.h[1].domain &&
           !p1272.h[1].ended);
}
static void p_end(const nl_node *n, size_t domain)
{
    size_t i = p_index(n);
    assert(!p1272.h[i].ended && domain == p1272.h[i].domain &&
           n->f1 == P1272_BYTE && !n->f0.tag && !n->f0.ptr);
    p1272.h[i].ended = true;
}
static void p_raw(const void *p, size_t n)
{
    size_t i = p_index(p);
    assert(n == sizeof(nl_node) && p1272.h[i].ended && !p1272.h[i].raw);
    p1272.h[i].raw = true;
}
static void p_finalize(size_t domain)
{
    for (size_t i = 0; i < p1272.success; ++i)
        if (p1272.h[i].domain == domain) {
            assert(p1272.h[i].ended && p1272.h[i].raw && !p1272.h[i].final);
            p1272.h[i].final = true;
            ++p1272.finalized;
            return;
        }
    assert(0);
}
static void p_release(const void *a, const void *raw, size_t n)
{
    size_t i = p_index(a);
    assert(a == raw && n == sizeof(nl_node) && p1272.h[i].raw &&
           p1272.h[i].final && !p1272.h[i].released);
    p1272.h[i].released = true;
}
static void p_finish(void)
{
    assert(p1272.success == p1272.quota && p1272.frees == p1272.success &&
           p1272.domains == p1272.success && p1272.finalized == p1272.success &&
           p1272.attempts == (p1272.quota ? 2u : 1u) &&
           p1272.changes == (p1272.quota == 2 ? 2u : 0u) &&
           p1272.copies == (p1272.quota == 2 ? 3u : 0u));
    for (size_t i = 0; i < p1272.success; ++i)
        assert(!p1272.h[i].pointer && p1272.h[i].ended && p1272.h[i].released);
    p1272.finished = true;
}
#define NL_NODE_BIND(f, s, k, p) p_binding(s, k, p)
#define NL_NODE_REPLACE(...) ((void)0)
#define NL_NODE_ARM(...) ((void)0)
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
#define malloc p_allocate
#define free p_free
#endif
