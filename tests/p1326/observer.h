#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static uint8_t *fields[2];
static size_t phases, before, after, reads, read_mask, results;
static bool pending;
static size_t helper_calls;
static const uint8_t old_values[] = {29, 83, 83};
static const uint8_t new_values[] = {83, 83, 227};
static void finish(void)
{
    /* Metadata only: the automatic original is no longer alive. */
    assert(phases == 3 && before == 3 && after == 3 && !pending && reads == 2 &&
           read_mask == 3 && results == 1 && helper_calls == 3);
    puts("P1326 HELPER original1 backedges2 Changes3 closed-loans3 calls3 "
         "old83 current227 sibling173");
}
static void bind_original(uint8_t *a, uint8_t *b)
{
    assert(!fields[0] && a != b && *a == 29 && *b == 173);
    fields[0] = a;
    fields[1] = b;
    assert(atexit(finish) == 0);
}
static void phase_at(size_t phase, uint8_t *a, uint8_t *b)
{
    assert(a == fields[0] && b == fields[1] && phase < 3 && phase == phases &&
           after == phase && !pending);
    ++phases;
}
static void pre(uint8_t *a, uint8_t *b, size_t index)
{
    assert(a == fields[0] && b == fields[1] && index == 0 && before < 3 &&
           !pending && *a == old_values[before] && *b == 173);
    ++before;
    pending = true;
}
static void post(uint8_t *a, uint8_t *b, size_t index, uint8_t old)
{
    assert(a == fields[0] && b == fields[1] && index == 0 && after < 3 &&
           pending && old == old_values[after] && *a == new_values[after] &&
           *b == 173);
    printf("Change%zu index%zu old%u new%u sibling%u\n", after, index,
           (unsigned)old, (unsigned)*a, (unsigned)*b);
    ++after;
    pending = false;
}
static void result_at(uint8_t old)
{
    assert(old == 83 && !pending && after == 3);
    ++results;
}
static void read_at(uint8_t *p, uint8_t value)
{
    const size_t i = p == fields[0] ? 0 : 1;
    assert(p == fields[i] && value == (i ? 173 : 227));
    assert(!(read_mask & (1u << i)));
    read_mask |= 1u << i;
    ++reads;
}
static void local_result(size_t symbol, uint8_t value)
{
    if (phases && before == after && after == phases - 1 &&
        helper_calls == phases - 1) {
        assert(!pending && phases <= 3 && value == new_values[phases - 1]);
        printf("helper phase%zu binding%zu actual_return%u\n", phases - 1,
               symbol, (unsigned)value);
        ++helper_calls;
    }
}
#define NL_CURRENT_LOCAL(s, v) local_result(s, v)
#define NL_CURRENT_PHASE(p, a, b) phase_at(p, a, b)
#define NL_CURRENT_RESULT(v) result_at(v)
#define NL_LOOP_BIND(a, b) bind_original(a, b)
#define NL_LOOP_BEFORE(a, b, i) pre(a, b, i)
#define NL_LOOP_AFTER(a, b, i, o) post(a, b, i, o)
#define NL_LOOP_READ(a, v) read_at(a, v)
