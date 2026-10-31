/* P1055 passive original-carrier observations. The P-owned H lifecycle
 * observer pattern is reused; dual-field T values/addresses are new. */
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define REQUIRE(x)                                                             \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "P1055 physical:%d: %s\n", __LINE__, #x);          \
            abort();                                                           \
        }                                                                      \
    } while (0)
size_t test_malloc_calls(void);
size_t test_free_calls(void);
uintptr_t test_allocated_address(size_t);
#ifndef P1055_T0
#define P1055_T0 83
#define P1055_T1 211
#define P1055_FIRST_INDEX 0
#define P1055_NEXT0 146
#define P1055_NEXT1 83
#define P1055_H0 71
#define P1055_H1 193
#endif
#ifndef P1055_RESULT0
#error actual caller old result bindings required
#endif
#ifndef P1055_RESULT1
#error actual caller second old result binding required
#endif
static size_t result_checks;
static uint8_t *t_fields[2];
static uintptr_t t_addresses[2], allocated[2];
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
static void bind_copy(size_t frame, size_t symbol, uint8_t *a, uint8_t *b)
{
    (void)frame;
    (void)symbol;
    REQUIRE(binds++ == 0 && !trials && a != b && peek(a) == P1055_T0 &&
            peek(b) == P1055_T1);
    t_fields[0] = a;
    t_fields[1] = b;
    t_addresses[0] = (uintptr_t)a;
    t_addresses[1] = (uintptr_t)b;
}
static void copy_call(uint8_t *p, uint8_t old, uint8_t next)
{
    const size_t index = calls ? 1 - P1055_FIRST_INDEX : P1055_FIRST_INDEX;
    REQUIRE(binds == 1 && calls < 2 && !trials && p == t_fields[index]);
    REQUIRE(old == (calls ? (P1055_FIRST_INDEX ? P1055_T0 : P1055_T1)
                          : (P1055_FIRST_INDEX ? P1055_T1 : P1055_T0)) &&
            next == (calls ? P1055_NEXT1 : P1055_NEXT0));
    REQUIRE(
        peek(p) == next &&
        peek(t_fields[1 - index]) ==
            (calls ? P1055_NEXT0 : (P1055_FIRST_INDEX ? P1055_T0 : P1055_T1)));
    observed_old[calls] = old;
    observed_new[calls] = peek(p);
    observed_sibling[calls++] = peek(t_fields[1 - index]);
}
static void trial(void *p, size_t bytes, size_t alignment)
{
    REQUIRE(binds == 1 && calls == 2 &&
            peek(t_fields[P1055_FIRST_INDEX]) == P1055_NEXT0 &&
            peek(t_fields[1 - P1055_FIRST_INDEX]) == P1055_NEXT1 &&
            trials < 2 && bytes == 24 && alignment == 8);
    REQUIRE(test_malloc_calls() == trials + 1 &&
            (uintptr_t)p == test_allocated_address(trials));
    allocated[trials++] = (uintptr_t)p;
}
static void initialize(const nl_node *p, size_t domain)
{
    size_t i = original(p);
    REQUIRE(i == initialized++ && !p->f0.tag && !p->f0.ptr &&
            p->f1 == (i ? P1055_H1 : P1055_H0) && domain);
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
            !p->f0.ptr && p->f1 == (i ? P1055_H1 : P1055_H0));
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
    REQUIRE(binds == 1 && calls == 2 && !active && frees == successes &&
            frees == test_free_calls() && trials == (successes ? 2u : 1u));
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
    REQUIRE(result_checks == trials);
    printf("P1055 physical actual_caller_returns=YES old=%u,%u new=%u,%u "
           "sibling=%u,%u calls=%zu "
           "indices=%zu,%zu same_original_T=YES live_original_reads=YES "
           "field0=%" PRIxPTR " field1=%" PRIxPTR
           " allocations=%zu frees=%zu trials=%zu live_tail=%zu "
           "original_H=YES teardown=YES\n",
           observed_old[0], observed_old[1], observed_new[0], observed_new[1],
           observed_sibling[0], observed_sibling[1], calls,
           (size_t)P1055_FIRST_INDEX, (size_t)(1 - P1055_FIRST_INDEX),
           t_addresses[0], t_addresses[1], successes, frees, trials, terminals);
}
#define NL_MIXED_COPY_BIND(f, s, a, b) bind_copy(f, s, a, b)
#define NL_MIXED_COPY_CALL(p, o, n) copy_call(p, o, n)
#define NL_MIXED_COPY_STORE(...) abort()
#define NL_MIXED_OWNER_LOCAL(p) local_packet(p)
/* These names expand only in main while both real caller bindings live. */
#define NL_HEAP_TRIAL(p, n, a)                                                 \
    do {                                                                       \
        REQUIRE(P1055_RESULT0 == (P1055_FIRST_INDEX ? P1055_T1 : P1055_T0));   \
        REQUIRE(P1055_RESULT1 == (P1055_FIRST_INDEX ? P1055_T0 : P1055_T1));   \
        ++result_checks;                                                       \
        trial(p, n, a);                                                        \
    } while (0)
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
