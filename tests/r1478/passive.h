#ifndef R1478_PASSIVE_H
#define R1478_PASSIVE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef R1478_SAMPLE
#define R1478_SAMPLE 0
#endif
#ifndef R1478_FAIL_AT
#define R1478_FAIL_AT 0
#endif

#define R1478_ASSERT(x)                                                        \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "R1478 observer %s:%d: %s\n", __FILE__, __LINE__,  \
                    #x);                                                       \
            abort();                                                           \
        }                                                                      \
    } while (0)

static size_t r1478_trials, r1478_inits, r1478_calls, r1478_payloads;
static size_t r1478_changes, r1478_ends, r1478_finalizes, r1478_frees;
static size_t r1478_roots, r1478_reads, r1478_finishes;
static const nl_node *r1478_originals[2];
static size_t r1478_domains[2];
static unsigned r1478_ended[2], r1478_finalized[2];
static void *r1478_pending_free;
static uint8_t *r1478_metric, *r1478_guard;

static size_t r1478_identity(const nl_node *p)
{
    R1478_ASSERT(p != NULL);
    if (p == r1478_originals[0])
        return 0;
    R1478_ASSERT(p == r1478_originals[1]);
    return 1;
}

void r1478_observe_free(void *p)
{
    R1478_ASSERT(p != NULL && p == r1478_pending_free);
    size_t i = r1478_identity((const nl_node *)p);
    R1478_ASSERT(r1478_ended[i] && r1478_finalized[i]);
    r1478_originals[i] = NULL;
    r1478_domains[i] = 0;
    r1478_pending_free = NULL;
    ++r1478_frees;
}

#define NL_NODE_BIND(...) ((void)0)
#define NL_NODE_REPLACE(...) ((void)0)
#define NL_NODE_RELOAN(...) ((void)0)
#define NL_NODE_ARM(...) ((void)0)
#define NL_NODE_FINISH() ((void)0)

#define NL_MIXED_COPY_BIND(frame, symbol, first, second)                       \
    do {                                                                       \
        (void)(frame);                                                         \
        (void)(symbol);                                                        \
        R1478_ASSERT(!r1478_metric && !r1478_guard);                           \
        r1478_metric = (first);                                                \
        r1478_guard = (second);                                                \
        R1478_ASSERT(r1478_metric != r1478_guard && *r1478_metric == 84 &&     \
                     *r1478_guard == 211);                                     \
    } while (0)

#define NL_MIXED_COPY_CALL(target, old, next)                                  \
    do {                                                                       \
        R1478_ASSERT(r1478_metric && (target) == r1478_metric &&               \
                     r1478_guard && *r1478_guard == 211 && r1478_calls < 2);   \
        if (r1478_calls == 0) {                                                \
            R1478_ASSERT(r1478_trials == 0 && (old) == 84 && (next) == 31);    \
        } else {                                                               \
            R1478_ASSERT(r1478_frees == 2 && !r1478_pending_free &&            \
                         (old) == 31 && (next) == R1478_SAMPLE);               \
        }                                                                      \
        R1478_ASSERT(*(target) == (next));                                     \
        ++r1478_calls;                                                         \
    } while (0)

#define NL_MIXED_COPY_STORE(...) R1478_ASSERT(0)
#define NL_MIXED_OWNER_LOCAL(...) ((void)0)

#define NL_HEAP_TRIAL(pointer, bytes, alignment)                               \
    do {                                                                       \
        ++r1478_trials;                                                        \
        R1478_ASSERT(r1478_calls == 1 && r1478_trials <= 2 &&                  \
                     (bytes) == sizeof(nl_node) && (alignment) == 8);          \
        R1478_ASSERT(((pointer) == NULL) ==                                    \
                     (R1478_FAIL_AT && r1478_trials == R1478_FAIL_AT));        \
    } while (0)

#define NL_HEAP_ARM(...) ((void)0)
#define NL_HEAP_DOMAIN(...) ((void)0)
#define NL_HEAP_SLOT(...) ((void)0)

#define NL_HEAP_INITIALIZE(pointer, token)                                     \
    do {                                                                       \
        R1478_ASSERT(r1478_inits < 2 && (pointer) && (token) &&                \
                     !(pointer)->f0.tag && !(pointer)->f0.ptr);                \
        R1478_ASSERT((pointer)->f1 == (r1478_inits ? R1478_SAMPLE : 107));     \
        if (r1478_inits)                                                       \
            R1478_ASSERT((pointer) != r1478_originals[0] &&                    \
                         (token) != r1478_domains[0]);                         \
        r1478_originals[r1478_inits] = (pointer);                              \
        r1478_domains[r1478_inits++] = (token);                                \
    } while (0)

#define NL_HEAP_RELOAN(...) ((void)0)
#define NL_HEAP_ROOT(pointer, token, write, place, incarnation, scope)         \
    do {                                                                       \
        size_t i = r1478_identity(pointer);                                    \
        (void)(write);                                                         \
        R1478_ASSERT((token) == r1478_domains[i] && !r1478_ended[i] &&         \
                     (place) && (incarnation) && (scope));                     \
        ++r1478_roots;                                                         \
    } while (0)

#define NL_HEAP_FIELD(pointer, field, write, nominal, index, child, inc,       \
                      scope)                                                   \
    do {                                                                       \
        size_t i = r1478_identity(pointer);                                    \
        R1478_ASSERT((nominal) && (child) && (inc) && (scope) &&               \
                     (index) < 2 && !r1478_ended[i]);                          \
        if ((index) == 1) {                                                    \
            R1478_ASSERT(i == 1 && !(write) &&                                 \
                         (const void *)(field) ==                              \
                             (const void *)&(pointer)->f1 &&                   \
                         (pointer)->f1 == R1478_SAMPLE);                       \
            ++r1478_payloads;                                                  \
        } else {                                                               \
            R1478_ASSERT(i == 0 && (const void *)(field) ==                    \
                                       (const void *)&(pointer)->f0);          \
        }                                                                      \
    } while (0)

#define NL_HEAP_COPY(source, destination)                                      \
    do {                                                                       \
        R1478_ASSERT(r1478_originals[0] && r1478_originals[1] &&               \
                     (source) == &r1478_originals[0]->f0 &&                    \
                     (source)->tag == 1 &&                                     \
                     (source)->ptr == r1478_originals[1]);                     \
        (void)(destination);                                                   \
        ++r1478_reads;                                                         \
    } while (0)

#define NL_HEAP_CHANGE(field, old, ordinal)                                    \
    do {                                                                       \
        R1478_ASSERT(r1478_originals[0] && r1478_originals[1] &&               \
                     (field) == &r1478_originals[0]->f0 &&                     \
                     (ordinal) == ++r1478_changes && r1478_changes <= 2);      \
        R1478_ASSERT((old)->tag == (r1478_changes == 1 ? 0u : 1u) &&           \
                     (old)->ptr ==                                             \
                         (r1478_changes == 1 ? NULL : r1478_originals[1]));    \
        R1478_ASSERT((field)->tag == (r1478_changes == 1 ? 1u : 0u) &&         \
                     (field)->ptr ==                                           \
                         (r1478_changes == 1 ? r1478_originals[1] : NULL));    \
    } while (0)

#define NL_HEAP_SCOPE_END(...) ((void)0)
#define NL_HEAP_END(pointer, token)                                            \
    do {                                                                       \
        size_t i = r1478_identity(pointer);                                    \
        R1478_ASSERT(!r1478_ended[i] && (token) == r1478_domains[i] &&         \
                     !(pointer)->f0.tag && !(pointer)->f0.ptr);                \
        ++r1478_ended[i];                                                      \
        ++r1478_ends;                                                          \
    } while (0)

#define NL_HEAP_RAW(bytes, length)                                             \
    R1478_ASSERT((bytes) && (length) == sizeof(nl_node))
#define NL_HEAP_FINALIZE(token)                                                \
    do {                                                                       \
        size_t i = (token) == r1478_domains[0] ? 0 : 1;                        \
        R1478_ASSERT((token) && (token) == r1478_domains[i] &&                 \
                     r1478_ended[i] && !r1478_finalized[i]);                   \
        ++r1478_finalized[i];                                                  \
        ++r1478_finalizes;                                                     \
    } while (0)

#define NL_HEAP_RELEASE(handle, bytes, length)                                 \
    do {                                                                       \
        size_t i = r1478_identity((const nl_node *)(handle));                  \
        R1478_ASSERT(!r1478_pending_free && (handle) == (void *)(bytes) &&     \
                     (length) == sizeof(nl_node) && r1478_ended[i] &&          \
                     r1478_finalized[i]);                                      \
        r1478_pending_free = (handle);                                         \
    } while (0)

#define NL_LIVE_SEND(...) ((void)0)
#define NL_LIVE_ENTER(...) ((void)0)
#define NL_LIVE_RETURN(...) ((void)0)
#define NL_LIVE_RECEIVE(...) ((void)0)
#define NL_HEAP_HANDOFF(...) ((void)0)
#define NL_HEAP_RECEIVER_ENTER(...) ((void)0)
#define NL_HEAP_RECEIVER_EXIT() ((void)0)
#define NL_HEAP_RETURNED(...) ((void)0)
#define NL_SCALAR_BOOL_RESULT(...) ((void)0)
#define NL_SCALAR_IF_CONDITION(...) ((void)0)
#define NL_SCALAR_IF_RESULT(...) ((void)0)

#define NL_HEAP_FINISH()                                                       \
    do {                                                                       \
        unsigned expected = R1478_FAIL_AT == 1   ? 0u                          \
                            : R1478_FAIL_AT == 2 ? 1u                          \
                                                 : 2u;                         \
        R1478_ASSERT(!r1478_finishes++ && r1478_metric && r1478_guard &&       \
                     r1478_metric != r1478_guard && *r1478_guard == 211 &&     \
                     *r1478_metric == (expected == 2 ? R1478_SAMPLE : 31));    \
        R1478_ASSERT(r1478_trials == (expected ? 2u : 1u) &&                   \
                     r1478_inits == expected && r1478_ends == expected &&      \
                     r1478_finalizes == expected && r1478_frees == expected && \
                     !r1478_pending_free && !r1478_originals[0] &&             \
                     !r1478_originals[1]);                                     \
        R1478_ASSERT(r1478_calls == (expected == 2 ? 2u : 1u) &&               \
                     r1478_changes == (expected == 2 ? 2u : 0u) &&             \
                     r1478_payloads == (expected == 2 ? 1u : 0u) &&            \
                     r1478_roots >= (expected == 2 ? 3u : 0u));                \
        printf("R1478-NATIVE-VERIFIED H%u calls=%zu metric=%u guard=%u "       \
               "ends=%zu frees=%zu PASS\n",                                    \
               expected, r1478_calls, (unsigned)*r1478_metric,                 \
               (unsigned)*r1478_guard, r1478_ends, r1478_frees);               \
    } while (0)

#endif
