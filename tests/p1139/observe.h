#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static const size_t c223_trace[] = C223_TRACE;
static size_t c223_phases, c223_results, c223_actuals, c223_tags, c223_copies;
static void c223_done(void)
{
    assert(c223_phases == sizeof(c223_trace) / sizeof(*c223_trace));
    assert(c223_results == 1 && c223_actuals == 1 && c223_tags == 1 &&
           c223_copies == 1);
    puts(
        "C223 runtime loop result/source Copy/actual/formal/decision/tag PASS");
}
static void c223_phase(size_t n)
{
    if (!c223_phases)
        assert(!atexit(c223_done));
    assert(c223_phases < sizeof(c223_trace) / sizeof(*c223_trace));
    assert(n == c223_trace[c223_phases++]);
}
static void c223_result(size_t n)
{
    assert(c223_results++ == 0 && n == C223_RESULT);
    assert(c223_phases == sizeof(c223_trace) / sizeof(*c223_trace));
}
static void c223_actual(size_t n)
{
    assert(c223_actuals++ == 0 && c223_results == 1 && n == C223_RESULT);
}
static void c223_tag(uint8_t n)
{
    assert(c223_tags++ == 0 && c223_actuals == 1 && n == C223_TAG);
}
static void c223_copy(uint8_t n)
{
    assert(c223_copies++ == 0 && c223_tags == 1 && n == C223_TAG);
}
#define NL_SCALAR_PHASE(n) c223_phase(n)
#define NL_SCALAR_RESULT(n) c223_result(n)
