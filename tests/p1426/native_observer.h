#ifndef P1426_NATIVE_OBSERVER_H
#define P1426_NATIVE_OBSERVER_H
#include <stdlib.h>

static const nl_node *p1426_nodes[2];
static size_t p1426_initializes, p1426_trials, p1426_releases;
static size_t p1426_conditions, p1426_ptr_arms, p1426_read_roots;
static size_t p1426_reads, p1426_finishes;

#define P1426_REQUIRE(x)                                                       \
    do {                                                                       \
        if (!(x))                                                              \
            abort();                                                           \
    } while (0)
#define NL_QUALIFIED_CONDITION(value, argument)                                \
    do {                                                                       \
        P1426_REQUIRE(p1426_conditions++ == 0);                                \
        P1426_REQUIRE((argument) == 11);                                       \
        P1426_REQUIRE((value) == P1426_EXPECT_TRUE);                           \
    } while (0)
#define NL_NODE_BIND(...) ((void)0)
#define NL_NODE_REPLACE(...) ((void)0)
#define NL_NODE_RELOAN(...) ((void)0)
#define NL_NODE_ARM(arm, pointer)                                              \
    do {                                                                       \
        P1426_REQUIRE(p1426_conditions == 1);                                  \
        P1426_REQUIRE(p1426_ptr_arms++ == 0);                                  \
        P1426_REQUIRE((arm) == (P1426_EXPECT_TRUE ? 0 : 1));                   \
        P1426_REQUIRE((pointer) == p1426_nodes[P1426_EXPECT_TRUE ? 1 : 0]);    \
    } while (0)
#define NL_NODE_FINISH() ((void)0)

#define NL_HEAP_TRIAL(...) (++p1426_trials)
#define NL_HEAP_ARM(...) ((void)0)
#define NL_HEAP_DOMAIN(...) ((void)0)
#define NL_HEAP_SLOT(...) ((void)0)
#define NL_HEAP_INITIALIZE(pointer, ...)                                       \
    do {                                                                       \
        P1426_REQUIRE(p1426_initializes < 2);                                  \
        p1426_nodes[p1426_initializes++] = (pointer);                          \
    } while (0)
#define NL_HEAP_RELOAN(...) ((void)0)
#define NL_HEAP_ROOT(pointer, token, write, place, incarnation, scope)         \
    do {                                                                       \
        (void)(token);                                                         \
        (void)(place);                                                         \
        (void)(incarnation);                                                   \
        (void)(scope);                                                         \
        if (!(write)) {                                                        \
            P1426_REQUIRE(p1426_read_roots++ == 0);                            \
            P1426_REQUIRE((pointer) ==                                         \
                          p1426_nodes[P1426_EXPECT_TRUE ? 1 : 0]);             \
        }                                                                      \
    } while (0)
#define NL_HEAP_FIELD(...) ((void)0)
#define NL_HEAP_COPY(source, destination)                                      \
    do {                                                                       \
        P1426_REQUIRE(p1426_reads++ == 0);                                     \
        P1426_REQUIRE((source)->tag == P1426_EXPECT_OPTION_TAG);               \
        P1426_REQUIRE((destination)->tag == P1426_EXPECT_OPTION_TAG);          \
        if (P1426_EXPECT_OPTION_TAG) {                                         \
            P1426_REQUIRE((source)->ptr == p1426_nodes[1]);                    \
            P1426_REQUIRE((destination)->ptr == p1426_nodes[1]);               \
        } else {                                                               \
            P1426_REQUIRE(!(source)->ptr && !(destination)->ptr);              \
        }                                                                      \
    } while (0)
#define NL_HEAP_CHANGE(...) ((void)0)
#define NL_HEAP_SCOPE_END(...) ((void)0)
#define NL_HEAP_END(...) ((void)0)
#define NL_HEAP_RAW(...) ((void)0)
#define NL_HEAP_FINALIZE(...) ((void)0)
#define NL_HEAP_RELEASE(...) (++p1426_releases)
#define NL_HEAP_HANDOFF(...) ((void)0)
#define NL_HEAP_RECEIVER_ENTER(...) ((void)0)
#define NL_HEAP_RECEIVER_EXIT() ((void)0)
#define NL_HEAP_RETURNED(...) ((void)0)
#define NL_LIVE_SEND(...) ((void)0)
#define NL_LIVE_ENTER(...) ((void)0)
#define NL_LIVE_RETURN(...) ((void)0)
#define NL_LIVE_RECEIVE(...) ((void)0)
#define NL_HEAP_FINISH()                                                       \
    do {                                                                       \
        P1426_REQUIRE(p1426_finishes++ == 0);                                  \
        P1426_REQUIRE(p1426_trials == (P1426_FAIL_AT == 1 ? 1 : 2));           \
        P1426_REQUIRE(p1426_initializes == (P1426_FAIL_AT == 1   ? 0           \
                                            : P1426_FAIL_AT == 2 ? 1           \
                                                                 : 2));        \
        P1426_REQUIRE(p1426_releases == (P1426_FAIL_AT == 1   ? 0              \
                                         : P1426_FAIL_AT == 2 ? 1              \
                                                              : 2));           \
        P1426_REQUIRE(p1426_conditions == (P1426_FAIL_AT ? 0 : 1));            \
        P1426_REQUIRE(p1426_ptr_arms == (P1426_FAIL_AT ? 0 : 1));              \
        P1426_REQUIRE(p1426_read_roots == (P1426_FAIL_AT ? 0 : 1));            \
        P1426_REQUIRE(p1426_reads == (P1426_FAIL_AT ? 0 : 1));                 \
    } while (0)
#endif
