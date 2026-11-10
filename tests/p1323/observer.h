#ifndef P1323_OBSERVER_H
#define P1323_OBSERVER_H
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* Only observer metadata is written. Guest values/pointers come from the
 * untouched generated C; each hook surrounds the actual scoped operation. */
#if P1323_PROFILE == 0
static const uint8_t expected_initial[2] = {17, 193};
static const size_t expected_index[5] = {0, 1, 0, 1, 0};
static const uint8_t expected_old[5] = {17, 193, 61, 61, 137};
static const uint8_t expected_new[5] = {61, 61, 137, 137, 229};
static const uint8_t expected_final[2] = {229, 137};
#else
static const uint8_t expected_initial[2] = {31, 211};
static const size_t expected_index[5] = {1, 0, 1, 0, 1};
static const uint8_t expected_old[5] = {211, 31, 73, 109, 163};
static const uint8_t expected_new[5] = {73, 109, 163, 109, 241};
static const uint8_t expected_final[2] = {109, 241};
#endif
static uint8_t *original_fields[2];
static uint8_t sibling_before;
static size_t phases, before_count, after_count, pending, results, reads,
    read_mask;
static void finish(void)
{
    /* The automatic original has expired: inspect metadata only. */
    assert(phases == 3 && before_count == 5 && after_count == 5 && !pending &&
           results == 1 && reads == 2 && read_mask == 3);
    puts("PAIR physical-original1 backedges2 source-changes5 closed-loans5 "
         "result1 reads2 PASS");
}
static void bind_original(uint8_t *a, uint8_t *b)
{
    assert(!original_fields[0] && a != b && *a == expected_initial[0] &&
           *b == expected_initial[1]);
    original_fields[0] = a;
    original_fields[1] = b;
    assert(atexit(finish) == 0);
}
static void phase(size_t value, uint8_t *a, uint8_t *b)
{
    assert(a == original_fields[0] && b == original_fields[1] &&
           value == phases && value < 3 && !pending &&
           before_count == value * 2 && after_count == value * 2);
    ++phases;
}
static void before(uint8_t *a, uint8_t *b, size_t index)
{
    assert(a == original_fields[0] && b == original_fields[1] &&
           before_count < 5 && index == expected_index[before_count] &&
           !pending && before_count == after_count);
    assert(*original_fields[index] == expected_old[before_count]);
    sibling_before = *original_fields[1 - index];
    pending = 1;
    ++before_count;
}
static void after(uint8_t *a, uint8_t *b, size_t index, uint8_t old)
{
    assert(a == original_fields[0] && b == original_fields[1] &&
           after_count < 5 && index == expected_index[after_count] && pending);
    assert(old == expected_old[after_count] &&
           *original_fields[index] == expected_new[after_count] &&
           *original_fields[1 - index] == sibling_before);
    printf("change%zu field%zu old%u new%u sibling%u\n", after_count, index,
           (unsigned)old, (unsigned)*original_fields[index],
           (unsigned)*original_fields[1 - index]);
    ++after_count;
    pending = 0;
}
static void result(uint8_t old)
{
    assert(!pending && after_count == 5 && old == expected_old[4] && !results &&
           *original_fields[0] == expected_final[0] &&
           *original_fields[1] == expected_final[1]);
    ++results;
}
static void read_field(uint8_t *field, uint8_t value)
{
    const size_t index = field == original_fields[0] ? 0 : 1;
    assert(reads < 2 && field == original_fields[index] &&
           !(read_mask & ((size_t)1 << index)) &&
           value == expected_final[index] && *field == value && results == 1);
    read_mask |= (size_t)1 << index;
    ++reads;
}
#define NL_LOOP_BIND(a, b) bind_original(a, b)
#define NL_CURRENT_PHASE(n, a, b) phase(n, a, b)
#define NL_CURRENT_RESULT(v) result(v)
#define NL_LOOP_BEFORE(a, b, i) before(a, b, i)
#define NL_LOOP_AFTER(a, b, i, v) after(a, b, i, v)
#define NL_LOOP_READ(p, v) read_field(p, v)
#endif
