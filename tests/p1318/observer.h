#ifndef P1318_OBSERVER_H
#define P1318_OBSERVER_H
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* Passive: every pointer/value below is supplied by emitted operations.
 * The observer writes only its own counters; it never writes guest fields. */
static uint8_t *p1318_fields[2];
static size_t p1318_phases, p1318_before, p1318_after, p1318_reads,
    p1318_results, p1318_pending, p1318_read_mask;
#if P1318_PROFILE == 0
static const unsigned p1318_index = 0;
static const uint8_t p1318_initial[2] = {17, 193};
static const uint8_t p1318_old[3] = {17, 61, 137};
static const uint8_t p1318_new[3] = {61, 137, 229};
static const uint8_t p1318_final[2] = {229, 193};
#else
static const unsigned p1318_index = 1;
static const uint8_t p1318_initial[2] = {59, 23};
static const uint8_t p1318_old[3] = {23, 85, 173};
static const uint8_t p1318_new[3] = {85, 173, 211};
static const uint8_t p1318_final[2] = {59, 211};
#endif
static void p1318_finish(void)
{
    /* The automatic original is no longer live here. Do not dereference it. */
    assert(p1318_phases == 3 && p1318_before == 3 && p1318_after == 3 &&
           !p1318_pending && p1318_reads == 2 && p1318_read_mask == 3 &&
           p1318_results == 1);
    puts("PASSIVE original1 backedges2 stores3 closed-loans3 result1 reads2");
}
static void p1318_bind(uint8_t *a, uint8_t *b)
{
    assert(!p1318_fields[0] && b == a + 1);
    assert(*a == p1318_initial[0] && *b == p1318_initial[1]);
    p1318_fields[0] = a;
    p1318_fields[1] = b;
    assert(atexit(p1318_finish) == 0);
}
static void p1318_phase(size_t phase, uint8_t *a, uint8_t *b)
{
    assert(a == p1318_fields[0] && b == p1318_fields[1] &&
           phase == p1318_phases && phase < 3 && !p1318_pending);
    assert(p1318_before == phase && p1318_after == phase);
    ++p1318_phases;
}
static void p1318_pre(uint8_t *a, uint8_t *b, size_t index)
{
    assert(a == p1318_fields[0] && b == p1318_fields[1] &&
           index == p1318_index && p1318_before < 3 && !p1318_pending);
    assert(*p1318_fields[index] == p1318_old[p1318_before]);
    assert(*p1318_fields[1 - index] == p1318_initial[1 - index]);
    ++p1318_before;
    p1318_pending = 1;
}
static void p1318_post(uint8_t *a, uint8_t *b, size_t index, uint8_t old)
{
    assert(a == p1318_fields[0] && b == p1318_fields[1] &&
           index == p1318_index && p1318_after < 3 && p1318_pending);
    assert(old == p1318_old[p1318_after] &&
           *p1318_fields[index] == p1318_new[p1318_after] &&
           *p1318_fields[1 - index] == p1318_initial[1 - index]);
    printf("replace%zu index%zu old%u current%u sibling%u\n", p1318_after,
           index, (unsigned)old, (unsigned)*p1318_fields[index],
           (unsigned)*p1318_fields[1 - index]);
    ++p1318_after;
    p1318_pending = 0;
}
static void p1318_result(uint8_t old)
{
    assert(!p1318_pending && p1318_after == 3 && old == p1318_old[2] &&
           !p1318_results);
    ++p1318_results;
}
static void p1318_read(uint8_t *field, uint8_t value)
{
    const size_t index = field == p1318_fields[0] ? 0 : 1;
    assert(p1318_reads < 2 && field == p1318_fields[index] &&
           !(p1318_read_mask & ((size_t)1 << index)) &&
           value == p1318_final[index] && *field == value);
    p1318_read_mask |= (size_t)1 << index;
    ++p1318_reads;
}
#define NL_LOOP_BIND(a, b) p1318_bind(a, b)
#define NL_CURRENT_PHASE(n, a, b) p1318_phase(n, a, b)
#define NL_CURRENT_RESULT(v) p1318_result(v)
#define NL_LOOP_BEFORE(a, b, i) p1318_pre(a, b, i)
#define NL_LOOP_AFTER(a, b, i, v) p1318_post(a, b, i, v)
#define NL_LOOP_READ(p, v) p1318_read(p, v)
#endif
