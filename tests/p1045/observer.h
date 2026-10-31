/* C137 passive live two-original T witnesses; P-owned original H hooks. */
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define REQUIRE(x)                                                             \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "P1045 physical:%d: %s\n", __LINE__, #x);          \
            abort();                                                           \
        }                                                                      \
    } while (0)
size_t test_malloc_calls(void);
size_t test_free_calls(void);
uintptr_t test_allocated_address(size_t);
#ifndef P1045_PHASE
#define P1045_PHASE 0
#endif
#define P1045_H0 53
#define P1045_H1 197
static uint8_t *t_fields[2][2];
static uintptr_t t_addresses[2][2], allocated[2];
static size_t t_frames[2], t_symbols[2];
static unsigned final_t[2][2];
static size_t domains[2], binds, calls, trials, initialized, changes, active;
static size_t sent, entered, local_owner, returned, received, terminals, frees;
static size_t caller_reads, callee_reads;
static bool in_receiver, ended[2], finalized[2], releasing[2], freed[2];
static unsigned observed_old[2], observed_new[2], observed_sibling[2];
static size_t original(const void *p)
{
    uintptr_t address = (uintptr_t)p;
    for (size_t i = 0; i < 2; ++i)
        if (address && address == allocated[i])
            return i;
    abort();
}
static uint8_t peek(const uint8_t *p)
{
    return *(const volatile uint8_t *)p;
}
static void t_phase(void)
{
    if (P1045_PHASE == 0)
        REQUIRE(!trials);
    else {
        REQUIRE(trials == 2 && initialized == 2 && !active && !terminals);
        if (P1045_PHASE == 1)
            REQUIRE(changes == 2 && received == 1);
        else
            REQUIRE(P1045_PHASE == 2 && !changes && !received && !sent);
    }
}
static void bind_copy(size_t frame, size_t symbol, uint8_t *a, uint8_t *b)
{
    t_phase();
    REQUIRE(binds < 2 && !calls && a != b && peek(a) == (binds ? 222 : 19) &&
            peek(b) == (binds ? 67 : 149));
    if (binds)
        REQUIRE((frame != t_frames[0] || symbol != t_symbols[0]) &&
                a != t_fields[0][0] && a != t_fields[0][1] &&
                b != t_fields[0][0] && b != t_fields[0][1]);
    t_fields[binds][0] = a;
    t_fields[binds][1] = b;
    t_addresses[binds][0] = (uintptr_t)a;
    t_addresses[binds][1] = (uintptr_t)b;
    t_frames[binds] = frame;
    t_symbols[binds++] = symbol;
}
static void copy_call(uint8_t *p, uint8_t old, uint8_t next)
{
    t_phase();
    REQUIRE(binds == 2 && calls < 2 && p == t_fields[calls][calls] &&
            old == (calls ? 67 : 19) && next == (calls ? 19 : 88));
    REQUIRE(peek(t_fields[0][0]) == 88 && peek(t_fields[0][1]) == 149 &&
            peek(t_fields[1][0]) == 222 &&
            peek(t_fields[1][1]) == (calls ? 19 : 67));
    observed_old[calls] = old;
    observed_new[calls] = peek(p);
    observed_sibling[calls] = peek(t_fields[calls][1 - calls]);
    ++calls;
    if (calls == 2)
        for (size_t i = 0; i < 2; ++i)
            for (size_t j = 0; j < 2; ++j)
                final_t[i][j] = peek(t_fields[i][j]);
}
static void trial(void *p, size_t bytes, size_t alignment)
{
    if (P1045_PHASE == 0)
        REQUIRE(binds == 2 && calls == 2 && peek(t_fields[0][0]) == 88 &&
                peek(t_fields[0][1]) == 149 && peek(t_fields[1][0]) == 222 &&
                peek(t_fields[1][1]) == 19);
    else
        REQUIRE(!binds && !calls);
    REQUIRE(trials < 2 && bytes == 24 && alignment == 8 &&
            test_malloc_calls() == trials + 1 &&
            (uintptr_t)p == test_allocated_address(trials));
    allocated[trials++] = (uintptr_t)p;
}
static void initialize(const nl_node *p, size_t domain)
{
    size_t i = original(p);
    REQUIRE(i == initialized++ && !p->f0.tag && !p->f0.ptr &&
            p->f1 == (i ? P1045_H1 : P1045_H0) && domain);
    domains[i] = domain;
    if (i)
        REQUIRE(allocated[0] != allocated[1] && domains[0] != domain);
}
static void root_ref(const nl_node *p, size_t domain, unsigned write,
                     size_t root, size_t incarnation, size_t scope)
{
    size_t i = original(p);
    REQUIRE(domain == domains[i] && root && incarnation && scope && !active);
    if (write)
        REQUIRE(i == 0 && !in_receiver && changes < 2);
    else {
        REQUIRE(i == 1 && changes == 2 && received == 1);
        if (in_receiver)
            ++callee_reads;
        else
            ++caller_reads;
    }
    active = scope;
}
static void scope_end(size_t scope)
{
    REQUIRE(active && active == scope);
    active = 0;
}
static void change(const nl_node_option *field, const nl_node_option *old,
                   size_t step)
{
    REQUIRE((uintptr_t)field == allocated[0] && active && step == changes + 1);
    if (!changes)
        REQUIRE(!old->tag && !old->ptr && field->tag == 1 &&
                (uintptr_t)field->ptr == allocated[1]);
    else
        REQUIRE(changes == 1 && old->tag == 1 &&
                (uintptr_t)old->ptr == allocated[1] && !field->tag &&
                !field->ptr);
    ++changes;
}
static void packet(nl_live_tail p)
{
    REQUIRE((uintptr_t)p.ptr == allocated[1] &&
            (uintptr_t)p.allocation.handle == allocated[1] &&
            p.domain.token == domains[1]);
}
static void send_owner(const nl_node *p, void *a, size_t d,
                       const nl_allocation *old_a, const nl_domain *old_d)
{
    if (P1045_PHASE == 1)
        REQUIRE(!binds && !calls);
    else
        REQUIRE(binds == 2 && calls == 2);
    REQUIRE(sent++ == 0 && (uintptr_t)p == allocated[1] &&
            (uintptr_t)a == allocated[1] && d == domains[1] && !old_a->handle &&
            !old_d->token);
}
static void enter_owner(nl_node_option *edge, const nl_node *p, void *a,
                        size_t d)
{
    REQUIRE(entered++ == 0 && (uintptr_t)edge == allocated[0] &&
            edge->tag == 1 && (uintptr_t)edge->ptr == allocated[1] &&
            (uintptr_t)p == allocated[1] && (uintptr_t)a == allocated[1] &&
            d == domains[1]);
}
static void local_packet(const nl_live_tail *p)
{
    REQUIRE(local_owner++ == 0);
    packet(*p);
}
static void return_owner(nl_live_tail p)
{
    REQUIRE(returned++ == 0 && changes == 2);
    packet(p);
}
static void receive_owner(nl_live_tail p, const nl_allocation *a,
                          const nl_domain *d)
{
    REQUIRE(received++ == 0 && !a->handle && !d->token);
    packet(p);
}
static void handoff(const nl_node *p, void *a, size_t d,
                    const nl_allocation *old_a, const nl_domain *old_d)
{
    REQUIRE(calls == 2 && !active && (uintptr_t)p == allocated[1] &&
            (uintptr_t)a == allocated[1] && d == domains[1] && !old_a->handle &&
            !old_d->token);
}
static void receiver(const nl_node *p, void *a, size_t d)
{
    REQUIRE(terminals++ == 0 && !in_receiver && (uintptr_t)p == allocated[1] &&
            (uintptr_t)a == allocated[1] && d == domains[1]);
    in_receiver = true;
}
static void receiver_exit(void)
{
    REQUIRE(in_receiver && callee_reads == 1);
    in_receiver = false;
}
static void end_root(const nl_node *p, size_t domain)
{
    size_t i = original(p);
    REQUIRE(!ended[i] && domain == domains[i] && !active && !p->f0.tag &&
            !p->f0.ptr && p->f1 == (i ? P1045_H1 : P1045_H0));
    if (allocated[1])
        REQUIRE(changes == 2 &&
                (i ? in_receiver : (!in_receiver && frees == 1)));
    ended[i] = true;
}
static void finalize(size_t domain)
{
    size_t i = domain == domains[0] ? 0 : 1;
    REQUIRE(domain && domain == domains[i] && ended[i] && !finalized[i]);
    finalized[i] = true;
}
static void release(void *allocation, unsigned char *bytes, size_t length)
{
    size_t i = original(allocation);
    REQUIRE((uintptr_t)bytes == allocated[i] && length == 24 && ended[i] &&
            finalized[i] && !releasing[i]);
    releasing[i] = true;
}
void test_observe_free(uintptr_t address)
{
    size_t i = original((void *)address);
    REQUIRE(releasing[i] && !freed[i] &&
            (i ? (in_receiver && !frees)
               : (!in_receiver && frees == (allocated[1] ? 1u : 0u))));
    freed[i] = true;
    ++frees;
}
static void finish(void)
{
    size_t successes = !!allocated[0] + !!allocated[1];
    const size_t t_reached = P1045_PHASE == 0 || successes == 2;
    REQUIRE(binds == 2 * t_reached && calls == 2 * t_reached &&
            (!t_reached || (final_t[0][0] == 88 && final_t[0][1] == 149 &&
                            final_t[1][0] == 222 && final_t[1][1] == 19)) &&
            !active && frees == successes && frees == test_free_calls() &&
            trials == (successes ? 2u : 1u));
    if (successes == 2)
        REQUIRE(sent == 1 && entered == 1 && local_owner == 1 &&
                returned == 1 && received == 1 && terminals == 1 &&
                caller_reads == 1 && callee_reads == 1 && changes == 2 &&
                !in_receiver);
    else
        REQUIRE(!sent && !entered && !local_owner && !returned && !received &&
                !terminals && !caller_reads && !callee_reads && !changes);
    for (size_t i = 0; i < successes; ++i)
        REQUIRE(ended[i] && finalized[i] && releasing[i] && freed[i]);
    printf("P1045 physical old=%u,%u new=%u,%u sibling=%u,%u calls=%zu "
           "T_observed=%zu separate_originals=YES live_original_reads=YES "
           "T0=%" PRIxPTR ",%" PRIxPTR " T1=%" PRIxPTR ",%" PRIxPTR
           " allocations=%zu frees=%zu trials=%zu live_tail=%zu "
           "original_H=YES teardown=YES phase=%d\n",
           observed_old[0], observed_old[1], observed_new[0], observed_new[1],
           observed_sibling[0], observed_sibling[1], calls, t_reached,
           t_addresses[0][0], t_addresses[0][1], t_addresses[1][0],
           t_addresses[1][1], successes, frees, trials, terminals, P1045_PHASE);
}
#define NL_MIXED_COPY_BIND(f, s, a, b) bind_copy(f, s, a, b)
#define NL_MIXED_COPY_CALL(p, o, n) copy_call(p, o, n)
#define NL_MIXED_COPY_STORE(...) abort()
#define NL_MIXED_OWNER_LOCAL(p) local_packet(p)
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
#define NL_HEAP_FINALIZE(d) finalize(d)
#define NL_HEAP_RELEASE(a, b, n) release(a, b, n)
#define NL_HEAP_FINISH() finish()
#define NL_HEAP_ARM(...) ((void)0)
#define NL_HEAP_DOMAIN(...) ((void)0)
#define NL_HEAP_SLOT(...) ((void)0)
#define NL_HEAP_RELOAN(...) ((void)0)
#define NL_HEAP_FIELD(...) ((void)0)
#define NL_HEAP_COPY(...) ((void)0)
#define NL_HEAP_RAW(...) ((void)0)
#define NL_HEAP_RETURNED(...) ((void)0)
