/* Independent read-only observer. Only bookkeeping is mutable; H bytes are
 * read while live, never initialized, linked, repaired or freed here.
 * Expectations are supplied by the separate checked-only probe, not emitter.
 * VERIFY is deliberately active under NDEBUG. No dynamic allocations. */
#include <stdbool.h>
#include <stdio.h>
#define VERIFY(x)                                                              \
    do {                                                                       \
        if (!(x))                                                              \
            abort();                                                           \
    } while (0)
size_t test_malloc_calls(void);
size_t test_free_calls(void);
uintptr_t test_allocated_address(size_t);
static const size_t domains[2] = {EXPECT_HD, EXPECT_TD};
static const size_t places[2] = {EXPECT_HP, EXPECT_TP};
static const size_t incarnations[2] = {EXPECT_HI, EXPECT_TI};
static const unsigned payloads[2] = {EXPECT_HV, EXPECT_TV};
static const size_t scopes[5] = {EXPECT_S0, EXPECT_S1, EXPECT_S2, EXPECT_S3,
                                 EXPECT_S4};
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
static unsigned region_of(const void *p)
{
    VERIFY(p != NULL);
    for (unsigned i = 0; i < 2; ++i)
        if ((uintptr_t)p == addresses[i])
            return i;
    abort();
}
static void siblings(void)
{
    VERIFY(phases[0] == 4 && phases[1] == 4 && addresses[0] != addresses[1]);
    VERIFY(nodes[0]->f1 == payloads[0] && nodes[1]->f1 == payloads[1]);
    VERIFY(nodes[1]->f0.tag == 0 && nodes[1]->f0.ptr == NULL);
}
static void trial(void *p, size_t n, size_t a)
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
static void arm(size_t v)
{
    VERIFY(arms++ < 2 && v == (addresses[selected_region] ? 2u : 1u));
}
static void slot(const void *p, size_t n)
{
    unsigned i = region_of(p);
    VERIFY(phases[i] == 1 && n == 24);
    selected_region = i;
    phases[i] = 2;
}
static void domain(size_t d)
{
    unsigned i = (unsigned)selected_region;
    VERIFY(phases[i] == 2 && d == domains[i] && d != 0);
    VERIFY(domains[0] != domains[1]);
    phases[i] = 3;
}
static void initialize(const nl_node *p, size_t d)
{
    unsigned i = region_of(p);
    VERIFY(phases[i] == 3 && d == domains[i]);
    VERIFY(p->f0.tag == 0 && p->f0.ptr == NULL && p->f1 == payloads[i]);
    nodes[i] = p;
    phases[i] = 4;
    if (i == 1)
        siblings();
}
static void binding(size_t frame, size_t symbol, size_t kind, const void *p)
{
    (void)frame;
    (void)symbol;
    VERIFY(kind !=
           1); /* no lexical Node standing in for either heap endpoint */
    if (kind == 2) {
        const nl_node *const *v = p;
        VERIFY(*v == nodes[0] || *v == nodes[1]);
    }
    if (kind == 3) {
        const nl_node_option *v = p;
        VERIFY(active_scope == 0 && ++copy_bindings <= 3 && p != &nodes[0]->f0);
        if (copy_bindings == 1)
            VERIFY(link_phase == 1 && v->tag == 0 && v->ptr == NULL);
        else
            VERIFY(link_phase == (copy_bindings == 2 ? 2u : 5u) &&
                   v->tag == 1 && v->ptr == nodes[1]);
    }
}
static void root_ref(const nl_node *p, size_t d, unsigned write, size_t r,
                     size_t inc, size_t scope)
{
    siblings();
    unsigned i = region_of(p);
    if (roots == 4) {
        VERIFY(EXPECT_RECEIVER_READ && in_receiver && receiver_entries == 1 &&
               i == 1 && d == domains[1] && r == places[1] &&
               inc == incarnations[1] && !write && scope == scopes[4] &&
               scope != 0 && active_scope == 0 && link_phase == 5);
        active_scope = scope;
        ++roots;
        return;
    }
    VERIFY(!in_receiver && roots < 4 && i == (roots == 2 ? 1u : 0u));
    VERIFY(d == domains[i] && r == places[i] && inc == incarnations[i]);
    VERIFY(scope == scopes[roots] && active_scope == 0 && scope != 0);
    VERIFY(write == (roots == 0 || roots == 3));
    VERIFY(link_phase == (roots == 0   ? 0u
                          : roots == 1 ? 1u
                          : roots == 2 ? 3u
                                       : 4u));
    if (roots == 2) {
        VERIFY(selected == 1);
        link_phase = 4;
    }
    active_scope = scope;
    ++roots;
}
static void field_ref(const nl_node *p, const nl_node_option *f, unsigned w,
                      size_t n, size_t x, size_t c, size_t inc, size_t scope)
{
    siblings();
    VERIFY(fields < 3 && p == nodes[0] && f == &nodes[0]->f0);
    VERIFY(w == (fields != 1) && n == EXPECT_NOMINAL && x == 0 &&
           c == EXPECT_CHILD && inc == EXPECT_CI);
    VERIFY(scope == active_scope && active_scope != 0);
    ++fields;
}
static void change(const nl_node_option *f, const nl_node_option *old,
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
        VERIFY(changes == 2 && link_phase == 4 && step == 2 && old->tag == 1 &&
               old->ptr == nodes[1]);
        VERIFY(f->tag == 0 && f->ptr == NULL);
        link_phase = 5;
    }
}
static void copy(const nl_node_option *f, const nl_node_option *v)
{
    siblings();
    VERIFY(link_phase == 1 && copies++ == 0 && f == &nodes[0]->f0 && v != f &&
           active_scope != 0);
    VERIFY(f->tag == 1 && f->ptr == nodes[1] && v->tag == f->tag &&
           v->ptr == f->ptr);
    link_phase = 2;
}
static void node_arm(size_t v, const nl_node *p)
{
    siblings();
    VERIFY(v == 2 && p == nodes[1] && link_phase == 2 && selected++ == 0 &&
           active_scope == 0);
    link_phase = 3;
}
static void scope_end(size_t scope)
{
    VERIFY(active_scope != 0 && scope == active_scope &&
           scopes_ended < 4 + EXPECT_RECEIVER_READ);
    active_scope = 0;
    ++scopes_ended;
}
static void end(const nl_node *p, size_t d)
{
    unsigned i = region_of(p);
    VERIFY(phases[i] == 4 && active_scope == 0 && d == domains[i]);
    VERIFY(p->f0.tag == 0 && p->f0.ptr == NULL && p->f1 == payloads[i]);
    if (addresses[1]) {
        VERIFY(link_phase == 5 && scopes_ended == 4 + EXPECT_RECEIVER_READ &&
               roots == 4 + EXPECT_RECEIVER_READ && fields == 3 &&
               changes == 2 && copy_bindings == 3);
        VERIFY(i == 1 ? (phases[0] == 4 && in_receiver && receiver_entries == 1)
                      : (phases[1] == 9 && !in_receiver && returns == 1));
    } else
        VERIFY(i == 0 && phases[1] == 99 && link_phase == 0);
    phases[i] = 5; /* after this callback no reads of this object */
}
static void raw(const void *p, size_t n)
{
    unsigned i = region_of(p);
    VERIFY(phases[i] == 5 && n == 24);
    selected_region = i;
    phases[i] = 6;
}
static void finalize(size_t d)
{
    unsigned i = (unsigned)selected_region;
    VERIFY(phases[i] == 6 && d == domains[i]);
    phases[i] = 7;
}
static void release(const void *p, const void *raw_ptr, size_t n)
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
static void handoff(const nl_node *p, const void *allocation, size_t d,
                    const nl_allocation *a, const nl_domain *life)
{
    siblings();
    VERIFY(handoffs++ == 0 && !in_receiver && !pending_handoff &&
           active_scope == 0);
    VERIFY(link_phase == 5 && nodes[0]->f0.tag == 0 &&
           nodes[0]->f0.ptr == NULL);
    VERIFY(p == nodes[1] && (uintptr_t)allocation == addresses[1] &&
           d == domains[1]);
    VERIFY(a->handle == NULL && life->token == 0);
    donor_allocation = a;
    donor_domain = life;
    pending_handoff = true;
}
static void receiver_enter(const nl_node *p, const void *allocation, size_t d,
                           size_t frame, size_t ps, size_t as, size_t ds,
                           const nl_allocation *a, const nl_domain *life)
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
static void receiver_exit(void)
{
    VERIFY(in_receiver && receiver_exits++ == 0 && phases[1] == 9 &&
           phases[0] == 4 && active_scope == 0);
    VERIFY(receiver_frees == 1 && donor_frees == 0);
    in_receiver = false;
}
static void returned(const nl_allocation *a, const nl_domain *life)
{
    VERIFY(!in_receiver && pending_handoff && returns++ == 0 &&
           receiver_exits == 1);
    VERIFY(a == donor_allocation && life == donor_domain && a->handle == NULL &&
           life->token == 0);
    VERIFY(phases[1] == 9 && phases[0] == 4);
    pending_handoff = false;
}
static void finish(void)
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
