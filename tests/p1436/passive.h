#ifndef P1436_PASSIVE_H
#define P1436_PASSIVE_H
#include <stdio.h>

static const nl_node *p1436_h[2];
static size_t p1436_domain[2];
static size_t p1436_trials, p1436_inits, p1436_ends, p1436_frees;
static size_t p1436_changes, p1436_reads, p1436_payloads, p1436_bools;
static size_t p1436_conditions, p1436_results, p1436_copies, p1436_bindings;
static size_t p1436_frame, p1436_symbol;
static uint8_t *p1436_level, *p1436_spare;
static unsigned p1436_decision, p1436_condition, p1436_last_tag;
static unsigned p1436_values[4];

#define REQUIRE(x)                                                             \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);            \
            abort();                                                           \
        }                                                                      \
    } while (0)
#define NL_NODE_BIND(frame, symbol, kind, address)                             \
    do {                                                                       \
        if (p1436_copies && (frame) == p1436_frame &&                          \
            (symbol) > p1436_symbol && (kind) == 4) {                          \
            static const unsigned values[] = {31, 31, 197, 251};               \
            REQUIRE(p1436_bindings < 4);                                       \
            p1436_values[p1436_bindings] = *(const unsigned char *)(address);  \
            REQUIRE(p1436_values[p1436_bindings] == values[p1436_bindings]);   \
            ++p1436_bindings;                                                  \
        }                                                                      \
    } while (0)
#define NL_NODE_REPLACE(...) ((void)0)
#define NL_NODE_RELOAN(...) ((void)0)
#define NL_NODE_ARM(...) ((void)0)
#define NL_NODE_FINISH() ((void)0)

#define NL_MIXED_COPY_BIND(frame, symbol, first, second)                       \
    do {                                                                       \
        REQUIRE(!p1436_level && *(first) == 31 && *(second) == 251);           \
        p1436_level = (first);                                                 \
        p1436_spare = (second);                                                \
        p1436_frame = (frame);                                                 \
        p1436_symbol = (symbol);                                               \
    } while (0)
#define NL_MIXED_COPY_CALL(target, old, next)                                  \
    do {                                                                       \
        REQUIRE(p1436_copies++ == 0);                                          \
        REQUIRE((target) == p1436_level && (old) == 31 && (next) == 197);      \
        REQUIRE(*(target) == 197 && *p1436_spare == 251);                      \
    } while (0)
#define NL_MIXED_COPY_STORE(...) ((void)0)

#define NL_HEAP_TRIAL(...) (++p1436_trials)
#define NL_HEAP_ARM(...) ((void)0)
#define NL_HEAP_DOMAIN(...) ((void)0)
#define NL_HEAP_SLOT(...) ((void)0)
#define NL_HEAP_INITIALIZE(pointer, token)                                     \
    do {                                                                       \
        REQUIRE(p1436_inits < 2 && (token));                                   \
        REQUIRE((pointer)->f0.tag == 0 && !(pointer)->f0.ptr);                 \
        REQUIRE((pointer)->f1 == (p1436_inits ? 197 : 41));                    \
        p1436_h[p1436_inits] = (pointer);                                      \
        p1436_domain[p1436_inits++] = (token);                                 \
    } while (0)
#define NL_HEAP_RELOAN(...) ((void)0)
#define NL_HEAP_ROOT(pointer, token, write, place, incarnation, scope)         \
    do {                                                                       \
        (void)(write);                                                         \
        REQUIRE((place) && (incarnation) && (scope));                          \
        size_t which = (pointer) == p1436_h[0] ? 0 : 1;                        \
        REQUIRE((pointer) && (pointer) == p1436_h[which]);                     \
        REQUIRE((token) == p1436_domain[which]);                               \
    } while (0)
#define NL_HEAP_FIELD(pointer, field, write, nominal, index, child, inc,       \
                      scope)                                                   \
    do {                                                                       \
        (void)(nominal);                                                       \
        (void)(child);                                                         \
        (void)(inc);                                                           \
        (void)(scope);                                                         \
        if ((index) == 1) {                                                    \
            REQUIRE(!(write) && (pointer) == p1436_h[1]);                      \
            REQUIRE((const void *)(field) == (const void *)&(pointer)->f1);    \
            REQUIRE((pointer)->f1 == 197 && p1436_payloads++ == 0);            \
        }                                                                      \
    } while (0)
#define NL_HEAP_COPY(source, destination)                                      \
    do {                                                                       \
        unsigned tag = p1436_reads++ ? 0 : 1;                                  \
        REQUIRE((source) == &p1436_h[0]->f0);                                  \
        REQUIRE((source)->tag == tag && (destination)->tag == tag);            \
        REQUIRE((source)->ptr == (tag ? p1436_h[1] : NULL));                   \
        REQUIRE((destination)->ptr == (source)->ptr);                          \
        p1436_last_tag = tag;                                                  \
    } while (0)
#define NL_HEAP_CHANGE(field, old, ordinal)                                    \
    do {                                                                       \
        REQUIRE((ordinal) == ++p1436_changes);                                 \
        REQUIRE((field) == &p1436_h[0]->f0);                                   \
        REQUIRE((old)->tag == ((ordinal) == 1 ? 0u : 1u));                     \
        REQUIRE((field)->tag == ((ordinal) == 1 ? 1u : 0u));                   \
        REQUIRE((field)->ptr == ((ordinal) == 1 ? p1436_h[1] : NULL));         \
    } while (0)
#define NL_HEAP_SCOPE_END(...) ((void)0)
#define NL_HEAP_END(...) (++p1436_ends)
#define NL_HEAP_RAW(...) ((void)0)
#define NL_HEAP_FINALIZE(...) ((void)0)
#define NL_HEAP_RELEASE(handle, bytes, length)                                 \
    do {                                                                       \
        REQUIRE((handle) == (void *)(bytes) && (length) == 24);                \
        size_t which = (handle) == (const void *)p1436_h[0] ? 0 : 1;           \
        REQUIRE((handle) && (handle) == (const void *)p1436_h[which]);         \
        REQUIRE(p1436_h[which]->f0.tag == 0);                                  \
        p1436_h[which] = NULL;                                                 \
        p1436_domain[which] = 0;                                               \
        ++p1436_frees;                                                         \
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
            REQUIRE(p1436_reads && (value) == !p1436_last_tag);                \
        }                                                                      \
        p1436_decision = (value);                                              \
        ++p1436_bools;                                                         \
    } while (0)
#define NL_SCALAR_IF_CONDITION(value)                                          \
    do {                                                                       \
        REQUIRE(p1436_conditions++ == 0 && (value) == p1436_decision);         \
        REQUIRE(!p1436_h[0] && !p1436_h[1]);                                   \
        p1436_condition = (value);                                             \
    } while (0)
#define NL_SCALAR_IF_RESULT(arm, value)                                        \
    do {                                                                       \
        REQUIRE(p1436_results++ == 0);                                         \
        REQUIRE((arm) == (p1436_condition ? 0u : 1u));                         \
        REQUIRE((value) == (p1436_condition ? 83u : 227u));                    \
    } while (0)
#define NL_HEAP_FINISH()                                                       \
    do {                                                                       \
        size_t roots = P1436_FAIL_AT == 1 ? 0 : P1436_FAIL_AT == 2 ? 1 : 2;    \
        unsigned decision =                                                    \
            P1436_FAIL_AT ? P1436_FAIL_AT == 2 : P1436_CASE_NONE;              \
        REQUIRE(p1436_trials == (P1436_FAIL_AT == 1 ? 1u : 2u));               \
        REQUIRE(p1436_inits == roots && p1436_ends == roots &&                 \
                p1436_frees == roots);                                         \
        REQUIRE(p1436_bools == (P1436_FAIL_AT == 1   ? 1u                      \
                                : P1436_FAIL_AT == 2 ? 2u                      \
                                                     : 3u));                   \
        REQUIRE(p1436_decision == decision && p1436_conditions == 1 &&         \
                p1436_results == 1);                                           \
        REQUIRE(p1436_copies == (P1436_FAIL_AT ? 0u : 1u));                    \
        REQUIRE(p1436_bindings == (P1436_FAIL_AT ? 0u : 4u));                  \
        REQUIRE(p1436_payloads == (P1436_FAIL_AT ? 0u : 1u));                  \
        REQUIRE(p1436_changes == (P1436_FAIL_AT ? 0u : 2u));                   \
        REQUIRE(p1436_reads == (P1436_FAIL_AT     ? 0u                         \
                                : P1436_CASE_NONE ? 2u                         \
                                                  : 1u));                      \
        printf("PASS H%zu cleanup=%zu Bool=%u arm=%u T-reached=%u "            \
               "observed-old/current/sibling=%u/%u/%u\n",                      \
               roots, p1436_frees, decision, decision ? 0u : 1u, roots == 2,   \
               p1436_values[0], p1436_values[2], p1436_values[3]);             \
    } while (0)
#endif
