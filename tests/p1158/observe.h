#ifndef P1158_PASSIVE_OBSERVATION_H
#define P1158_PASSIVE_OBSERVATION_H
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static const size_t c232_trace[] = C232_TRACE;
static const uint8_t c232_initial[] = {C232_FIELD0, C232_FIELD1};
static const uint8_t *c232_fields[2];
static size_t c232_bound, c232_phases, c232_results, c232_operands_count;
static size_t c232_conditions, c232_arms[2], c232_before, c232_after;
static size_t c232_reads[2], c232_old_count, c232_early_count;
static void c232_initial_fields(void)
{
    assert(c232_bound == 1 && *c232_fields[0] == c232_initial[0] &&
           *c232_fields[1] == c232_initial[1]);
}
static void c232_bind(const uint8_t *a, const uint8_t *b)
{
    assert(!c232_bound++ && a && b && a != b);
    c232_fields[0] = a;
    c232_fields[1] = b;
    c232_initial_fields();
}
static void c232_phase(size_t value)
{
    c232_initial_fields();
    assert(!c232_results &&
           c232_phases < sizeof(c232_trace) / sizeof(*c232_trace));
    assert(value == c232_trace[c232_phases++]);
}
static void c232_result(size_t value)
{
    c232_initial_fields();
    assert(!c232_results++ && value == C232_RESULT &&
           c232_phases == sizeof(c232_trace) / sizeof(*c232_trace));
}
static void c232_operands(size_t left, size_t right)
{
    c232_initial_fields();
    assert(c232_results == 1 && !c232_operands_count++ && left == C232_RESULT &&
           right == C232_COMPARE);
}
static void c232_condition(_Bool actual)
{
    c232_initial_fields();
    assert(c232_operands_count == 1 && !c232_conditions++ &&
           actual == C232_TRUTH);
}
static void c232_arm(size_t arm)
{
    c232_initial_fields();
    assert(c232_conditions == 1 && arm == C232_ARM && !c232_arms[0] &&
           !c232_arms[1]);
    ++c232_arms[arm];
}
static void c232_before_store(const uint8_t *place, uint8_t incoming)
{
    c232_initial_fields();
    assert(c232_arms[C232_ARM] == 1 && !c232_before++ &&
           place == c232_fields[C232_INDEX] && incoming == C232_UPDATED);
}
static void c232_after_store(const uint8_t *place)
{
    assert(c232_before == 1 && !c232_after++ &&
           place == c232_fields[C232_INDEX] && *place == C232_UPDATED &&
           *c232_fields[1 - C232_INDEX] == c232_initial[1 - C232_INDEX]);
}
static void c232_read(const uint8_t *place, uint8_t value)
{
    assert(c232_after == 1);
    const size_t index = place == c232_fields[0] ? 0 : 1;
    assert(place == c232_fields[index] && !c232_reads[index]++ &&
           value == *place &&
           value == (index == C232_INDEX ? C232_UPDATED : c232_initial[index]));
}
static void c232_old(uint8_t value)
{
    assert(c232_reads[0] == 1 && c232_reads[1] == 1 && !c232_old_count++ &&
           value == c232_initial[C232_INDEX]);
    assert(*c232_fields[C232_INDEX] == C232_UPDATED &&
           *c232_fields[1 - C232_INDEX] == c232_initial[1 - C232_INDEX]);
}
static void c232_early(void)
{
    c232_initial_fields();
    assert(C232_EARLY && c232_arms[C232_RETURN_ARM] == 1 &&
           !c232_early_count++ && !c232_before && !c232_after &&
           !c232_reads[0] && !c232_reads[1] && !c232_old_count);
}
static void c232_finish(void)
{
    /* Only counters after T's lifetime; never dereference retained pointers. */
    assert(c232_bound == 1 && c232_results == 1 && c232_operands_count == 1 &&
           c232_conditions == 1 && c232_arms[C232_ARM] == 1 &&
           c232_arms[1 - C232_ARM] == 0);
    if (C232_EARLY) {
        assert(c232_early_count == 1 && !c232_before && !c232_after &&
               !c232_reads[0] && !c232_reads[1] && !c232_old_count);
    } else {
        assert(!c232_early_count && c232_before == 1 && c232_after == 1 &&
               c232_reads[0] == 1 && c232_reads[1] == 1 && c232_old_count == 1);
    }
    puts("C232 real comparison/RETURN-or-normal/original/old/current/sibling "
         "PASS");
}
#define NL_LOOP_BIND(a, b) c232_bind((a), (b))
#define NL_LOOP_READ(p, v) c232_read((p), (v))
#define NL_SCALAR_PHASE(v) c232_phase((v))
#define NL_SCALAR_RESULT(v) c232_result((v))
#endif
