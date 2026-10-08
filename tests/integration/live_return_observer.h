/* Independent read-only return observer extends the existing terminal gate.
 * Only observer bookkeeping changes. No heap byte write, free or authority. */
#include "handoff_observer.h"
static bool in_producer, pending_return;
static size_t producer_entries, producer_returns, producer_receipts;
static const nl_allocation *return_donor_allocation;
static const nl_domain *return_donor_domain;
static void live_send(const nl_node *p, const void *a, size_t d,
                      const nl_allocation *donor_a, const nl_domain *donor_d)
{
    siblings();
    VERIFY(!in_producer && !pending_return && active_scope == scopes[3]);
    VERIFY(p == nodes[1] && (uintptr_t)a == addresses[1] && d == domains[1]);
    VERIFY(nodes[0]->f0.tag == 1 && nodes[0]->f0.ptr == p);
    VERIFY(donor_a->handle == NULL && donor_d->token == 0);
    return_donor_allocation = donor_a;
    return_donor_domain = donor_d;
    pending_return = true;
}
static void live_enter(const nl_node_option *head, const nl_node *p,
                       const void *a, size_t d)
{
    siblings();
    VERIFY(pending_return && !in_producer && producer_entries++ == 0);
    VERIFY(head == &nodes[0]->f0 && head->tag == 1 && head->ptr == p);
    VERIFY(p == nodes[1] && (uintptr_t)a == addresses[1] && d == domains[1]);
    VERIFY(active_scope == scopes[3] && link_phase == 4);
    in_producer = true;
}
static void live_return(nl_live_tail value)
{
    siblings();
    VERIFY(in_producer && pending_return && producer_returns++ == 0);
    VERIFY(link_phase == 5 && active_scope == scopes[3]);
    VERIFY(nodes[0]->f0.tag == 0 && nodes[0]->f0.ptr == NULL);
    VERIFY(value.ptr == nodes[1] &&
           (uintptr_t)value.allocation.handle == addresses[1] &&
           value.domain.token == domains[1] && test_free_calls() == 0);
    in_producer = false;
}
static void live_receive(nl_live_tail value, const nl_allocation *a,
                         const nl_domain *d)
{
    siblings();
    VERIFY(!in_producer && pending_return && producer_returns == 1 &&
           producer_receipts++ == 0 && active_scope == scopes[3]);
    VERIFY(value.ptr == nodes[1] &&
           (uintptr_t)value.allocation.handle == addresses[1] &&
           value.domain.token == domains[1]);
    VERIFY(a == return_donor_allocation && d == return_donor_domain &&
           a->handle == NULL && d->token == 0 && test_free_calls() == 0);
    pending_return = false;
}
static void live_finish(void)
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
