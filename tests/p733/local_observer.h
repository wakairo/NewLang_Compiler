/* Passive only: zero guest writes/rights; all expected identities are the
 * independently observed original heap nodes, original Allocation and Domain.
 */
#include "../integration/live_return_observer.h"
static size_t local_bindings, local_moves;
static void p733_bound(const nl_live_tail *local, const nl_live_tail *temporary)
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
static void p733_moved(const nl_live_tail *local, const nl_live_tail *returned)
{
    siblings();
    VERIFY(in_producer && local_bindings == 1 && local_moves++ == 0);
    VERIFY(local->ptr == NULL && local->allocation.handle == NULL &&
           local->domain.token == 0);
    VERIFY(returned->ptr == nodes[1] &&
           returned->allocation.handle == (void *)addresses[1] &&
           returned->domain.token == domains[1]);
}
static void p733_finish(void)
{
    VERIFY(local_bindings == (addresses[1] != 0) &&
           local_moves == local_bindings);
    live_finish();
}
#undef NL_HEAP_FINISH
#define NL_HEAP_FINISH() p733_finish()
#define P733_BOUND(a, b) p733_bound(&(a), &(b))
#define P733_MOVED(a, b) p733_moved(&(a), &(b))
