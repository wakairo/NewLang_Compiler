/* Independent read-only oracle; no allocation, mutation, repair or cleanup.
 * All physical field reads precede EndRoot. NDEBUG cannot disable VERIFY.
 * EXPECT_* values/identities are extracted by a separate checked-only probe. */
#include <stdio.h>
#include <stdlib.h>
#define VERIFY(x)                                                              \
    do {                                                                       \
        if (!(x))                                                              \
            abort();                                                           \
    } while (0)
size_t test_malloc_calls(void);
size_t test_free_calls(void);
uintptr_t test_allocated_address(void);
static uintptr_t allocation_address;
static uintptr_t lexical_address;
static const nl_node *head, *tail;
static size_t phase, trials, arms, roots, fields, changes, copies, active_scope,
    scopes_ended, domain_token, selected, tail_reads;
static void observe_trial(void *p, size_t n, size_t a)
{
    VERIFY(trials++ == 0 && phase == 0 && n == 24 && a == 8);
    VERIFY(test_malloc_calls() == 1);
    allocation_address = (uintptr_t)p;
    VERIFY(allocation_address == test_allocated_address());
    phase = p ? 1 : 99;
}
static void observe_arm(size_t v)
{
    VERIFY(arms++ == 0 && v == (allocation_address ? 2u : 1u));
}
static void observe_slot(const void *p, size_t n)
{
    VERIFY(phase == 1 && (uintptr_t)p == allocation_address && n == 24);
    phase = 2;
}
static void observe_domain(size_t d)
{
    VERIFY(phase == 2 && d != 0 && d == EXPECT_DOMAIN);
    domain_token = d;
    phase = 3;
}
static void observe_initialize(const nl_node *p, size_t d)
{
    VERIFY(phase == 3 && (uintptr_t)p == allocation_address &&
           d == domain_token);
    VERIFY(p->f0.tag == 0 && p->f0.ptr == NULL && p->f1 == EXPECT_HEAP);
    head = p;
    phase = 4;
}
static void observe_bind(size_t frame, size_t symbol, size_t kind,
                         const void *v)
{
    (void)frame;
    (void)symbol;
    if (kind == 1) {
        const nl_node *p = v;
        VERIFY(phase == 4 && tail == NULL && p != head);
        VERIFY(p->f1 == EXPECT_LEXICAL && p->f0.tag == 0 && p->f0.ptr == NULL);
        tail = p;
        lexical_address = (uintptr_t)p;
    } else if (kind == 2) {
        const nl_node *const *p = v;
        VERIFY(phase == 4 && (*p == head || *p == tail));
    } else if (kind == 3) {
        const nl_node_option *p = v;
        VERIFY(active_scope == 0);
        if (++copies == 1)
            VERIFY(phase == 5 && p->tag == 0 && p->ptr == NULL);
        else {
            VERIFY(copies <= 3 && phase == (copies == 2 ? 6u : 8u));
            VERIFY(p->tag == 1 && p->ptr == tail && p != &head->f0);
        }
    }
}
static void observe_root(const nl_node *p, size_t d, unsigned write,
                         size_t place, size_t incarnation, size_t scope)
{
    VERIFY(roots < 3 && active_scope == 0 && p == head && d == domain_token);
    VERIFY(phase == (roots == 0 ? 4u : roots == 1 ? 5u : 7u));
    VERIFY(write == (roots != 1) && scope != 0);
    VERIFY(place == EXPECT_ROOT && incarnation == EXPECT_ROOT_INC);
    VERIFY(p->f1 == EXPECT_HEAP && tail->f1 == EXPECT_LEXICAL);
    active_scope = scope;
    ++roots;
}
static void observe_field(const nl_node *p, const nl_node_option *f,
                          unsigned write, size_t nominal, size_t index,
                          size_t child, size_t incarnation, size_t scope)
{
    VERIFY(fields < 3 && p == head && f == &head->f0 && scope == active_scope);
    VERIFY(write == (fields != 1) && nominal == EXPECT_NOMINAL && index == 0);
    VERIFY(child == EXPECT_CHILD && incarnation == EXPECT_CHILD_INC);
    ++fields;
}
static void observe_change(const nl_node_option *f, const nl_node_option *old,
                           size_t step)
{
    VERIFY(f == &head->f0 && old != f && active_scope != 0);
    VERIFY(head->f1 == EXPECT_HEAP && tail->f1 == EXPECT_LEXICAL);
    if (changes++ == 0) {
        VERIFY(phase == 4 && step == 1 && old->tag == 0 && old->ptr == NULL);
        VERIFY(f->tag == 1 && f->ptr == tail);
        phase = 5;
    } else {
        VERIFY(changes == 2 && phase == 7 && step == 2 && old->tag == 1 &&
               old->ptr == tail);
        VERIFY(f->tag == 0 && f->ptr == NULL);
        phase = 8;
    }
}
static void observe_copy(const nl_node_option *f, const nl_node_option *copy)
{
    VERIFY(phase == 5 && fields == 2 && active_scope != 0);
    VERIFY(f == &head->f0 && copy != f && f->tag == 1 && f->ptr == tail);
    VERIFY(copy->tag == f->tag && copy->ptr == f->ptr);
    phase = 6;
}
static void observe_scope_end(size_t scope)
{
    VERIFY(scope == active_scope && active_scope != 0 && scopes_ended < 3);
    active_scope = 0;
    ++scopes_ended;
}
static void observe_node_arm(size_t v, const nl_node *p)
{
    VERIFY(phase == 6 && copies == 2 && v == 2 && p == tail);
    VERIFY(selected++ == 0);
}
static void observe_tail(const nl_node *p)
{
    VERIFY(phase == 6 && selected == 1 && active_scope == 0 && p == tail);
    VERIFY(p->f1 == EXPECT_LEXICAL && p->f0.tag == 0 && p->f0.ptr == NULL);
    ++tail_reads;
    phase = 7;
}
static void observe_end(const nl_node *p, size_t d)
{
    VERIFY(phase == 8 && active_scope == 0 && scopes_ended == 3);
    VERIFY(roots == 3 && fields == 3 && changes == 2 && copies == 3 &&
           tail_reads == 1);
    VERIFY(p == head && d == domain_token && p->f0.tag == 0 &&
           p->f0.ptr == NULL);
    phase = 9; /* no further Node reads */
}
static void observe_raw(const void *p, size_t n)
{
    VERIFY(phase == 9 && (uintptr_t)p == allocation_address && n == 24);
    phase = 10;
}
static void observe_finalize(size_t d)
{
    VERIFY(phase == 10 && d == domain_token);
    phase = 11;
}
static void observe_release(const void *a, const void *r, size_t n)
{
    VERIFY(phase == 11 && (uintptr_t)a == allocation_address &&
           (uintptr_t)r == allocation_address && n == 24);
    phase = 12;
}
void test_observe_free(uintptr_t p)
{
    VERIFY(phase == 12 && p == allocation_address);
    phase = 13;
}
static void observe_finish(void)
{
    VERIFY(trials == 1 && arms == 1 && test_malloc_calls() == 1);
    if (allocation_address)
        VERIFY(phase == 13 && test_free_calls() == 1);
    else
        VERIFY(phase == 99 && roots == 0 && fields == 0 && changes == 0 &&
               copies == 0 && domain_token == 0 && test_free_calls() == 0);
    /* Integer observations captured while roots were live; no reads after end.
     */
    printf("OBSERVED heap=%zx lexical=%zx roots=%zu fields=%zu changes=%zu "
           "copies=%zu scopes=%zu free=%zu\n",
           (size_t)allocation_address, (size_t)lexical_address, roots, fields,
           changes, copies, scopes_ended, test_free_calls());
}
#undef NL_NODE_BIND
#undef NL_NODE_ARM
#undef NL_NODE_RELOAN
#define NL_NODE_BIND(f, s, k, v) observe_bind(f, s, k, v)
#define NL_NODE_ARM(v, p) observe_node_arm(v, p)
#define NL_NODE_RELOAN(p) observe_tail(p)
#define NL_HEAP_TRIAL(p, n, a) observe_trial(p, n, a)
#define NL_HEAP_ARM(v) observe_arm(v)
#define NL_HEAP_SLOT(p, n) observe_slot(p, n)
#define NL_HEAP_DOMAIN(d) observe_domain(d)
#define NL_HEAP_INITIALIZE(p, d) observe_initialize(p, d)
#define NL_HEAP_ROOT(p, d, w, r, i, s) observe_root(p, d, w, r, i, s)
#define NL_HEAP_FIELD(p, f, w, n, x, c, i, s)                                  \
    observe_field(p, f, w, n, x, c, i, s)
#define NL_HEAP_CHANGE(f, o, s) observe_change(f, o, s)
#define NL_HEAP_COPY(f, c) observe_copy(f, c)
#define NL_HEAP_SCOPE_END(s) observe_scope_end(s)
#define NL_HEAP_END(p, d) observe_end(p, d)
#define NL_HEAP_RAW(p, n) observe_raw(p, n)
#define NL_HEAP_FINALIZE(d) observe_finalize(d)
#define NL_HEAP_RELEASE(a, r, n) observe_release(a, r, n)
#define NL_HEAP_FINISH() observe_finish()
