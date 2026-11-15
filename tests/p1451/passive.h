#ifndef P1451_PASSIVE_H
#define P1451_PASSIVE_H
#include <stdio.h>
#if P1451_CASE_BACK
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
#define NEXT (P1451_CASE_BACK ? BACK : FRONT)
static size_t p1451_call_values, p1451_completed;

static const nl_node *p1451_h[2];
static size_t p1451_domain[2];
static size_t p1451_trials, p1451_inits, p1451_ends, p1451_frees;
static size_t p1451_changes, p1451_reads, p1451_payloads, p1451_bools;
static size_t p1451_conditions, p1451_results, p1451_copies, p1451_bindings;
static size_t p1451_frame, p1451_symbol;
static uint8_t *p1451_level, *p1451_spare;
static unsigned p1451_decision, p1451_condition, p1451_last_tag;
static unsigned p1451_values[4];

#define REQUIRE(x)                                                             \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);            \
            abort();                                                           \
        }                                                                      \
    } while (0)
#define NL_NODE_BIND(frame, symbol, kind, address)                             \
    do {                                                                       \
        if (!p1451_level && p1451_results && (kind) == 4) {                    \
            REQUIRE(*(const unsigned char *)(address) == NEXT);                \
            ++p1451_call_values;                                               \
        }                                                                      \
        if (p1451_trials && p1451_frees == p1451_inits && (kind) == 4) {       \
            REQUIRE(p1451_completed++ == 0);                                   \
            REQUIRE(*(const unsigned char *)(address) ==                       \
                    (P1451_FAIL_AT == 1   ? 5u                                 \
                     : P1451_FAIL_AT == 2 ? 17u                                \
                                          : OLD));                             \
        }                                                                      \
        if (p1451_copies && (frame) == p1451_frame &&                          \
            (symbol) > p1451_symbol && (kind) == 4) {                          \
            static const unsigned values[] = {OLD, OLD, NEXT, SPARE};          \
            REQUIRE(p1451_bindings < 4);                                       \
            p1451_values[p1451_bindings] = *(const unsigned char *)(address);  \
            REQUIRE(p1451_values[p1451_bindings] == values[p1451_bindings]);   \
            ++p1451_bindings;                                                  \
        }                                                                      \
    } while (0)
#define NL_NODE_REPLACE(...) ((void)0)
#define NL_NODE_RELOAN(...) ((void)0)
#define NL_NODE_ARM(...) ((void)0)
#define NL_NODE_FINISH() ((void)0)

#define NL_MIXED_COPY_BIND(frame, symbol, first, second)                       \
    do {                                                                       \
        REQUIRE(!p1451_level && *(first) == OLD && *(second) == SPARE);        \
        p1451_level = (first);                                                 \
        p1451_spare = (second);                                                \
        p1451_frame = (frame);                                                 \
        p1451_symbol = (symbol);                                               \
    } while (0)
#define NL_MIXED_COPY_CALL(target, old, next)                                  \
    do {                                                                       \
        REQUIRE(p1451_copies++ == 0);                                          \
        REQUIRE((target) == p1451_level && (old) == OLD && (next) == NEXT);    \
        REQUIRE(*(target) == NEXT && *p1451_spare == SPARE);                   \
    } while (0)
#define NL_MIXED_COPY_STORE(...) ((void)0)

#define NL_HEAP_TRIAL(...) (++p1451_trials)
#define NL_HEAP_ARM(...) ((void)0)
#define NL_HEAP_DOMAIN(...) ((void)0)
#define NL_HEAP_SLOT(...) ((void)0)
#define NL_HEAP_INITIALIZE(pointer, token)                                     \
    do {                                                                       \
        REQUIRE(p1451_inits < 2 && (token));                                   \
        REQUIRE((pointer)->f0.tag == 0 && !(pointer)->f0.ptr);                 \
        REQUIRE((pointer)->f1 == (p1451_inits ? BACK : FRONT));                \
        p1451_h[p1451_inits] = (pointer);                                      \
        p1451_domain[p1451_inits++] = (token);                                 \
    } while (0)
#define NL_HEAP_RELOAN(...) ((void)0)
#define NL_HEAP_ROOT(pointer, token, write, place, incarnation, scope)         \
    do {                                                                       \
        (void)(write);                                                         \
        REQUIRE((place) && (incarnation) && (scope));                          \
        size_t which = (pointer) == p1451_h[0] ? 0 : 1;                        \
        REQUIRE((pointer) && (pointer) == p1451_h[which]);                     \
        REQUIRE((token) == p1451_domain[which]);                               \
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
                    (pointer) == p1451_h[P1451_CASE_BACK ? 1 : 0]);            \
            REQUIRE((const void *)(field) == (const void *)&(pointer)->f1);    \
            REQUIRE((pointer)->f1 == NEXT && p1451_payloads++ == 0);           \
        }                                                                      \
    } while (0)
#define NL_HEAP_COPY(source, destination)                                      \
    do {                                                                       \
        unsigned tag = p1451_reads++ ? 0 : 1;                                  \
        REQUIRE((source) == &p1451_h[p1451_reads == 2 ? 1 : 0]->f0);           \
        REQUIRE((source)->tag == tag && (destination)->tag == tag);            \
        REQUIRE((source)->ptr == (tag ? p1451_h[1] : NULL));                   \
        REQUIRE((destination)->ptr == (source)->ptr);                          \
        p1451_last_tag = tag;                                                  \
    } while (0)
#define NL_HEAP_CHANGE(field, old, ordinal)                                    \
    do {                                                                       \
        REQUIRE((ordinal) == ++p1451_changes);                                 \
        REQUIRE((field) == &p1451_h[0]->f0);                                   \
        REQUIRE((old)->tag == ((ordinal) == 1 ? 0u : 1u));                     \
        REQUIRE((field)->tag == ((ordinal) == 1 ? 1u : 0u));                   \
        REQUIRE((field)->ptr == ((ordinal) == 1 ? p1451_h[1] : NULL));         \
    } while (0)
#define NL_HEAP_SCOPE_END(...) ((void)0)
#define NL_HEAP_END(...) (++p1451_ends)
#define NL_HEAP_RAW(...) ((void)0)
#define NL_HEAP_FINALIZE(...) ((void)0)
#define NL_HEAP_RELEASE(handle, bytes, length)                                 \
    do {                                                                       \
        REQUIRE((handle) == (void *)(bytes) && (length) == 24);                \
        size_t which = (handle) == (const void *)p1451_h[0] ? 0 : 1;           \
        REQUIRE((handle) && (handle) == (const void *)p1451_h[which]);         \
        REQUIRE(p1451_h[which]->f0.tag == 0);                                  \
        p1451_h[which] = NULL;                                                 \
        p1451_domain[which] = 0;                                               \
        ++p1451_frees;                                                         \
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
            REQUIRE(p1451_reads && (value) == p1451_last_tag);                 \
        }                                                                      \
        p1451_decision = (value);                                              \
        ++p1451_bools;                                                         \
    } while (0)
#define NL_SCALAR_IF_CONDITION(value)                                          \
    do {                                                                       \
        REQUIRE(p1451_conditions++ == 0 && (value) == p1451_decision);         \
        REQUIRE(p1451_h[0] && p1451_h[1] && !p1451_frees);                     \
        p1451_condition = (value);                                             \
    } while (0)
#define NL_SCALAR_IF_RESULT(arm, value)                                        \
    do {                                                                       \
        REQUIRE(p1451_results++ == 0);                                         \
        REQUIRE((arm) == (p1451_condition ? 0u : 1u));                         \
        REQUIRE((value) == (p1451_condition ? FRONT : BACK));                  \
    } while (0)
#define NL_HEAP_FINISH()                                                       \
    do {                                                                       \
        size_t roots = P1451_FAIL_AT == 1 ? 0 : P1451_FAIL_AT == 2 ? 1 : 2;    \
        unsigned decision = P1451_FAIL_AT ? 0 : !P1451_CASE_BACK;              \
        REQUIRE(p1451_trials == (P1451_FAIL_AT == 1 ? 1u : 2u));               \
        REQUIRE(p1451_inits == roots && p1451_ends == roots &&                 \
                p1451_frees == roots);                                         \
        REQUIRE(p1451_bools == (P1451_FAIL_AT ? 0u : 1u));                     \
        REQUIRE(p1451_decision == decision &&                                  \
                p1451_conditions == (P1451_FAIL_AT ? 0u : 1u) &&               \
                p1451_results == (P1451_FAIL_AT ? 0u : 1u));                   \
        REQUIRE(p1451_copies == (P1451_FAIL_AT ? 0u : 1u));                    \
        REQUIRE(p1451_bindings == (P1451_FAIL_AT ? 0u : 4u));                  \
        REQUIRE(p1451_payloads == (P1451_FAIL_AT ? 0u : 1u));                  \
        REQUIRE(p1451_completed == 1);                                         \
        REQUIRE(p1451_call_values == (P1451_FAIL_AT     ? 0u                   \
                                      : P1451_CASE_BACK ? 3u                   \
                                                        : 2u));                \
        REQUIRE(p1451_changes == (P1451_FAIL_AT ? 0u : 2u));                   \
        REQUIRE(p1451_reads == (P1451_FAIL_AT ? 0u : 1u));                     \
        printf("PASS H%zu cleanup=%zu Bool=%u arm=%u T-reached=%u "            \
               "observed-old/current/sibling=%u/%u/%u\n",                      \
               roots, p1451_frees, decision, decision ? 0u : 1u, roots == 2,   \
               p1451_values[0], p1451_values[2], p1451_values[3]);             \
    } while (0)
#endif
