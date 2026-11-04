#ifndef P1146_PASSIVE_OBSERVATION_H
#define P1146_PASSIVE_OBSERVATION_H
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static const size_t c226_trace[] = C226_TRACE;
static const uint8_t c226_initial[2] = {C226_FIELD0, C226_FIELD1};
static const uint8_t *c226_fields[2];
static size_t c226_bound, c226_phase_count, c226_result_count;
static size_t c226_actual_count, c226_tag_count, c226_before_count,
    c226_after_count;
static size_t c226_reads[2], c226_old_count;
static void c226_initial_fields(void)
{
    assert(c226_bound == 1 && *c226_fields[0] == c226_initial[0] &&
           *c226_fields[1] == c226_initial[1]);
}
static void c226_bind(const uint8_t *a, const uint8_t *b)
{
    assert(!c226_bound && a && b && a != b);
    c226_fields[0] = a;
    c226_fields[1] = b;
    c226_bound = 1;
    c226_initial_fields();
}
static void c226_phase(size_t value)
{
    c226_initial_fields();
    assert(!c226_result_count &&
           c226_phase_count < sizeof(c226_trace) / sizeof(c226_trace[0]));
    assert(value == c226_trace[c226_phase_count++]);
}
static void c226_result(size_t value)
{
    c226_initial_fields();
    assert(!c226_result_count++ && value == C226_RESULT &&
           c226_phase_count == sizeof(c226_trace) / sizeof(c226_trace[0]));
}
static void c226_actual(size_t value)
{
    c226_initial_fields();
    assert(c226_result_count == 1 && !c226_actual_count++ &&
           value == C226_RESULT);
}
static void c226_tag(uint8_t value)
{
    c226_initial_fields();
    assert(c226_actual_count == 1 && !c226_tag_count++ && value == C226_TAG);
}
static void c226_before_store(const uint8_t *place, uint8_t incoming)
{
    c226_initial_fields();
    assert(c226_tag_count == 1 && !c226_before_count++ &&
           place == c226_fields[C226_WRITE_INDEX] && incoming == C226_TAG);
}
static void c226_after_store(const uint8_t *place)
{
    assert(c226_before_count == 1 && !c226_after_count++ &&
           place == c226_fields[C226_WRITE_INDEX] && *place == C226_TAG &&
           *c226_fields[1 - C226_WRITE_INDEX] ==
               c226_initial[1 - C226_WRITE_INDEX]);
}
static void c226_read(const uint8_t *place, uint8_t value)
{
    assert(c226_after_count == 1);
    const size_t index = place == c226_fields[0] ? 0 : 1;
    assert(place == c226_fields[index] && !c226_reads[index]++ &&
           value == *place &&
           value ==
               (index == C226_WRITE_INDEX ? C226_TAG : c226_initial[index]));
}
static void c226_old(uint8_t value)
{
    assert(c226_reads[0] == 1 && c226_reads[1] == 1 && !c226_old_count++ &&
           value == c226_initial[C226_WRITE_INDEX]);
    assert(*c226_fields[C226_WRITE_INDEX] == C226_TAG &&
           *c226_fields[1 - C226_WRITE_INDEX] ==
               c226_initial[1 - C226_WRITE_INDEX]);
}
static void c226_finish(void)
{
    /* The source T lifetime has ended. Do not read even the retained pointer
     * values here; all pointer/current observations happened while T lived. */
    assert(c226_bound == 1 && c226_result_count == 1 &&
           c226_actual_count == 1 && c226_tag_count == 1 &&
           c226_before_count == 1 && c226_after_count == 1 &&
           c226_reads[0] == 1 && c226_reads[1] == 1 && c226_old_count == 1);
    puts("C226 runtime result/Copy/predicate/original/scoped "
         "store/old/current/sibling PASS");
}
#define NL_LOOP_BIND(a, b) c226_bind((a), (b))
#define NL_LOOP_READ(p, v) c226_read((p), (v))
#define NL_SCALAR_PHASE(v) c226_phase((v))
#define NL_SCALAR_RESULT(v) c226_result((v))
#endif
