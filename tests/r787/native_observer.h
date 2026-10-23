/* R787 passive legacy A regression: real host malloc/free and OOM worlds. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
size_t test_malloc_calls(void);
size_t test_free_calls(void);
uintptr_t test_allocated_address(size_t);
static void r787_finish(void)
{
    const char *fault = getenv("NEWLANG_TEST_FAIL_CALL");
    char f = fault ? fault[0] : '0';
    size_t requests = f == '1' ? 1u : 2u;
    size_t frees = f == '1' ? 0u : f == '2' ? 1u : 2u;
    if (test_malloc_calls() != requests || test_free_calls() != frees ||
        (f == '0' &&
         (!test_allocated_address(0) || !test_allocated_address(1) ||
          test_allocated_address(0) == test_allocated_address(1))) ||
        (f == '2' && !test_allocated_address(0))) {
        fprintf(stderr, "R787 legacy original heap accounting mismatch\n");
        abort();
    }
    printf("R787_RA_NATIVE requests=%zu frees=%zu fail=%c\n", requests, frees,
           f);
}
#define NL_HEAP_FINISH() r787_finish()

/* Historical R-A hooks: passive observations, never guest-state repair. */
static uintptr_t r787_tail_address;
static size_t r787_tail_domain, r787_sent, r787_received, r787_handed;

static void r787_field(const nl_node *p, const nl_node_option *f,
                       unsigned w, size_t n, size_t i, size_t c, size_t x,
                       size_t s)
{
    if (!p || (uintptr_t)p != test_allocated_address(0) || f != &p->f0 ||
        w != 1 || !n || i != 0 || !c || !x || !s)
        abort();
}

static void r787_send(const nl_node *p, const void *a, size_t d,
                      const nl_allocation *old_a, const nl_domain *old_d)
{
    if (!p || (uintptr_t)p != test_allocated_address(1) ||
        a != (const void *)p || !d || !old_a || !old_d ||
        old_a->handle != NULL || old_d->token != 0 || r787_sent++)
        abort();
    r787_tail_address = (uintptr_t)p;
    r787_tail_domain = d;
}

static void r787_receive(nl_live_tail v, const nl_allocation *old_a,
                         const nl_domain *old_d)
{
    if (r787_sent != 1 || r787_received++ || !old_a || !old_d ||
        (uintptr_t)v.ptr != r787_tail_address ||
        (uintptr_t)v.allocation.handle != r787_tail_address ||
        v.domain.token != r787_tail_domain || old_a->handle != NULL ||
        old_d->token != 0)
        abort();
}

static void r787_handoff(const nl_node *p, const void *a, size_t d,
                         const nl_allocation *old_a, const nl_domain *old_d)
{
    if (r787_received != 1 || r787_handed++ || !p ||
        (uintptr_t)p != r787_tail_address ||
        (uintptr_t)a != r787_tail_address || d != r787_tail_domain ||
        !old_a || !old_d || old_a->handle != NULL || old_d->token != 0)
        abort();
}

static void r787_returned(const nl_allocation *a, const nl_domain *d)
{
    if (r787_handed != 1 || !a || !d || a->handle != NULL || d->token != 0)
        abort();
}

#define NL_HEAP_FIELD(...) r787_field(__VA_ARGS__)
#define NL_LIVE_SEND(...) r787_send(__VA_ARGS__)
#define NL_LIVE_RECEIVE(...) r787_receive(__VA_ARGS__)
#define NL_HEAP_HANDOFF(...) r787_handoff(__VA_ARGS__)
#define NL_HEAP_RETURNED(...) r787_returned(__VA_ARGS__)
