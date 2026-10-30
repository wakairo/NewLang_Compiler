/* P1036 passive original-carrier observations. The P-owned H lifecycle
 * observer pattern is reused; dual-field T values/addresses are new. */
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define REQUIRE(x)                                                             \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "P1036 physical:%d: %s\n", __LINE__, #x);          \
            abort();                                                           \
        }                                                                      \
    } while (0)
size_t test_malloc_calls(void);
size_t test_free_calls(void);
uintptr_t test_allocated_address(size_t);
#ifndef P1036_FIRST_INDEX
#error P1036_FIRST_INDEX required from the frozen source profile
#endif
#ifndef P1036_THIRD_RESULT
#error P1036_THIRD_RESULT required from actual public binding and emitted C name
#endif
#if P1036_FIRST_INDEX == 2
static const unsigned t_initial[2] = {41, 173}, t_indices[3] = {0, 0, 0},
                      t_old[3] = {41, 67, 109}, t_next[3] = {67, 109, 151},
                      t_sibling[3] = {173, 173, 173}, t_final[2] = {151, 173};
#define P1036_H0 23
#define P1036_H1 199
#elif P1036_FIRST_INDEX == 0
static const unsigned t_initial[2] = {37, 173}, t_indices[3] = {0, 1, 0},
                      t_old[3] = {37, 173, 94}, t_next[3] = {94, 37, 173},
                      t_sibling[3] = {173, 94, 37}, t_final[2] = {173, 37};
#define P1036_H0 61
#define P1036_H1 209
#elif P1036_FIRST_INDEX == 1
static const unsigned t_initial[2] = {37, 173}, t_indices[3] = {1, 0, 1},
                      t_old[3] = {173, 37, 94}, t_next[3] = {94, 173, 37},
                      t_sibling[3] = {37, 94, 173}, t_final[2] = {173, 37};
#define P1036_H0 61
#define P1036_H1 209
#else
#error invalid P1036_FIRST_INDEX
#endif
static uint8_t *t_fields[2];
static uintptr_t t_addresses[2], allocated[2];
static size_t domains[2], binds, calls, trials, initialized, changes, active;
static size_t sent, entered, local_owner, returned, received, terminals, frees;
static size_t caller_reads, callee_reads;
static bool in_receiver, ended[2], finalized[2], releasing[2], freed[2];
static unsigned observed_old[3], observed_new[3], observed_sibling[3];
static size_t returned_results;
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
    REQUIRE(binds++ == 0 && !trials && a != b && peek(a) == t_initial[0] &&
            peek(b) == t_initial[1]);
    t_fields[0] = a;
    t_fields[1] = b;
    t_addresses[0] = (uintptr_t)a;
    t_addresses[1] = (uintptr_t)b;
}
static void copy_call(uint8_t *p, uint8_t old, uint8_t next)
{
    REQUIRE(calls < 3);
    const size_t index = t_indices[calls];
    REQUIRE(binds == 1 && !trials && p == t_fields[index]);
    REQUIRE(old == t_old[calls] && next == t_next[calls] && peek(p) == next &&
            peek(t_fields[1 - index]) == t_sibling[calls]);
    observed_old[calls] = old;
    observed_new[calls] = peek(p);
    observed_sibling[calls++] = peek(t_fields[1 - index]);
}
static void trial(void *p, size_t bytes, size_t alignment)
{
    REQUIRE(binds == 1 && calls == 3 && peek(t_fields[0]) == t_final[0] &&
            peek(t_fields[1]) == t_final[1] && trials < 2 && bytes == 24 &&
            alignment == 8);
    REQUIRE(test_malloc_calls() == trials + 1 &&
            (uintptr_t)p == test_allocated_address(trials));
    allocated[trials++] = (uintptr_t)p;
}
static void initialize(const nl_node *p, size_t domain)
{
    size_t i = original(p);
    REQUIRE(i == initialized++ && !p->f0.tag && !p->f0.ptr &&
            p->f1 == (i ? P1036_H1 : P1036_H0) && domain);
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
    REQUIRE(calls == 3 && !active && (uintptr_t)p == allocated[1] &&
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
            !p->f0.ptr && p->f1 == (i ? P1036_H1 : P1036_H0));
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
    REQUIRE(binds == 1 && calls == 3 && !active && frees == successes &&
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
    REQUIRE(returned_results == trials);
    printf(
        "P1036 physical old=%u,%u,%u new=%u,%u,%u sibling=%u,%u,%u calls=%zu "
        "indices=%u,%u,%u same_original_T=YES live_original_reads=YES "
        "third_actual_return=%u field0=%" PRIxPTR " field1=%" PRIxPTR
        " allocations=%zu frees=%zu trials=%zu live_tail=%zu "
        "original_H=YES teardown=YES\n",
        observed_old[0], observed_old[1], observed_old[2], observed_new[0],
        observed_new[1], observed_new[2], observed_sibling[0],
        observed_sibling[1], observed_sibling[2], calls, t_indices[0],
        t_indices[1], t_indices[2], t_old[2], t_addresses[0], t_addresses[1],
        successes, frees, trials, terminals);
}
#define NL_MIXED_COPY_BIND(f, s, a, b) bind_copy(f, s, a, b)
#define NL_MIXED_COPY_CALL(p, o, n) copy_call(p, o, n)
#define NL_MIXED_COPY_STORE(...) abort()
#define NL_MIXED_OWNER_LOCAL(p) local_packet(p)
/* This expands in main, where the actual caller's third old-result binding
 * is alive. Checking the helper-local old before return alone is insufficient.
 */
#define NL_HEAP_TRIAL(p, n, a)                                                 \
    do {                                                                       \
        REQUIRE((P1036_THIRD_RESULT) == t_old[2]);                             \
        ++returned_results;                                                    \
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
