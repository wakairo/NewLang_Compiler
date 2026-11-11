#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if P1343_PROFILE == 0
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
static size_t binds, phases, before, after, locals, callee_reads, read_mask,
    results, calls, copies, caller_reads, caller_mask, stores;
static bool pending;
static uint8_t actual_old, actual_caller_old;

static void caller_value(bool changed)
{
    assert(caller[0] && caller[1] &&
           *caller[index] == (changed ? input : caller_old) &&
           *caller[1 - index] == input);
}

static void finish(void)
{
    /* Both lexical originals have ended. Inspect counters/scalars only. */
    assert(binds == 2 && phases == limit + 1 && before == limit + 1 &&
           after == limit + 1 && locals == limit && !pending &&
           callee_reads == 2 && read_mask == 3 && results == 1 && calls == 1 &&
           copies == 1 && caller_reads == 3 && caller_mask == 3 &&
           stores == 1 && actual_old == input &&
           actual_caller_old == caller_old);
    printf("P1343 distinct live originals input%u old%u caller-old%u "
           "caller-current%u sibling%u PASS\n",
           (unsigned)input, (unsigned)actual_old, (unsigned)actual_caller_old,
           (unsigned)input, (unsigned)input);
}

static void bind(uint8_t *a, uint8_t *b)
{
    assert(a != b && binds < 2);
    if (!binds) {
        caller[0] = a;
        caller[1] = b;
        caller_value(false);
        assert(atexit(finish) == 0);
    } else {
        caller_value(false);
        assert(a != caller[0] && a != caller[1] && b != caller[0] &&
               b != caller[1]);
        callee[0] = a;
        callee[1] = b;
        assert(*callee[index] == initial && *callee[1 - index] == peer);
    }
    ++binds;
}

static void phase(size_t p, uint8_t *a, uint8_t *b)
{
    caller_value(false);
    assert(binds == 2 && a == callee[0] && b == callee[1] && p == phases &&
           p <= limit && after == p && !pending);
    ++phases;
}

static void before_change(uint8_t *a, uint8_t *b, size_t field)
{
    caller_value(false);
    assert(a == callee[0] && b == callee[1] && field == index &&
           before <= limit && !pending &&
           *callee[index] == (before ? input : initial) &&
           *callee[1 - index] == peer);
    ++before;
    pending = true;
}

static void after_change(uint8_t *a, uint8_t *b, size_t field, uint8_t old)
{
    caller_value(false);
    assert(a == callee[0] && b == callee[1] && field == index &&
           after <= limit && pending && old == (after ? input : initial) &&
           *callee[index] == (after == limit ? final : input) &&
           *callee[1 - index] == peer);
    ++after;
    pending = false;
}

static void local(size_t symbol, uint8_t old)
{
    assert(symbol && !pending);
    if (!calls) {
        assert(locals < limit && old == (locals ? input : initial));
        ++locals;
    } else {
        assert(!stores && old == caller_old);
        caller_value(true);
        actual_caller_old = old;
        ++stores;
    }
}

static void result(uint8_t old)
{
    assert(old == input && !pending && after == limit + 1 && !results);
    actual_old = old;
    ++results;
}

static void read_field(uint8_t *field, uint8_t value)
{
    if (field == caller[0] || field == caller[1]) {
        const size_t i = field == caller[0] ? 0 : 1;
        if (!calls) {
            assert(binds == 1 && !caller_reads && i == 1 - index &&
                   value == input);
            caller_value(false);
        } else {
            assert(stores == 1 && !(caller_mask & (1u << i)) && value == input);
            caller_value(true);
            caller_mask |= 1u << i;
        }
        ++caller_reads;
    } else {
        const size_t i = field == callee[0] ? 0 : 1;
        assert(!calls && field == callee[i] &&
               value == (i == index ? final : peer) &&
               !(read_mask & (1u << i)));
        read_mask |= 1u << i;
        ++callee_reads;
    }
}

static void call_result(size_t function, size_t call, uint8_t old)
{
    /* Never dereference the ended callee here; caller is still live. */
    caller_value(false);
    assert(function && call && !calls && results == 1 && callee_reads == 2 &&
           old == actual_old && old == input);
    ++calls;
}

static void copy(size_t symbol, uint8_t old)
{
    caller_value(false);
    assert(symbol && calls == 1 && !copies && old == actual_old);
    ++copies;
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
