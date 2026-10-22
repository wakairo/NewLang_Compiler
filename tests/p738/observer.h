/* Passive P738 no-pre-read observer. Existing lifecycle/receiver primitives
 * are retained; explicit no-read roles replace the old read/match phases.
 * No guest writes, synthetic ownership, preseeded values or allocation repair.
 */
/* Independent read-only observer. Only bookkeeping is mutable; H bytes are
 * read while live, never initialized, linked, repaired or freed here.
 * Expectations are supplied by the separate checked-only probe, not emitter.
 * VERIFY is deliberately active under NDEBUG. No dynamic allocations. */
#include <stdbool.h>
#include <stdio.h>
#define VERIFY(x)                                                              \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "OBSERVER_REJECT %s:%d\n", __FILE__, __LINE__);    \
            abort();                                                           \
        }                                                                      \
    } while (0)
size_t test_malloc_calls(void);
size_t test_free_calls(void);
uintptr_t test_allocated_address(size_t);
static const size_t domains[2] = {EXPECT_HD, EXPECT_TD};
static const size_t places[2] = {EXPECT_HP, EXPECT_TP};
static const size_t incarnations[2] = {EXPECT_HI, EXPECT_TI};
static const unsigned payloads[2] = {EXPECT_HV, EXPECT_TV};
#ifndef EXPECT_RETURN_READ
#define EXPECT_RETURN_READ 0
#endif
#ifndef EXPECT_S5
#define EXPECT_S5 0
#endif
static const size_t scopes[6] = {EXPECT_S0, EXPECT_S1, EXPECT_S2,
                                 EXPECT_S3, EXPECT_S4, EXPECT_S5};
static bool in_receiver, pending_handoff;
static size_t handoffs, receiver_entries, receiver_exits, returns,
    receiver_frees, donor_frees;
static const nl_allocation *donor_allocation;
static const nl_domain *donor_domain;
static uintptr_t addresses[2];
static const nl_node *nodes[2];
static unsigned phases[2];
static size_t trials, arms, selected_region, link_phase, roots, fields, changes,
    copies, copy_bindings, active_scope, scopes_ended, selected;
static inline unsigned region_of(const void *p)
{
    VERIFY(p != NULL);
    for (unsigned i = 0; i < 2; ++i)
        if ((uintptr_t)p == addresses[i])
            return i;
    abort();
}
static inline void siblings(void)
{
    VERIFY(phases[0] == 4 && phases[1] == 4 && addresses[0] != addresses[1]);
    VERIFY(nodes[0]->f1 == payloads[0] && nodes[1]->f1 == payloads[1]);
    VERIFY(nodes[1]->f0.tag == 0 && nodes[1]->f0.ptr == NULL);
}
static inline void trial(void *p, size_t n, size_t a)
{
    VERIFY(trials < 2 && test_malloc_calls() == trials + 1 && n == 24 &&
           a == 8);
    if (trials == 1)
        VERIFY(phases[0] == 4 && phases[1] == 0);
    selected_region = trials;
    addresses[trials] = (uintptr_t)p;
    VERIFY(addresses[trials] == test_allocated_address(trials));
    phases[trials++] = p ? 1u : 99u;
}
static inline void arm(size_t v)
{
    VERIFY(arms++ < 2 && v == (addresses[selected_region] ? 2u : 1u));
}
static inline void slot(const void *p, size_t n)
{
    unsigned i = region_of(p);
    VERIFY(phases[i] == 1 && n == 24);
    selected_region = i;
    phases[i] = 2;
}
static inline void domain(size_t d)
{
    unsigned i = (unsigned)selected_region;
    VERIFY(phases[i] == 2 && d == domains[i] && d != 0);
    VERIFY(domains[0] != domains[1]);
    phases[i] = 3;
}
static inline void initialize(const nl_node *p, size_t d)
{
    unsigned i = region_of(p);
    VERIFY(phases[i] == 3 && d == domains[i]);
    VERIFY(p->f0.tag == 0 && p->f0.ptr == NULL && p->f1 == payloads[i]);
    nodes[i] = p;
    phases[i] = 4;
    if (i == 1)
        siblings();
}
static inline void binding(size_t frame, size_t symbol, size_t kind,
                           const void *p)
{
    (void)frame;
    (void)symbol;
    VERIFY(kind != 1);
    if (kind == 2) {
        const nl_node *const *v = p;
        VERIFY(*v == nodes[0] || *v == nodes[1]);
    }
    if (kind == 3) {
        const nl_node_option *v = p;
        VERIFY(active_scope == 0 && ++copy_bindings == 1 && p != &nodes[0]->f0);
        VERIFY(link_phase == 1 && v->tag == 0 && v->ptr == NULL);
    }
}
static inline void root_ref(const nl_node *p, size_t d, unsigned write,
                            size_t r, size_t inc, size_t scope)
{
    siblings();
    unsigned i = region_of(p);
    const size_t expected_roots = 2 + EXPECT_RETURN_READ + EXPECT_RECEIVER_READ;
    VERIFY(roots < expected_roots && active_scope == 0 && scope != 0 &&
           scope == scopes[roots]);
    VERIFY(i == (roots < 2 ? 0u : 1u) && d == domains[i] && r == places[i] &&
           inc == incarnations[i]);
    VERIFY(write == (roots < 2));
    VERIFY(link_phase == (roots == 0 ? 0u : roots == 1 ? 1u : 2u));
    VERIFY(in_receiver == (roots == 2 + EXPECT_RETURN_READ));
    active_scope = scope;
    ++roots;
}
static inline void field_ref(const nl_node *p, const nl_node_option *f,
                             unsigned w, size_t n, size_t x, size_t c,
                             size_t inc, size_t scope)
{
    siblings();
    VERIFY(fields < 2 && p == nodes[0] && f == &nodes[0]->f0 && w == 1);
    VERIFY(n == EXPECT_NOMINAL && x == 0 && c == EXPECT_CHILD &&
           inc == EXPECT_CI);
    VERIFY(scope == active_scope && scope != 0);
    ++fields;
}
static inline void change(const nl_node_option *f, const nl_node_option *old,
                          size_t step)
{
    siblings();
    VERIFY(f == &nodes[0]->f0 && old != f && active_scope != 0);
    if (changes++ == 0) {
        VERIFY(link_phase == 0 && step == 1 && old->tag == 0 &&
               old->ptr == NULL);
        VERIFY(f->tag == 1 && f->ptr == nodes[1]);
        link_phase = 1;
    } else {
        VERIFY(changes == 2 && link_phase == 1 && step == 2 && old->tag == 1 &&
               old->ptr == nodes[1]);
        VERIFY(f->tag == 0 && f->ptr == NULL);
        link_phase = 2;
    }
}
static inline void copy(const nl_node_option *f, const nl_node_option *v)
{
    (void)f;
    (void)v;
    VERIFY(false); /* no implicit source link read */
}
static inline void node_arm(size_t v, const nl_node *p)
{
    (void)v;
    (void)p;
    VERIFY(false); /* no fabricated pre-detach match */
}
static inline void scope_end(size_t scope)
{
    VERIFY(active_scope != 0 && scope == active_scope &&
           scopes_ended < 2 + EXPECT_RETURN_READ + EXPECT_RECEIVER_READ);
    active_scope = 0;
    ++scopes_ended;
}
static inline void end(const nl_node *p, size_t d)
{
    unsigned i = region_of(p);
    VERIFY(phases[i] == 4 && active_scope == 0 && d == domains[i]);
    VERIFY(p->f0.tag == 0 && p->f0.ptr == NULL && p->f1 == payloads[i]);
    if (addresses[1]) {
        VERIFY(link_phase == 2 &&
               scopes_ended == 2 + EXPECT_RETURN_READ + EXPECT_RECEIVER_READ &&
               roots == 2 + EXPECT_RETURN_READ + EXPECT_RECEIVER_READ &&
               fields == 2 && changes == 2 && copy_bindings == 1);
        VERIFY(i == 1 ? (phases[0] == 4 && in_receiver && receiver_entries == 1)
                      : (phases[1] == 9 && !in_receiver && returns == 1));
    } else
        VERIFY(i == 0 && phases[1] == 99 && link_phase == 0);
    phases[i] = 5; /* after this callback no reads of this object */
}
static inline void raw(const void *p, size_t n)
{
    unsigned i = region_of(p);
    VERIFY(phases[i] == 5 && n == 24);
    selected_region = i;
    phases[i] = 6;
}
static inline void finalize(size_t d)
{
    unsigned i = (unsigned)selected_region;
    VERIFY(phases[i] == 6 && d == domains[i]);
    phases[i] = 7;
}
static inline void release(const void *p, const void *raw_ptr, size_t n)
{
    unsigned i = region_of(p);
    VERIFY(phases[i] == 7 && (uintptr_t)raw_ptr == addresses[i] && n == 24);
    phases[i] = 8;
}
void test_observe_free(uintptr_t p)
{
    unsigned i = region_of((const void *)p);
    VERIFY(phases[i] == 8 && (i == 1 ? in_receiver : !in_receiver));
    if (in_receiver)
        ++receiver_frees;
    else
        ++donor_frees;
    phases[i] = 9;
}
static inline void handoff(const nl_node *p, const void *allocation, size_t d,
                           const nl_allocation *a, const nl_domain *life)
{
    siblings();
    VERIFY(handoffs++ == 0 && !in_receiver && !pending_handoff &&
           active_scope == 0);
    VERIFY(link_phase == 2 && nodes[0]->f0.tag == 0 &&
           nodes[0]->f0.ptr == NULL);
    VERIFY(p == nodes[1] && (uintptr_t)allocation == addresses[1] &&
           d == domains[1]);
    VERIFY(a->handle == NULL && life->token == 0);
    donor_allocation = a;
    donor_domain = life;
    pending_handoff = true;
}
static inline void receiver_enter(const nl_node *p, const void *allocation,
                                  size_t d, size_t frame, size_t ps, size_t as,
                                  size_t ds, const nl_allocation *a,
                                  const nl_domain *life)
{
    siblings();
    VERIFY(pending_handoff && !in_receiver && receiver_entries++ == 0 &&
           frame != 0);
    VERIFY(p == nodes[1] && (uintptr_t)allocation == addresses[1] &&
           d == domains[1]);
    VERIFY(ps == EXPECT_PP && as == EXPECT_PA && ds == EXPECT_PD);
    VERIFY(a != donor_allocation && life != donor_domain &&
           a->handle == allocation && life->token == d &&
           (const void *)a != (const void *)p);
    VERIFY(donor_allocation->handle == NULL && donor_domain->token == 0);
    in_receiver = true;
}
static inline void receiver_exit(void)
{
    VERIFY(in_receiver && receiver_exits++ == 0 && phases[1] == 9 &&
           phases[0] == 4 && active_scope == 0);
    VERIFY(receiver_frees == 1 && donor_frees == 0);
    in_receiver = false;
}
static inline void returned(const nl_allocation *a, const nl_domain *life)
{
    VERIFY(!in_receiver && pending_handoff && returns++ == 0 &&
           receiver_exits == 1);
    VERIFY(a == donor_allocation && life == donor_domain && a->handle == NULL &&
           life->token == 0);
    VERIFY(phases[1] == 9 && phases[0] == 4);
    pending_handoff = false;
}
static inline void finish(void)
{
    VERIFY(trials == arms && trials == test_malloc_calls());
    size_t frees = (addresses[0] != 0) + (addresses[1] != 0);
    VERIFY(test_free_calls() == frees);
    VERIFY(phases[0] == (addresses[0] ? 9u : 99u));
    if (!addresses[0])
        VERIFY(trials == 1 && phases[1] == 0 && roots == 0);
    else
        VERIFY(trials == 2 && phases[1] == (addresses[1] ? 9u : 99u));
    if (!addresses[1])
        VERIFY(fields == 0 && changes == 0 && copies == 0 && selected == 0);
    VERIFY(!in_receiver && !pending_handoff);
    VERIFY(handoffs == (addresses[1] != 0) && receiver_entries == handoffs &&
           receiver_exits == handoffs && returns == handoffs);
    VERIFY(receiver_frees == (addresses[1] != 0) &&
           donor_frees == (addresses[0] != 0));
    printf("OBSERVED head=%zx tail=%zx trials=%zu free=%zu roots=%zu "
           "changes=%zu copy=%zu scopes=%zu receiver=%zu receiver_free=%zu "
           "donor_free=%zu\n",
           (size_t)addresses[0], (size_t)addresses[1], trials, frees, roots,
           changes, copies, scopes_ended, receiver_entries, receiver_frees,
           donor_frees);
}
#undef NL_NODE_BIND
#undef NL_NODE_ARM
#define NL_NODE_BIND(f, s, k, p) binding(f, s, k, p)
#define NL_NODE_ARM(v, p) node_arm(v, p)
#define NL_HEAP_TRIAL(p, n, a) trial(p, n, a)
#define NL_HEAP_ARM(v) arm(v)
#define NL_HEAP_SLOT(p, n) slot(p, n)
#define NL_HEAP_DOMAIN(d) domain(d)
#define NL_HEAP_INITIALIZE(p, d) initialize(p, d)
#define NL_HEAP_ROOT(p, d, w, r, i, s) root_ref(p, d, w, r, i, s)
#define NL_HEAP_FIELD(p, f, w, n, x, c, i, s) field_ref(p, f, w, n, x, c, i, s)
#define NL_HEAP_CHANGE(f, o, s) change(f, o, s)
#define NL_HEAP_COPY(f, c) copy(f, c)
#define NL_HEAP_SCOPE_END(s) scope_end(s)
#define NL_HEAP_END(p, d) end(p, d)
#define NL_HEAP_RAW(p, n) raw(p, n)
#define NL_HEAP_FINALIZE(d) finalize(d)
#define NL_HEAP_RELEASE(p, r, n) release(p, r, n)
#define NL_HEAP_FINISH() finish()

#define NL_HEAP_HANDOFF(p, a, d, da, dd) handoff(p, a, d, da, dd)
#define NL_HEAP_RECEIVER_ENTER(p, a, d, f, ps, as, ds, ap, dp)                 \
    receiver_enter(p, a, d, f, ps, as, ds, ap, dp)
#define NL_HEAP_RECEIVER_EXIT() receiver_exit()
#define NL_HEAP_RETURNED(a, d) returned(a, d)

/* Independent read-only return observer extends the existing terminal gate.
 * Only observer bookkeeping changes. No heap byte write, free or authority. */

static bool in_producer, pending_return;
static size_t producer_entries, producer_returns, producer_receipts;
static const nl_allocation *return_donor_allocation;
static const nl_domain *return_donor_domain;
static inline void live_send(const nl_node *p, const void *a, size_t d,
                             const nl_allocation *donor_a,
                             const nl_domain *donor_d)
{
    siblings();
    VERIFY(!in_producer && !pending_return && active_scope == scopes[1]);
    VERIFY(p == nodes[1] && (uintptr_t)a == addresses[1] && d == domains[1]);
    VERIFY(nodes[0]->f0.tag == 1 && nodes[0]->f0.ptr == p);
    VERIFY(donor_a->handle == NULL && donor_d->token == 0);
    return_donor_allocation = donor_a;
    return_donor_domain = donor_d;
    pending_return = true;
}
static inline void live_enter(const nl_node_option *head, const nl_node *p,
                              const void *a, size_t d)
{
    siblings();
    VERIFY(pending_return && !in_producer && producer_entries++ == 0);
    VERIFY(head == &nodes[0]->f0 && head->tag == 1 && head->ptr == p);
    VERIFY(p == nodes[1] && (uintptr_t)a == addresses[1] && d == domains[1]);
    VERIFY(active_scope == scopes[1] && link_phase == 1);
    in_producer = true;
}
static inline void live_return(nl_live_tail value)
{
    siblings();
    VERIFY(in_producer && pending_return && producer_returns++ == 0);
    VERIFY(link_phase == 2 && active_scope == scopes[1]);
    VERIFY(nodes[0]->f0.tag == 0 && nodes[0]->f0.ptr == NULL);
    VERIFY(value.ptr == nodes[1] &&
           (uintptr_t)value.allocation.handle == addresses[1] &&
           value.domain.token == domains[1] && test_free_calls() == 0);
    in_producer = false;
}
static inline void live_receive(nl_live_tail value, const nl_allocation *a,
                                const nl_domain *d)
{
    siblings();
    VERIFY(!in_producer && pending_return && producer_returns == 1 &&
           producer_receipts++ == 0 && active_scope == scopes[1]);
    VERIFY(value.ptr == nodes[1] &&
           (uintptr_t)value.allocation.handle == addresses[1] &&
           value.domain.token == domains[1]);
    VERIFY(a == return_donor_allocation && d == return_donor_domain &&
           a->handle == NULL && d->token == 0 && test_free_calls() == 0);
    pending_return = false;
}
static inline void live_finish(void)
{
    VERIFY(!pending_return && !in_producer);
    VERIFY(producer_entries == (addresses[1] != 0) &&
           producer_returns == producer_entries &&
           producer_receipts == producer_entries);
    finish();
    printf("LIVE_RETURN producer=%zu returned=%zu received=%zu "
           "original_tail=%zx\n",
           producer_entries, producer_returns, producer_receipts,
           (size_t)addresses[1]);
}
#define NL_LIVE_SEND(p, a, d, da, dd) live_send(p, a, d, da, dd)
#define NL_LIVE_ENTER(h, p, a, d) live_enter(h, p, a, d)
#define NL_LIVE_RETURN(v) live_return(v)
#define NL_LIVE_RECEIVE(v, a, d) live_receive(v, a, d)
#undef NL_HEAP_FINISH
#define NL_HEAP_FINISH() live_finish()

/* Passive only: zero guest writes/rights; all expected identities are the
 * independently observed original heap nodes, original Allocation and Domain.
 */

static size_t local_bindings, local_moves;
static inline void p738_bound(const nl_live_tail *local,
                              const nl_live_tail *temporary)
{
    siblings();
    VERIFY(in_producer && local_bindings++ == 0 && local_moves == 0);
    VERIFY(local->ptr == nodes[1] &&
           local->allocation.handle == (void *)addresses[1] &&
           local->domain.token == domains[1]);
    VERIFY(temporary->ptr == local->ptr &&
           temporary->allocation.handle == local->allocation.handle &&
           temporary->domain.token == local->domain.token);
    VERIFY(test_free_calls() == 0 && nodes[0]->f0.tag == 0);
}
static inline void p738_moved(const nl_live_tail *local,
                              const nl_live_tail *returned)
{
    siblings();
    VERIFY(in_producer && local_bindings == 1 && local_moves++ == 0);
    VERIFY(local->ptr == NULL && local->allocation.handle == NULL &&
           local->domain.token == 0);
    VERIFY(returned->ptr == nodes[1] &&
           returned->allocation.handle == (void *)addresses[1] &&
           returned->domain.token == domains[1]);
}
static inline void p738_finish(void)
{
    VERIFY(local_bindings == (addresses[1] != 0) &&
           local_moves == local_bindings);
    live_finish();
}
#undef NL_HEAP_FINISH
#define NL_HEAP_FINISH() p738_finish()
#define P738_BOUND(a, b) p738_bound(&(a), &(b))
#define P738_MOVED(a, b) p738_moved(&(a), &(b))
