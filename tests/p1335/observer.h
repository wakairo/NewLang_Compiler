#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#if P1335_PROFILE == 0
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
static size_t invocations, calls, copies, function_id, call_ids[2], symbols[3];
static size_t phases, before, after, reads, mask, results;
static bool pending, active;
static uint8_t actual_old, returned[2];
static void finish(void)
{
    /* No original pointer is read after either callee's lifetime. */
    assert(invocations == 2 && calls == 2 && copies == 3 && !active &&
           !pending && returned[0] == changed && returned[1] == changed &&
           call_ids[0] != call_ids[1]);
    puts("P1335 two actual calls / first Copy survives PASS");
}
static void bind_original(uint8_t *a, uint8_t *b)
{
    assert(!active && invocations < 2 && calls == invocations &&
           copies == (invocations ? 1u : 0u) && a != b);
    if (!invocations)
        assert(atexit(finish) == 0);
    /* Overwrite before any pointer inspection. Reused stack storage is legal.
     */
    fields[0] = a;
    fields[1] = b;
    active = true;
    ++invocations;
    phases = before = after = reads = mask = results = 0;
    pending = false;
    assert(*fields[index] == initial && *fields[1 - index] == sibling);
}
static void phase_at(size_t phase, uint8_t *a, uint8_t *b)
{
    assert(active && a == fields[0] && b == fields[1] && phase == phases &&
           phase <= limit && after == phase && !pending);
    ++phases;
}
static void pre(uint8_t *a, uint8_t *b, size_t i)
{
    assert(active && a == fields[0] && b == fields[1] && i == index &&
           !pending && before <= limit &&
           *fields[index] == (before ? changed : initial) &&
           *fields[1 - index] == sibling);
    ++before;
    pending = true;
}
static void post(uint8_t *a, uint8_t *b, size_t i, uint8_t old)
{
    assert(active && a == fields[0] && b == fields[1] && i == index &&
           pending && after <= limit && old == (after ? changed : initial) &&
           *fields[index] == (after == limit ? final : changed) &&
           *fields[1 - index] == sibling);
    printf("invocation%zu Change%zu index%zu old%u new%u sibling%u\n",
           invocations, after, i, (unsigned)old, (unsigned)*fields[index],
           (unsigned)*fields[1 - index]);
    ++after;
    pending = false;
}
static void result_at(uint8_t old)
{
    assert(active && !pending && !results && old == changed &&
           after == limit + 1);
    actual_old = old;
    ++results;
}
static void read_at(uint8_t *p, uint8_t value)
{
    assert(active);
    size_t i = p == fields[0] ? 0 : 1;
    assert(p == fields[i] && !(mask & (1u << i)) &&
           value == (i == index ? final : sibling));
    mask |= 1u << i;
    ++reads;
    if (mask == 3) {
        /* Clear while live. Subsequent call/Copy/atexit hooks read only
         * scalars. */
        fields[0] = NULL;
        fields[1] = NULL;
        active = false;
    }
}
static void call_result(size_t function, size_t call, uint8_t value)
{
    assert(!active && calls < 2 && invocations == calls + 1 && results == 1 &&
           reads == 2 && phases == limit + 1 && before == limit + 1 &&
           after == limit + 1 && !pending && value == actual_old &&
           value == changed && function && call);
    if (!calls)
        function_id = function;
    else
        assert(function_id == function && call_ids[0] != call);
    call_ids[calls] = call;
    returned[calls] = value;
    ++calls;
    printf("actual call%zu invocation%zu function%zu returned%u\n", call, calls,
           function, (unsigned)value);
}
static void caller_copy(size_t symbol, uint8_t value)
{
    assert(!active && copies < 3 && symbol && value == changed &&
           calls == (copies ? 2u : 1u));
    for (size_t i = 0; i < copies; ++i)
        assert(symbols[i] != symbol);
    symbols[copies++] = symbol;
    printf("caller Copy%zu symbol%zu value%u after-calls%zu\n", copies, symbol,
           (unsigned)value, calls);
}
#define NL_CURRENT_PHASE(p, a, b) phase_at(p, a, b)
#define NL_CURRENT_RESULT(v) result_at(v)
#define NL_LOOP_BIND(a, b) bind_original(a, b)
#define NL_LOOP_BEFORE(a, b, i) pre(a, b, i)
#define NL_LOOP_AFTER(a, b, i, o) post(a, b, i, o)
#define NL_LOOP_READ(a, v) read_at(a, v)
#define NL_CURRENT_CALL_RESULT(f, c, v) call_result(f, c, v)
#define NL_CURRENT_CALLER_COPY(s, v) caller_copy(s, v)
