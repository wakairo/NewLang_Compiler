#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#if P1332_PROFILE == 0
enum {
    index = 0,
    initial = 43,
    sibling = 181,
    changed = 95,
    final = 233,
    limit = 2
};
#else
enum {
    index = 1,
    initial = 41,
    sibling = 157,
    changed = 107,
    final = 239,
    limit = 3
};
#endif
static uint8_t *fields[2];
static size_t phases, before, after, reads, read_mask, results, calls, copies;
static size_t caller_symbols[2];
static uint8_t actual_old;
static bool pending;
static void finish(void)
{
    /* The callee's automatic original has ended. Scalar metadata only. */
    assert(phases == limit + 1 && before == limit + 1 && after == limit + 1 &&
           !pending && reads == 2 && read_mask == 3 && results == 1 &&
           calls == 1 && copies == 2 && actual_old == changed);
    puts(
        "P1332 physical-original1 real-backedges old->tail->call->caller PASS");
}
static void bind_original(uint8_t *a, uint8_t *b)
{
    assert(!fields[0] && !phases && a != b);
    fields[0] = a;
    fields[1] = b;
    assert(*fields[index] == initial && *fields[1 - index] == sibling);
    assert(atexit(finish) == 0);
}
static void phase_at(size_t phase, uint8_t *a, uint8_t *b)
{
    assert(a == fields[0] && b == fields[1] && phase <= limit &&
           phase == phases && after == phase && !pending);
    ++phases;
}
static void pre(uint8_t *a, uint8_t *b, size_t field)
{
    assert(a == fields[0] && b == fields[1] && field == index &&
           before <= limit && !pending &&
           *fields[index] == (before ? changed : initial) &&
           *fields[1 - index] == sibling);
    ++before;
    pending = true;
}
static void post(uint8_t *a, uint8_t *b, size_t field, uint8_t old)
{
    assert(a == fields[0] && b == fields[1] && field == index &&
           after <= limit && pending && old == (after ? changed : initial) &&
           *fields[index] == (after == limit ? final : changed) &&
           *fields[1 - index] == sibling);
    printf("Change%zu index%zu old%u new%u sibling%u\n", after, field,
           (unsigned)old, (unsigned)*fields[index],
           (unsigned)*fields[1 - index]);
    ++after;
    pending = false;
}
static void result_at(uint8_t old)
{
    assert(old == changed && !pending && after == limit + 1 && !results);
    actual_old = old;
    ++results;
}
static void read_at(uint8_t *p, uint8_t value)
{
    const size_t i = p == fields[0] ? 0 : 1;
    assert(p == fields[i] && value == (i == index ? final : sibling));
    assert(!(read_mask & (1u << i)));
    read_mask |= 1u << i;
    ++reads;
}
static void call_result(size_t function, size_t call, uint8_t value)
{
    /* Actual return happened; never read fields/pointers after this point. */
    assert(function && call && !calls && results == 1 && reads == 2 &&
           value == actual_old && value == changed);
    ++calls;
    printf("actual call%zu function%zu returned%u\n", call, function,
           (unsigned)value);
}
static void caller_copy(size_t symbol, uint8_t value)
{
    assert(calls == 1 && copies < 2 && symbol && value == actual_old &&
           (!copies || symbol != caller_symbols[0]));
    caller_symbols[copies++] = symbol;
    printf("caller Copy%zu symbol%zu value%u\n", copies, symbol,
           (unsigned)value);
}
#define NL_CURRENT_PHASE(p, a, b) phase_at(p, a, b)
#define NL_CURRENT_RESULT(v) result_at(v)
#define NL_LOOP_BIND(a, b) bind_original(a, b)
#define NL_LOOP_BEFORE(a, b, i) pre(a, b, i)
#define NL_LOOP_AFTER(a, b, i, o) post(a, b, i, o)
#define NL_LOOP_READ(a, v) read_at(a, v)
#define NL_CURRENT_CALL_RESULT(f, c, v) call_result(f, c, v)
#define NL_CURRENT_CALLER_COPY(s, v) caller_copy(s, v)
