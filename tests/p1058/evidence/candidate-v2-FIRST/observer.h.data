/* C148 passive all-H-closed two-original T and actual-return witnesses. */
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define REQUIRE(x)                                                             \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "P1058 physical:%d: %s\n", __LINE__, #x);          \
            abort();                                                           \
        }                                                                      \
    } while (0)
size_t test_malloc_calls(void);
size_t test_free_calls(void);
uintptr_t test_allocated_address(size_t);
#ifndef P1058_T00
#error frozen T/H observation parameters required
#endif
static const unsigned initial_t[2][2] = {{P1058_T00, P1058_T01},
                                         {P1058_T10, P1058_T11}};
static const unsigned final_expected[2][2] = {{P1058_F00, P1058_F01},
                                              {P1058_F10, P1058_F11}};
static const unsigned old_expected[2] = {P1058_OLD0, P1058_OLD1};
static const unsigned next_expected[2] = {P1058_NEXT0, P1058_NEXT1};
static const unsigned sibling_expected[2] = {P1058_SIB0, P1058_SIB1};
static const size_t helper_index[2] = {P1058_I0, P1058_I1};
static size_t stores;
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
/* This is called at every T operation: all H branches must have completed
 * their actual cleanup, including both allocation-failure outcomes. */
static void t_phase(void)
{
    size_t successes = !!allocated[0] + !!allocated[1];
    REQUIRE(trials == (successes ? 2u : 1u) && initialized == successes &&
            frees == successes && test_free_calls() == successes && !active &&
            !in_receiver);
    if (successes == 2)
        REQUIRE(changes == 2 && sent == 1 && entered == 1 && local_owner == 1 &&
                returned == 1 && received == 1 && terminals == 1 &&
                caller_reads == 1 && callee_reads == 1);
    else
        REQUIRE(!changes && !sent && !entered && !local_owner && !returned &&
                !received && !terminals && !caller_reads && !callee_reads);
    for (size_t i = 0; i < successes; ++i)
        REQUIRE(ended[i] && finalized[i] && releasing[i] && freed[i]);
}
static void bind_copy(size_t frame, size_t symbol, uint8_t *a, uint8_t *b)
{
    t_phase();
    REQUIRE(binds < 2 && !calls && !stores && a != b &&
            peek(a) == initial_t[binds][0] && peek(b) == initial_t[binds][1]);
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
    REQUIRE(binds == 2 && calls < 2 && !stores &&
            p == t_fields[calls][helper_index[calls]] &&
            old == old_expected[calls] && next == next_expected[calls] &&
            peek(p) == next &&
            peek(t_fields[calls][1 - helper_index[calls]]) ==
                sibling_expected[calls]);
    for (size_t root = 0; root < 2; ++root)
        for (size_t field = 0; field < 2; ++field)
            REQUIRE(peek(t_fields[root][field]) ==
                    ((root <= calls && field == helper_index[root])
                         ? next_expected[root]
                         : initial_t[root][field]));
    observed_old[calls] = old;
    observed_new[calls] = peek(p);
    observed_sibling[calls] = peek(t_fields[calls][1 - helper_index[calls]]);
    ++calls;
}
static void copy_store(uint8_t *p, uint8_t old, uint8_t next)
{
    t_phase();
    REQUIRE(binds == 2 && calls == 2 && stores++ == 0 &&
            p == t_fields[1][P1058_DIRECT_INDEX] && old == P1058_DIRECT_OLD &&
            next == P1058_DIRECT_NEXT && peek(p) == next);
    for (size_t root = 0; root < 2; ++root)
        for (size_t field = 0; field < 2; ++field) {
            final_t[root][field] = peek(t_fields[root][field]);
            REQUIRE(final_t[root][field] == final_expected[root][field]);
        }
}
static void trial(void *p, size_t bytes, size_t alignment)
{
    REQUIRE(!binds && !calls && !stores && trials < 2 && bytes == 24 &&
            alignment == 8 && test_malloc_calls() == trials + 1 &&
            (uintptr_t)p == test_allocated_address(trials));
    allocated[trials++] = (uintptr_t)p;
}
static void initialize(const nl_node *p, size_t domain)
{
    size_t i = original(p);
    REQUIRE(i == initialized++ && !p->f0.tag && !p->f0.ptr &&
            p->f1 == (i ? P1058_H1 : P1058_H0) && domain);
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
    REQUIRE(!binds && !calls && !stores);
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
            !p->f0.ptr && p->f1 == (i ? P1058_H1 : P1058_H0));
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
    t_phase();
    REQUIRE(binds == 2 && calls == 2 && stores == 1);
    for (size_t root = 0; root < 2; ++root)
        for (size_t field = 0; field < 2; ++field)
            REQUIRE(
                peek(t_fields[root][field]) == final_expected[root][field] &&
                final_t[root][field] == final_expected[root][field] &&
                (uintptr_t)t_fields[root][field] == t_addresses[root][field]);
    printf("P1058 physical all_H_closed_before_T=YES actual_caller_returns=YES "
           "T_originals=2 typed_live_fields=YES no_alias=YES calls=2 "
           "direct_writes=1 "
           "old=%u,%u direct_old=%u final=%u,%u;%u,%u "
           "T0=%" PRIxPTR ",%" PRIxPTR " T1=%" PRIxPTR ",%" PRIxPTR
           " allocations=%zu frees=%zu trials=%zu live_tail=%zu "
           "original_H=YES teardown=YES\n",
           observed_old[0], observed_old[1], (unsigned)P1058_DIRECT_OLD,
           final_t[0][0], final_t[0][1], final_t[1][0], final_t[1][1],
           t_addresses[0][0], t_addresses[0][1], t_addresses[1][0],
           t_addresses[1][1], (size_t)!!allocated[0] + (size_t)!!allocated[1],
           frees, trials, terminals);
}
#define NL_MIXED_COPY_BIND(f, s, a, b) bind_copy(f, s, a, b)
#define NL_MIXED_COPY_CALL(p, o, n) copy_call(p, o, n)
#define NL_MIXED_COPY_STORE(p, o, n) copy_store(p, o, n)
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
#define NL_HEAP_FINISH()                                                       \
    do {                                                                       \
        REQUIRE(P1058_RET0 == P1058_OLD0);                                     \
        REQUIRE(P1058_RET1 == P1058_OLD1);                                     \
        REQUIRE(P1058_RET2 == P1058_DIRECT_OLD);                               \
        finish();                                                              \
    } while (0)
#define NL_HEAP_ARM(...) ((void)0)
#define NL_HEAP_DOMAIN(...) ((void)0)
#define NL_HEAP_SLOT(...) ((void)0)
#define NL_HEAP_RELOAN(...) ((void)0)
#define NL_HEAP_FIELD(...) ((void)0)
#define NL_HEAP_COPY(...) ((void)0)
#define NL_HEAP_RAW(...) ((void)0)
#define NL_HEAP_RETURNED(...) ((void)0)
