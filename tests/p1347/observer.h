#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if P1347_PROFILE == 0
enum {
    index = 0,
    input = 137,
    initial = 29,
    peer = 173,
    final = 229,
    caller_old = 61,
    limit = 2
};
#else
enum {
    index = 1,
    input = 103,
    initial = 37,
    peer = 157,
    final = 211,
    caller_old = 47,
    limit = 3
};
#endif

static uint8_t *caller[2], *callee[2];
static size_t binds, invocations, calls, copies, stores, caller_reads;
static size_t phases, before, after, locals, reads, read_mask, results;
static size_t function_id, call_ids[2], symbols[4];
static bool active, pending;
static uint8_t returned[2], old_stores[2];

static void caller_value(void)
{
    assert(caller[0] && caller[1] &&
           *caller[index] == (stores ? input : caller_old) &&
           *caller[1 - index] == input);
}

static void finish(void)
{
    /* All originals have ended: only counters/scalars may be inspected. */
    assert(binds == 3 && invocations == 2 && calls == 2 && copies == 4 &&
           stores == 2 && caller_reads == 4 && !active && !pending &&
           returned[0] == input && returned[1] == input &&
           old_stores[0] == caller_old && old_stores[1] == input &&
           call_ids[0] != call_ids[1]);
    puts("P1347 two calls / same live caller / first Copy survives PASS");
}

static void bind(uint8_t *a, uint8_t *b)
{
    assert(a != b && binds < 3 && !active);
    if (!binds) {
        caller[0] = a;
        caller[1] = b;
        caller_value();
        assert(atexit(finish) == 0);
    } else {
        caller_value();
        assert(invocations < 2 && calls == invocations && stores == calls &&
               a != caller[0] && a != caller[1] && b != caller[0] &&
               b != caller[1]);
        /* Do not inspect the previous callee's expired addresses. A later
         * call may legally reuse the exact same stack storage. */
        callee[0] = a;
        callee[1] = b;
        active = true;
        ++invocations;
        phases = before = after = locals = reads = read_mask = results = 0;
        assert(*callee[index] == initial && *callee[1 - index] == peer);
    }
    ++binds;
}

static void phase(size_t p, uint8_t *a, uint8_t *b)
{
    caller_value();
    assert(active && a == callee[0] && b == callee[1] && p == phases &&
           p <= limit && after == p && !pending);
    ++phases;
}

static void before_change(uint8_t *a, uint8_t *b, size_t field)
{
    caller_value();
    assert(active && a == callee[0] && b == callee[1] && field == index &&
           before <= limit && !pending &&
           *callee[index] == (before ? input : initial) &&
           *callee[1 - index] == peer);
    ++before;
    pending = true;
}

static void after_change(uint8_t *a, uint8_t *b, size_t field, uint8_t old)
{
    caller_value();
    assert(active && a == callee[0] && b == callee[1] && field == index &&
           after <= limit && pending && old == (after ? input : initial) &&
           *callee[index] == (after == limit ? final : input) &&
           *callee[1 - index] == peer);
    ++after;
    pending = false;
}

static void local(size_t symbol, uint8_t old)
{
    assert(symbol && !pending);
    if (active) {
        assert(locals < limit && old == (locals ? input : initial));
        ++locals;
    } else {
        assert(stores < 2 && calls == stores + 1 &&
               old == (stores ? input : caller_old));
        old_stores[stores++] = old;
        caller_value();
    }
}

static void result(uint8_t old)
{
    assert(active && old == input && !pending && after == limit + 1 &&
           !results);
    ++results;
}

static void read_field(uint8_t *field, uint8_t value)
{
    if (field == caller[0] || field == caller[1]) {
        const size_t i = field == caller[0] ? 0 : 1;
        assert(!active && caller_reads < 4 && value == input);
        if (caller_reads < 2)
            assert(calls == caller_reads && stores == caller_reads &&
                   i == (caller_reads ? index : 1 - index));
        else
            assert(calls == 2 && stores == 2 &&
                   i == (caller_reads == 2 ? index : 1 - index));
        caller_value();
        ++caller_reads;
    } else {
        const size_t i = field == callee[0] ? 0 : 1;
        assert(active && field == callee[i] &&
               value == (i == index ? final : peer) &&
               !(read_mask & (1u << i)));
        read_mask |= 1u << i;
        ++reads;
        if (read_mask == 3) {
            /* Clear while both fields are still alive, before function
             * return. Every later call/result/exit hook reads scalars. */
            callee[0] = NULL;
            callee[1] = NULL;
            active = false;
        }
    }
}

static void call_result(size_t function, size_t call, uint8_t old)
{
    caller_value();
    assert(!active && function && call && calls < 2 &&
           invocations == calls + 1 && results == 1 && reads == 2 &&
           read_mask == 3 && phases == limit + 1 && before == limit + 1 &&
           after == limit + 1 && locals == limit && !pending && old == input);
    if (!calls)
        function_id = function;
    else
        assert(function == function_id && call != call_ids[0]);
    call_ids[calls] = call;
    returned[calls++] = old;
}

static void copy(size_t symbol, uint8_t value)
{
    caller_value();
    assert(!active && symbol && copies < 4 && value == input &&
           calls == (copies < 2 ? 1u : 2u));
    for (size_t i = 0; i < copies; ++i)
        assert(symbols[i] != symbol);
    symbols[copies++] = symbol;
}

#define NL_LOOP_BIND(a, b) bind(a, b)
#define NL_CURRENT_PHASE(p, a, b) phase(p, a, b)
#define NL_LOOP_BEFORE(a, b, i) before_change(a, b, i)
#define NL_LOOP_AFTER(a, b, i, o) after_change(a, b, i, o)
#define NL_CURRENT_LOCAL(s, v) local(s, v)
#define NL_CURRENT_RESULT(v) result(v)
#define NL_LOOP_READ(a, v) read_field(a, v)
#define NL_CURRENT_CALL_RESULT(f, c, v) call_result(f, c, v)
#define NL_CURRENT_CALLER_COPY(s, v) copy(s, v)
