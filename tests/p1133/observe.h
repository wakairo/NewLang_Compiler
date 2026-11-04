/* Passive observation of genuine generated C storage. No ended-storage read. */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
static const uint8_t *p1133_fields[2][2];
static size_t p1133_binds, p1133_before, p1133_after, p1133_reads;
static size_t p1133_calls, p1133_stores, p1133_phases, p1133_continues;
static size_t p1133_prefix, p1133_caller;
static const size_t p1133_trace[] = P1133_TRACE;
static void p1133_frame(const uint8_t *a, const uint8_t *b)
{
    assert(a == p1133_fields[P1133_SELECTED][0]);
    assert(b == p1133_fields[P1133_SELECTED][1]);
    assert(*a == P1133_ENTRY0 && *b == P1133_ENTRY1);
    assert(*p1133_fields[1 - P1133_SELECTED][0] == P1133_Q0);
    assert(*p1133_fields[1 - P1133_SELECTED][1] == P1133_Q1);
}
static void p1133_finish(void)
{
    assert(p1133_binds == 2 && p1133_before == 1 && p1133_after == 1);
    assert(p1133_reads == 4 && p1133_calls == 1 && p1133_stores == 1);
    assert(p1133_continues == P1133_CONTINUES);
    assert(p1133_phases == P1133_CONTINUES + 1);
    assert(p1133_caller == 1 && p1133_prefix == P1133_PREFIX);
    puts("P1133 actual backedge/fresh phase/same "
         "original/old/postcurrent/sibling/ambient/caller PASS");
}
static void p1133_bind(const uint8_t *a, const uint8_t *b)
{
    assert(a && b && a != b && p1133_binds < 2);
    size_t n = p1133_binds++;
    p1133_fields[n][0] = a;
    p1133_fields[n][1] = b;
    bool selected = n == P1133_SELECTED;
    assert(*a == (selected ? P1133_F0 : P1133_Q0));
    assert(*b == (selected ? P1133_F1 : P1133_Q1));
    if (!n)
        assert(atexit(p1133_finish) == 0);
    else
        assert(a != p1133_fields[0][0] && a != p1133_fields[0][1] &&
               b != p1133_fields[0][0] && b != p1133_fields[0][1]);
}
static void p1133_phase(size_t phase, const uint8_t *a, const uint8_t *b)
{
    assert(p1133_binds == 2 && !p1133_before && !p1133_after && !p1133_calls);
    assert(p1133_phases <= P1133_CONTINUES);
    assert(phase == p1133_trace[p1133_phases]);
    assert(p1133_prefix == P1133_PREFIX);
    ++p1133_phases;
    p1133_frame(a, b);
}
static void p1133_continue(size_t old, size_t next, const uint8_t *a,
                           const uint8_t *b)
{
    assert(p1133_phases <= P1133_CONTINUES);
    assert(p1133_phases == p1133_continues + 1 && !p1133_before &&
           !p1133_calls);
    assert(old == p1133_trace[p1133_continues] &&
           next == p1133_trace[p1133_continues + 1]);
    ++p1133_continues;
    p1133_frame(a, b);
}
static void p1133_enter(const uint8_t *a, const uint8_t *b, size_t index)
{
    assert(p1133_before++ == 0 && index == P1133_INDEX);
    assert(p1133_phases == P1133_CONTINUES + 1 &&
           p1133_continues == P1133_CONTINUES);
    p1133_frame(a, b);
}
static void p1133_exit(const uint8_t *a, const uint8_t *b, size_t index,
                       uint8_t old)
{
    assert(p1133_after++ == 0 && index == P1133_INDEX && old == P1133_OLD);
    assert(a == p1133_fields[P1133_SELECTED][0] &&
           b == p1133_fields[P1133_SELECTED][1]);
    assert(*a == (P1133_INDEX == 0 ? P1133_NEXT : P1133_ENTRY0));
    assert(*b == (P1133_INDEX == 1 ? P1133_NEXT : P1133_ENTRY1));
    assert(*p1133_fields[1 - P1133_SELECTED][0] == P1133_Q0);
    assert(*p1133_fields[1 - P1133_SELECTED][1] == P1133_Q1);
}
static void p1133_read(const uint8_t *p, uint8_t value)
{
    assert(p1133_after == 1 && p1133_caller == 1 && p1133_reads++ < 4);
    bool found = false;
    for (size_t i = 0; i < 2; ++i)
        for (size_t j = 0; j < 2; ++j)
            if (p == p1133_fields[i][j]) {
                unsigned expected =
                    i == P1133_SELECTED
                        ? (j == P1133_INDEX ? P1133_NEXT : P1133_OLD)
                        : (j ? P1133_Q1 : P1133_Q0);
                assert(value == expected && *p == expected);
                found = true;
            }
    assert(found);
}
static void p1133_helper(const uint8_t *p, uint8_t incoming)
{
    assert(p1133_calls++ == 0 && p1133_before == 1 && !p1133_after);
    assert(p == p1133_fields[P1133_SELECTED][P1133_INDEX]);
    assert(*p == P1133_OLD && incoming == P1133_NEXT);
}
static void p1133_changed(const uint8_t *p)
{
    assert(p1133_stores++ == 0 && p1133_calls == 1);
    assert(p == p1133_fields[P1133_SELECTED][P1133_INDEX] && *p == P1133_NEXT);
}
static void p1133_outer_store(const uint8_t *p)
{
    if (!p1133_before) {
        assert(P1133_PREFIX == 1 && p1133_prefix++ == 0 && !p1133_phases);
        assert(p == p1133_fields[P1133_SELECTED][P1133_INDEX] &&
               *p == P1133_OLD);
    } else {
        assert(p1133_after == 1 && p1133_caller++ == 0);
        assert(p == p1133_fields[P1133_SELECTED][1 - P1133_INDEX] &&
               *p == P1133_OLD);
    }
}
#define NL_LOOP_BIND(a, b) p1133_bind(a, b)
#define NL_LOOP_PHASE(p, a, b) p1133_phase(p, a, b)
#define NL_LOOP_CONTINUE(o, n, a, b) p1133_continue(o, n, a, b)
#define NL_LOOP_BEFORE(a, b, i) p1133_enter(a, b, i)
#define NL_LOOP_AFTER(a, b, i, o) p1133_exit(a, b, i, o)
#define NL_LOOP_READ(p, v) p1133_read(p, v)
