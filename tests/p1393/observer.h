/* P1393 phase-aware read-only application observer, derived from the kept
 * six-case: no guest writes, allocation, repair or free. Expected checked IDs
 * come from a separate public artifact probe. Library topology is an
 * independent policy oracle, never an emitter input. Checks stay active under
 * NDEBUG. Invalid free requests stop BEFORE UB. */
#include <stdbool.h>
#include <stdio.h>
size_t five_requests(void);
size_t five_releases(void);
uintptr_t five_address(size_t);
#include "five_expect.h"
#define REQUIRE(x)                                                             \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FIVE OBSERVER FAIL line %d: %s\n", __LINE__, #x); \
            exit(77);                                                          \
        }                                                                      \
    } while (0)
static uintptr_t addresses[5];
static const nl_node *nodes[5];
static unsigned phases[5];
static size_t trials, arms, selected, live, freed, changes, roots, fields;
static size_t active_domain, active_scope, reads, matched;
static const nl_node *actual_tail;
void p1393_observe_live_copy(const nl_option *, const nl_option *);
static unsigned exclusive;
static uintptr_t free_order[5];
static size_t index_of(uintptr_t p)
{
    REQUIRE(p != 0);
    for (size_t i = 0; i < 5; ++i)
        if (addresses[i] == p)
            return i;
    REQUIRE(0);
    return 0;
}
static void trial(size_t site, void *p, size_t n, size_t a)
{
    REQUIRE(site == trials && trials < 5 && five_requests() == trials + 1 &&
            n == 56 && a == 8);
    REQUIRE(active_scope == 0);
    for (size_t i = 0; i < trials; ++i)
        REQUIRE(phases[i] == 4 && addresses[i] != (uintptr_t)p);
    selected = site;
    addresses[site] = (uintptr_t)p;
    REQUIRE(addresses[site] == five_address(site));
    phases[site] = p ? 1u : 99u;
    ++trials;
}
static void arm(size_t site, size_t variant)
{
    REQUIRE(site == selected && ++arms == trials &&
            variant == (addresses[site] ? 2u : 1u));
}
static void slot(const void *p, size_t n)
{
    selected = index_of((uintptr_t)p);
    REQUIRE(phases[selected] == 1 && n == 56);
    phases[selected] = 2;
}
static void domain(size_t d)
{
    REQUIRE(phases[selected] == 2 && d == expected_init[selected][3]);
    phases[selected] = 3;
}
static void initialize(const nl_node *p, size_t d, size_t r, size_t inc)
{
    size_t i = index_of((uintptr_t)p);
    REQUIRE(phases[i] == 3 && i == live && d == expected_init[i][3] &&
            r == expected_init[i][1] && inc == expected_init[i][2]);
    REQUIRE(active_domain == d && !exclusive && active_scope != 0);
    REQUIRE(p->f0.tag == 0 && p->f0.ptr == NULL && p->f1.tag == 0 &&
            p->f1.ptr == NULL && p->f2.tag == 0 && p->f2.ptr == NULL);
    REQUIRE(p->f3 == expected_init[i][4]);
    nodes[i] = p;
    phases[i] = 4;
    ++live;
}
static void loan(size_t d, size_t scope, unsigned ending)
{
    REQUIRE(active_scope == 0 && d != 0 && scope != 0 && ending <= 1);
    size_t i = 0;
    while (i < 5 && expected_init[i][3] != d)
        ++i;
    REQUIRE(i < 5 && phases[i] == (ending ? 4u : (live == i ? 3u : 4u)));
    active_domain = d;
    active_scope = scope;
    exclusive = ending;
}
static void scope_end(size_t scope)
{
    REQUIRE(scope == active_scope && scope != 0);
    active_scope = active_domain = 0;
    exclusive = 0;
}
static void root_ref(const nl_node *p, size_t d, unsigned write, size_t r,
                     size_t inc, size_t scope)
{
    REQUIRE(roots < 11 && live == 5);
    size_t i = index_of((uintptr_t)p);
    if (roots < 9)
        REQUIRE(freed == 0 && write == 1);
    else
        REQUIRE(freed == 1 && phases[3] == 9 && nodes[3] == NULL &&
                i == (roots == 9 ? 1u : 2u) && write == 0);
    REQUIRE(phases[i] == 4 && d == expected_init[i][3] &&
            r == expected_init[i][1] && inc == expected_init[i][2]);
    REQUIRE(active_domain == d && active_scope == scope && !exclusive);
    REQUIRE(r == expected_root[roots][2] && inc == expected_root[roots][3] &&
            d == expected_root[roots][4] && scope == expected_root[roots][5] &&
            write == expected_root[roots][6]);
    ++roots;
}
static void field_ref(const nl_node *p, const nl_option *f, unsigned write,
                      size_t index, size_t nominal, size_t child, size_t inc,
                      size_t scope)
{
    REQUIRE(fields < 11 && roots == fields + 1 && active_scope == scope);
    size_t i = index_of((uintptr_t)p);
    const nl_option *expected = index == 0   ? &p->f0
                                : index == 1 ? &p->f1
                                             : &p->f2;
    REQUIRE(index < 3 && f == expected && phases[i] == 4 && !exclusive &&
            write == (fields < 9 ? 1u : 0u) &&
            write == expected_field[fields][6]);
    REQUIRE(expected_field[fields][0] == expected_init[i][1] &&
            index == expected_field[fields][1] &&
            nominal == expected_field[fields][2] &&
            child == expected_field[fields][3] &&
            inc == expected_field[fields][4] &&
            scope == expected_field[fields][5]);
    ++fields;
}
static void snapshot(bool unlinked)
{
    /* Both actual checkpoints: old setup, then selected unlink. Inspect all
     * fifteen fields while all five originals are live, never after End. */
    const int targets[5][3] = {{-1, -1, 1},
                               {2, unlinked ? 2 : 3, -1},
                               {unlinked ? -1 : 3, 1, -1},
                               {-1, unlinked ? -1 : 2, -1},
                               {-1, -1, -1}};
    for (size_t i = 0; i < 5; ++i) {
        REQUIRE(phases[i] == 4 && nodes[i]->f3 == expected_init[i][4]);
        const nl_option *values[3] = {&nodes[i]->f0, &nodes[i]->f1,
                                      &nodes[i]->f2};
        for (size_t f = 0; f < 3; ++f) {
            int t = targets[i][f];
            REQUIRE(values[f]->tag == (t < 0 ? 0u : 1u));
            REQUIRE(values[f]->ptr == (t < 0 ? NULL : nodes[t]));
            printf("LINK %zu %zu %u %zx\n", i, f, values[f]->tag,
                   (size_t)(uintptr_t)values[f]->ptr);
        }
    }
}
static void change(const nl_option *f, const nl_option *old, size_t occurrence,
                   size_t post)
{
    REQUIRE(changes < 9 && fields == changes + 1 && roots == changes + 1 &&
            active_scope != 0 && !exclusive && live == 5 && freed == 0);
    static const size_t owners[9] = {0, 1, 1, 2, 2, 3, 2, 1, 3};
    static const int previous[9] = {-1, -1, -1, -1, -1, -1, 3, 3, 2};
    static const int target[9] = {1, 3, 2, 1, 3, 2, -1, 2, -1};
    size_t index = expected_field[changes][1];
    const nl_node *p = nodes[owners[changes]];
    const nl_option *actual = index == 0   ? &p->f0
                              : index == 1 ? &p->f1
                                           : &p->f2;
    int before = previous[changes], after = target[changes];
    REQUIRE(f == actual && old != f);
    REQUIRE(old->tag == (before < 0 ? 0u : 1u) &&
            old->ptr == (before < 0 ? NULL : nodes[before]));
    REQUIRE(f->tag == (after < 0 ? 0u : 1u) &&
            f->ptr == (after < 0 ? NULL : nodes[after]));
    REQUIRE(occurrence == expected_change[changes][0] &&
            post == expected_change[changes][1] && occurrence != post);
    printf("CHANGE %zu %zu %u %zx %u %zx %zu %zu\n", changes, owners[changes],
           old->tag, (size_t)(uintptr_t)old->ptr, f->tag,
           (size_t)(uintptr_t)f->ptr, occurrence, post);
    ++changes;
    if (changes == 6 || changes == 9)
        snapshot(changes == 9);
}
static void end(const nl_node *p, size_t d, size_t r, size_t inc)
{
    size_t i = index_of((uintptr_t)p);
    static const size_t order[5] = {3, 4, 2, 1, 0};
    REQUIRE(phases[i] == 4 &&
            i == (live == 5 ? order[freed] : live - freed - 1) && exclusive &&
            active_domain == d && active_scope != 0);
    REQUIRE(d == expected_init[i][3] && r == expected_init[i][1] &&
            inc == expected_init[i][2]);
    REQUIRE(changes == (live == 5 ? 9u : 0u));
    REQUIRE(reads == (live == 5 && freed > 0 ? 2u : 0u));
    if (i == 2)
        actual_tail = NULL;
    /* Retire every typed alias before EndRoot; no C pointer evaluation later.
     */
    nodes[i] = NULL; /* no typed observer pointer survives target EndRoot */
    phases[i] = 5;
    printf("END %zu %zx %zu %zu %zu\n", i, (size_t)(uintptr_t)p, d, r, inc);
}
static void raw(const void *p, size_t n)
{
    selected = index_of((uintptr_t)p);
    REQUIRE(phases[selected] == 5 && active_scope == 0 && n == 56);
    phases[selected] = 6;
}
static void finalize(size_t d)
{
    REQUIRE(phases[selected] == 6 && d == expected_init[selected][3] &&
            active_scope == 0);
    phases[selected] = 7;
}
static void release(const void *p, const void *raw_p, size_t n)
{
    size_t i = index_of((uintptr_t)p);
    REQUIRE(i == selected && phases[i] == 7 &&
            (uintptr_t)raw_p == addresses[i] && n == 56 && active_scope == 0);
    phases[i] = 8;
}
void five_observe_free(uintptr_t p)
{
    size_t i = index_of(p);
    static const size_t order[5] = {3, 4, 2, 1, 0};
    REQUIRE(phases[i] == 8 && freed < live &&
            i == (live == 5 ? order[freed] : live - freed - 1));
    phases[i] = 9;
    free_order[freed++] = p;
}
static void copy_read(const nl_option *f, const nl_option *value, size_t root,
                      size_t index, size_t fact, size_t occurrence)
{
    REQUIRE(reads < 2 && changes == 9 && freed == 1 && five_releases() == 1 &&
            phases[3] == 9 && nodes[3] == NULL && active_scope && !exclusive);
    REQUIRE(root == expected_read[reads][0] &&
            index == expected_read[reads][1] &&
            fact == expected_read[reads][3] &&
            occurrence == expected_read[reads][4]);
    if (reads == 0) {
        REQUIRE(f == &nodes[1]->f1 && value->tag == 1 &&
                value->ptr == nodes[2] && matched == 0);
        REQUIRE(nodes[1]->f0.tag == 1 && nodes[1]->f0.ptr == nodes[2] &&
                nodes[1]->f2.tag == 0 && nodes[1]->f2.ptr == NULL);
    } else {
        REQUIRE(matched == 1 && actual_tail == nodes[2] &&
                f == &actual_tail->f0 && value->tag == 0 && value->ptr == NULL);
        REQUIRE(actual_tail->f1.tag == 1 && actual_tail->f1.ptr == nodes[1] &&
                actual_tail->f2.tag == 0 && actual_tail->f2.ptr == NULL);
    }
    p1393_observe_live_copy(f, value);
    printf("READ %zu %zu %zu %u %zx\n", reads, root, index, value->tag,
           (size_t)(uintptr_t)value->ptr);
    ++reads;
}
static void some_pointer(const nl_node *p)
{
    REQUIRE(reads == 1 && matched == 0 && freed == 1 && active_scope == 0 &&
            phases[3] == 9 && nodes[3] == NULL && p == nodes[2]);
    actual_tail = p;
    ++matched;
    printf("SOME actual-live-B=%zx\n", (size_t)(uintptr_t)p);
}
static void finish(void)
{
    REQUIRE(active_scope == 0 && trials == arms && five_requests() == trials &&
            five_releases() == live && freed == live && roots == fields &&
            fields == changes + reads && reads == (live == 5 ? 2u : 0u) &&
            matched == (live == 5 ? 1u : 0u));
    REQUIRE(trials == (live == 5 ? 5u : live + 1));
    for (size_t i = 0; i < 5; ++i)
        REQUIRE(phases[i] == (i < live ? 9u : i == live ? 99u : 0u));
    printf(
        "OBSERVED trials=%zu allocations=%zu frees=%zu changes=%zu addresses=",
        trials, live, freed, changes);
    for (size_t i = 0; i < 5; ++i)
        printf("%s%zx", i ? "," : "", (size_t)addresses[i]);
    printf(" order=");
    for (size_t i = 0; i < freed; ++i)
        printf("%s%zx", i ? "," : "", (size_t)free_order[i]);
    printf("\n");
}
#define NL_FIVE_TRIAL(s, p, n, a) trial(s, p, n, a)
#define NL_FIVE_ARM(s, v) arm(s, v)
#define NL_FIVE_SLOT(p, n) slot(p, n)
#define NL_FIVE_DOMAIN(d) domain(d)
#define NL_FIVE_INIT(p, d, r, i) initialize(p, d, r, i)
#define NL_FIVE_LOAN(d, s, e) loan(d, s, e)
#define NL_FIVE_SCOPE_END(s) scope_end(s)
#define NL_FIVE_ROOT(p, d, w, r, i, s) root_ref(p, d, w, r, i, s)
#define NL_FIVE_FIELD(p, f, w, x, n, c, i, s) field_ref(p, f, w, x, n, c, i, s)
#define NL_FIVE_CHANGE(f, o, b, a) change(f, o, b, a)
#define NL_FIVE_END(p, d, r, i) end(p, d, r, i)
#define NL_FIVE_RAW(p, n) raw(p, n)
#define NL_FIVE_FINALIZE(d) finalize(d)
#define NL_FIVE_RELEASE(p, r, n) release(p, r, n)
#define NL_FIVE_FINISH() finish()

#define NL_FIVE_READ(f, v, r, i, c, o) copy_read(f, v, r, i, c, o)
#define NL_FIVE_SOME(p) some_pointer(p)
