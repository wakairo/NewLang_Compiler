#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#if P1372_PROFILE == 0
enum {
    field_index = 1,
    initial = 47,
    terminal = 239,
    peer = 173,
    limit = 3,
    local_new = 227,
    local_peer = 109,
    iteration_locals = 3,
    pre_copies = 0,
    tail_copies = 1
};
#else
enum {
    field_index = 0,
    initial = 83,
    terminal = 197,
    peer = 211,
    limit = 2,
    local_new = 201,
    local_peer = 157,
    iteration_locals = 5,
    pre_copies = 1,
    tail_copies = 3
};
#endif
static uint8_t *storage[2], *callee[2];
static bool live, pending, entered, local_live, changed, ended;
static size_t phases, pres, posts, calls, results, reads, locals, fn, site;
static size_t local_reads, copies, inputs, ends;
static unsigned read_mask, local_mask;
static void finish(void)
{
    assert(!live && !storage[0] && !storage[1] && !callee[0] && !callee[1] &&
           !pending && !entered && !local_live && !ended &&
           phases == limit + 1 && pres == limit + 1 && posts == limit + 1 &&
           inputs == limit && calls == limit && ends == limit && results == 1 &&
           reads == 2 && read_mask == 3);
    puts("P1372 live caller/local distinct; input/old/Copy/current/sibling "
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
           *storage[1 - field_index] == peer);
    assert(atexit(finish) == 0);
}
static void phase(size_t n, uint8_t *a, uint8_t *b)
{
    same(a, b);
    assert(n == phases && n <= limit && posts == n && !pending && !entered &&
           !local_live && !ended && !callee[0] && !callee[1] &&
           *storage[field_index] == initial);
    if (n)
        assert(locals == iteration_locals);
    locals = 0;
    ++phases;
}
static void local(size_t symbol, uint8_t value)
{
    assert(live && symbol && phases && phases <= limit && !pending &&
           !entered && !local_live && !ended && value == initial);
    if (locals < 1u + pre_copies)
        assert(calls == phases - 1 && value == *storage[field_index]);
    else
        assert(calls == phases);
    assert(locals < iteration_locals);
    ++locals;
}
static void callee_input(size_t function, uint8_t input)
{
    assert(live && !pending && phases <= limit && calls == phases - 1 &&
           inputs == calls && !entered && !local_live && !ended && !callee[0] &&
           !callee[1] && locals == 1u + pre_copies && input == initial &&
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
    assert(live && entered && !local_live && !ended && a != b &&
           copies == pre_copies && !changed && !local_reads);
    /* Compare only simultaneously live objects. Never retain a pointer
     * through callee ending or compare stack addresses between calls. */
    for (size_t i = 0; i < 2; ++i)
        assert(a != storage[i] && b != storage[i]);
    callee[0] = a;
    callee[1] = b;
    local_live = true;
    assert(*callee[field_index] == initial &&
           *callee[1 - field_index] == local_peer &&
           *storage[field_index] == initial);
}
static void callee_read(uint8_t *p, uint8_t value)
{
    assert(live && entered && local_live && !ended && local_reads < 3 &&
           *storage[field_index] == initial);
    size_t i = p == callee[0] ? 0 : p == callee[1] ? 1 : 2;
    assert(i < 2 && value == *p);
    if (!changed)
        assert(!local_reads && i == field_index && value == initial);
    else {
        assert(local_reads > 0 && !(local_mask & (1u << i)) &&
               value == (i == field_index ? local_new : local_peer));
        local_mask |= 1u << i;
    }
    ++local_reads;
}
static void callee_change(uint8_t *a, uint8_t *b, size_t i, uint8_t old)
{
    assert(
        live && entered && local_live && !ended && !changed && a == callee[0] &&
        b == callee[1] && i == field_index && local_reads == 1 &&
        old == initial && *callee[field_index] == local_new &&
        *callee[1 - field_index] == local_peer &&
        *storage[field_index] == initial && *storage[1 - field_index] == peer);
    changed = true;
}
static void callee_copy(size_t symbol, uint8_t value)
{
    assert(live && entered && !ended && symbol && value == initial &&
           *storage[field_index] == initial);
    if (copies < pre_copies)
        assert(!local_live && !changed);
    else
        assert(local_live && changed && local_reads == 3 && local_mask == 3 &&
               copies < pre_copies + tail_copies);
    ++copies;
}
static void callee_end(uint8_t value)
{
    assert(live && entered && local_live && !ended && changed &&
           local_reads == 3 && local_mask == 3 &&
           copies == pre_copies + tail_copies && value == initial &&
           *callee[field_index] == local_new &&
           *callee[1 - field_index] == local_peer &&
           *storage[field_index] == initial);
    /* The C local is still live here, immediately before the normal return. */
    callee[0] = callee[1] = NULL;
    local_live = entered = false;
    ended = true;
    ++ends;
}
static void call_result(size_t function, size_t call, uint8_t value)
{
    assert(live && !pending && !entered && !local_live && ended && !callee[0] &&
           !callee[1] && phases <= limit && calls == phases - 1 &&
           inputs == calls + 1 && ends == inputs && function == fn &&
           locals == 1u + pre_copies && value == initial &&
           value == *storage[field_index]);
    if (!calls) {
        site = call;
        assert(site);
    } else
        assert(call == site);
    ended = false;
    ++calls;
}
static void before(uint8_t *a, uint8_t *b, size_t index)
{
    same(a, b);
    assert(index == field_index && !pending && !entered && !local_live &&
           !ended && pres == phases - 1 && *storage[field_index] == initial &&
           calls == (phases <= limit ? phases : limit) &&
           locals == (phases <= limit ? iteration_locals - 1 : 0));
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
           posts == limit + 1 && locals == 0 && value == initial &&
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
