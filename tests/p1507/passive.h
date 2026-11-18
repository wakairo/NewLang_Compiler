#ifndef P1507_PASSIVE_H
#define P1507_PASSIVE_H
#include <stdbool.h>
#include <stdio.h>
#ifndef P1507_INDEX
#define P1507_INDEX 0
#endif
#ifndef P1507_FAIL_AT
#define P1507_FAIL_AT                                                          \
    (getenv("P1507_FAIL_AT")                                                   \
         ? (unsigned)strtoul(getenv("P1507_FAIL_AT"), NULL, 10)                \
         : 0u)
#endif
#define REQUIRE(x)                                                             \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "P1507 %d: %s\n", __LINE__, #x);                   \
            abort();                                                           \
        }                                                                      \
    } while (0)
static const nl_node *volatile live[2];
static size_t domains[2], trials, inits, ends, finals, roots, payloads;
static volatile size_t frees;
static size_t calls, old_copies, final_copies, t_frame, t_symbol;
static bool ended[2], finalized[2];
static const void *volatile pending_free;
static const uint8_t *target, *sibling;
static unsigned expected_roots(void)
{
    return P1507_FAIL_AT ? P1507_FAIL_AT - 1u : 2u;
}
static unsigned initial(void)
{
    return P1507_INDEX ? 91u : 37u;
}
static unsigned other(void)
{
    return P1507_INDEX ? 37u : 91u;
}
static unsigned payload(void)
{
    return P1507_INDEX ? 173u : 0u;
}
static unsigned current(void)
{
    return expected_roots() == 2 ? payload() : 7u;
}
static size_t identity(const nl_node *p)
{
    REQUIRE(p && (p == live[0] || p == live[1]));
    return p == live[0] ? 0u : 1u;
}
void p1507_observe_free(void *p)
{
    size_t i = identity(p);
    REQUIRE(p == pending_free && ended[i] && finalized[i]);
    REQUIRE(i == (expected_roots() == 2 ? 1u - frees : 0u));
    live[i] = NULL;
    pending_free = NULL;
    ++frees;
}
#define NL_NODE_BIND(frame, symbol, kind, address)                             \
    do {                                                                       \
        if (target && (frame) == t_frame && (symbol) > t_symbol &&             \
            (kind) == 4) {                                                     \
            unsigned x = *(const uint8_t *)(address);                          \
            if (!trials) {                                                     \
                REQUIRE(calls == 1 && old_copies++ == 0 && x == initial());    \
            } else {                                                           \
                REQUIRE(final_copies < 3);                                     \
                REQUIRE(x == (final_copies == 0   ? initial()                  \
                              : final_copies == 1 ? current()                  \
                                                  : other()));                 \
                ++final_copies;                                                \
            }                                                                  \
        }                                                                      \
    } while (0)
#define NL_NODE_REPLACE(...) REQUIRE(0)
#define NL_NODE_RELOAN(...) REQUIRE(0)
#define NL_NODE_ARM(...) REQUIRE(0)
#define NL_NODE_FINISH() REQUIRE(0)
#define NL_MIXED_COPY_BIND(frame, symbol, first, second)                       \
    do {                                                                       \
        REQUIRE(!target);                                                      \
        target = P1507_INDEX ? (second) : (first);                             \
        sibling = P1507_INDEX ? (first) : (second);                            \
        REQUIRE(target != sibling && *target == initial() &&                   \
                *sibling == other());                                          \
        t_frame = (frame);                                                     \
        t_symbol = (symbol);                                                   \
    } while (0)
#define NL_MIXED_COPY_CALL(address, old, next)                                 \
    do {                                                                       \
        REQUIRE(calls < 2 && (address) == target && *sibling == other());      \
        if (!calls)                                                            \
            REQUIRE(!trials && (old) == initial() && (next) == 7);             \
        else                                                                   \
            REQUIRE(expected_roots() == 2 && frees == 2 && !pending_free &&    \
                    (old) == 7 && (next) == payload() && payloads == 1);       \
        REQUIRE(*(address) == (next));                                         \
        ++calls;                                                               \
    } while (0)
#define NL_MIXED_COPY_STORE(...) REQUIRE(0)
#define NL_MIXED_OWNER_LOCAL(...) REQUIRE(0)
#define NL_HEAP_TRIAL(pointer, bytes, alignment)                               \
    do {                                                                       \
        ++trials;                                                              \
        REQUIRE(calls == 1 && trials <= 2 && (bytes) == 24 &&                  \
                (alignment) == 8);                                             \
        REQUIRE(((pointer) == NULL) ==                                         \
                (P1507_FAIL_AT && trials == P1507_FAIL_AT));                   \
    } while (0)
#define NL_HEAP_ARM(...) ((void)0)
#define NL_HEAP_DOMAIN(...) ((void)0)
#define NL_HEAP_SLOT(...) ((void)0)
#define NL_HEAP_INITIALIZE(pointer, token)                                     \
    do {                                                                       \
        REQUIRE(inits < 2 && (pointer) && (token) && !(pointer)->f0.tag &&     \
                !(pointer)->f0.ptr);                                           \
        REQUIRE((pointer)->f1 == (inits ? payload() : 51u));                   \
        if (inits)                                                             \
            REQUIRE((pointer) != live[0] && (token) != domains[0]);            \
        live[inits] = (pointer);                                               \
        domains[inits++] = (token);                                            \
    } while (0)
#define NL_HEAP_RELOAN(...) ((void)0)
#define NL_HEAP_ROOT(pointer, token, write, place, inc, scope)                 \
    do {                                                                       \
        size_t i = identity(pointer);                                          \
        REQUIRE(i == 1 && !ended[i] && !(write) && (token) == domains[i] &&    \
                (place) && (inc) && (scope));                                  \
        ++roots;                                                               \
    } while (0)
#define NL_HEAP_FIELD(pointer, field, write, nominal, index, child, inc,       \
                      scope)                                                   \
    do {                                                                       \
        size_t i = identity(pointer);                                          \
        REQUIRE(i == 1 && !ended[i] && !(write) && (nominal) &&                \
                (index) == 1 && (child) && (inc) && (scope) &&                 \
                (const void *)(field) == (const void *)&(pointer)->f1 &&       \
                (pointer)->f1 == payload() && payloads++ == 0);                \
    } while (0)
#define NL_HEAP_COPY(...) REQUIRE(0)
#define NL_HEAP_CHANGE(...) REQUIRE(0)
#define NL_HEAP_SCOPE_END(...) ((void)0)
#define NL_HEAP_END(pointer, token)                                            \
    do {                                                                       \
        size_t i = identity(pointer);                                          \
        REQUIRE(!ended[i] && (token) == domains[i] && !(pointer)->f0.tag &&    \
                !(pointer)->f0.ptr);                                           \
        ended[i] = true;                                                       \
        ++ends;                                                                \
    } while (0)
#define NL_HEAP_RAW(bytes, length) REQUIRE((bytes) && (length) == 24)
#define NL_HEAP_FINALIZE(token)                                                \
    do {                                                                       \
        size_t i = (token) == domains[0] ? 0u : 1u;                            \
        REQUIRE((token) && (token) == domains[i] && ended[i] &&                \
                !finalized[i]);                                                \
        finalized[i] = true;                                                   \
        ++finals;                                                              \
    } while (0)
#define NL_HEAP_RELEASE(handle, bytes, length)                                 \
    do {                                                                       \
        size_t i = identity(handle);                                           \
        REQUIRE(!pending_free && (void *)(handle) == (void *)(bytes) &&        \
                (length) == 24 && ended[i] && finalized[i]);                   \
        pending_free = (handle);                                               \
    } while (0)
#define NL_HEAP_HANDOFF(...) REQUIRE(0)
#define NL_HEAP_RECEIVER_ENTER(...) REQUIRE(0)
#define NL_HEAP_RECEIVER_EXIT() REQUIRE(0)
#define NL_HEAP_RETURNED(...) REQUIRE(0)
#define NL_LIVE_SEND(...) REQUIRE(0)
#define NL_LIVE_ENTER(...) REQUIRE(0)
#define NL_LIVE_RETURN(...) REQUIRE(0)
#define NL_LIVE_RECEIVE(...) REQUIRE(0)
#define NL_HEAP_FINISH()                                                       \
    do {                                                                       \
        unsigned n = expected_roots();                                         \
        REQUIRE(trials == (n ? 2u : 1u) && inits == n && ends == n &&          \
                finals == n && frees == n);                                    \
        REQUIRE(!live[0] && !live[1] && !pending_free &&                       \
                calls == (n == 2 ? 2u : 1u));                                  \
        REQUIRE(old_copies == 1 && final_copies == 3 &&                        \
                *target == current() && *sibling == other());                  \
        REQUIRE(roots == (n == 2 ? 1u : 0u) && payloads == roots);             \
        printf("P1507 H%u physical free order, same T old/current=%u "         \
               "sibling=%u second-writes=%zu PASS\n",                          \
               n, *target, *sibling, calls - 1);                               \
    } while (0)
#endif
