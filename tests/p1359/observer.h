#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#if P1359_PROFILE == 0
enum {
    index = 1,
    initial = 17,
    peer = 211,
    final = 197,
    original = 59,
    limit = 3
};
static const uint8_t inputs[2] = {43, 181};
#else
enum {
    index = 0,
    initial = 23,
    peer = 173,
    final = 233,
    original = 61,
    limit = 2
};
static const uint8_t inputs[2] = {91, 157};
#endif
static uint8_t *wrapper[2], *inner[2];
static bool wrapper_live, inner_live, pending, changed[2], returned[2];
static size_t entered, epoch, binds, calls, main_copies, inner_function,
    outer_function;
static size_t phases[2], pres[2], posts[2], locals[2], reads[2],
    wrapper_reads[2], mask[2], wrapper_mask[2], results[2], stores[2],
    wrapper_copies[2];
static uint8_t real_old[2], real_displaced[2];
static void wrapper_value(void)
{
    assert(wrapper_live &&
           *wrapper[index] == (changed[epoch] ? inputs[epoch] : original) &&
           *wrapper[1 - index] == inputs[epoch]);
}
static void finish(void)
{
    /* No expired pointer is compared or dereferenced here. */
    assert(!wrapper_live && !inner_live && !pending && entered == 2 &&
           binds == 4 && calls == 4 && main_copies == 6);
    for (size_t i = 0; i < 2; ++i)
        assert(returned[i] && phases[i] == limit + 1 && pres[i] == limit + 1 &&
               posts[i] == limit + 1 && locals[i] == limit && reads[i] == 2 &&
               wrapper_reads[i] == 3 && mask[i] == 3 && wrapper_mask[i] == 3 &&
               results[i] == 1 && stores[i] == 1 && wrapper_copies[i] == 2 &&
               real_old[i] == inputs[i] && real_displaced[i] == original);
    puts("P1359 two wrapper calls retain first Copy after second PASS");
}
static void bind(uint8_t *a, uint8_t *b)
{
    assert(a != b);
    if (!wrapper_live) {
        assert(!inner_live && !pending && entered < 2 && calls == 2 * entered &&
               main_copies == 2 * entered);
        epoch = entered++;
        wrapper[0] = a;
        wrapper[1] = b;
        wrapper_live = true;
        wrapper_value();
        if (entered == 1)
            assert(atexit(finish) == 0);
    } else {
        wrapper_value();
        assert(!inner_live && a != wrapper[0] && a != wrapper[1] &&
               b != wrapper[0] && b != wrapper[1]);
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
    assert(inner_live && a == inner[0] && b == inner[1] &&
           value == phases[epoch] && value <= limit && posts[epoch] == value &&
           !pending);
    ++phases[epoch];
}
static void pre(uint8_t *a, uint8_t *b, size_t field)
{
    wrapper_value();
    assert(inner_live && a == inner[0] && b == inner[1] && field == index &&
           pres[epoch] <= limit && !pending &&
           *inner[index] == (pres[epoch] ? inputs[epoch] : initial) &&
           *inner[1 - index] == peer);
    ++pres[epoch];
    pending = true;
}
static void post(uint8_t *a, uint8_t *b, size_t field, uint8_t old)
{
    wrapper_value();
    assert(inner_live && a == inner[0] && b == inner[1] && field == index &&
           posts[epoch] <= limit && pending &&
           old == (posts[epoch] ? inputs[epoch] : initial) &&
           *inner[index] == (posts[epoch] == limit ? final : inputs[epoch]) &&
           *inner[1 - index] == peer);
    ++posts[epoch];
    pending = false;
}
static void local(size_t symbol, uint8_t old)
{
    assert(symbol && !pending);
    if (inner_live) {
        assert(locals[epoch] < limit &&
               old == (locals[epoch] ? inputs[epoch] : initial));
        ++locals[epoch];
    } else {
        assert(wrapper_live && calls == 2 * epoch + 1 && !stores[epoch] &&
               old == original);
        real_displaced[epoch] = old;
        ++stores[epoch];
        changed[epoch] = true;
        wrapper_value();
    }
}
static void result(uint8_t old)
{
    assert(inner_live && !pending && !results[epoch] &&
           posts[epoch] == limit + 1 && old == inputs[epoch]);
    real_old[epoch] = old;
    ++results[epoch];
}
static void read_field(uint8_t *field, uint8_t value)
{
    if (wrapper_live && (field == wrapper[0] || field == wrapper[1])) {
        const size_t i = field == wrapper[0] ? 0 : 1;
        assert(value == inputs[epoch] && !inner_live);
        wrapper_value();
        if (!wrapper_reads[epoch])
            assert(calls == 2 * epoch && i == 1 - index);
        else {
            assert(calls == 2 * epoch + 1 && stores[epoch] == 1 &&
                   !(wrapper_mask[epoch] & (1u << i)));
            wrapper_mask[epoch] |= 1u << i;
        }
        ++wrapper_reads[epoch];
        if (wrapper_mask[epoch] == 3) {
            wrapper[0] = NULL;
            wrapper[1] = NULL;
            wrapper_live = false;
        }
    } else {
        const size_t i = field == inner[0] ? 0 : 1;
        assert(inner_live && field == inner[i] &&
               value == (i == index ? final : peer) &&
               !(mask[epoch] & (1u << i)));
        mask[epoch] |= 1u << i;
        ++reads[epoch];
        if (mask[epoch] == 3) {
            inner[0] = NULL;
            inner[1] = NULL;
            inner_live = false;
        }
    }
}
static void call_result(size_t function, size_t call, uint8_t value)
{
    assert(function && call && value == inputs[epoch] && !inner_live &&
           calls < 4);
    if (calls % 2 == 0) {
        wrapper_value();
        assert(reads[epoch] == 2 && results[epoch] == 1 &&
               locals[epoch] == limit);
        if (!epoch)
            inner_function = function;
        else
            assert(function == inner_function);
    } else {
        assert(!wrapper_live && wrapper_copies[epoch] == 2 &&
               stores[epoch] == 1 && function != inner_function);
        if (!epoch)
            outer_function = function;
        else
            assert(function == outer_function);
        returned[epoch] = true;
    }
    ++calls;
}
static void copy(size_t symbol, uint8_t value)
{
    assert(symbol && !inner_live);
    if (!returned[epoch]) {
        assert(value == inputs[epoch] && calls == 2 * epoch + 1 &&
               wrapper_copies[epoch] < 2);
        if (!wrapper_copies[epoch])
            wrapper_value();
        else
            assert(!wrapper_live && stores[epoch] == 1);
        ++wrapper_copies[epoch];
    } else {
        const uint8_t expected[6] = {inputs[0], inputs[0], inputs[1],
                                     inputs[1], inputs[0], inputs[1]};
        assert(!wrapper_live && main_copies < 6 &&
               value == expected[main_copies]);
        if (main_copies >= 2)
            assert(returned[1] && calls == 4);
        ++main_copies;
    }
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
