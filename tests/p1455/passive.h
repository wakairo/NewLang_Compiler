#ifndef P1455_PASSIVE_H
#define P1455_PASSIVE_H
#include <stdio.h>
#if P1455_CASE_BACK
#define FRONT 101u
#define BACK 207u
#define OLD 47u
#define SPARE 241u
#else
#define FRONT 59u
#define BACK 173u
#define OLD 23u
#define SPARE 229u
#endif
#define INPUT (P1455_CASE_BACK ? BACK : FRONT)
#define NEXT (P1455_CASE_BACK ? 0u : FRONT)
static size_t p1455_call_values, p1455_completed;
static _Bool p1455_T_bound;

static const nl_node *p1455_h[2];
static size_t p1455_domain[2];
static size_t p1455_trials, p1455_inits, p1455_ends, p1455_frees;
static size_t p1455_changes, p1455_reads, p1455_payloads, p1455_bools;
static size_t p1455_conditions, p1455_results, p1455_copies, p1455_bindings;
static size_t p1455_frame, p1455_symbol;
static uint8_t *p1455_level, *p1455_spare;
static unsigned p1455_decision, p1455_condition, p1455_last_tag;
static unsigned p1455_values[4];

#define REQUIRE(x)                                                             \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);            \
            abort();                                                           \
        }                                                                      \
    } while (0)
#define NL_NODE_BIND(frame, symbol, kind, address)                             \
    do {                                                                       \
        if (!p1455_T_bound && p1455_results && (kind) == 4) {                  \
            REQUIRE(*(const unsigned char *)(address) ==                       \
                    (p1455_results == 1 ? INPUT : NEXT));                      \
            ++p1455_call_values;                                               \
        }                                                                      \
        if (p1455_trials && p1455_frees == p1455_inits && (frame) == 0 &&      \
            (kind) == 4) {                                                     \
            REQUIRE(p1455_completed++ < 2);                                    \
            REQUIRE(*(const unsigned char *)(address) ==                       \
                    (P1455_FAIL_AT == 1   ? 5u                                 \
                     : P1455_FAIL_AT == 2 ? 17u                                \
                                          : OLD));                             \
        }                                                                      \
        if (p1455_copies && (frame) == p1455_frame &&                          \
            (symbol) > p1455_symbol && (kind) == 4) {                          \
            static const unsigned values[] = {OLD, OLD, NEXT, SPARE};          \
            REQUIRE(p1455_bindings < 4);                                       \
            p1455_values[p1455_bindings] = *(const unsigned char *)(address);  \
            REQUIRE(p1455_values[p1455_bindings] == values[p1455_bindings]);   \
            ++p1455_bindings;                                                  \
        }                                                                      \
    } while (0)
#define NL_NODE_REPLACE(...) ((void)0)
#define NL_NODE_RELOAN(...) ((void)0)
#define NL_NODE_ARM(...) ((void)0)
#define NL_NODE_FINISH() ((void)0)

#define NL_MIXED_COPY_BIND(frame, symbol, first, second)                       \
    do {                                                                       \
        REQUIRE(!p1455_T_bound && *(first) == OLD && *(second) == SPARE);      \
        p1455_T_bound = 1;                                                     \
        p1455_level = (first);                                                 \
        p1455_spare = (second);                                                \
        p1455_frame = (frame);                                                 \
        p1455_symbol = (symbol);                                               \
    } while (0)
#define NL_MIXED_COPY_CALL(target, old, next)                                  \
    do {                                                                       \
        REQUIRE(p1455_copies++ == 0);                                          \
        REQUIRE((target) == p1455_level && (old) == OLD && (next) == NEXT);    \
        REQUIRE(*(target) == NEXT && *p1455_spare == SPARE);                   \
    } while (0)
#define NL_MIXED_COPY_STORE(...) ((void)0)

#define NL_HEAP_TRIAL(...) (++p1455_trials)
#define NL_HEAP_ARM(...) ((void)0)
#define NL_HEAP_DOMAIN(...) ((void)0)
#define NL_HEAP_SLOT(...) ((void)0)
#define NL_HEAP_INITIALIZE(pointer, token)                                     \
    do {                                                                       \
        REQUIRE(p1455_inits < 2 && (token));                                   \
        REQUIRE((pointer)->f0.tag == 0 && !(pointer)->f0.ptr);                 \
        REQUIRE((pointer)->f1 == (p1455_inits ? BACK : FRONT));                \
        p1455_h[p1455_inits] = (pointer);                                      \
        p1455_domain[p1455_inits++] = (token);                                 \
    } while (0)
#define NL_HEAP_RELOAN(...) ((void)0)
#define NL_HEAP_ROOT(pointer, token, write, place, incarnation, scope)         \
    do {                                                                       \
        (void)(write);                                                         \
        REQUIRE((place) && (incarnation) && (scope));                          \
        size_t which = (pointer) == p1455_h[0] ? 0 : 1;                        \
        REQUIRE((pointer) && (pointer) == p1455_h[which]);                     \
        REQUIRE((token) == p1455_domain[which]);                               \
    } while (0)
#define NL_HEAP_FIELD(pointer, field, write, nominal, index, child, inc,       \
                      scope)                                                   \
    do {                                                                       \
        (void)(nominal);                                                       \
        (void)(child);                                                         \
        (void)(inc);                                                           \
        (void)(scope);                                                         \
        if ((index) == 1) {                                                    \
            REQUIRE(!(write) &&                                                \
                    (pointer) == p1455_h[P1455_CASE_BACK ? 1 : 0]);            \
            REQUIRE((const void *)(field) == (const void *)&(pointer)->f1);    \
            REQUIRE((pointer)->f1 == INPUT && p1455_payloads++ == 0);          \
        }                                                                      \
    } while (0)
#define NL_HEAP_COPY(source, destination)                                      \
    do {                                                                       \
        unsigned tag = p1455_reads++ ? 0 : 1;                                  \
        REQUIRE((source) == &p1455_h[p1455_reads == 2 ? 1 : 0]->f0);           \
        REQUIRE((source)->tag == tag && (destination)->tag == tag);            \
        REQUIRE((source)->ptr == (tag ? p1455_h[1] : NULL));                   \
        REQUIRE((destination)->ptr == (source)->ptr);                          \
        p1455_last_tag = tag;                                                  \
    } while (0)
#define NL_HEAP_CHANGE(field, old, ordinal)                                    \
    do {                                                                       \
        REQUIRE((ordinal) == ++p1455_changes);                                 \
        REQUIRE((field) == &p1455_h[0]->f0);                                   \
        REQUIRE((old)->tag == ((ordinal) == 1 ? 0u : 1u));                     \
        REQUIRE((field)->tag == ((ordinal) == 1 ? 1u : 0u));                   \
        REQUIRE((field)->ptr == ((ordinal) == 1 ? p1455_h[1] : NULL));         \
    } while (0)
#define NL_HEAP_SCOPE_END(...) ((void)0)
#define NL_HEAP_END(...) (++p1455_ends)
#define NL_HEAP_RAW(...) ((void)0)
#define NL_HEAP_FINALIZE(...) ((void)0)
#define NL_HEAP_RELEASE(handle, bytes, length)                                 \
    do {                                                                       \
        REQUIRE((handle) == (void *)(bytes) && (length) == 24);                \
        size_t which = (handle) == (const void *)p1455_h[0] ? 0 : 1;           \
        REQUIRE((handle) && (handle) == (const void *)p1455_h[which]);         \
        REQUIRE(p1455_h[which]->f0.tag == 0);                                  \
        p1455_h[which] = NULL;                                                 \
        p1455_domain[which] = 0;                                               \
        ++p1455_frees;                                                         \
    } while (0)
#define NL_HEAP_HANDOFF(...) ((void)0)
#define NL_HEAP_RECEIVER_ENTER(...) ((void)0)
#define NL_HEAP_RECEIVER_EXIT() ((void)0)
#define NL_HEAP_RETURNED(...) ((void)0)
#define NL_LIVE_SEND(...) ((void)0)
#define NL_LIVE_ENTER(...) ((void)0)
#define NL_LIVE_RETURN(...) ((void)0)
#define NL_LIVE_RECEIVE(...) ((void)0)

#define NL_SCALAR_BOOL_RESULT(value, allocation)                               \
    do {                                                                       \
        if (!(allocation)) {                                                   \
            REQUIRE(p1455_reads &&                                             \
                    (value) ==                                                 \
                        (P1455_CASE_BACK ? !p1455_last_tag : p1455_last_tag)); \
        }                                                                      \
        p1455_decision = (value);                                              \
        ++p1455_bools;                                                         \
    } while (0)
#define NL_SCALAR_IF_CONDITION(value)                                          \
    do {                                                                       \
        REQUIRE(p1455_conditions < 2 && (value) == p1455_decision);            \
        REQUIRE(p1455_results == p1455_conditions++);                          \
        REQUIRE(p1455_h[0] && p1455_h[1] && !p1455_frees);                     \
        p1455_condition = (value);                                             \
    } while (0)
#define NL_SCALAR_IF_RESULT(arm, value)                                        \
    do {                                                                       \
        REQUIRE(p1455_results < 2);                                            \
        REQUIRE((arm) == (p1455_condition ? 0u : 1u));                         \
        REQUIRE((value) == (p1455_results++ == 0 ? INPUT : NEXT));             \
    } while (0)
#define NL_HEAP_FINISH()                                                       \
    do {                                                                       \
        size_t roots = P1455_FAIL_AT == 1 ? 0 : P1455_FAIL_AT == 2 ? 1 : 2;    \
        unsigned decision = P1455_FAIL_AT ? 0 : !P1455_CASE_BACK;              \
        REQUIRE(p1455_trials == (P1455_FAIL_AT == 1 ? 1u : 2u));               \
        REQUIRE(p1455_inits == roots && p1455_ends == roots &&                 \
                p1455_frees == roots);                                         \
        REQUIRE(p1455_bools == (P1455_FAIL_AT ? 0u : 1u));                     \
        REQUIRE(p1455_decision == decision &&                                  \
                p1455_conditions == (P1455_FAIL_AT ? 0u : 2u) &&               \
                p1455_results == (P1455_FAIL_AT ? 0u : 2u));                   \
        REQUIRE(p1455_copies == (P1455_FAIL_AT ? 0u : 1u));                    \
        REQUIRE(p1455_bindings == (P1455_FAIL_AT ? 0u : 4u));                  \
        REQUIRE(p1455_payloads == (P1455_FAIL_AT ? 0u : 1u));                  \
        REQUIRE(p1455_completed == 2);                                         \
        REQUIRE(p1455_call_values == (P1455_FAIL_AT     ? 0u                   \
                                      : P1455_CASE_BACK ? 4u                   \
                                                        : 3u));                \
        REQUIRE(p1455_changes == (P1455_FAIL_AT ? 0u : 2u));                   \
        REQUIRE(p1455_reads == (P1455_FAIL_AT ? 0u : 1u));                     \
        printf("PASS H%zu cleanup=%zu Bool=%u arm=%u T-reached=%u "            \
               "observed-old/current/sibling=%u/%u/%u\n",                      \
               roots, p1455_frees, decision, decision ? 0u : 1u, roots == 2,   \
               p1455_values[0], p1455_values[2], p1455_values[3]);             \
    } while (0)
#endif
