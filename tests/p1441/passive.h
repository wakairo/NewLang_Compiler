#ifndef P1441_PASSIVE_H
#define P1441_PASSIVE_H
#include <stdio.h>

static const nl_node *p1441_h[2];
static size_t p1441_domain[2];
static size_t p1441_trials, p1441_inits, p1441_ends, p1441_frees;
static size_t p1441_changes, p1441_reads, p1441_payloads, p1441_bools;
static size_t p1441_conditions, p1441_results, p1441_copies, p1441_bindings;
static size_t p1441_frame, p1441_symbol;
static uint8_t *p1441_level, *p1441_spare;
static unsigned p1441_decision, p1441_condition, p1441_last_tag;
static unsigned p1441_values[4];

#define REQUIRE(x)                                                             \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);            \
            abort();                                                           \
        }                                                                      \
    } while (0)
#define NL_NODE_BIND(frame, symbol, kind, address)                             \
    do {                                                                       \
        if (p1441_copies && (frame) == p1441_frame &&                          \
            (symbol) > p1441_symbol && (kind) == 4) {                          \
            static const unsigned values[] = {                                 \
                31, 31, P1441_CASE_NONE ? 197 : 41, 251};                      \
            REQUIRE(p1441_bindings < 4);                                       \
            p1441_values[p1441_bindings] = *(const unsigned char *)(address);  \
            REQUIRE(p1441_values[p1441_bindings] == values[p1441_bindings]);   \
            ++p1441_bindings;                                                  \
        }                                                                      \
    } while (0)
#define NL_NODE_REPLACE(...) ((void)0)
#define NL_NODE_RELOAN(...) ((void)0)
#define NL_NODE_ARM(...) ((void)0)
#define NL_NODE_FINISH() ((void)0)

#define NL_MIXED_COPY_BIND(frame, symbol, first, second)                       \
    do {                                                                       \
        REQUIRE(!p1441_level && *(first) == 31 && *(second) == 251);           \
        p1441_level = (first);                                                 \
        p1441_spare = (second);                                                \
        p1441_frame = (frame);                                                 \
        p1441_symbol = (symbol);                                               \
    } while (0)
#define NL_MIXED_COPY_CALL(target, old, next)                                  \
    do {                                                                       \
        REQUIRE(p1441_copies++ == 0);                                          \
        REQUIRE((target) == p1441_level && (old) == 31 &&                      \
                (next) == (P1441_CASE_NONE ? 197 : 41));                       \
        REQUIRE(*(target) == (P1441_CASE_NONE ? 197 : 41) &&                   \
                *p1441_spare == 251);                                          \
    } while (0)
#define NL_MIXED_COPY_STORE(...) ((void)0)

#define NL_HEAP_TRIAL(...) (++p1441_trials)
#define NL_HEAP_ARM(...) ((void)0)
#define NL_HEAP_DOMAIN(...) ((void)0)
#define NL_HEAP_SLOT(...) ((void)0)
#define NL_HEAP_INITIALIZE(pointer, token)                                     \
    do {                                                                       \
        REQUIRE(p1441_inits < 2 && (token));                                   \
        REQUIRE((pointer)->f0.tag == 0 && !(pointer)->f0.ptr);                 \
        REQUIRE((pointer)->f1 == (p1441_inits ? 197 : 41));                    \
        p1441_h[p1441_inits] = (pointer);                                      \
        p1441_domain[p1441_inits++] = (token);                                 \
    } while (0)
#define NL_HEAP_RELOAN(...) ((void)0)
#define NL_HEAP_ROOT(pointer, token, write, place, incarnation, scope)         \
    do {                                                                       \
        (void)(write);                                                         \
        REQUIRE((place) && (incarnation) && (scope));                          \
        size_t which = (pointer) == p1441_h[0] ? 0 : 1;                        \
        REQUIRE((pointer) && (pointer) == p1441_h[which]);                     \
        REQUIRE((token) == p1441_domain[which]);                               \
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
                    (pointer) == p1441_h[P1441_CASE_NONE ? 1 : 0]);            \
            REQUIRE((const void *)(field) == (const void *)&(pointer)->f1);    \
            REQUIRE((pointer)->f1 == (P1441_CASE_NONE ? 197 : 41) &&           \
                    p1441_payloads++ == 0);                                    \
        }                                                                      \
    } while (0)
#define NL_HEAP_COPY(source, destination)                                      \
    do {                                                                       \
        unsigned tag = p1441_reads++ ? 0 : 1;                                  \
        REQUIRE((source) == &p1441_h[p1441_reads == 2 ? 1 : 0]->f0);           \
        REQUIRE((source)->tag == tag && (destination)->tag == tag);            \
        REQUIRE((source)->ptr == (tag ? p1441_h[1] : NULL));                   \
        REQUIRE((destination)->ptr == (source)->ptr);                          \
        p1441_last_tag = tag;                                                  \
    } while (0)
#define NL_HEAP_CHANGE(field, old, ordinal)                                    \
    do {                                                                       \
        REQUIRE((ordinal) == ++p1441_changes);                                 \
        REQUIRE((field) == &p1441_h[0]->f0);                                   \
        REQUIRE((old)->tag == ((ordinal) == 1 ? 0u : 1u));                     \
        REQUIRE((field)->tag == ((ordinal) == 1 ? 1u : 0u));                   \
        REQUIRE((field)->ptr == ((ordinal) == 1 ? p1441_h[1] : NULL));         \
    } while (0)
#define NL_HEAP_SCOPE_END(...) ((void)0)
#define NL_HEAP_END(...) (++p1441_ends)
#define NL_HEAP_RAW(...) ((void)0)
#define NL_HEAP_FINALIZE(...) ((void)0)
#define NL_HEAP_RELEASE(handle, bytes, length)                                 \
    do {                                                                       \
        REQUIRE((handle) == (void *)(bytes) && (length) == 24);                \
        size_t which = (handle) == (const void *)p1441_h[0] ? 0 : 1;           \
        REQUIRE((handle) && (handle) == (const void *)p1441_h[which]);         \
        REQUIRE(p1441_h[which]->f0.tag == 0);                                  \
        p1441_h[which] = NULL;                                                 \
        p1441_domain[which] = 0;                                               \
        ++p1441_frees;                                                         \
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
            REQUIRE(p1441_reads && (value) == p1441_last_tag);                 \
        }                                                                      \
        p1441_decision = (value);                                              \
        ++p1441_bools;                                                         \
    } while (0)
#define NL_SCALAR_IF_CONDITION(value)                                          \
    do {                                                                       \
        REQUIRE(p1441_conditions++ == 0 && (value) == p1441_decision);         \
        REQUIRE(p1441_h[0] && p1441_h[1] && !p1441_frees);                     \
        p1441_condition = (value);                                             \
    } while (0)
#define NL_SCALAR_IF_RESULT(arm, value)                                        \
    do {                                                                       \
        REQUIRE(p1441_results++ == 0);                                         \
        REQUIRE((arm) == (p1441_condition ? 0u : 1u));                         \
        REQUIRE((value) == (p1441_condition ? 41u : 197u));                    \
    } while (0)
#define NL_HEAP_FINISH()                                                       \
    do {                                                                       \
        size_t roots = P1441_FAIL_AT == 1 ? 0 : P1441_FAIL_AT == 2 ? 1 : 2;    \
        unsigned decision = P1441_FAIL_AT ? 0 : !P1441_CASE_NONE;              \
        REQUIRE(p1441_trials == (P1441_FAIL_AT == 1 ? 1u : 2u));               \
        REQUIRE(p1441_inits == roots && p1441_ends == roots &&                 \
                p1441_frees == roots);                                         \
        REQUIRE(p1441_bools == (P1441_FAIL_AT ? 0u : 1u));                     \
        REQUIRE(p1441_decision == decision &&                                  \
                p1441_conditions == (P1441_FAIL_AT ? 0u : 1u) &&               \
                p1441_results == (P1441_FAIL_AT ? 0u : 1u));                   \
        REQUIRE(p1441_copies == (P1441_FAIL_AT ? 0u : 1u));                    \
        REQUIRE(p1441_bindings == (P1441_FAIL_AT ? 0u : 4u));                  \
        REQUIRE(p1441_payloads == (P1441_FAIL_AT ? 0u : 1u));                  \
        REQUIRE(p1441_changes == (P1441_FAIL_AT ? 0u : 2u));                   \
        REQUIRE(p1441_reads == (P1441_FAIL_AT     ? 0u                         \
                                : P1441_CASE_NONE ? 2u                         \
                                                  : 1u));                      \
        printf("PASS H%zu cleanup=%zu Bool=%u arm=%u T-reached=%u "            \
               "observed-old/current/sibling=%u/%u/%u\n",                      \
               roots, p1441_frees, decision, decision ? 0u : 1u, roots == 2,   \
               p1441_values[0], p1441_values[2], p1441_values[3]);             \
    } while (0)
#endif
