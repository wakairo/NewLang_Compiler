/* Passive observation of genuine generated C storage. No ended-storage read. */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
static const uint8_t *p1123_fields[2][2];
static size_t p1123_binds, p1123_before, p1123_after, p1123_reads;
static size_t p1123_calls, p1123_stores, p1123_phases, p1123_continues;
static size_t p1123_prefix, p1123_caller;
static void p1123_frame(const uint8_t *a, const uint8_t *b)
{
    assert(a == p1123_fields[P1123_SELECTED][0]);
    assert(b == p1123_fields[P1123_SELECTED][1]);
    assert(*a == P1123_ENTRY0 && *b == P1123_ENTRY1);
    assert(*p1123_fields[1 - P1123_SELECTED][0] == P1123_Q0);
    assert(*p1123_fields[1 - P1123_SELECTED][1] == P1123_Q1);
}
static void p1123_finish(void)
{
    assert(p1123_binds == 2 && p1123_before == 1 && p1123_after == 1);
    assert(p1123_reads == 4 && p1123_calls == 1 && p1123_stores == 1);
    assert(p1123_continues == P1123_CONTINUES);
    assert(p1123_phases == P1123_CONTINUES + 1);
    assert(p1123_caller == 1 && p1123_prefix == P1123_PREFIX);
    puts("P1123 actual backedge/fresh phase/same "
         "original/old/postcurrent/sibling/ambient/caller PASS");
}
static void p1123_bind(const uint8_t *a, const uint8_t *b)
{
    assert(a && b && a != b && p1123_binds < 2);
    size_t n = p1123_binds++;
    p1123_fields[n][0] = a;
    p1123_fields[n][1] = b;
    bool selected = n == P1123_SELECTED;
    assert(*a == (selected ? P1123_F0 : P1123_Q0));
    assert(*b == (selected ? P1123_F1 : P1123_Q1));
    if (!n)
        assert(atexit(p1123_finish) == 0);
    else
        assert(a != p1123_fields[0][0] && a != p1123_fields[0][1] &&
               b != p1123_fields[0][0] && b != p1123_fields[0][1]);
}
static void p1123_phase(size_t phase, const uint8_t *a, const uint8_t *b)
{
    assert(p1123_binds == 2 && !p1123_before && !p1123_after && !p1123_calls);
    assert(p1123_phases <= P1123_CONTINUES);
    assert(phase == (p1123_phases ? P1123_NEXT_PHASE : P1123_INITIAL));
    assert(p1123_prefix == P1123_PREFIX);
    ++p1123_phases;
    p1123_frame(a, b);
}
static void p1123_continue(size_t old, size_t next, const uint8_t *a,
                           const uint8_t *b)
{
    assert(p1123_continues++ == 0 && P1123_CONTINUES == 1);
    assert(p1123_phases == 1 && !p1123_before && !p1123_calls);
    assert(old == P1123_INITIAL && next == P1123_NEXT_PHASE);
    p1123_frame(a, b);
}
static void p1123_enter(const uint8_t *a, const uint8_t *b, size_t index)
{
    assert(p1123_before++ == 0 && index == P1123_INDEX);
    assert(p1123_phases == P1123_CONTINUES + 1 &&
           p1123_continues == P1123_CONTINUES);
    p1123_frame(a, b);
}
static void p1123_exit(const uint8_t *a, const uint8_t *b, size_t index,
                       uint8_t old)
{
    assert(p1123_after++ == 0 && index == P1123_INDEX && old == P1123_OLD);
    assert(a == p1123_fields[P1123_SELECTED][0] &&
           b == p1123_fields[P1123_SELECTED][1]);
    assert(*a == (P1123_INDEX == 0 ? P1123_NEXT : P1123_ENTRY0));
    assert(*b == (P1123_INDEX == 1 ? P1123_NEXT : P1123_ENTRY1));
    assert(*p1123_fields[1 - P1123_SELECTED][0] == P1123_Q0);
    assert(*p1123_fields[1 - P1123_SELECTED][1] == P1123_Q1);
}
static void p1123_read(const uint8_t *p, uint8_t value)
{
    assert(p1123_after == 1 && p1123_caller == 1 && p1123_reads++ < 4);
    bool found = false;
    for (size_t i = 0; i < 2; ++i)
        for (size_t j = 0; j < 2; ++j)
            if (p == p1123_fields[i][j]) {
                unsigned expected =
                    i == P1123_SELECTED
                        ? (j == P1123_INDEX ? P1123_NEXT : P1123_OLD)
                        : (j ? P1123_Q1 : P1123_Q0);
                assert(value == expected && *p == expected);
                found = true;
            }
    assert(found);
}
static void p1123_helper(const uint8_t *p, uint8_t incoming)
{
    assert(p1123_calls++ == 0 && p1123_before == 1 && !p1123_after);
    assert(p == p1123_fields[P1123_SELECTED][P1123_INDEX]);
    assert(*p == P1123_OLD && incoming == P1123_NEXT);
}
static void p1123_changed(const uint8_t *p)
{
    assert(p1123_stores++ == 0 && p1123_calls == 1);
    assert(p == p1123_fields[P1123_SELECTED][P1123_INDEX] && *p == P1123_NEXT);
}
static void p1123_outer_store(const uint8_t *p)
{
    if (!p1123_before) {
        assert(P1123_PREFIX == 1 && p1123_prefix++ == 0 && !p1123_phases);
        assert(p == p1123_fields[P1123_SELECTED][P1123_INDEX] &&
               *p == P1123_OLD);
    } else {
        assert(p1123_after == 1 && p1123_caller++ == 0);
        assert(p == p1123_fields[P1123_SELECTED][1 - P1123_INDEX] &&
               *p == P1123_OLD);
    }
}
#define NL_LOOP_BIND(a, b) p1123_bind(a, b)
#define NL_LOOP_PHASE(p, a, b) p1123_phase(p, a, b)
#define NL_LOOP_CONTINUE(o, n, a, b) p1123_continue(o, n, a, b)
#define NL_LOOP_BEFORE(a, b, i) p1123_enter(a, b, i)
#define NL_LOOP_AFTER(a, b, i, o) p1123_exit(a, b, i, o)
#define NL_LOOP_READ(p, v) p1123_read(p, v)
