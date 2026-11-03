/* R1109 C205 passive original-field observer.
 * No authority issuance. All addresses and values sampled from actual live C17 locals.
 */
#ifndef R1109_SIDECAR_OBSERVER_H
#define R1109_SIDECAR_OBSERVER_H
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifndef R1109_CASE
#error R1109_CASE required
#endif

static const uint8_t *r1109_fields[2][2];
static uintptr_t r1109_address[4];
static unsigned r1109_before[4];
static unsigned r1109_after[4];
static unsigned r1109_bind_count;
static unsigned r1109_before_count;
static unsigned r1109_after_count;
static unsigned r1109_read_before;
static unsigned r1109_read_after;
static unsigned r1109_bad;
static unsigned r1109_target;
static unsigned r1109_old;
static unsigned r1109_field_index;

static inline void r1109_bind(const uint8_t *first, const uint8_t *second)
{
    if (r1109_bind_count >= 2U) {
        r1109_bad++;
        return;
    }
    r1109_fields[r1109_bind_count][0] = first;
    r1109_fields[r1109_bind_count][1] = second;
    r1109_bind_count++;
}

static inline void r1109_read(const uint8_t *field, uint8_t copied)
{
    if (r1109_after_count == 0U) {
        r1109_read_before++;
    } else if (r1109_target < 2U &&
               field == r1109_fields[r1109_target][r1109_field_index]) {
        r1109_read_after++;
        if (copied != *field) {
            r1109_bad++;
        }
        fprintf(stderr, "R1109 ACTUAL post_BREAK_READ=%u\n", (unsigned)copied);
    }
}

static inline void r1109_pre(const uint8_t *first, const uint8_t *second,
                              size_t field_index)
{
    if (r1109_bind_count != 2U || r1109_before_count != 0U ||
        field_index >= 2U) {
        r1109_bad++;
        return;
    }
    r1109_before_count++;
    r1109_field_index = (unsigned)field_index;
    r1109_target = 99U;
    for (unsigned root = 0U; root < 2U; ++root) {
        if (r1109_fields[root][0] == first &&
            r1109_fields[root][1] == second) {
            r1109_target = root;
        }
    }
    if (r1109_target >= 2U ||
        r1109_fields[0][0] == r1109_fields[1][0] ||
        r1109_fields[0][1] == r1109_fields[1][1]) {
        r1109_bad++;
    }
    for (unsigned root = 0U; root < 2U; ++root) {
        for (unsigned field = 0U; field < 2U; ++field) {
            const unsigned k = root * 2U + field;
            r1109_address[k] =
                (uintptr_t)(const void *)r1109_fields[root][field];
            r1109_before[k] = (unsigned)*r1109_fields[root][field];
        }
    }
    fprintf(stderr,
            "R1109 ACTUAL BEFORE target=%u index=%zu root0=[%u,%u] "
            "root1=[%u,%u] addr=[%" PRIxPTR ",%" PRIxPTR ",%"
            PRIxPTR ",%" PRIxPTR "]\n",
            r1109_target, field_index, r1109_before[0],
            r1109_before[1], r1109_before[2], r1109_before[3],
            r1109_address[0], r1109_address[1], r1109_address[2],
            r1109_address[3]);
}

static inline void r1109_post(const uint8_t *first, const uint8_t *second,
                               size_t field_index, uint8_t old)
{
    if (r1109_before_count != 1U || r1109_after_count != 0U ||
        r1109_target >= 2U || field_index != r1109_field_index ||
        first != r1109_fields[r1109_target][0] ||
        second != r1109_fields[r1109_target][1]) {
        r1109_bad++;
        return;
    }
    r1109_after_count++;
    r1109_old = (unsigned)old;
    for (unsigned root = 0U; root < 2U; ++root) {
        for (unsigned field = 0U; field < 2U; ++field) {
            const unsigned k = root * 2U + field;
            if ((uintptr_t)(const void *)r1109_fields[root][field] !=
                r1109_address[k]) {
                r1109_bad++;
            }
            r1109_after[k] = (unsigned)*r1109_fields[root][field];
        }
    }
    fprintf(stderr,
            "R1109 ACTUAL AFTER old=%u root0=[%u,%u] root1=[%u,%u] "
            "same_orig_addresses=%s\n",
            r1109_old, r1109_after[0], r1109_after[1],
            r1109_after[2], r1109_after[3],
            r1109_bad == 0U ? "YES" : "NO");
}

/* Called after generated main returns: inspect saved values ONLY.
 * Original local lifetimes have ended; never dereference saved pointers.
 */
static inline int r1109_finish(int generated_exit)
{
    const unsigned expected_before[2][4] = {
        {19U, 7U, 91U, 23U}, {31U, 13U, 31U, 13U}
    };
    const unsigned expected_after[2][4] = {
        {61U, 7U, 91U, 23U}, {31U, 13U, 55U, 13U}
    };
    const unsigned row = (unsigned)R1109_CASE - 1U;
    const unsigned expected_target = R1109_CASE == 1 ? 0U : 1U;
    const unsigned expected_old = R1109_CASE == 1 ? 19U : 31U;
    if (row > 1U || generated_exit != 0 ||
        r1109_bind_count != 2U || r1109_before_count != 1U ||
        r1109_after_count != 1U || r1109_target != expected_target ||
        r1109_field_index != 0U || r1109_old != expected_old ||
        r1109_read_before < 1U || r1109_read_after < 1U) {
        r1109_bad++;
    }
    if (row <= 1U) {
        for (unsigned k = 0U; k < 4U; ++k) {
            if (r1109_before[k] != expected_before[row][k] ||
                r1109_after[k] != expected_after[row][k]) {
                r1109_bad++;
            }
        }
    }
    fprintf(stderr,
            "R1109 ACTUAL VERDICT case=%d bind=%u before=%u after=%u "
            "pre_read=%u post_read=%u old=%u wrong=%u\n",
            R1109_CASE, r1109_bind_count, r1109_before_count,
            r1109_after_count, r1109_read_before, r1109_read_after,
            r1109_old, r1109_bad);
    if (r1109_bad != 0U) {
        return 21;
    }
    fprintf(stderr, "R1109_NATIVE_OBSERVER_PASS\n");
    return 0;
}

#define NL_LOOP_BIND(a, b) r1109_bind((a), (b))
#define NL_LOOP_READ(a, v) r1109_read((a), (v))
#define NL_LOOP_BEFORE(a, b, i) r1109_pre((a), (b), (i))
#define NL_LOOP_AFTER(a, b, i, old) r1109_post((a), (b), (i), (old))
#endif
