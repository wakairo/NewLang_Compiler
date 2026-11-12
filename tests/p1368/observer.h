#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#if P1368_PROFILE == 0
enum {
    field_index = 1,
    initial = 37,
    terminal = 203,
    peer = 181,
    limit = 2,
    iteration_locals = 3
};
#else
enum {
    field_index = 0,
    initial = 91,
    terminal = 151,
    peer = 217,
    limit = 3,
    iteration_locals = 4
};
#endif
static uint8_t *storage[2];
static bool live, pending;
static size_t phases, pres, posts, calls, results, reads, locals, fn, site;
static unsigned read_mask;
static void finish(void)
{
    assert(!live && !storage[0] && !storage[1] && !pending &&
           phases == limit + 1 && pres == limit + 1 && posts == limit + 1 &&
           calls == limit && results == 1 && reads == 2 && read_mask == 3);
    puts("P1368 current read/call input/result/old/new/sibling same live "
         "original PASS");
}
static void same(uint8_t *a, uint8_t *b)
{
    assert(live && a == storage[0] && b == storage[1] && a != b &&
           *storage[1 - field_index] == peer);
}
static void bind(uint8_t *a, uint8_t *b)
{
    assert(!live && !phases && a != b);
    storage[0] = a;
    storage[1] = b;
    live = true;
    assert(*storage[field_index] == initial &&
           *storage[1 - field_index] == peer);
    assert(atexit(finish) == 0);
}
static void phase(size_t n, uint8_t *a, uint8_t *b)
{
    same(a, b);
    assert(n == phases && n <= limit && posts == n && !pending &&
           *storage[field_index] == initial);
    if (n)
        assert(locals == iteration_locals);
    locals = 0;
    ++phases;
}
static void local(size_t symbol, uint8_t value)
{
    assert(live && symbol && phases && !pending && value == initial);
    if (phases <= limit) {
        if (!locals)
            assert(calls == phases - 1 && posts == phases - 1 &&
                   value == *storage[field_index]);
        else
            assert(calls == phases);
        assert(locals < iteration_locals);
    } else
        assert(posts == limit + 1 && !locals);
    ++locals;
}
static void call_result(size_t function, size_t call, uint8_t value)
{
    assert(live && !pending && phases <= limit && calls == phases - 1 &&
           locals == 1 && value == initial && value == *storage[field_index]);
    if (!calls) {
        fn = function;
        site = call;
        assert(fn && site);
    } else
        assert(function == fn && call == site);
    ++calls;
}
static void before(uint8_t *a, uint8_t *b, size_t index)
{
    same(a, b);
    assert(index == field_index && !pending && pres == phases - 1 &&
           *storage[field_index] == initial &&
           calls == (phases <= limit ? phases : limit));
    assert(locals == (phases <= limit ? iteration_locals - 1 : 0));
    pending = true;
    ++pres;
}
static void after(uint8_t *a, uint8_t *b, size_t index, uint8_t old)
{
    same(a, b);
    assert(index == field_index && pending && posts + 1 == pres &&
           old == initial &&
           *storage[field_index] == (phases <= limit ? initial : terminal));
    pending = false;
    ++posts;
}
static void result(uint8_t value)
{
    assert(live && !pending && !results && calls == limit &&
           posts == limit + 1 && locals == 1 && value == initial &&
           *storage[field_index] == terminal);
    ++results;
}
static void read_field(uint8_t *p, uint8_t value)
{
    assert(live && results == 1 && !pending && reads < 2);
    size_t index = p == storage[0] ? 0 : p == storage[1] ? 1 : 2;
    assert(index < 2 && !(read_mask & (1u << index)) && *p == value &&
           value == (index == field_index ? terminal : peer));
    read_mask |= 1u << index;
    ++reads;
    if (read_mask == 3) {
        storage[0] = storage[1] = NULL;
        live = false;
    }
}
#define NL_LOOP_BIND(a, b) bind(a, b)
#define NL_CURRENT_PHASE(n, a, b) phase(n, a, b)
#define NL_CURRENT_LOCAL(s, v) local(s, v)
#define NL_CURRENT_CALL_RESULT(f, c, v) call_result(f, c, v)
#define NL_LOOP_BEFORE(a, b, i) before(a, b, i)
#define NL_LOOP_AFTER(a, b, i, o) after(a, b, i, o)
#define NL_CURRENT_RESULT(v) result(v)
#define NL_LOOP_READ(p, v) read_field(p, v)
