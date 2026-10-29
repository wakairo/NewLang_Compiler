#ifndef P1020_OBSERVER_H
#define P1020_OBSERVER_H
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define P1020_REQUIRE(x)                                                       \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "P1020 physical failure line=%d\n", __LINE__);     \
            abort();                                                           \
        }                                                                      \
    } while (0)

static uint8_t *p1020_original_fields[2];
static uintptr_t p1020_addresses[2], p1020_changed_address;
static unsigned p1020_bindings, p1020_changes, p1020_reads;
static unsigned p1020_old_value, p1020_new_value, p1020_sibling_value;
static unsigned p1020_read_mask;

static uint8_t p1020_peek(const uint8_t *actual)
{
    return *(const volatile uint8_t *)actual;
}

/* At exit the original stack carrier has ended. Inspect only saved integer
 * addresses/observations and counters, never an expired pointer or object. */
static void p1020_finish(void)
{
    P1020_REQUIRE(p1020_bindings == 1 && p1020_changes == 1 &&
                  p1020_reads == 2 && p1020_read_mask == 3 &&
                  p1020_changed_address == p1020_addresses[P1020_INDEX]);
    printf("P1020 physical old=%u new=%u sibling=%u changes=1 reads=2 "
           "field=%" PRIxPTR " sibling_address=%" PRIxPTR
           " same_original=YES live_original_reads=YES\n",
           p1020_old_value, p1020_new_value, p1020_sibling_value,
           p1020_addresses[P1020_INDEX], p1020_addresses[1 - P1020_INDEX]);
}
static void p1020_bind(uint8_t *first, uint8_t *second)
{
    P1020_REQUIRE(P1020_INDEX < 2 && !p1020_bindings && first != second);
    p1020_original_fields[0] = first;
    p1020_original_fields[1] = second;
    p1020_addresses[0] = (uintptr_t)first;
    p1020_addresses[1] = (uintptr_t)second;
    P1020_REQUIRE(p1020_peek(p1020_original_fields[P1020_INDEX]) == P1020_OLD &&
                  p1020_peek(p1020_original_fields[1 - P1020_INDEX]) ==
                      P1020_SIBLING);
    ++p1020_bindings;
    P1020_REQUIRE(atexit(p1020_finish) == 0);
}
static void p1020_change(uint8_t *actual, uint8_t old)
{
    P1020_REQUIRE(p1020_bindings == 1 && !p1020_changes && !p1020_reads &&
                  actual == p1020_original_fields[P1020_INDEX] &&
                  old == P1020_OLD && p1020_peek(actual) == P1020_NEW &&
                  p1020_peek(p1020_original_fields[1 - P1020_INDEX]) ==
                      P1020_SIBLING);
    p1020_changed_address = (uintptr_t)actual;
    p1020_old_value = old;
    p1020_new_value = p1020_peek(actual);
    p1020_sibling_value = p1020_peek(p1020_original_fields[1 - P1020_INDEX]);
    ++p1020_changes;
}
static void p1020_read(uint8_t *actual, uint8_t copied)
{
    P1020_REQUIRE(p1020_changes == 1 && p1020_reads < 2);
    const unsigned i = actual == p1020_original_fields[0] ? 0 : 1;
    P1020_REQUIRE(actual == p1020_original_fields[i] &&
                  !(p1020_read_mask & (1U << i)) &&
                  copied == p1020_peek(actual) &&
                  copied == (i == P1020_INDEX ? P1020_NEW : P1020_SIBLING));
    p1020_read_mask |= 1U << i;
    ++p1020_reads;
}
#define NL_UNIT_CALL_BIND(a, b) p1020_bind((a), (b))
#define NL_UNIT_CALL_CHANGE(p, old) p1020_change((p), (old))
#define NL_UNIT_CALL_READ(p, value) p1020_read((p), (value))
#endif
