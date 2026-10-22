/* R736 read-only observer: never alters guest heap, creates rights or selects
 * arms. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
size_t r736_alloc_calls(void);
size_t r736_free_calls(void);
uintptr_t r736_alloc_at(size_t);
#define R736_ASSERT(x)                                                         \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "R736_OBSERVER_REJECT line=%d\n", __LINE__);       \
            abort();                                                           \
        }                                                                      \
    } while (0)
static uintptr_t r736_regions[2];
static const nl_node *r736_roots[2];
static size_t r736_domain_ids[2], r736_current_region;
static unsigned r736_phases[2];
static size_t r736_trials, r736_links, r736_sends, r736_enters;
static size_t r736_returns, r736_receives, r736_endings, r736_releases,
    r736_frees;
static const nl_node_option *r736_edge;
static unsigned r736_region(const void *p)
{
    for (unsigned i = 0; i < 2; ++i)
        if (p && (uintptr_t)p == r736_regions[i])
            return i;
    R736_ASSERT(false);
    return 0;
}
static void r736_trial(void *p, size_t bytes, size_t alignment)
{
    R736_ASSERT(r736_trials < 2 && r736_alloc_calls() == r736_trials + 1);
    R736_ASSERT(bytes == sizeof(nl_node) && alignment >= 8);
    R736_ASSERT((uintptr_t)p == r736_alloc_at(r736_trials));
    r736_regions[r736_trials] = (uintptr_t)p;
    r736_phases[r736_trials] = p ? 1 : 99;
    ++r736_trials;
}
static void r736_slot(const void *p, size_t bytes)
{
    unsigned i = r736_region(p);
    R736_ASSERT(bytes == sizeof(nl_node) && r736_phases[i] == 1);
    r736_current_region = i;
    r736_phases[i] = 2;
}
static void r736_domain(size_t domain)
{
    size_t i = r736_current_region;
    R736_ASSERT(i < 2 && r736_phases[i] == 2 && domain != 0);
    if (i)
        R736_ASSERT(domain != r736_domain_ids[0]);
    r736_domain_ids[i] = domain;
    r736_phases[i] = 3;
}
static void r736_init(const nl_node *p, size_t domain)
{
    unsigned i = r736_region(p);
    R736_ASSERT(r736_phases[i] == 3 && domain == r736_domain_ids[i]);
    r736_roots[i] = p;
    r736_phases[i] = 4;
}
static void r736_change(const nl_node_option *field, const nl_node_option *old,
                        size_t step)
{
    R736_ASSERT(r736_phases[0] == 4 && r736_phases[1] == 4);
    R736_ASSERT(field && old && field != old);
    if (r736_links == 0) {
        R736_ASSERT(step == 1 && old->tag == 0 && !old->ptr);
        R736_ASSERT(field->tag == 1 && field->ptr == r736_roots[1]);
        r736_edge = field;
    } else {
        R736_ASSERT(r736_links == 1 && step == 2 && field == r736_edge);
        R736_ASSERT(old->tag == 1 && old->ptr == r736_roots[1]);
        R736_ASSERT(field->tag == 0 && !field->ptr);
    }
    ++r736_links;
}
static void r736_send(const nl_node *p, const void *a, size_t d,
                      const nl_allocation *old_a, const nl_domain *old_d)
{
    R736_ASSERT(r736_links == 1 && r736_sends++ == 0 && r736_trials == 2);
    R736_ASSERT(p == r736_roots[1] && (uintptr_t)a == r736_regions[1]);
    R736_ASSERT(d == r736_domain_ids[1] && !old_a->handle && !old_d->token);
}
static void r736_enter(const nl_node_option *edge, const nl_node *p,
                       const void *a, size_t d)
{
    R736_ASSERT(r736_sends == 1 && r736_enters++ == 0 && edge == r736_edge);
    R736_ASSERT(edge->tag == 1 && edge->ptr == p && p == r736_roots[1]);
    R736_ASSERT((uintptr_t)a == r736_regions[1] && d == r736_domain_ids[1]);
}
static void r736_return(nl_live_tail value)
{
    R736_ASSERT(r736_enters == 1 && r736_returns++ == 0 && r736_links == 2);
    R736_ASSERT(value.ptr == r736_roots[1]);
    R736_ASSERT((uintptr_t)value.allocation.handle == r736_regions[1]);
    R736_ASSERT(value.domain.token == r736_domain_ids[1]);
    R736_ASSERT(r736_edge->tag == 0 && !r736_edge->ptr && !r736_free_calls());
}
static void r736_receive(nl_live_tail v, const nl_allocation *a,
                         const nl_domain *d)
{
    R736_ASSERT(r736_returns == 1 && r736_receives++ == 0);
    R736_ASSERT(v.ptr == r736_roots[1] &&
                (uintptr_t)v.allocation.handle == r736_regions[1] &&
                v.domain.token == r736_domain_ids[1]);
    R736_ASSERT(a && d && !a->handle && !d->token);
}
static void r736_end(const nl_node *p, size_t d)
{
    unsigned i = r736_region(p);
    R736_ASSERT(r736_phases[i] == 4 && r736_roots[i] == p &&
                r736_domain_ids[i] == d);
    if (i == 1)
        R736_ASSERT(r736_returns == 1 && r736_receives == 1);
    else if (r736_regions[1])
        R736_ASSERT(r736_phases[1] == 9);
    r736_phases[i] = 5;
    ++r736_endings;
}
static void r736_raw(const void *p, size_t bytes)
{
    unsigned i = r736_region(p);
    R736_ASSERT(r736_phases[i] == 5 && bytes == sizeof(nl_node));
    r736_current_region = i;
    r736_phases[i] = 6;
}
static void r736_finalize(size_t d)
{
    unsigned i = r736_current_region;
    R736_ASSERT(r736_phases[i] == 6 && r736_domain_ids[i] == d);
    r736_phases[i] = 7;
}
static void r736_release(const void *p, const void *raw, size_t bytes)
{
    unsigned i = r736_region(p);
    R736_ASSERT(r736_phases[i] == 7 && (uintptr_t)raw == r736_regions[i] &&
                bytes == sizeof(nl_node));
    r736_phases[i] = 8;
    ++r736_releases;
}
void test_observe_free(uintptr_t p)
{
    unsigned i = r736_region((void *)p);
    R736_ASSERT(r736_phases[i] == 8);
    r736_phases[i] = 9;
    ++r736_frees;
}
static void r736_finish(void)
{
    const size_t expected = !!r736_regions[0] + !!r736_regions[1];
    R736_ASSERT(r736_trials == r736_alloc_calls() &&
                r736_trials == (r736_regions[0] ? 2u : 1u));
    R736_ASSERT(r736_endings == expected && r736_releases == expected &&
                r736_frees == expected && r736_free_calls() == expected);
    R736_ASSERT(r736_phases[0] == (r736_regions[0] ? 9u : 99u));
    if (r736_trials == 2)
        R736_ASSERT(r736_phases[1] == (r736_regions[1] ? 9u : 99u));
    if (r736_regions[0] && r736_regions[1])
        R736_ASSERT(r736_regions[0] != r736_regions[1]);
    R736_ASSERT(r736_links == (r736_regions[1] ? 2u : 0u));
    R736_ASSERT(r736_returns == (r736_regions[1] ? 1u : 0u) &&
                r736_receives == r736_returns && r736_sends == r736_returns);
    printf("R736_NATIVE allocations=%zu frees=%zu linked=%zu returned=%zu "
           "received=%zu original_h=%zx original_t=%zx\n",
           expected, r736_frees, r736_links / 2, r736_returns, r736_receives,
           (size_t)r736_regions[0], (size_t)r736_regions[1]);
}
/* R740 post-first-failure mechanical repair: instrument NEW visible emitted
 * hooks, read-only. These callbacks do not write guest heap state. */
static unsigned r740_arms, r740_roots_seen, r740_fields_seen;
static unsigned r740_scope_exits, r740_handoffs, r740_receiver_enters;
static unsigned r740_receiver_exits, r740_return_count;

static void r740_arm(unsigned choice)
{
    R736_ASSERT(choice == 1 || choice == 2);
    ++r740_arms;
}
static void r740_root(const nl_node *p, size_t domain, unsigned write,
                      size_t place, size_t incarnation, size_t scope)
{
    unsigned i = r736_region(p);
    R736_ASSERT(r736_roots[i] == p && r736_phases[i] == 4);
    R736_ASSERT(domain == r736_domain_ids[i] && write <= 1);
    R736_ASSERT(place && incarnation && scope);
    ++r740_roots_seen;
}
static void r740_field(const nl_node *parent, const nl_node_option *field,
                       unsigned write, size_t nominal, size_t index,
                       size_t child, size_t incarnation, size_t scope)
{
    R736_ASSERT(parent == r736_roots[0]);
    R736_ASSERT(field == &r736_roots[0]->f0);
    R736_ASSERT(write == 1 && nominal && index == 0);
    R736_ASSERT(child && incarnation && scope);
    ++r740_fields_seen;
}
static void r740_scope_end(size_t scope)
{
    R736_ASSERT(scope != 0);
    ++r740_scope_exits;
}
static void r740_handoff(const nl_node *p, const void *handle, size_t domain,
                         const nl_allocation *a, const nl_domain *d)
{
    R736_ASSERT(r740_handoffs++ == 0 && r736_links == 2);
    R736_ASSERT(r736_returns == 1 && r736_receives == 1);
    R736_ASSERT(p == r736_roots[1]);
    R736_ASSERT((uintptr_t)handle == r736_regions[1]);
    R736_ASSERT(domain == r736_domain_ids[1] && a && d);
    R736_ASSERT(r736_phases[1] == 4 && r736_free_calls() == 0);
}
static void r740_receiver_enter(const nl_node *p, const void *handle,
                                size_t domain, size_t pointer_id,
                                size_t allocation_id, size_t domain_id,
                                size_t unused_id, const nl_allocation *a,
                                const nl_domain *d)
{
    R736_ASSERT(r740_receiver_enters++ == 0 && r740_handoffs == 1);
    R736_ASSERT(p == r736_roots[1]);
    R736_ASSERT((uintptr_t)handle == r736_regions[1]);
    R736_ASSERT(domain == r736_domain_ids[1]);
    R736_ASSERT(pointer_id && allocation_id && domain_id && unused_id);
    R736_ASSERT(a && d && (uintptr_t)a->handle == r736_regions[1]);
    R736_ASSERT(d->token == r736_domain_ids[1]);
    R736_ASSERT(r736_phases[1] == 4);
}
static void r740_receiver_exit(void)
{
    R736_ASSERT(r740_receiver_exits++ == 0 && r740_receiver_enters == 1);
    R736_ASSERT(r736_phases[1] == 9 && r736_frees == 1);
}
static void r740_returned(const nl_allocation *a, const nl_domain *d)
{
    R736_ASSERT(r740_return_count++ == 0 && r740_receiver_exits == 1);
    R736_ASSERT(a && d && r736_phases[1] == 9);
}
static void r740_finish(void)
{
    unsigned success = !!r736_regions[1];
    R736_ASSERT(r740_handoffs == success);
    R736_ASSERT(r740_receiver_enters == success);
    R736_ASSERT(r740_receiver_exits == success);
    R736_ASSERT(r740_return_count == success);
    if (success) {
        R736_ASSERT(r740_fields_seen >= 2 && r740_roots_seen >= 2);
        R736_ASSERT(r740_scope_exits > 0 && r740_arms > 0);
    } else {
        R736_ASSERT(r740_fields_seen == 0);
    }
}
#define NL_HEAP_TRIAL(p, n, a) r736_trial(p, n, a)
#define NL_HEAP_SLOT(p, n) r736_slot(p, n)
#define NL_HEAP_DOMAIN(d) r736_domain(d)
#define NL_HEAP_INITIALIZE(p, d) r736_init(p, d)
#define NL_HEAP_CHANGE(f, o, s) r736_change(f, o, s)
#define NL_HEAP_END(p, d) r736_end(p, d)
#define NL_HEAP_RAW(p, n) r736_raw(p, n)
#define NL_HEAP_FINALIZE(d) r736_finalize(d)
#define NL_HEAP_RELEASE(p, r, n) r736_release(p, r, n)
#define NL_LIVE_SEND(p, a, d, da, dd) r736_send(p, a, d, da, dd)
#define NL_LIVE_ENTER(h, p, a, d) r736_enter(h, p, a, d)
#define NL_LIVE_RETURN(v) r736_return(v)
#define NL_LIVE_RECEIVE(v, a, d) r736_receive(v, a, d)
#define NL_HEAP_FINISH()                                                       \
    do {                                                                       \
        r736_finish();                                                         \
        r740_finish();                                                         \
    } while (0)

#define NL_HEAP_ARM(choice) r740_arm(choice)
#define NL_HEAP_ROOT(p, d, write, place, inc, scope)                           \
    r740_root(p, d, write, place, inc, scope)
#define NL_HEAP_FIELD(parent, field, w, nominal, index, child, inc, scope)     \
    r740_field(parent, field, w, nominal, index, child, inc, scope)
#define NL_HEAP_SCOPE_END(scope) r740_scope_end(scope)
#define NL_HEAP_HANDOFF(p, a, d, ap, dp) r740_handoff(p, a, d, ap, dp)
#define NL_HEAP_RECEIVER_ENTER(p, a, d, pid, aid, did, uid, ap, dp)            \
    r740_receiver_enter(p, a, d, pid, aid, did, uid, ap, dp)
#define NL_HEAP_RECEIVER_EXIT() r740_receiver_exit()
#define NL_HEAP_RETURNED(ap, dp) r740_returned(ap, dp)
