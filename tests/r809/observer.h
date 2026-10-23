/* R809 independent PASSIVE read-only runtime observer.
 * Distinct observation-only C17 build, never an ordinary O2 native claim.
 * Inspect real generated fields, original pointers, owner transfers and frees.
 * Never initialize, repair, write or synthesize guest memory/authority.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define R809_ASSERT(x)                                                         \
    do {                                                                       \
        if (!(x)) {                                                             \
            fprintf(stderr, "R809 observer violation line %d: %s\n", __LINE__, \
                    #x);                                                       \
            abort();                                                           \
        }                                                                      \
    } while (0)
#define R809_UNSUPPORTED(label)                                                 \
    do {                                                                       \
        fprintf(stderr, "R809 UNSUPPORTED observer hook: %s\n", label);         \
        abort();                                                               \
    } while (0)
size_t test_malloc_calls(void);
size_t test_free_calls(void);
uintptr_t test_allocated_address(size_t index);
unsigned test_fail_number(void);
static uintptr_t r809_addr[2];
static const nl_node *r809_node[2];
static size_t r809_phase[2], r809_domain[2], r809_trials, r809_selected;
static size_t r809_change, r809_active, r809_scope_count, r809_frees;
static size_t r809_live_send, r809_live_enter, r809_local_count;
static size_t r809_live_return, r809_live_receive, r809_receiver_enter;
static size_t r809_receiver_exit, r809_handoff, r809_returned;
static const nl_live_tail *r809_local_packet;
static uint8_t *r809_t[2][2];
static size_t r809_troots, r809_copies;
static unsigned r809_tlast[2][2];
static unsigned r809_old[2] = {172, 94};
static unsigned r809_new[2] = {206, 172};
static unsigned r809_index[2] = {1, 0};

static size_t r809_slot_index(const void *p)
{
    for (size_t i = 0; i < 2; ++i)
        if (r809_addr[i] && (uintptr_t)p == r809_addr[i])
            return i;
    R809_ASSERT(false);
    return 0;
}

static void r809_trial(void *p, size_t bytes, size_t alignment)
{
    R809_ASSERT(r809_trials < 2 && bytes == 24 && alignment == 8);
    R809_ASSERT(test_malloc_calls() == r809_trials + 1);
    r809_selected = r809_trials;
    R809_ASSERT((uintptr_t)p == test_allocated_address(r809_trials));
    r809_addr[r809_trials] = (uintptr_t)p;
    r809_phase[r809_trials] = p ? 1 : 0;
    if (p && r809_trials)
        R809_ASSERT(r809_addr[0] && r809_addr[0] != r809_addr[1]);
    ++r809_trials;
}
static void r809_arm(unsigned arm)
{
    R809_ASSERT(arm == (r809_addr[r809_selected] ? 2u : 1u));
}
static void r809_slot(void *p, size_t size)
{
    size_t i = r809_slot_index(p);
    R809_ASSERT(size == 24 && r809_phase[i] == 1);
    r809_phase[i] = 2;
    r809_selected = i;
}
static void r809_new_domain(size_t domain)
{
    R809_ASSERT(r809_phase[r809_selected] == 2 && domain != 0);
    if (r809_selected)
        R809_ASSERT(domain != r809_domain[0]);
    r809_domain[r809_selected] = domain;
    r809_phase[r809_selected] = 3;
}
static void r809_initialize(const nl_node *p, size_t domain)
{
    size_t i = r809_slot_index(p);
    R809_ASSERT(r809_phase[i] == 3 && r809_domain[i] == domain);
    R809_ASSERT(p->f0.tag == 0 && p->f0.ptr == NULL);
    R809_ASSERT(p->f1 == (i == 0 ? 71u : 182u));
    r809_node[i] = p;
    r809_phase[i] = 4;
}
static void r809_root(const nl_node *p, size_t domain, unsigned write,
                      size_t root, size_t incarnation, size_t scope)
{
    size_t i = r809_slot_index(p);
    R809_ASSERT(r809_phase[i] == 4 && domain == r809_domain[i]);
    R809_ASSERT(!r809_active && scope && root && incarnation && write <= 1u);
    r809_active = scope;
}
static void r809_field(const nl_node *p, const nl_node_option *field,
                       unsigned write, size_t nominal, size_t index,
                       size_t child, size_t incarnation, size_t scope)
{
    R809_ASSERT(p == r809_node[0] && field == &p->f0);
    R809_ASSERT(write == 1 && index == 0 && nominal && child && incarnation);
    R809_ASSERT(r809_active && scope == r809_active);
}
static void r809_change_link(const nl_node_option *field,
                             const nl_node_option *prior, size_t change)
{
    R809_ASSERT(field == &r809_node[0]->f0 && prior != field);
    R809_ASSERT(r809_phase[0] == 4 && r809_phase[1] == 4);
    R809_ASSERT(r809_active && change == ++r809_change && change <= 2);
    if (change == 1) {
        R809_ASSERT(prior->tag == 0 && prior->ptr == NULL);
        R809_ASSERT(field->tag == 1 && field->ptr == r809_node[1]);
    } else {
        R809_ASSERT(prior->tag == 1 && prior->ptr == r809_node[1]);
        R809_ASSERT(field->tag == 0 && field->ptr == NULL);
    }
}
static void r809_scope_end(size_t scope)
{
    R809_ASSERT(r809_active == scope && scope != 0);
    r809_active = 0;
    ++r809_scope_count;
}
static void r809_packet(const nl_live_tail *value)
{
    R809_ASSERT(r809_node[1] && r809_phase[1] == 4);
    R809_ASSERT(value->ptr == r809_node[1] &&
                value->allocation.handle == (void *)r809_node[1] &&
                value->domain.token == r809_domain[1]);
}
static void r809_send(const nl_node *p, void *a, size_t d,
                      const nl_allocation *old_a, const nl_domain *old_d)
{
    R809_ASSERT(r809_live_send++ == 0 && p == r809_node[1] &&
                a == (void *)p && d == r809_domain[1]);
    R809_ASSERT(old_a->handle == NULL && old_d->token == 0);
}
static void r809_enter(nl_node_option *edge, const nl_node *p, void *a, size_t d)
{
    R809_ASSERT(r809_live_enter++ == 0 && edge == &r809_node[0]->f0);
    R809_ASSERT(edge->tag == 1 && edge->ptr == r809_node[1]);
    R809_ASSERT(p == r809_node[1] && a == (void *)p && d == r809_domain[1]);
}
static void r809_local(const nl_live_tail *local)
{
    R809_ASSERT(r809_live_enter == 1 && r809_live_return == 0);
    R809_ASSERT(r809_change == 2 && r809_local_count++ == 0);
    r809_packet(local);
    r809_local_packet = local;
}
static void r809_return_packet(nl_live_tail returned)
{
    R809_ASSERT(r809_local_count == 1 && r809_live_return++ == 0);
    r809_packet(&returned);
    R809_ASSERT(r809_local_packet && r809_local_packet->ptr == NULL &&
                r809_local_packet->allocation.handle == NULL &&
                r809_local_packet->domain.token == 0);
}
static void r809_receive_packet(nl_live_tail received,
                                const nl_allocation *old_a, const nl_domain *old_d)
{
    R809_ASSERT(r809_live_return == 1 && r809_live_receive++ == 0);
    r809_packet(&received);
    R809_ASSERT(old_a->handle == NULL && old_d->token == 0);
}
static void r809_copy_bind(size_t frame, size_t symbol, uint8_t *a, uint8_t *b)
{
    R809_ASSERT(frame && symbol && a && b && a != b);
    R809_ASSERT(r809_live_receive == 1 && r809_troots < 2);
    size_t i = r809_troots++;
    r809_t[i][0] = a;
    r809_t[i][1] = b;
    R809_ASSERT(*a == (i == 0 ? 41u : 94u));
    R809_ASSERT(*b == (i == 0 ? 172u : 63u));
    if (i)
        R809_ASSERT(a != r809_t[0][0] && a != r809_t[0][1] &&
                    b != r809_t[0][0] && b != r809_t[0][1]);
}
static void r809_copy_call(uint8_t *field, uint8_t old, uint8_t value)
{
    R809_ASSERT(r809_troots == 2 && r809_copies < 2);
    size_t call = r809_copies++;
    R809_ASSERT(field == r809_t[call][r809_index[call]]);
    R809_ASSERT(old == r809_old[call] && value == r809_new[call]);
    R809_ASSERT(*field == value);
    R809_ASSERT(*r809_t[call][1 - r809_index[call]] ==
                (call == 0 ? 41u : 63u));
}
static void r809_verify_t(void)
{
    R809_ASSERT(r809_troots == 2 && r809_copies == 2);
    static const unsigned values[2][2] = {{41, 206}, {172, 63}};
    for (size_t i = 0; i < 2; ++i)
        for (size_t j = 0; j < 2; ++j) {
            R809_ASSERT(r809_t[i][j] != NULL);
            r809_tlast[i][j] = *r809_t[i][j];
            R809_ASSERT(r809_tlast[i][j] == values[i][j]);
        }
}
static void r809_handoff_owner(const nl_node *p, void *a, size_t d,
                               const nl_allocation *old_a, const nl_domain *old_d)
{
    R809_ASSERT(r809_handoff++ == 0 && !r809_active);
    R809_ASSERT(p == r809_node[1] && a == (void *)p && d == r809_domain[1]);
    R809_ASSERT(old_a->handle == NULL && old_d->token == 0);
    r809_verify_t();
}
static void r809_receive_enter(const nl_node *p, void *a, size_t d,
                               size_t frame, size_t ps, size_t as, size_t ds,
                               const nl_allocation *ap, const nl_domain *dp)
{
    R809_ASSERT(frame && ps && as && ds && r809_receiver_enter++ == 0);
    R809_ASSERT(r809_handoff == 1 && p == r809_node[1] && a == (void *)p &&
                d == r809_domain[1] && ap->handle == a && dp->token == d);
}
static void r809_end(const nl_node *p, size_t d)
{
    size_t i = r809_slot_index(p);
    R809_ASSERT(r809_phase[i] == 4 && d == r809_domain[i]);
    R809_ASSERT(p == r809_node[i] && p->f1 == (i ? 182u : 71u));
    R809_ASSERT(p->f0.tag == 0 && p->f0.ptr == NULL && !r809_active);
    if (i)
        R809_ASSERT(r809_receiver_enter == 1 && r809_change == 2);
    else if (r809_node[1])
        R809_ASSERT(r809_receiver_exit == 1 && r809_frees == 1);
    r809_phase[i] = 5;
}
static void r809_raw(const void *p, size_t bytes)
{
    size_t i = r809_slot_index(p);
    R809_ASSERT(r809_phase[i] == 5 && bytes == 24);
    r809_selected = i;
    r809_phase[i] = 6;
}
static void r809_finalize(size_t domain)
{
    R809_ASSERT(r809_phase[r809_selected] == 6 &&
                r809_domain[r809_selected] == domain);
    r809_phase[r809_selected] = 7;
}
static void r809_release(const void *p, const void *raw, size_t bytes)
{
    size_t i = r809_slot_index(p);
    R809_ASSERT(r809_phase[i] == 7 && p == raw && bytes == 24);
    r809_phase[i] = 8;
}
/* Called by independently linked wrapper for real original pointer just
 * BEFORE forwarding to libc free. Does not invent or modify any guest bytes. */
void r809_observe_free(uintptr_t pointer)
{
    size_t i = r809_slot_index((void *)pointer);
    R809_ASSERT(r809_phase[i] == 8);
    if (r809_node[1])
        R809_ASSERT(i == (r809_frees == 0 ? 1u : 0u));
    else
        R809_ASSERT(i == 0);
    r809_phase[i] = 9;
    ++r809_frees;
}
static void r809_receiver_leave(void)
{
    R809_ASSERT(r809_receiver_enter == 1 && r809_receiver_exit++ == 0);
    R809_ASSERT(r809_phase[1] == 9);
}
static void r809_owner_returned(const nl_allocation *a, const nl_domain *d)
{
    R809_ASSERT(r809_receiver_exit == 1 && r809_returned++ == 0);
    R809_ASSERT(a->handle == NULL && d->token == 0);
}
static void r809_finish(void)
{
    unsigned fail = test_fail_number();
    size_t requests = fail == 1 ? 1u : 2u;
    size_t successful = fail == 1 ? 0u : fail == 2 ? 1u : 2u;
    R809_ASSERT(test_malloc_calls() == requests && r809_trials == requests);
    R809_ASSERT(test_free_calls() == successful && r809_frees == successful);
    R809_ASSERT(!r809_active);
    for (size_t i = 0; i < 2; ++i)
        if (r809_addr[i])
            R809_ASSERT(r809_phase[i] == 9);
    if (successful == 2) {
        R809_ASSERT(r809_node[0] && r809_node[1] && r809_change == 2);
        R809_ASSERT(r809_live_send == 1 && r809_live_enter == 1 &&
                    r809_live_return == 1 && r809_live_receive == 1);
        R809_ASSERT(r809_local_count == 1 && r809_receiver_enter == 1 &&
                    r809_receiver_exit == 1 && r809_handoff == 1 &&
                    r809_returned == 1 && r809_copies == 2);
    } else {
        R809_ASSERT(r809_change == 0 && r809_troots == 0 && r809_copies == 0);
        R809_ASSERT(r809_local_count == 0 && r809_live_send == 0 &&
                    r809_live_receive == 0 && r809_receiver_enter == 0);
    }
    printf("R809_OBSERVER requests=%zu allocations=%zu frees=%zu "
           "T=%u,%u/%u,%u COPY=%zu lexical=%zu received=%zu "
           "original_head=%p original_tail=%p scopes=%zu\n",
           requests, successful, r809_frees,
           r809_tlast[0][0], r809_tlast[0][1],
           r809_tlast[1][0], r809_tlast[1][1], r809_copies,
           r809_local_count, r809_live_receive,
           (void *)r809_addr[0], (void *)r809_addr[1], r809_scope_count);
}
#define NL_HEAP_TRIAL(p,n,a) r809_trial(p,n,a)
#define NL_HEAP_ARM(a) r809_arm(a)
#define NL_HEAP_SLOT(p,n) r809_slot(p,n)
#define NL_HEAP_DOMAIN(d) r809_new_domain(d)
#define NL_HEAP_INITIALIZE(p,d) r809_initialize(p,d)
#define NL_HEAP_ROOT(p,d,w,r,i,s) r809_root(p,d,w,r,i,s)
#define NL_HEAP_FIELD(p,f,w,n,i,c,x,s) r809_field(p,f,w,n,i,c,x,s)
#define NL_HEAP_CHANGE(f,o,s) r809_change_link(f,o,s)
#define NL_HEAP_SCOPE_END(s) r809_scope_end(s)
#define NL_HEAP_END(p,d) r809_end(p,d)
#define NL_HEAP_RAW(p,n) r809_raw(p,n)
#define NL_HEAP_FINALIZE(d) r809_finalize(d)
#define NL_HEAP_RELEASE(p,b,n) r809_release(p,b,n)
#define NL_HEAP_HANDOFF(p,a,d,ap,dp) r809_handoff_owner(p,a,d,ap,dp)
#define NL_HEAP_RECEIVER_ENTER(p,a,d,f,ps,as,ds,ap,dp)                         \
    r809_receive_enter(p,a,d,f,ps,as,ds,ap,dp)
#define NL_HEAP_RECEIVER_EXIT() r809_receiver_leave()
#define NL_HEAP_RETURNED(a,d) r809_owner_returned(a,d)
#define NL_HEAP_FINISH() r809_finish()
#define NL_LIVE_SEND(p,a,d,ap,dp) r809_send(p,a,d,ap,dp)
#define NL_LIVE_ENTER(e,p,a,d) r809_enter(e,p,a,d)
#define NL_LIVE_RETURN(p) r809_return_packet(p)
#define NL_LIVE_RECEIVE(p,a,d) r809_receive_packet(p,a,d)
#define NL_MIXED_OWNER_LOCAL(p) r809_local(p)
#define NL_MIXED_COPY_BIND(f,s,a,b) r809_copy_bind(f,s,a,b)
#define NL_MIXED_COPY_CALL(p,o,n) r809_copy_call(p,o,n)
/* Deliberately STOP instead of silently dropping an unverified extra hook. */
#define NL_HEAP_RELOAN(...) R809_UNSUPPORTED("NL_HEAP_RELOAN")
#define NL_HEAP_COPY(...) R809_UNSUPPORTED("NL_HEAP_COPY")
#define NL_MIXED_COPY_STORE(...) R809_UNSUPPORTED("NL_MIXED_COPY_STORE")
