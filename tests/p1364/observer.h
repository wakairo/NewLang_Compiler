#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#if P1364_PROFILE == 0
enum {
    field_index = 1,
    input = 89,
    main_old = 29,
    main_peer = 199,
    wrapper_old = 59,
    inner_old = 17,
    inner_peer = 211,
    terminal = 197,
    limit = 3
};
#else
enum {
    field_index = 0,
    input = 111,
    main_old = 47,
    main_peer = 167,
    wrapper_old = 61,
    inner_old = 23,
    inner_peer = 173,
    terminal = 233,
    limit = 2
};
#endif
static uint8_t *originals[3][2];
static bool live[3], changed[2], pending;
static size_t binds, phases, pres, posts, locals, calls, results, inner_fn;
static unsigned masks[3];
static size_t read_count[3], stores[2], copies;
static uint8_t observed_old[3];
static void outer_state(void)
{
    assert(live[0] &&
           *originals[0][field_index] == (changed[0] ? input : main_old) &&
           *originals[0][1 - field_index] == main_peer);
    if (live[1])
        assert(*originals[1][field_index] ==
                   (changed[1] ? input : wrapper_old) &&
               *originals[1][1 - field_index] == input);
}
static void finish(void)
{
    /* All pointers were cleared while their lexical objects were live. */
    assert(!live[0] && !live[1] && !live[2] && binds == 3 && calls == 2 &&
           results == 1 && phases == limit + 1 && pres == limit + 1 &&
           posts == limit + 1 && locals == limit && !pending &&
           stores[0] == 1 && stores[1] == 1 && read_count[0] == 3 &&
           read_count[1] == 3 && read_count[2] == 2 && masks[0] == 3 &&
           masks[1] == 3 && masks[2] == 3 && copies >= 3 &&
           observed_old[0] == main_old && observed_old[1] == wrapper_old &&
           observed_old[2] == input);
    puts("P1364 same main original survives wrapper/inner; real "
         "old/current/sibling PASS");
}
static void bind(uint8_t *a, uint8_t *b)
{
    assert(binds < 3 && a != b);
    for (size_t k = 0; k < binds; ++k)
        assert(live[k] && a != originals[k][0] && a != originals[k][1] &&
               b != originals[k][0] && b != originals[k][1]);
    originals[binds][0] = a;
    originals[binds][1] = b;
    live[binds] = true;
    if (!binds) {
        assert(*originals[0][field_index] == main_old &&
               *originals[0][1 - field_index] == main_peer);
        assert(atexit(finish) == 0);
    } else
        outer_state();
    if (binds == 2)
        assert(*a == (field_index ? inner_peer : inner_old) &&
               *b == (field_index ? inner_old : inner_peer));
    ++binds;
}
static void phase(size_t p, uint8_t *a, uint8_t *b)
{
    outer_state();
    assert(live[2] && a == originals[2][0] && b == originals[2][1] &&
           p == phases && p <= limit && posts == p && !pending);
    ++phases;
}
static void pre(uint8_t *a, uint8_t *b, size_t index)
{
    outer_state();
    assert(live[2] && a == originals[2][0] && b == originals[2][1] &&
           index == field_index && pres <= limit && !pending &&
           *originals[2][index] == (pres ? input : inner_old) &&
           *originals[2][1 - index] == inner_peer);
    ++pres;
    pending = true;
}
static void post(uint8_t *a, uint8_t *b, size_t index, uint8_t old)
{
    outer_state();
    assert(live[2] && a == originals[2][0] && b == originals[2][1] &&
           index == field_index && posts <= limit && pending &&
           old == (posts ? input : inner_old) &&
           *originals[2][index] == (posts == limit ? terminal : input) &&
           *originals[2][1 - index] == inner_peer);
    ++posts;
    pending = false;
}
static void local(size_t symbol, uint8_t old)
{
    assert(symbol && !pending);
    if (live[2]) {
        assert(locals < limit && old == (locals ? input : inner_old));
        ++locals;
    } else {
        const size_t role = live[1] ? 1 : 0;
        assert(live[role] && calls == (role ? 1u : 2u) && !stores[role] &&
               old == (role ? wrapper_old : main_old));
        changed[role] = true;
        observed_old[role] = old;
        ++stores[role];
        outer_state();
    }
}
static void result(uint8_t old)
{
    outer_state();
    assert(live[2] && posts == limit + 1 && !pending && !results &&
           old == input);
    observed_old[2] = old;
    ++results;
}
static void read_field(uint8_t *p, uint8_t value)
{
    outer_state();
    size_t role = 3, index = 0;
    for (size_t k = 0; k < 3; ++k)
        for (size_t j = 0; j < 2; ++j)
            if (live[k] && p == originals[k][j]) {
                assert(role == 3);
                role = k;
                index = j;
            }
    assert(role < 3 && *p == value);
    if ((role == 0 && !calls) || (role == 1 && !live[2] && !calls)) {
        assert(!read_count[role] &&
               index == (role ? 1 - field_index : field_index) &&
               value == (role ? input : main_old));
    } else {
        const uint8_t expected =
            role == 2
                ? (index == field_index ? terminal : inner_peer)
                : (role == 1 ? input
                             : (index == field_index ? input : main_peer));
        assert(value == expected && !(masks[role] & (1u << index)));
        masks[role] |= 1u << index;
    }
    ++read_count[role];
    if (masks[role] == 3) {
        originals[role][0] = NULL;
        originals[role][1] = NULL;
        live[role] = false;
    }
}
static void call_result(size_t function, size_t call, uint8_t value)
{
    outer_state();
    assert(function && call && value == input && !live[2] && calls < 2);
    if (!calls) {
        assert(live[1] && results == 1);
        inner_fn = function;
    } else
        assert(!live[1] && stores[1] == 1 && function != inner_fn);
    ++calls;
}
static void copy(size_t symbol, uint8_t value)
{
    assert(symbol && value == input && calls && !live[2]);
    if (live[0])
        outer_state();
    ++copies;
}
#define NL_LOOP_BIND(a, b) bind(a, b)
#define NL_CURRENT_PHASE(p, a, b) phase(p, a, b)
#define NL_LOOP_BEFORE(a, b, i) pre(a, b, i)
#define NL_LOOP_AFTER(a, b, i, o) post(a, b, i, o)
#define NL_CURRENT_LOCAL(s, v) local(s, v)
#define NL_CURRENT_RESULT(v) result(v)
#define NL_LOOP_READ(a, v) read_field(a, v)
#define NL_CURRENT_CALL_RESULT(f, c, v) call_result(f, c, v)
#define NL_CURRENT_CALLER_COPY(s, v) copy(s, v)
