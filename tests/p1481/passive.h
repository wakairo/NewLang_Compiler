#ifndef P1481_PASSIVE_H
#define P1481_PASSIVE_H
#ifndef P1481_SAMPLE
#define P1481_SAMPLE 181
#endif
#define P1471_SAMPLE P1481_SAMPLE
#include "../p1471/passive.h"

/* Reuse the original H/Domain/physical-free observer, with a prospective
 * normal-continuation oracle. No generated C or source value is replaced. */
#undef NL_NODE_BIND
#define NL_NODE_BIND(frame, symbol, kind, address)                              \
    do {                                                                      \
        if (reading && (kind) == 4 && (frame) == t_frame &&                    \
            (symbol) > t_symbol) {                                            \
            unsigned value = *(const unsigned char *)(address);               \
            if (!trials) {                                                    \
                REQUIRE(calls == 1 && !startup_copies++ && value == 87);       \
            } else {                                                          \
                REQUIRE(bindings < 3);                                        \
                if (!bindings)                                               \
                    REQUIRE(value == 87);                                     \
                else if (bindings == 1) {                                    \
                    REQUIRE(value == expected_delivery());                    \
                    observed_delivery = value;                                \
                } else                                                        \
                    REQUIRE(value == 13);                                     \
                ++bindings;                                                   \
            }                                                                 \
        }                                                                     \
    } while (0)
#undef NL_MIXED_COPY_CALL
#define NL_MIXED_COPY_CALL(target, old, next)                                   \
    do {                                                                      \
        REQUIRE(calls < 2 && (target) == reading && *sibling == 13);            \
        if (!calls)                                                           \
            REQUIRE(!trials && (old) == 87 && !(next));                         \
        else                                                                  \
            REQUIRE((old) == 0 && (next) == selected &&                       \
                    selected == expected_delivery() && frees == 2 &&         \
                    !pending_free && !originals[0] && !originals[1]);          \
        REQUIRE(*(target) == (next));                                          \
        ++calls;                                                              \
    } while (0)
#undef NL_HEAP_FINISH
#define NL_HEAP_FINISH()                                                       \
    do {                                                                      \
        unsigned n = expected_cleanup();                                      \
        REQUIRE(reading && sibling && reading != sibling &&                   \
                calls == (n == 2 ? 2u : 1u) && startup_copies == 1 &&          \
                bindings == 3 && *reading == expected_delivery() &&           \
                observed_delivery == *reading && *sibling == 13);             \
        REQUIRE(!delivery_copies);                                            \
        REQUIRE(trials == (n ? 2u : 1u) && inits == n && ends == n &&          \
                finals == n && frees == n && !pending_free && !originals[0] && \
                !originals[1]);                                               \
        REQUIRE(changes == (n == 2 ? 2u : 0u) &&                              \
                payloads == (n == 2 ? 1u : 0u) &&                             \
                reads == (n == 2 ? 1u : 0u) && roots == (n == 2 ? 4u : 0u) && \
                scope_ends == (n == 2 ? 4u : 0u));                            \
        REQUIRE(bools == (n == 2 ? 1u : 0u) && conditions == bools &&          \
                results == bools);                                            \
        REQUIRE(sends == (n == 2 ? 1u : 0u) && receives == sends &&            \
                entries == sends && returns_ == sends &&                      \
                receiver_entries == sends && receiver_exits == sends);        \
        REQUIRE(n != 2 || selected == expected_delivery());                   \
        printf("P1481 H%u common normal continuation: original T "            \
               "old87, H2-only second calls%zu old0/current%u sibling13 "      \
               "cleanup%zu PASS\n", n, calls - 1, *reading, frees);           \
    } while (0)
#endif
