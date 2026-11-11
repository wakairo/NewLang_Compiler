#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if P1339_PROFILE == 0
enum { input = 67 };
#else
enum { input = 109 };
#endif
enum { initial = 43, sibling = 181, final = 233, limit = 2 };

static uint8_t *fields[2];
static size_t phases, before, after, locals, reads, read_mask, results, calls,
    copies;
static size_t caller_symbols[2];
static uint8_t actual_old;
static bool pending;

static void p1339_finish(void)
{
    /* The callee-local original has ended; keep only scalar observations. */
    assert(phases == limit + 1 && before == limit + 1 &&
           after == limit + 1 && locals == limit && !pending && reads == 2 &&
           read_mask == 3 && results == 1 && calls == 1 && copies == 2 &&
           actual_old == input);
    printf("P1339 physical original input%u old%u current%u sibling%u PASS\n",
           (unsigned)input, (unsigned)actual_old, (unsigned)final,
           (unsigned)sibling);
}

static void p1339_bind(uint8_t *a, uint8_t *b)
{
    assert(!fields[0] && !phases && a != b && *a == initial &&
           *b == sibling);
    fields[0] = a;
    fields[1] = b;
    assert(atexit(p1339_finish) == 0);
}

static void p1339_phase(size_t phase, uint8_t *a, uint8_t *b)
{
    assert(a == fields[0] && b == fields[1] && phase == phases &&
           phase <= limit && after == phase && !pending);
    ++phases;
}

static void p1339_before(uint8_t *a, uint8_t *b, size_t field)
{
    assert(a == fields[0] && b == fields[1] && field == 0 &&
           before <= limit && !pending &&
           *a == (before == 0 ? initial : input) && *b == sibling);
    ++before;
    pending = true;
}

static void p1339_after(uint8_t *a, uint8_t *b, size_t field, uint8_t old)
{
    const uint8_t expected_old = after == 0 ? initial : input;
    const uint8_t expected_new = after == limit ? final : input;
    assert(a == fields[0] && b == fields[1] && field == 0 &&
           after <= limit && pending && old == expected_old &&
           *a == expected_new && *b == sibling);
    ++after;
    pending = false;
}

static void p1339_local(size_t symbol, uint8_t old)
{
    assert(symbol && locals < limit && old == (locals ? input : initial) &&
           !pending);
    ++locals;
}

static void p1339_result(uint8_t old)
{
    assert(old == input && !pending && after == limit + 1 && !results);
    actual_old = old;
    ++results;
}

static void p1339_read(uint8_t *field, uint8_t value)
{
    const size_t index = field == fields[0] ? 0 : 1;
    assert(field == fields[index] && value == (index ? sibling : final) &&
           !(read_mask & (1u << index)));
    read_mask |= 1u << index;
    ++reads;
}

static void p1339_call(size_t function, size_t call, uint8_t value)
{
    /* Never dereference the ended callee-local original here. */
    assert(function && call && !calls && results == 1 && reads == 2 &&
           value == actual_old && value == input);
    ++calls;
}

static void p1339_copy(size_t symbol, uint8_t value)
{
    assert(calls == 1 && copies < 2 && symbol && value == actual_old &&
           (!copies || symbol != caller_symbols[0]));
    caller_symbols[copies++] = symbol;
}

#define NL_LOOP_BIND(a, b) p1339_bind(a, b)
#define NL_CURRENT_PHASE(p, a, b) p1339_phase(p, a, b)
#define NL_LOOP_BEFORE(a, b, i) p1339_before(a, b, i)
#define NL_LOOP_AFTER(a, b, i, o) p1339_after(a, b, i, o)
#define NL_CURRENT_LOCAL(s, v) p1339_local(s, v)
#define NL_CURRENT_RESULT(v) p1339_result(v)
#define NL_LOOP_READ(a, v) p1339_read(a, v)
#define NL_CURRENT_CALL_RESULT(f, c, v) p1339_call(f, c, v)
#define NL_CURRENT_CALLER_COPY(s, v) p1339_copy(s, v)
