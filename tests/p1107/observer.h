/* Passive observations at live program points; no stored pointer is read
 * after its original lifetime. External oracle supplies expected scalars. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static const uint8_t *p1107_roots[2][2];
static size_t p1107_roots_count, p1107_before_count, p1107_after_count;
static size_t p1107_read_count, p1107_seen[2][2];
static size_t p1107_calls, p1107_changes;
static void p1107_finish(void)
{
    assert(p1107_roots_count == EXPECT_ROOTS);
    assert(p1107_before_count == 1 && p1107_after_count == 1);
    assert(p1107_read_count == EXPECT_ROOTS * 2);
    assert(p1107_calls == EXPECT_HELPER && p1107_changes == EXPECT_HELPER);
    for (size_t r = 0; r < EXPECT_ROOTS; ++r)
        for (size_t i = 0; i < 2; ++i)
            assert(p1107_seen[r][i] == 1);
    puts("P1107 native same-original, old/current/sibling/frame, one Change "
         "pair: PASS");
}
static void p1107_bind(const uint8_t *a, const uint8_t *b)
{
    assert(a && b && a != b && p1107_roots_count < EXPECT_ROOTS);
    if (p1107_roots_count == 0)
        assert(atexit(p1107_finish) == 0);
    else
        assert(a != p1107_roots[0][0] && b != p1107_roots[0][1]);
    p1107_roots[p1107_roots_count][0] = a;
    p1107_roots[p1107_roots_count++][1] = b;
}
static void p1107_before(const uint8_t *a, const uint8_t *b, size_t index)
{
    assert(p1107_before_count++ == 0 && p1107_after_count == 0);
    assert(p1107_roots_count == EXPECT_ROOTS && index == EXPECT_INDEX);
    assert(a == p1107_roots[EXPECT_SELECTED - 1][0]);
    assert(b == p1107_roots[EXPECT_SELECTED - 1][1]);
    assert((index ? *b : *a) == EXPECT_OLD);
    assert((index ? *a : *b) == EXPECT_SIBLING);
}
static void p1107_after(const uint8_t *a, const uint8_t *b, size_t index,
                        uint8_t old)
{
    assert(p1107_before_count == 1 && p1107_after_count++ == 0);
    assert(index == EXPECT_INDEX && old == EXPECT_OLD);
    assert(a == p1107_roots[EXPECT_SELECTED - 1][0]);
    assert(b == p1107_roots[EXPECT_SELECTED - 1][1]);
    assert((index ? *b : *a) == EXPECT_NEW);
    assert((index ? *a : *b) == EXPECT_SIBLING);
}
static void p1107_read(const uint8_t *p, uint8_t value)
{
    assert(p1107_after_count == 1);
    bool found = false;
    for (size_t r = 0; r < p1107_roots_count; ++r)
        for (size_t i = 0; i < 2; ++i)
            if (p == p1107_roots[r][i]) {
                const uint8_t expected[2][2] = {
                    {EXPECT_FINAL_00, EXPECT_FINAL_01},
                    {EXPECT_FINAL_10, EXPECT_FINAL_11}};
                assert(!found && value == expected[r][i] && *p == value);
                found = true;
                ++p1107_seen[r][i];
            }
    assert(found);
    ++p1107_read_count;
}
#define NL_LOOP_BIND(a, b) p1107_bind(a, b)
#define NL_LOOP_BEFORE(a, b, i) p1107_before(a, b, i)
#define NL_LOOP_AFTER(a, b, i, old) p1107_after(a, b, i, old)
#define NL_LOOP_READ(p, v) p1107_read(p, v)

#if EXPECT_HELPER
static void p1107_helper_enter(const uint8_t *p, uint8_t incoming)
{
    assert(p1107_before_count == 1 && p1107_after_count == 0);
    assert(p1107_calls++ == 0 && p1107_changes == 0);
    assert(p == p1107_roots[EXPECT_SELECTED - 1][EXPECT_INDEX]);
    assert(*p == EXPECT_OLD && incoming == EXPECT_NEW);
}
static void p1107_helper_changed(const uint8_t *p)
{
    assert(p1107_calls == 1 && p1107_changes++ == 0 && p1107_after_count == 0);
    assert(p == p1107_roots[EXPECT_SELECTED - 1][EXPECT_INDEX] &&
           *p == EXPECT_NEW);
}
#endif
