/* Passive observations at live program points; no stored pointer is read
 * after its original lifetime. External oracle supplies expected scalars. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static const uint8_t *p1098_roots[2][2];
static size_t p1098_roots_count, p1098_before_count, p1098_after_count;
static size_t p1098_read_count, p1098_seen[2][2];
static void p1098_finish(void)
{
    assert(p1098_roots_count == EXPECT_ROOTS);
    assert(p1098_before_count == 1 && p1098_after_count == 1);
    assert(p1098_read_count == EXPECT_ROOTS * 2);
    for (size_t r = 0; r < EXPECT_ROOTS; ++r)
        for (size_t i = 0; i < 2; ++i)
            assert(p1098_seen[r][i] == 1);
    puts("P1098 native same-original, old/current/sibling/frame, one Change "
         "pair: PASS");
}
static void p1098_bind(const uint8_t *a, const uint8_t *b)
{
    assert(a && b && a != b && p1098_roots_count < EXPECT_ROOTS);
    if (p1098_roots_count == 0)
        assert(atexit(p1098_finish) == 0);
    else
        assert(a != p1098_roots[0][0] && b != p1098_roots[0][1]);
    p1098_roots[p1098_roots_count][0] = a;
    p1098_roots[p1098_roots_count++][1] = b;
}
static void p1098_before(const uint8_t *a, const uint8_t *b, size_t index)
{
    assert(p1098_before_count++ == 0 && p1098_after_count == 0);
    assert(p1098_roots_count == EXPECT_ROOTS && index == EXPECT_INDEX);
    assert(a == p1098_roots[EXPECT_SELECTED - 1][0]);
    assert(b == p1098_roots[EXPECT_SELECTED - 1][1]);
    assert((index ? *b : *a) == EXPECT_OLD);
    assert((index ? *a : *b) == EXPECT_SIBLING);
}
static void p1098_after(const uint8_t *a, const uint8_t *b, size_t index,
                        uint8_t old)
{
    assert(p1098_before_count == 1 && p1098_after_count++ == 0);
    assert(index == EXPECT_INDEX && old == EXPECT_OLD);
    assert(a == p1098_roots[EXPECT_SELECTED - 1][0]);
    assert(b == p1098_roots[EXPECT_SELECTED - 1][1]);
    assert((index ? *b : *a) == EXPECT_NEW);
    assert((index ? *a : *b) == EXPECT_SIBLING);
}
static void p1098_read(const uint8_t *p, uint8_t value)
{
    assert(p1098_after_count == 1);
    bool found = false;
    for (size_t r = 0; r < p1098_roots_count; ++r)
        for (size_t i = 0; i < 2; ++i)
            if (p == p1098_roots[r][i]) {
                const uint8_t expected[2][2] = {
                    {EXPECT_FINAL_00, EXPECT_FINAL_01},
                    {EXPECT_FINAL_10, EXPECT_FINAL_11}};
                assert(!found && value == expected[r][i] && *p == value);
                found = true;
                ++p1098_seen[r][i];
            }
    assert(found);
    ++p1098_read_count;
}
#define NL_LOOP_BIND(a, b) p1098_bind(a, b)
#define NL_LOOP_BEFORE(a, b, i) p1098_before(a, b, i)
#define NL_LOOP_AFTER(a, b, i, old) p1098_after(a, b, i, old)
#define NL_LOOP_READ(p, v) p1098_read(p, v)
