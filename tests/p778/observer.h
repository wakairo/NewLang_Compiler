/* Passive assertions only. No initialization, stores, ownership repair, heap
 * allocation or guest-state mutation. The separate platform wrapper supplies
 * only NULL injection and delegates successful malloc/free to libc. */
#include <stdbool.h>
#include <stdio.h>
#define VERIFY(x)                                                              \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "P778 observer line%d\n", __LINE__);               \
            abort();                                                           \
        }                                                                      \
    } while (0)
size_t test_malloc_calls(void);
size_t test_free_calls(void);
uintptr_t test_allocated_address(size_t);
static uintptr_t addresses[2];
static const nl_node *nodes[2];
static size_t phases[2], domains[2], selected, trials, frees, changes, active,
    scopes, root_refs;
static size_t sent, entered, returned, received, receiver_enter, receiver_exit,
    local_bind;
static bool in_producer, in_receiver;
static const nl_live_tail *local_packet, *caller_packet;
static size_t caller_hold;
static uint8_t *tfields[2][2];
static size_t troots, tstores, tcalls;
static const unsigned tinitial[2][2] = {{T00, T01}, {T10, T11}};
static const unsigned tfinal[2][2] = {{F00, F01}, {F10, F11}};
static const unsigned old_values[2] = {OLD0, OLD1},
                      new_values[2] = {NEW0, NEW1};
static size_t region(const void *p)
{
    for (size_t i = 0; i < 2; ++i)
        if ((uintptr_t)p == addresses[i] && p)
            return i;
    abort();
}
static void copy_bind(size_t frame, size_t symbol, uint8_t *a, uint8_t *b)
{
    (void)frame;
    (void)symbol;
    VERIFY(troots < 2 && a != b && !trials);
    size_t i = troots++;
    tfields[i][0] = a;
    tfields[i][1] = b;
    VERIFY(*a == tinitial[i][0] && *b == tinitial[i][1]);
    if (i)
        VERIFY(a != tfields[0][0] && a != tfields[0][1] && b != tfields[0][0] &&
               b != tfields[0][1]);
}
static void copy_store(uint8_t *field, uint8_t old, uint8_t next, bool call)
{
    VERIFY(troots == 2 && tstores < 2 && !trials);
    size_t r = tstores, index = r ? INDEX1 : INDEX0;
    VERIFY(field == tfields[r][index] && old == old_values[r] &&
           next == new_values[r] && *field == next);
    VERIFY(*tfields[r][1 - index] == tinitial[r][1 - index]);
    ++tstores;
    tcalls += call;
}
static void trial(void *p, size_t n, size_t a)
{
    VERIFY(trials < 2 && tstores == 2 && n == 24 && a == 8 &&
           test_malloc_calls() == trials + 1);
    selected = trials;
    addresses[trials] = (uintptr_t)p;
    VERIFY(addresses[trials] == test_allocated_address(trials));
    phases[trials++] = p ? 1 : 99;
}
static void arm(size_t v)
{
    VERIFY(v == (addresses[selected] ? 2u : 1u));
}
static void slot(const void *p, size_t n)
{
    size_t i = region(p);
    VERIFY(phases[i] == 1 && n == 24);
    phases[i] = 2;
    selected = i;
}
static void domain(size_t d)
{
    VERIFY(phases[selected] == 2 && d == (selected ? TD : HD));
    domains[selected] = d;
    phases[selected] = 3;
}
static void initialize(const nl_node *p, size_t d)
{
    size_t i = region(p);
    VERIFY(phases[i] == 3 && d == domains[i] && p->f0.tag == 0 &&
           p->f0.ptr == NULL && p->f1 == (i ? TV : HV));
    nodes[i] = p;
    phases[i] = 4;
    if (i)
        VERIFY(addresses[0] != addresses[1] && domains[0] != domains[1]);
}
static void root_ref(const nl_node *p, size_t d, unsigned write, size_t root,
                     size_t inc, size_t scope)
{
    size_t i = region(p);
    VERIFY(phases[i] == 4 && !active && scope && d == domains[i]);
    VERIFY(root == (i ? TP : HP) && inc == (i ? TI : HI) &&
           write == (i ? 0u : 1u));
    const size_t expected_scopes[4] = {S0, S1, S2, S3};
    VERIFY(root_refs < 4 && scope == expected_scopes[root_refs]);
    active = scope;
    ++root_refs;
}
static void field_ref(const nl_node *p, const nl_node_option *f, unsigned w,
                      size_t nominal, size_t index, size_t child, size_t inc,
                      size_t scope)
{
    VERIFY(nominal == HN && child == HC && inc == HCI);
    VERIFY(p == nodes[0] && f == &nodes[0]->f0 && w == 1 && index == 0 &&
           child && inc && scope == active);
}
static void change(const nl_node_option *f, const nl_node_option *old,
                   size_t step)
{
    VERIFY(phases[0] == 4 && phases[1] == 4 && f == &nodes[0]->f0 && old != f &&
           active && step == changes + 1);
    if (!changes)
        VERIFY(!in_producer && old->tag == 0 && old->ptr == NULL &&
               f->tag == 1 && f->ptr == nodes[1]);
    else
        VERIFY(in_producer && changes == 1 && old->tag == 1 &&
               old->ptr == nodes[1] && f->tag == 0 && f->ptr == NULL);
    ++changes;
}
static void scope_end(size_t s)
{
    VERIFY(active == s && s);
    active = 0;
    ++scopes;
}
static void packet(const nl_live_tail *p)
{
    VERIFY(p->ptr == nodes[1] && p->allocation.handle == (void *)nodes[1] &&
           p->domain.token == domains[1]);
}
static void live_send(const nl_node *p, void *a, size_t d,
                      const nl_allocation *old_a, const nl_domain *old_d)
{
    VERIFY(sent++ == 0 && p == nodes[1] && a == (void *)p && d == domains[1] &&
           old_a->handle == NULL && old_d->token == 0);
}
static void live_enter(nl_node_option *edge, const nl_node *p, void *a,
                       size_t d)
{
    VERIFY(entered++ == 0 && edge == &nodes[0]->f0 && edge->ptr == p &&
           p == nodes[1] && a == (void *)p && d == domains[1]);
    in_producer = true;
}
static void owner_local(const nl_live_tail *p)
{
    VERIFY(in_producer && local_bind++ == 0);
    local_packet = p;
    packet(p);
}
static void p774_binding(size_t frame, size_t symbol, size_t kind,
                         const void *p)
{
    (void)frame;
    if (kind == 4 && symbol == CALLER_LOCAL) {
        VERIFY(caller_hold++ == 0 && !in_producer && received == 1);
        caller_packet = p;
        packet(caller_packet);
    }
}
static void live_return(nl_live_tail p)
{
    VERIFY(in_producer && returned++ == 0 && changes == 2 && local_bind == 1 &&
           local_packet);
    packet(&p);
    VERIFY(local_packet->ptr == NULL &&
           local_packet->allocation.handle == NULL &&
           local_packet->domain.token == 0);
    in_producer = false;
}
static void live_receive(nl_live_tail p, const nl_allocation *old_a,
                         const nl_domain *old_d)
{
    VERIFY(returned == 1 && received++ == 0 && old_a->handle == NULL &&
           old_d->token == 0);
    packet(&p);
}
static void handoff(const nl_node *p, void *a, size_t d,
                    const nl_allocation *old_a, const nl_domain *old_d)
{
    VERIFY(caller_hold == 1 && caller_packet && !caller_packet->ptr &&
           !caller_packet->allocation.handle && !caller_packet->domain.token);
    VERIFY(received == 1 && !active && p == nodes[1] && a == (void *)p &&
           d == domains[1] && old_a->handle == NULL && old_d->token == 0);
}
static void receiver(const nl_node *p, void *a, size_t d, size_t frame,
                     size_t ps, size_t as, size_t ds, const nl_allocation *ap,
                     const nl_domain *dp)
{
    (void)frame;
    (void)ps;
    (void)as;
    (void)ds;
    VERIFY(receiver_enter++ == 0 && p == nodes[1] && a == (void *)p &&
           d == domains[1] && ap->handle == a && dp->token == d);
    in_receiver = true;
}
static void receiver_leave(void)
{
    VERIFY(in_receiver && receiver_exit++ == 0 && phases[1] == 9);
    in_receiver = false;
}
static void caller_returned(const nl_allocation *a, const nl_domain *d)
{
    VERIFY(!in_receiver && receiver_exit == 1 && a->handle == NULL &&
           d->token == 0);
}
static void end(const nl_node *p, size_t d)
{
    size_t i = region(p);
    VERIFY(phases[i] == 4 && !active && d == domains[i] &&
           p->f1 == (i ? TV : HV) && p->f0.tag == 0 && p->f0.ptr == NULL);
    if (addresses[1])
        VERIFY(changes == 2 && received == 1 &&
               (i ? in_receiver : (!in_receiver && phases[1] == 9)));
    phases[i] = 5;
}
static void raw(const void *p, size_t n)
{
    size_t i = region(p);
    VERIFY(phases[i] == 5 && n == 24);
    selected = i;
    phases[i] = 6;
}
static void finalize(size_t d)
{
    VERIFY(phases[selected] == 6 && d == domains[selected]);
    phases[selected] = 7;
}
static void release(const void *p, const void *bytes, size_t n)
{
    size_t i = region(p);
    VERIFY(phases[i] == 7 && p == bytes && n == 24);
    phases[i] = 8;
}
void test_observe_free(uintptr_t p)
{
    size_t i = region((void *)p);
    VERIFY(phases[i] == 8 && (i ? in_receiver : !in_receiver));
    phases[i] = 9;
    ++frees;
}
static void finish(void)
{
    VERIFY(!active && troots == 2 && tstores == 2 && tcalls == COPY_CALLS &&
           test_free_calls() == frees);
    for (size_t r = 0; r < 2; ++r)
        for (size_t i = 0; i < 2; ++i)
            VERIFY(*tfields[r][i] == tfinal[r][i]);
    size_t allocated = !!addresses[0] + !!addresses[1];
    VERIFY(frees == allocated);
    if (allocated == 2)
        VERIFY(sent == 1 && entered == 1 && returned == 1 && received == 1 &&
               receiver_enter == 1 && receiver_exit == 1 && local_bind == 1 &&
               root_refs == 4 && scopes == 4 && caller_hold == 1);
    else
        VERIFY(!sent && !entered && !returned && !received && !changes &&
               !local_bind && !caller_hold);
    for (size_t i = 0; i < 2; ++i)
        if (addresses[i])
            VERIFY(phases[i] == 9);
    printf("OBSERVED T=%u,%u/%u,%u CopyCalls=%zu trials=%zu allocations=%zu "
           "frees=%zu producer=%zu local_consumed=%zu returned=%zu "
           "received=%zu H=%lx,%lx\n",
           *tfields[0][0], *tfields[0][1], *tfields[1][0], *tfields[1][1],
           tcalls, trials, allocated, frees, entered, local_bind, returned,
           received, (unsigned long)addresses[0], (unsigned long)addresses[1]);
}
#define NL_MIXED_COPY_BIND(f, s, a, b) copy_bind(f, s, a, b)
#define NL_MIXED_COPY_CALL(p, o, n) copy_store(p, o, n, true)
#define NL_MIXED_COPY_STORE(p, o, n) copy_store(p, o, n, false)
#define NL_HEAP_TRIAL(p, n, a) trial(p, n, a)
#define NL_HEAP_ARM(v) arm(v)
#define NL_HEAP_SLOT(p, n) slot(p, n)
#define NL_HEAP_DOMAIN(d) domain(d)
#define NL_HEAP_INITIALIZE(p, d) initialize(p, d)
#define NL_HEAP_ROOT(p, d, w, r, i, s) root_ref(p, d, w, r, i, s)
#define NL_HEAP_FIELD(p, f, w, n, i, c, x, s) field_ref(p, f, w, n, i, c, x, s)
#define NL_HEAP_CHANGE(f, o, s) change(f, o, s)
#define NL_HEAP_SCOPE_END(s) scope_end(s)
#define NL_HEAP_END(p, d) end(p, d)
#define NL_HEAP_RAW(p, n) raw(p, n)
#define NL_HEAP_FINALIZE(d) finalize(d)
#define NL_HEAP_RELEASE(p, b, n) release(p, b, n)
#define NL_HEAP_FINISH() finish()
#define NL_HEAP_HANDOFF(p, a, d, ap, dp) handoff(p, a, d, ap, dp)
#define NL_HEAP_RECEIVER_ENTER(p, a, d, f, ps, as, ds, ap, dp)                 \
    receiver(p, a, d, f, ps, as, ds, ap, dp)
#define NL_HEAP_RECEIVER_EXIT() receiver_leave()
#define NL_HEAP_RETURNED(a, d) caller_returned(a, d)
#define NL_LIVE_SEND(p, a, d, ap, dp) live_send(p, a, d, ap, dp)
#define NL_LIVE_ENTER(e, p, a, d) live_enter(e, p, a, d)
#define NL_LIVE_RETURN(p) live_return(p)
#define NL_LIVE_RECEIVE(p, a, d) live_receive(p, a, d)

#define NL_MIXED_OWNER_LOCAL(p) owner_local(p)
