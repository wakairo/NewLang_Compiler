#ifndef P1152_PASSIVE_OBSERVATION_H
#define P1152_PASSIVE_OBSERVATION_H
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static const size_t c228_trace[] = C228_TRACE;
static const uint8_t c228_initial[] = {C228_FIELD0, C228_FIELD1};
static const uint8_t *c228_fields[2];
static size_t c228_bound, c228_phases, c228_results, c228_operands_count;
static size_t c228_conditions, c228_arms[2], c228_before, c228_after;
static size_t c228_reads[2], c228_old_count;
static void c228_initial_fields(void)
{
    assert(c228_bound == 1 && *c228_fields[0] == c228_initial[0] &&
           *c228_fields[1] == c228_initial[1]);
}
static void c228_bind(const uint8_t *a, const uint8_t *b)
{
    assert(!c228_bound++ && a && b && a != b);
    c228_fields[0] = a;
    c228_fields[1] = b;
    c228_initial_fields();
}
static void c228_phase(size_t value)
{
    c228_initial_fields();
    assert(!c228_results &&
           c228_phases < sizeof(c228_trace) / sizeof(*c228_trace));
    assert(value == c228_trace[c228_phases++]);
}
static void c228_result(size_t value)
{
    c228_initial_fields();
    assert(!c228_results++ && value == C228_RESULT &&
           c228_phases == sizeof(c228_trace) / sizeof(*c228_trace));
}
static void c228_operands(size_t left, size_t right)
{
    c228_initial_fields();
    assert(c228_results == 1 && !c228_operands_count++ && left == C228_RESULT &&
           right == C228_COMPARE);
}
static void c228_condition(_Bool actual)
{
    c228_initial_fields();
    assert(c228_operands_count == 1 && !c228_conditions++ &&
           actual == C228_TRUTH);
}
static void c228_arm(size_t arm)
{
    c228_initial_fields();
    assert(c228_conditions == 1 && arm == C228_ARM && !c228_arms[0] &&
           !c228_arms[1]);
    ++c228_arms[arm];
}
static void c228_before_store(const uint8_t *place, uint8_t incoming)
{
    c228_initial_fields();
    assert(c228_arms[C228_ARM] == 1 && !c228_before++ &&
           place == c228_fields[C228_INDEX] && incoming == C228_UPDATED);
}
static void c228_after_store(const uint8_t *place)
{
    assert(c228_before == 1 && !c228_after++ &&
           place == c228_fields[C228_INDEX] && *place == C228_UPDATED &&
           *c228_fields[1 - C228_INDEX] == c228_initial[1 - C228_INDEX]);
}
static void c228_read(const uint8_t *place, uint8_t value)
{
    assert(c228_after == 1);
    const size_t index = place == c228_fields[0] ? 0 : 1;
    assert(place == c228_fields[index] && !c228_reads[index]++ &&
           value == *place &&
           value == (index == C228_INDEX ? C228_UPDATED : c228_initial[index]));
}
static void c228_old(uint8_t value)
{
    assert(c228_reads[0] == 1 && c228_reads[1] == 1 && !c228_old_count++ &&
           value == c228_initial[C228_INDEX]);
    assert(*c228_fields[C228_INDEX] == C228_UPDATED &&
           *c228_fields[1 - C228_INDEX] == c228_initial[1 - C228_INDEX]);
}
static void c228_finish(void)
{
    /* T's lifetime has ended. Only counters, never retained pointers, here. */
    assert(c228_bound == 1 && c228_results == 1 && c228_operands_count == 1 &&
           c228_conditions == 1 && c228_arms[C228_ARM] == 1 &&
           c228_arms[1 - C228_ARM] == 0 && c228_before == 1 &&
           c228_after == 1 && c228_reads[0] == 1 && c228_reads[1] == 1 &&
           c228_old_count == 1);
    puts("C228 real comparison/one arm/one store/original/old/current/sibling "
         "PASS");
}
#define NL_LOOP_BIND(a, b) c228_bind((a), (b))
#define NL_LOOP_READ(p, v) c228_read((p), (v))
#define NL_SCALAR_PHASE(v) c228_phase((v))
#define NL_SCALAR_RESULT(v) c228_result((v))
#endif
