#ifndef P1217_NATIVE_H
#define P1217_NATIVE_H
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static const uint8_t *p_fields[2];
static size_t p_phase, p_visits, p_stores, p_reads, p_results;
static unsigned p_role_mask;
static const uint8_t p_expected_old[] = P_OLD_VALUES;
static const uint8_t p_expected_next[] = P_NEXT_VALUES;
static const unsigned p_roles[] = {P_ROLE_INCOMING, P_ROLE_OLD, P_ROLE_COPY,
                                   P_ROLE_CURRENT, P_ROLE_SIBLING};
#if P_HAS_HEAP
static struct {
    nl_node *live;
    uintptr_t address;
    bool ended, released;
} p_heap[2];
static size_t p_attempts, p_success, p_frees, p_domains, p_finalized, p_changes;
static bool p_domain_done[2], p_heap_done;
static inline size_t p_budget(void)
{
    const char *s = getenv("P1217_HEAP_BUDGET");
    assert(s);
    size_t n = (size_t)strtoul(s, NULL, 10);
    assert(n <= 2);
    return n;
}
static inline size_t p_live_index(const void *address)
{
    uintptr_t key = (uintptr_t)address;
    for (size_t i = 0; i < p_success; ++i)
        if (p_heap[i].live && p_heap[i].address == key)
            return i;
    assert(0);
    return 0;
}
static inline void *p_allocate(size_t bytes)
{
    assert(bytes == 24);
    ++p_attempts;
    if (p_success == p_budget())
        return NULL;
    void *address = malloc(bytes);
    assert(address && p_success < 2);
    p_heap[p_success].live = address;
    p_heap[p_success].address = (uintptr_t)address;
    ++p_success;
    return address;
}
static inline void p_deallocate(void *address)
{
    size_t i = p_live_index(address);
    assert(p_heap[i].ended && p_heap[i].released);
    p_heap[i].live = NULL;
    ++p_frees;
    free(address);
}
static inline void p_closed_heap(void)
{
    assert(p_success == p_budget() && p_attempts == (p_budget() ? 2 : 1));
    assert(p_frees == p_success && p_domains == p_success &&
           p_finalized == p_success);
    for (size_t i = 0; i < p_success; ++i)
        assert(!p_heap[i].live && p_heap[i].ended && p_heap[i].released &&
               p_domain_done[i]);
    assert(p_changes == (p_success == 2 ? 2 : 0));
}
static inline void p_initialized(const nl_node *node, size_t domain)
{
    size_t i = p_live_index(node);
    assert(domain == i + 1);
    assert(node->f0.tag == 0 && node->f0.ptr == NULL &&
           node->f1 == (i ? 235 : 14));
}
static inline void p_end(const nl_node *node, size_t domain)
{
    size_t i = p_live_index(node);
    assert(!p_heap[i].ended && domain == i + 1);
    assert(node->f0.tag == 0 && node->f0.ptr == NULL &&
           node->f1 == (i ? 235 : 14));
    p_heap[i].ended = true;
}
static inline void p_domain(size_t token)
{
    assert(token == p_domains + 1 && token <= 2);
    ++p_domains;
}
static inline void p_finalize(size_t token)
{
    assert(token && token <= p_domains && !p_domain_done[token - 1]);
    p_domain_done[token - 1] = true;
    ++p_finalized;
}
static inline void p_release(const void *allocation, const void *raw,
                             size_t bytes)
{
    size_t i = p_live_index(allocation);
    assert(allocation == raw && bytes == 24 && p_heap[i].ended &&
           !p_heap[i].released && p_domain_done[i]);
    p_heap[i].released = true;
}
static inline void p_change(nl_node_option *field, const nl_node_option *old)
{
    assert(p_success == 2 && p_heap[0].live && p_heap[1].live &&
           field == &p_heap[0].live->f0);
    if (!p_changes)
        assert(old->tag == 0 && !old->ptr && field->tag == 1 &&
               field->ptr == p_heap[1].live);
    else
        assert(p_changes == 1 && old->tag == 1 && old->ptr == p_heap[1].live &&
               field->tag == 0 && !field->ptr);
    ++p_changes;
}
static inline void p_tail(const nl_node *node, const void *allocation,
                          size_t domain)
{
    assert(p_success == 2 && p_heap[1].live == node && allocation == node &&
           domain == 2 && !p_heap[1].ended);
}
#define malloc(n) p_allocate(n)
#define free(p) p_deallocate(p)
#define NL_HEAP_TRIAL(...) ((void)0)
#define NL_HEAP_ARM(...) ((void)0)
#define NL_HEAP_DOMAIN(t) p_domain(t)
#define NL_HEAP_SLOT(...) ((void)0)
#define NL_HEAP_INITIALIZE(p, d) p_initialized(p, d)
#define NL_HEAP_RELOAN(...) ((void)0)
#define NL_HEAP_ROOT(...) ((void)0)
#define NL_HEAP_FIELD(...) ((void)0)
#define NL_HEAP_COPY(...) ((void)0)
#define NL_HEAP_CHANGE(p, o, ...) p_change(p, o)
#define NL_HEAP_SCOPE_END(...) ((void)0)
#define NL_HEAP_END(p, d) p_end(p, d)
#define NL_HEAP_RAW(...) ((void)0)
#define NL_HEAP_FINALIZE(t) p_finalize(t)
#define NL_HEAP_RELEASE(a, r, n) p_release(a, r, n)
#define NL_HEAP_HANDOFF(p, a, d, ...) p_tail(p, a, d)
#define NL_HEAP_RECEIVER_ENTER(p, a, d, ...) p_tail(p, a, d)
#define NL_HEAP_RECEIVER_EXIT() ((void)0)
#define NL_HEAP_RETURNED(a, d) assert((a)->handle == NULL && (d)->token == 0)
#define NL_LIVE_SEND(p, a, d, ...) p_tail(p, a, d)
#define NL_LIVE_ENTER(h, p, a, d) p_tail(p, a, d)
#define NL_LIVE_RETURN(v)                                                      \
    p_tail((v).ptr, (v).allocation.handle, (v).domain.token)
#define NL_LIVE_RECEIVE(...) ((void)0)
#endif
static inline void p_finish(void)
{
    assert(p_fields[0] && p_fields[1] && p_visits == P_ITERATIONS + 1 &&
           p_results == 1 && p_reads == 2);
    assert(p_stores == (P_NO_WRITE ? 0 : P_ITERATIONS));
#if P_HAS_HEAP
    assert(p_heap_done);
#endif
    puts("P1217 original T/old/current/sibling/real backedges and H cleanup "
         "PASS");
}
static inline void p_bind(const uint8_t *a, const uint8_t *b)
{
    assert(!p_fields[0] && a && b == a + 1 && *a == P_FIRST_VALUE &&
           *b == P_SECOND_VALUE);
    p_fields[0] = a;
    p_fields[1] = b;
    assert(atexit(p_finish) == 0);
#if P_HAS_HEAP
    if (P_PRE_HEAP)
        assert(!p_attempts && !p_success);
    else
        p_closed_heap();
#endif
}
static inline void p_enter(size_t phase, const uint8_t *a, const uint8_t *b)
{
    assert(phase == p_visits && phase <= P_ITERATIONS && a == p_fields[0] &&
           b == p_fields[1]);
#if P_HAS_HEAP
    p_closed_heap();
#endif
    if (phase && !P_NO_WRITE)
        assert(p_role_mask == 31);
    p_phase = phase;
    ++p_visits;
    p_role_mask = 0;
    assert(*p_fields[P_INDEX] ==
           (phase < P_ITERATIONS ? p_expected_old[phase] : P_FINAL_VALUE));
    assert(*p_fields[1 - P_INDEX] == P_SIBLING_VALUE);
}
static inline void p_local(size_t symbol, uintmax_t value)
{
    assert(!P_NO_WRITE && p_phase < P_ITERATIONS);
    for (size_t i = 0; i < 5; ++i)
        if (symbol == p_roles[i]) {
            unsigned bit = 1u << i;
            assert(!(p_role_mask & bit));
            p_role_mask |= bit;
            uintmax_t expected = i == 0 || i == 3 ? p_expected_next[p_phase]
                                 : i == 4         ? P_SIBLING_VALUE
                                                  : p_expected_old[p_phase];
            assert(value == expected);
#if !P_HAS_HEAP
            if (i == 1) {
                assert(p_stores == p_phase &&
                       *p_fields[P_INDEX] == p_expected_next[p_phase]);
                ++p_stores;
            }
#endif
            return;
        }
}
static inline void p_store(uint8_t *field, uint8_t old, uint8_t next)
{
    assert(!P_NO_WRITE && p_phase < P_ITERATIONS && p_stores == p_phase);
    assert(field == p_fields[P_INDEX] && old == p_expected_old[p_phase] &&
           next == p_expected_next[p_phase] && *field == next);
    assert(*p_fields[1 - P_INDEX] == P_SIBLING_VALUE);
    ++p_stores;
}
static inline void p_result(size_t result)
{
    assert(result == P_ITERATIONS && p_visits == P_ITERATIONS + 1 &&
           !p_results);
    ++p_results;
}
static inline void p_read(const uint8_t *field, uint8_t value)
{
    assert(p_results == 1 && p_reads < 2);
    assert(field == p_fields[p_reads ? 1 - P_INDEX : P_INDEX]);
    assert(value == (p_reads ? P_SIBLING_VALUE : P_FINAL_VALUE));
    ++p_reads;
}
#if P_HAS_HEAP
static inline void p_heap_finish(void)
{
    p_closed_heap();
    assert(*p_fields[P_INDEX] == P_FINAL_VALUE &&
           *p_fields[1 - P_INDEX] == P_SIBLING_VALUE);
    p_heap_done = true;
}
#define NL_HEAP_FINISH() p_heap_finish()
#endif
#define NL_MIXED_COPY_BIND(f, s, a, b) p_bind(a, b)
#define NL_MIXED_COPY_STORE(p, o, n) p_store(p, o, n)
#define NL_CURRENT_PHASE(p, a, b) p_enter(p, a, b)
#define NL_CURRENT_LOCAL(s, v) p_local(s, (uintmax_t)(v))
#define NL_CURRENT_RESULT(v) p_result(v)
#define NL_LOOP_BIND(a, b) p_bind(a, b)
#define NL_LOOP_READ(p, v) p_read(p, v)
#define NL_LOOP_BEFORE(...) ((void)0)
#define NL_LOOP_AFTER(...) ((void)0)
#define NL_SCALAR_PHASE(p) p_enter(p, p_fields[0], p_fields[1])
#define NL_SCALAR_RESULT(v) p_result(v)
#define NL_NODE_BIND(...) ((void)0)
#define NL_NODE_REPLACE(...) ((void)0)
#define NL_NODE_ARM(...) ((void)0)
#define NL_NODE_RELOAN(...) ((void)0)
#define NL_NODE_FINISH() ((void)0)
#endif
