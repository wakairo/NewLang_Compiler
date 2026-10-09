/* Independent, read-only physical observer. Writes only its own trace state.
 * These assertions detect codegen corruption; they never supply language
 * authority, initialize/repair application storage, or release a Node. */
#include <stdbool.h>
#include <stdio.h>
static void verify(bool ok)
{
    if (!ok) {
        fputs("OBSERVER_REJECT custody\n", stderr);
        abort();
    }
}
size_t test_malloc_calls(void);
size_t test_free_calls(void);
uintptr_t test_allocated_address(size_t);
static uintptr_t addresses[2];
static const nl_node *nodes[2];
static unsigned phase[2];
static size_t domains[2], trials, changes, producer_calls, receiver_calls;
static size_t policy, recipient_calls, stored, returned, extracted, none[2];
static size_t scope, old_scope, root_scope, head_reads;
static bool in_producer, in_recipient, in_receiver;
static nl_custody const *sink_address;
static const nl_allocation *donor_a;
static const nl_domain *donor_d;
static size_t index_of(const void *p)
{
    uintptr_t address = (uintptr_t)p;
    verify(address != 0);
    for (size_t i = 0; i < 2; ++i)
        if (addresses[i] == address)
            return i;
    verify(false);
    return 0;
}
static void original(nl_live_tail packet)
{
    verify(phase[0] == 3 && phase[1] == 3 && nodes[0] != nodes[1]);
    verify(packet.ptr == nodes[1] &&
           (uintptr_t)packet.allocation.handle == addresses[1] &&
           packet.domain.token == domains[1] && nodes[1]->f1 == EXPECT_TV);
    verify(nodes[0]->f0.tag == 0 && nodes[0]->f0.ptr == NULL);
}
static void trial(void *p, size_t size, size_t alignment)
{
    verify(trials < 2 && size == 24 && alignment == 8);
    addresses[trials] = (uintptr_t)p;
    verify(addresses[trials] == test_allocated_address(trials));
    phase[trials] = p ? 1u : 0u;
    ++trials;
}
static void slot(const void *p, size_t n)
{
    size_t i = index_of(p);
    verify(phase[i] == 1 && n == 24);
    phase[i] = 2;
}
static void domain(size_t d)
{
    size_t i = trials - 1;
    verify(phase[i] == 2 && domains[i] == 0);
    verify(d == (i ? EXPECT_TD : EXPECT_HD));
    domains[i] = d;
}
static void initialize(const nl_node *p, size_t d)
{
    size_t i = index_of(p);
    verify(phase[i] == 2 && d == domains[i]);
    verify(p->f0.tag == 0 && p->f0.ptr == NULL &&
           p->f1 == (i ? EXPECT_TV : EXPECT_HV));
    nodes[i] = p;
    phase[i] = 3;
}
static void root_ref(const nl_node *p, size_t d, unsigned write, size_t place,
                     size_t incarnation, size_t s)
{
    size_t i = index_of(p);
    verify(phase[i] == 3 && domains[i] == d && s != 0 &&
           place == (i ? EXPECT_TP : EXPECT_HP) &&
           incarnation == (i ? EXPECT_TI : EXPECT_HI));
    verify(write == (i ? 0u : 1u) && root_scope == 0);
    verify(s == (i ? (policy == 1 ? EXPECT_TSA : EXPECT_TSF)
                   : (head_reads++ == 0 ? EXPECT_HS0 : EXPECT_HS1)));
    root_scope = s;
    verify(!i || in_receiver);
}
static void field_ref(const nl_node *p, const nl_node_option *r, unsigned write,
                      size_t nominal, size_t field, size_t child,
                      size_t incarnation, size_t s)
{
    verify(p == nodes[0] && phase[0] == 3 && r == &p->f0 && write == 1 &&
           nominal == EXPECT_NOMINAL && field == 0 && child == EXPECT_CHILD &&
           incarnation == EXPECT_CI && s == root_scope && s != 0);
}
static void change(const nl_node_option *current, const nl_node_option *old,
                   size_t step)
{
    verify(phase[0] == 3 && phase[1] == 3 && current == &nodes[0]->f0);
    verify(step == ++changes);
    if (step == 1)
        verify(old->tag == 0 && old->ptr == NULL && current->tag == 1 &&
               current->ptr == nodes[1]);
    else
        verify(step == 2 && in_producer && old->tag == 1 &&
               old->ptr == nodes[1] && current->tag == 0 &&
               current->ptr == NULL);
}
static void live_send(const nl_node *p, const void *a, size_t d,
                      const nl_allocation *da, const nl_domain *dd)
{
    verify(phase[0] == 3 && phase[1] == 3 && p == nodes[1] &&
           (uintptr_t)a == addresses[1] && d == domains[1] &&
           da->handle == NULL && dd->token == 0 && test_free_calls() == 0);
}
static void live_enter(const nl_node_option *h, const nl_node *p, const void *a,
                       size_t d)
{
    verify(root_scope == EXPECT_HS1 && !in_producer && producer_calls++ == 0 &&
           h == &nodes[0]->f0 && h->tag == 1 && h->ptr == p && p == nodes[1] &&
           (uintptr_t)a == addresses[1] && d == domains[1]);
    in_producer = true;
}
static void live_return(nl_live_tail p)
{
    verify(in_producer && changes == 2);
    original(p);
    in_producer = false;
}
static void live_receive(nl_live_tail p, const nl_allocation *a,
                         const nl_domain *d)
{
    verify(!in_producer && producer_calls == 1 && a->handle == NULL &&
           d->token == 0);
    original(p);
}
static void custody_policy(size_t v)
{
    verify(policy == 0 && v == EXPECT_POLICY && changes == 2);
    policy = v;
}
static void custody_loan(const nl_custody *c, size_t s, unsigned begin)
{
    verify(!in_recipient && s != 0);
    if (begin) {
        verify(scope == 0 && s != old_scope);
        scope = s;
        if (sink_address == NULL)
            sink_address = c;
        verify(c == sink_address);
    } else {
        verify(scope == s && c == sink_address);
        old_scope = s;
        scope = 0;
        if (policy == 1 && extracted == 0) {
            verify(returned == 1 && c->tag == 1);
            original(c->packet); /* recipient and first loan have ended */
        } else
            verify(extracted == 1 && c->tag == 0);
    }
}
static void custody_enter(const nl_custody *c, nl_live_tail p)
{
    verify(policy == 1 && scope != 0 && c == sink_address && c->tag == 0 &&
           recipient_calls++ == 0 && !in_recipient && test_free_calls() == 0);
    original(p);
    in_recipient = true;
}
static void custody_none(unsigned site, nl_custody v)
{
    verify((site == 1 || site == 2) && v.tag == 0 && v.packet.ptr == NULL &&
           v.packet.allocation.handle == NULL && v.packet.domain.token == 0 &&
           none[site - 1]++ == 0);
    verify(site == 1 ? in_recipient
                     : (!in_recipient && scope == 0 && phase[1] == 8));
}
static void custody_stored(const nl_custody *c, const nl_live_tail *parameter)
{
    verify(in_recipient && c == sink_address && c->tag == 1 && stored++ == 0 &&
           none[0] == 1 && parameter->ptr == NULL &&
           parameter->allocation.handle == NULL &&
           parameter->domain.token == 0);
    original(c->packet);
    in_recipient = false;
}
static void custody_returned(const nl_custody *c, const nl_live_tail *donor)
{
    verify(!in_recipient && stored == 1 && scope != 0 && returned++ == 0 &&
           c == sink_address && c->tag == 1 && donor->ptr == NULL &&
           donor->allocation.handle == NULL && donor->domain.token == 0);
    original(c->packet);
}
static void custody_extract(const nl_custody *c, nl_custody previous)
{
    verify(!in_recipient && scope != 0 && c == sink_address && c->tag == 0 &&
           extracted++ == 0);
    verify(previous.tag == (policy == 1 ? 1u : 0u));
    if (policy == 1) {
        verify(returned == 1);
        original(previous.packet);
    } else
        verify(receiver_calls == 1 && phase[1] == 8);
}
static void handoff(const nl_node *p, const void *a, size_t d,
                    const nl_allocation *da, const nl_domain *dd)
{
    verify(root_scope == 0 && scope == 0 && !in_recipient && !in_receiver &&
           phase[1] == 3 && p == nodes[1] && (uintptr_t)a == addresses[1] &&
           d == domains[1] && da->handle == NULL && dd->token == 0);
    if (policy == 1)
        verify(extracted == 1 && sink_address->tag == 0);
    donor_a = da;
    donor_d = dd;
}
static void receiver_enter(const nl_node *p, const void *a, size_t d, size_t f,
                           size_t pp, size_t pa, size_t pd,
                           const nl_allocation *ca, const nl_domain *cd)
{
    verify(!in_receiver && receiver_calls++ == 0 && scope == 0 &&
           phase[1] == 3 && p == nodes[1] && (uintptr_t)a == addresses[1] &&
           d == domains[1] && f != 0 && pp != pa && pa != pd &&
           ca->handle == a && cd->token == d);
    in_receiver = true;
}
static void end_root(const nl_node *p, size_t d)
{
    size_t i = index_of(p);
    verify(root_scope == 0 && scope == 0 && phase[i] == 3 && d == domains[i] &&
           p->f0.tag == 0 && p->f0.ptr == NULL);
    verify(i ? in_receiver : !in_receiver);
    phase[i] = 4;
}
static void raw(const void *p, size_t n)
{
    size_t i = index_of(p);
    verify(phase[i] == 4 && n == 24);
    phase[i] = 5;
}
static void finalize(size_t d)
{
    size_t i = d == domains[0] ? 0u : 1u;
    verify(domains[i] == d && d != 0 && phase[i] == 5);
    phase[i] = 6;
}
static void release(const void *p, const void *storage, size_t n)
{
    size_t i = index_of(p);
    verify(phase[i] == 6 && p == storage && n == 24 &&
           (i ? in_receiver : !in_receiver));
    phase[i] = 7;
}
void test_observe_free(uintptr_t p)
{
    size_t i = index_of((const void *)p);
    /* Reject BEFORE forwarding duplicate/incorrect frees to the platform. */
    verify(phase[i] == 7 && (i ? in_receiver : !in_receiver));
    phase[i] = 8;
}
static void receiver_exit(void)
{
    verify(in_receiver && phase[1] == 8 && phase[0] == 3);
    in_receiver = false;
}
static void receiver_returned(const nl_allocation *a, const nl_domain *d)
{
    verify(!in_receiver && a == donor_a && d == donor_d && a->handle == NULL &&
           d->token == 0 && phase[1] == 8);
}
static void root_scope_end(size_t s)
{
    verify(root_scope == s && s != 0);
    root_scope = 0;
}
static void finish(void)
{
    verify(root_scope == 0 && scope == 0 && !in_receiver && !in_recipient &&
           !in_producer);
    size_t freed = 0;
    for (size_t i = 0; i < 2; ++i) {
        verify(!addresses[i] || phase[i] == 8);
        freed += addresses[i] != 0;
    }
    verify(test_malloc_calls() == trials && test_free_calls() == freed);
    verify(receiver_calls == (addresses[1] != 0) &&
           producer_calls == receiver_calls);
    verify(recipient_calls == ((policy == 1) ? 1u : 0u) &&
           stored == recipient_calls && returned == recipient_calls &&
           none[0] == recipient_calls && none[1] == receiver_calls &&
           extracted == receiver_calls);
    printf("CUSTODY head=%zx tail=%zx trials=%zu free=%zu policy=%zu "
           "recipient=%zu stored=%zu returned=%zu extract=%zu terminal=%zu "
           "none=%zu/%zu\n",
           (size_t)addresses[0], (size_t)addresses[1], trials, freed, policy,
           recipient_calls, stored, returned, extracted, receiver_calls,
           none[0], none[1]);
}
#define NL_HEAP_TRIAL(p, s, a) trial(p, s, a)
#define NL_HEAP_ARM(...) ((void)0)
#define NL_HEAP_SLOT(p, n) slot(p, n)
#define NL_HEAP_DOMAIN(d) domain(d)
#define NL_HEAP_INITIALIZE(p, d) initialize(p, d)
#define NL_HEAP_ROOT(p, d, w, o, i, s) root_ref(p, d, w, o, i, s)
#define NL_HEAP_FIELD(p, r, w, n, f, c, i, s) field_ref(p, r, w, n, f, c, i, s)
#define NL_HEAP_CHANGE(p, o, n) change(p, o, n)
#define NL_HEAP_SCOPE_END(s) root_scope_end(s)
#define NL_HEAP_END(p, d) end_root(p, d)
#define NL_HEAP_RAW(p, n) raw(p, n)
#define NL_HEAP_FINALIZE(d) finalize(d)
#define NL_HEAP_RELEASE(p, s, n) release(p, s, n)
#define NL_HEAP_HANDOFF(p, a, d, da, dd) handoff(p, a, d, da, dd)
#define NL_HEAP_RECEIVER_ENTER(p, a, d, f, pp, pa, pd, ca, cd)                 \
    receiver_enter(p, a, d, f, pp, pa, pd, ca, cd)
#define NL_HEAP_RECEIVER_EXIT() receiver_exit()
#define NL_HEAP_RETURNED(a, d) receiver_returned(a, d)
#define NL_HEAP_FINISH() finish()
#define NL_LIVE_SEND(p, a, d, da, dd) live_send(p, a, d, da, dd)
#define NL_LIVE_ENTER(h, p, a, d) live_enter(h, p, a, d)
#define NL_LIVE_RETURN(p) live_return(p)
#define NL_LIVE_RECEIVE(p, a, d) live_receive(p, a, d)
#define NL_CUSTODY_POLICY(v) custody_policy(v)
#define NL_CUSTODY_LOAN(c, s, b) custody_loan(c, s, b)
#define NL_CUSTODY_ENTER(c, p) custody_enter(c, p)
#define NL_CUSTODY_NONE(s, v) custody_none(s, v)
#define NL_CUSTODY_STORED(c, p) custody_stored(c, p)
#define NL_CUSTODY_RETURNED(c, p) custody_returned(c, p)
#define NL_CUSTODY_EXTRACT(c, p) custody_extract(c, p)
