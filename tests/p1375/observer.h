#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#if P1375_PROFILE == 0
enum {
    field_index = 1,
    initial = 43,
    old_value = 137,
    local_new = 229,
    terminal = 241,
    peer = 181,
    limit = 3
};
#else
enum {
    field_index = 0,
    initial = 89,
    old_value = 211,
    local_new = 73,
    terminal = 193,
    peer = 157,
    limit = 2
};
#endif
static uint8_t *storage[2], *callee[2];
static uint8_t actual_now;
static bool live, entered, local_live, changed, ended, pending;
static size_t phases, calls, inputs, ends, pres, posts, locals, results, reads;
static size_t fn, site, local_reads, copies, first_changes;
static unsigned local_mask, read_mask;
static void finish(void)
{
    assert(!live && !storage[0] && !storage[1] && !callee[0] && !callee[1] &&
           !entered && !local_live && !ended && !pending &&
           phases == limit + 1 && calls == limit && inputs == limit &&
           ends == limit && pres == limit + 1 && posts == limit + 1 &&
           results == 1 && reads == 2 && read_mask == 3 && first_changes == 1);
    puts("P1375 live caller/local distinct; input/old/Copy/current/sibling "
         "and local ending PASS");
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
           *storage[1 - field_index] == peer && atexit(finish) == 0);
}
static void phase(size_t n, uint8_t *a, uint8_t *b)
{
    same(a, b);
    assert(n == phases && n <= limit && posts == n && !pending && !entered &&
           !local_live && !ended && !callee[0] && !callee[1] && calls == n &&
           *storage[field_index] == (n ? old_value : initial));
    if (n)
        assert(locals == 3);
    actual_now = *storage[field_index];
    locals = 0;
    ++phases;
}
static void local(size_t symbol, uint8_t value)
{
    assert(live && symbol && phases <= limit && !pending && !entered &&
           !local_live && !ended && locals < 3);
    if (!locals)
        assert(calls == phases - 1 && value == actual_now);
    else if (locals == 1)
        assert(calls == phases && value == old_value && posts == phases - 1);
    else
        assert(calls == phases && posts == phases && value == actual_now);
    ++locals;
}
static void callee_input(size_t function, uint8_t input)
{
    assert(live && !pending && !entered && !local_live && !ended &&
           phases <= limit && calls == phases - 1 && inputs == calls &&
           locals == 1 && input == actual_now &&
           input == *storage[field_index]);
    if (!inputs) {
        fn = function;
        assert(fn);
    } else
        assert(function == fn);
    entered = true;
    changed = false;
    local_reads = copies = local_mask = 0;
    ++inputs;
}
static void callee_bind(uint8_t *a, uint8_t *b)
{
    assert(live && entered && !local_live && !ended && a != b && !changed &&
           !local_reads && !copies);
    /* Only compare simultaneously live originals, never recycled addresses. */
    for (size_t i = 0; i < 2; ++i)
        assert(a != storage[i] && b != storage[i]);
    callee[0] = a;
    callee[1] = b;
    local_live = true;
    assert(*callee[field_index] == old_value &&
           *callee[1 - field_index] == actual_now &&
           *storage[field_index] == actual_now);
}
static void callee_read(uint8_t *p, uint8_t value)
{
    assert(live && entered && local_live && !ended && local_reads < 3 &&
           *storage[field_index] == actual_now);
    size_t i = p == callee[0] ? 0 : p == callee[1] ? 1 : 2;
    assert(i < 2 && value == *p);
    if (!changed)
        assert(!local_reads && i == field_index && value == old_value);
    else {
        assert(local_reads > 0 && !(local_mask & (1u << i)) &&
               value == (i == field_index ? local_new : actual_now));
        local_mask |= 1u << i;
    }
    ++local_reads;
}
static void callee_change(uint8_t *a, uint8_t *b, size_t i, uint8_t old)
{
    assert(live && entered && local_live && !ended && !changed &&
           a == callee[0] && b == callee[1] && i == field_index &&
           local_reads == 1 && old == old_value &&
           *callee[field_index] == local_new &&
           *callee[1 - field_index] == actual_now &&
           *storage[field_index] == actual_now &&
           *storage[1 - field_index] == peer);
    changed = true;
}
static void callee_copy(size_t symbol, uint8_t value)
{
    assert(live && entered && local_live && !ended && symbol && changed &&
           !copies && local_reads == 3 && local_mask == 3 &&
           value == old_value && *storage[field_index] == actual_now);
    ++copies;
}
static void callee_end(uint8_t value)
{
    assert(live && entered && local_live && !ended && changed &&
           local_reads == 3 && local_mask == 3 && copies == 1 &&
           value == old_value && *callee[field_index] == local_new &&
           *callee[1 - field_index] == actual_now &&
           *storage[field_index] == actual_now);
    /* Still in the callee lexical lifetime, before its normal return. */
    callee[0] = callee[1] = NULL;
    entered = local_live = false;
    ended = true;
    ++ends;
}
static void call_result(size_t function, size_t call, uint8_t value)
{
    assert(live && !pending && !entered && !local_live && ended && !callee[0] &&
           !callee[1] && phases <= limit && calls == phases - 1 &&
           inputs == calls + 1 && ends == inputs && function == fn &&
           locals == 1 && value == old_value &&
           *storage[field_index] == actual_now);
    if (!calls) {
        site = call;
        assert(site && value != actual_now);
    } else
        assert(call == site);
    ended = false;
    ++calls;
}
static void before(uint8_t *a, uint8_t *b, size_t index)
{
    same(a, b);
    assert(index == field_index && !pending && !entered && !local_live &&
           !ended && pres == phases - 1 &&
           *storage[field_index] == actual_now &&
           calls == (phases <= limit ? phases : limit) &&
           locals == (phases <= limit ? 2u : 0u));
    pending = true;
    ++pres;
}
static void after(uint8_t *a, uint8_t *b, size_t index, uint8_t old)
{
    same(a, b);
    assert(index == field_index && pending && posts + 1 == pres &&
           old == actual_now &&
           *storage[field_index] == (phases <= limit ? old_value : terminal));
    if (phases == 1) {
        assert(old == initial && *storage[field_index] == old_value &&
               old != *storage[field_index]);
        ++first_changes;
    }
    pending = false;
    ++posts;
}
static void result(uint8_t value)
{
    assert(live && !pending && !results && calls == limit &&
           posts == limit + 1 && locals == 0 && value == old_value &&
           *storage[field_index] == terminal);
    ++results;
}
static void read_field(uint8_t *p, uint8_t value)
{
    assert(live && results == 1 && !pending && reads < 2);
    size_t i = p == storage[0] ? 0 : p == storage[1] ? 1 : 2;
    assert(i < 2 && !(read_mask & (1u << i)) && *p == value &&
           value == (i == field_index ? terminal : peer));
    read_mask |= 1u << i;
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
#define NL_CURRENT_CALLEE_INPUT(f, v) callee_input(f, v)
#define NL_CURRENT_CALLEE_BIND(a, b) callee_bind(a, b)
#define NL_CURRENT_CALLEE_READ(p, v) callee_read(p, v)
#define NL_CURRENT_CALLEE_CHANGE(a, b, i, v) callee_change(a, b, i, v)
#define NL_CURRENT_CALLEE_COPY(s, v) callee_copy(s, v)
#define NL_CURRENT_CALLEE_END(v) callee_end(v)
