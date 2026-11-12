#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#if P1351_PROFILE == 0
enum {
    index = 0,
    input = 167,
    initial = 17,
    peer = 193,
    final = 211,
    original = 41,
    limit = 2
};
#else
enum {
    index = 1,
    input = 149,
    initial = 23,
    peer = 181,
    final = 239,
    original = 59,
    limit = 3
};
#endif
static uint8_t *wrapper[2], *inner[2];
static bool wrapper_live, inner_live, pending, changed;
static size_t binds, calls, copies, phases, pres, posts, locals, reads,
    wrapper_reads, mask, wrapper_mask, results, stores, inner_function;
static uint8_t real_old, real_displaced;
static void wrapper_value(void)
{
    assert(wrapper_live && *wrapper[index] == (changed ? input : original) &&
           *wrapper[1 - index] == input);
}
static void finish(void)
{
    /* Both originals ended: inspect only counters and copied scalars. */
    assert(!wrapper_live && !inner_live && !pending && binds == 2 &&
           calls == 2 && copies == 4 && phases == limit + 1 &&
           pres == limit + 1 && posts == limit + 1 && locals == limit &&
           reads == 2 && wrapper_reads == 3 && mask == 3 && wrapper_mask == 3 &&
           results == 1 && stores == 1 && real_old == input &&
           real_displaced == original);
    puts("P1351 original wrapper -> inner old -> outer Copy PASS");
}
static void bind(uint8_t *a, uint8_t *b)
{
    assert(a != b && binds < 2);
    if (!binds) {
        wrapper[0] = a;
        wrapper[1] = b;
        wrapper_live = true;
        wrapper_value();
        assert(atexit(finish) == 0);
    } else {
        wrapper_value();
        assert(a != wrapper[0] && a != wrapper[1] && b != wrapper[0] &&
               b != wrapper[1]);
        inner[0] = a;
        inner[1] = b;
        inner_live = true;
        assert(*inner[index] == initial && *inner[1 - index] == peer);
    }
    ++binds;
}
static void phase(size_t value, uint8_t *a, uint8_t *b)
{
    wrapper_value();
    assert(inner_live && a == inner[0] && b == inner[1] && value == phases &&
           value <= limit && posts == value && !pending);
    ++phases;
}
static void pre(uint8_t *a, uint8_t *b, size_t field)
{
    wrapper_value();
    assert(inner_live && a == inner[0] && b == inner[1] && field == index &&
           pres <= limit && !pending &&
           *inner[index] == (pres ? input : initial) &&
           *inner[1 - index] == peer);
    ++pres;
    pending = true;
}
static void post(uint8_t *a, uint8_t *b, size_t field, uint8_t old)
{
    wrapper_value();
    assert(inner_live && a == inner[0] && b == inner[1] && field == index &&
           posts <= limit && pending && old == (posts ? input : initial) &&
           *inner[index] == (posts == limit ? final : input) &&
           *inner[1 - index] == peer);
    ++posts;
    pending = false;
}
static void local(size_t symbol, uint8_t old)
{
    assert(symbol && !pending);
    if (inner_live) {
        assert(locals < limit && old == (locals ? input : initial));
        ++locals;
    } else {
        assert(wrapper_live && calls == 1 && !stores && old == original);
        real_displaced = old;
        ++stores;
        changed = true;
        wrapper_value();
    }
}
static void result(uint8_t old)
{
    assert(inner_live && !pending && !results && posts == limit + 1 &&
           old == input);
    real_old = old;
    ++results;
}
static void read_field(uint8_t *field, uint8_t value)
{
    if (wrapper_live && (field == wrapper[0] || field == wrapper[1])) {
        const size_t i = field == wrapper[0] ? 0 : 1;
        assert(value == input && !inner_live);
        wrapper_value();
        if (!wrapper_reads)
            assert(!calls && i == 1 - index);
        else {
            assert(calls == 1 && stores == 1 && !(wrapper_mask & (1u << i)));
            wrapper_mask |= 1u << i;
        }
        ++wrapper_reads;
        if (wrapper_mask == 3) {
            wrapper[0] = NULL;
            wrapper[1] = NULL;
            wrapper_live = false;
        }
    } else {
        const size_t i = field == inner[0] ? 0 : 1;
        assert(inner_live && field == inner[i] &&
               value == (i == index ? final : peer) && !(mask & (1u << i)));
        mask |= 1u << i;
        ++reads;
        if (mask == 3) {
            inner[0] = NULL;
            inner[1] = NULL;
            inner_live = false;
        }
    }
}
static void call_result(size_t function, size_t call, uint8_t value)
{
    assert(function && call && value == input && !inner_live && calls < 2);
    if (!calls) {
        wrapper_value();
        assert(reads == 2 && results == 1 && locals == limit);
        inner_function = function;
    } else
        assert(!wrapper_live && copies == 2 && stores == 1 &&
               function != inner_function);
    ++calls;
}
static void copy(size_t symbol, uint8_t value)
{
    assert(symbol && value == input && !inner_live && copies < 4);
    if (copies < 2) {
        assert(calls == 1);
        if (!copies)
            wrapper_value();
        else
            assert(!wrapper_live && stores == 1);
    } else
        assert(calls == 2 && !wrapper_live);
    ++copies;
}
#define NL_LOOP_BIND(a, b) bind(a, b)
#define NL_LOOP_READ(p, v) read_field(p, v)
#define NL_CURRENT_PHASE(p, a, b) phase(p, a, b)
#define NL_LOOP_BEFORE(a, b, i) pre(a, b, i)
#define NL_LOOP_AFTER(a, b, i, o) post(a, b, i, o)
#define NL_CURRENT_LOCAL(s, v) local(s, v)
#define NL_CURRENT_RESULT(v) result(v)
#define NL_CURRENT_CALL_RESULT(f, c, v) call_result(f, c, v)
#define NL_CURRENT_CALLER_COPY(s, v) copy(s, v)
