/* Passive: read guest state only while its original C objects are live.
 * Cached pointers are never dereferenced by finish() after lexical exit. */
#include <stdbool.h>
#include <stdio.h>
#define VERIFY(x)                                                              \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "P990 observer:%d\n", __LINE__);                   \
            abort();                                                           \
        }                                                                      \
    } while (0)
size_t test_malloc_calls(void);
size_t test_free_calls(void);
uintptr_t test_allocated_address(size_t);
static uintptr_t allocated[2];
static const nl_node *nodes[2];
static size_t domains[2], trials, initialized, frees, changes, active_scope;
static size_t sent, entered, returned, received, local_owner, terminals;
static size_t caller_reads, callee_reads;
static size_t head_reads, before_reads, head_copies;
static bool in_receiver;
static uint8_t *t[2][2];
static unsigned initial[2][2], observed[2][2], copied_old;
static size_t troots, stores, calls;
static size_t heap_index(const void *p)
{
    for (size_t i = 0; i < 2; ++i)
        if (p && (uintptr_t)p == allocated[i])
            return i;
    abort();
}
static void bind_copy(size_t frame, size_t symbol, uint8_t *a, uint8_t *b)
{
    (void)frame;
    (void)symbol;
    if (POST)
        VERIFY(initialized == 2 && frees == 2 && test_free_calls() == 2 &&
               terminals == 1 && returned == 1 && received == 1 &&
               !in_receiver && !active_scope);
    else
        VERIFY(!trials && !initialized && !frees && !sent && !active_scope);
    VERIFY(troots < 2 && a != b);
    size_t r = troots++;
    t[r][0] = a;
    t[r][1] = b;
    initial[r][0] = *a;
    initial[r][1] = *b;
    if (r)
        VERIFY(a != t[0][0] && a != t[0][1] && b != t[0][0] && b != t[0][1]);
    static const unsigned expect[2][2] = {{T00, T01}, {T10, T11}};
    VERIFY(*a == expect[r][0] && *b == expect[r][1]);
}
static void store_copy(uint8_t *p, uint8_t old, uint8_t next, bool call)
{
    VERIFY(troots == 2 && stores < 2);
    VERIFY(POST ? (frees == 2 && terminals == 1) : trials == 0);
    size_t r = stores ? 1 - CALL_ROOT : CALL_ROOT;
    size_t index = stores ? I1 : I0;
    VERIFY(p == t[r][index] && old == initial[r][index] && *p == next);
    VERIFY(*t[r][1 - index] == initial[r][1 - index]);
    if (!stores) {
        VERIFY(call && next == NEXT);
        copied_old = old;
    } else {
        VERIFY(!call && next == copied_old);
    }
    ++stores;
    calls += call;
    if (stores == 2)
        for (size_t j = 0; j < 2; ++j)
            for (size_t k = 0; k < 2; ++k)
                observed[j][k] = *t[j][k];
}
static void trial(void *p, size_t n, size_t align)
{
    VERIFY(POST ? !troots : (troots == 2 && stores == 2 && calls == 1));
    VERIFY(trials < 2 && n == 24 && align == 8 &&
           test_malloc_calls() == trials + 1);
    allocated[trials] = (uintptr_t)p;
    VERIFY((uintptr_t)p == test_allocated_address(trials));
    ++trials;
}
static void initialize(const nl_node *p, size_t d)
{
    size_t i = heap_index(p);
    VERIFY(i == initialized++ && p->f0.tag == 0 && !p->f0.ptr &&
           p->f1 == (i ? HV1 : HV0));
    nodes[i] = p;
    domains[i] = d;
    if (i)
        VERIFY(nodes[0] != p && domains[0] != d);
}
static void root_ref(const nl_node *p, size_t d, unsigned w, size_t root,
                     size_t inc, size_t scope)
{
    size_t i = heap_index(p);
    VERIFY(nodes[i] == p && domains[i] == d && root && inc && scope &&
           !active_scope);
    if (w)
        VERIFY(i == 0 && !in_receiver && changes < 2);
    else if (HEAD_READ && i == 0 && changes == 1 && !sent) {
        VERIFY(head_reads++ == 0);
    } else if (HEAD_READ && i == 1 && changes == 1 && !sent) {
        VERIFY(before_reads++ == 0 && head_copies == 1);
    } else {
        VERIFY(i == 1 && changes == 2 && received == 1);
        if (in_receiver)
            ++callee_reads;
        else
            ++caller_reads;
    }
    active_scope = scope;
}
static void scope_end(size_t s)
{
    VERIFY(s == active_scope && s);
    active_scope = 0;
}
static void change(const nl_node_option *p, const nl_node_option *old,
                   size_t step)
{
    VERIFY(p == &nodes[0]->f0 && step == changes + 1 && active_scope);
    if (!changes)
        VERIFY(!old->tag && !old->ptr && p->tag == 1 && p->ptr == nodes[1]);
    else
        VERIFY(changes == 1 && old->tag == 1 && old->ptr == nodes[1] &&
               !p->tag && !p->ptr);
    ++changes;
}
static void packet(nl_live_tail p)
{
    VERIFY(p.ptr == nodes[1] && p.allocation.handle == (void *)nodes[1] &&
           p.domain.token == domains[1]);
}
static void send_owner(const nl_node *p, void *a, size_t d,
                       const nl_allocation *old_a, const nl_domain *old_d)
{
    VERIFY(sent++ == 0 && p == nodes[1] && a == (void *)p && d == domains[1] &&
           !old_a->handle && !old_d->token);
}
static void enter_owner(nl_node_option *p, const nl_node *q, void *a, size_t d)
{
    VERIFY(entered++ == 0 && p == &nodes[0]->f0 && p->ptr == q &&
           q == nodes[1] && a == (void *)q && d == domains[1]);
}
#if H_LOCAL
static void local_packet(const nl_live_tail *p)
{
    VERIFY(local_owner++ == 0);
    packet(*p);
}
#endif
#if HEAD_READ
static void copy_link(const nl_node_option *p, const nl_node_option *value)
{
    VERIFY(p == &nodes[0]->f0 && changes == 1 && active_scope && p->tag == 1 &&
           value->tag == 1 && p->ptr == nodes[1] && value->ptr == nodes[1] &&
           head_copies++ == 0);
}
#endif
static void return_owner(nl_live_tail p)
{
    VERIFY(returned++ == 0 && changes == 2);
    packet(p);
}
static void receive_owner(nl_live_tail p, const nl_allocation *a,
                          const nl_domain *d)
{
    VERIFY(received++ == 0 && !a->handle && !d->token);
    packet(p);
}
static void handoff(const nl_node *p, void *a, size_t d,
                    const nl_allocation *old_a, const nl_domain *old_d)
{
    VERIFY((POST ? (!troots && !stores && !calls)
                 : (troots == 2 && stores == 2 && calls == 1)) &&
           !active_scope && p == nodes[1] && a == (void *)p &&
           d == domains[1] && !old_a->handle && !old_d->token);
}
static void receiver(const nl_node *p, void *a, size_t d)
{
    VERIFY(terminals++ == 0 && !in_receiver && p == nodes[1] &&
           a == (void *)p && d == domains[1]);
    in_receiver = true;
}
static void receiver_exit(void)
{
    VERIFY(in_receiver && callee_reads == 1);
    in_receiver = false;
}
static void end_root(const nl_node *p, size_t d)
{
    size_t i = heap_index(p);
    VERIFY(d == domains[i] && !active_scope && !p->f0.tag && !p->f0.ptr &&
           p->f1 == (i ? HV1 : HV0));
    if (allocated[1])
        VERIFY(changes == 2 &&
               (i ? in_receiver : (!in_receiver && frees == 1)));
}
void test_observe_free(uintptr_t p)
{
    size_t i = heap_index((void *)p);
    VERIFY(i ? (in_receiver && frees == 0)
             : (!in_receiver && (allocated[1] ? frees == 1 : frees == 0)));
    ++frees;
}
static void finish(void)
{
    size_t successes = !!allocated[0] + !!allocated[1];
    VERIFY(frees == successes && frees == test_free_calls() && !active_scope);
    if (successes == 2)
        VERIFY(sent == 1 && entered == 1 && returned == 1 && received == 1 &&
               local_owner == H_LOCAL && terminals == 1 && callee_reads == 1 &&
               caller_reads == CALLER_READ && head_reads == HEAD_READ &&
               before_reads == HEAD_READ && head_copies == HEAD_READ);
    else
        VERIFY(!sent && !entered && !returned && !received && !local_owner &&
               !terminals && !callee_reads && !caller_reads && !changes &&
               !head_reads && !before_reads && !head_copies);
    const bool constructed = !POST || successes == 2;
    VERIFY(troots == (constructed ? 2u : 0u) &&
           stores == (constructed ? 2u : 0u) &&
           calls == (constructed ? 1u : 0u));
    printf("P990 mixed physical original=PASS T=%u,%u/%u,%u old=%u "
           "calls=%zu "
           "allocations=%zu frees=%zu caller_read=%zu callee_read=%zu "
           "head_read=%zu phase_post=%d\n",
           observed[0][0], observed[0][1], observed[1][0], observed[1][1],
           copied_old, calls, successes, frees, caller_reads, callee_reads,
           head_reads, POST);
}
#define NL_MIXED_COPY_BIND(f, s, a, b) bind_copy(f, s, a, b)
#define NL_MIXED_COPY_CALL(p, o, n) store_copy(p, o, n, true)
#define NL_MIXED_COPY_STORE(p, o, n) store_copy(p, o, n, false)
#if H_LOCAL
#define NL_MIXED_OWNER_LOCAL(p) local_packet(p)
#else
#define NL_MIXED_OWNER_LOCAL(p) ((void)0)
#endif
#define NL_HEAP_TRIAL(p, n, a) trial(p, n, a)
#define NL_HEAP_INITIALIZE(p, d) initialize(p, d)
#define NL_HEAP_ROOT(p, d, w, r, i, s) root_ref(p, d, w, r, i, s)
#define NL_HEAP_SCOPE_END(s) scope_end(s)
#define NL_HEAP_CHANGE(p, o, s) change(p, o, s)
#define NL_LIVE_SEND(p, a, d, ap, dp) send_owner(p, a, d, ap, dp)
#define NL_LIVE_ENTER(e, p, a, d) enter_owner(e, p, a, d)
#define NL_LIVE_RETURN(p) return_owner(p)
#define NL_LIVE_RECEIVE(p, a, d) receive_owner(p, a, d)
#define NL_HEAP_HANDOFF(p, a, d, ap, dp) handoff(p, a, d, ap, dp)
#define NL_HEAP_RECEIVER_ENTER(p, a, d, f, ps, as, ds, ap, dp) receiver(p, a, d)
#define NL_HEAP_RECEIVER_EXIT() receiver_exit()
#define NL_HEAP_END(p, d) end_root(p, d)
#define NL_HEAP_FINISH() finish()
#define NL_HEAP_ARM(...) ((void)0)
#define NL_HEAP_DOMAIN(...) ((void)0)
#define NL_HEAP_SLOT(...) ((void)0)
#define NL_HEAP_RELOAN(...) ((void)0)
#define NL_HEAP_FIELD(...) ((void)0)
#if HEAD_READ
#define NL_HEAP_COPY(p, v) copy_link(p, v)
#else
#define NL_HEAP_COPY(...) ((void)0)
#endif
#define NL_HEAP_RAW(...) ((void)0)
#define NL_HEAP_FINALIZE(...) ((void)0)
#define NL_HEAP_RELEASE(...) ((void)0)
#define NL_HEAP_RETURNED(...) ((void)0)
