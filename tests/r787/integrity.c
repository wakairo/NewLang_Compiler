/* R787: public copied-view rejection. Never mutate source-issued seal. */
#include "../../src/copy_body_internal.h"
#include "../support/node_checked.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    NLCheckedFragment *arm, *body, *parent;
    NLCheckedNodeId call, match;
    size_t arm_index;
} Found;
static bool discover(NLCheckedFragment *f, NLCheckedFragment *parent,
                     NLCheckedNodeId match, size_t arm_index,
                     Found found[4], size_t *n, size_t depth)
{
    CHECK(f && depth < 24);
    for (size_t i = 0; i < f->body_count; ++i) {
        NLCheckedFragment *body = f->bodies[i];
        if (parent && body->copy_body && body->copy_body->formal_write) {
            CHECK(*n < 4);
            found[(*n)++] = (Found){f, body, parent, f->body_calls[i],
                                   match, arm_index};
        }
        CHECK(discover(body, NULL, 0, 0, found, n, depth + 1));
    }
    for (size_t i = 0; i < f->arm_count; ++i) {
        size_t ordinal = 0;
        for (size_t j = 0; j < i; ++j)
            ordinal += f->arms[j].match == f->arms[i].match;
        CHECK(discover(f->arms[i].artifact, f, f->arms[i].match, ordinal,
                       found, n, depth + 1));
    }
    return true;
}
static bool test(const char *path, size_t expected)
{
    TestNode own = {0}, alien = {0};
    CHECK(node_checked_load(path, &own) && node_checked_load(path, &alien));
    Found good[4], bad[4];
    size_t count = 0, foreign_count = 0, rejections = 0;
    CHECK(discover(own.entry, NULL, 0, 0, good, &count, 0) &&
          discover(alien.entry, NULL, 0, 0, bad, &foreign_count, 0) &&
          count == expected && count == foreign_count);
    for (size_t i = 0; i < count; ++i) {
        Found a = good[i], x = bad[i];
        NLCopyBodyView view = {0}, foreign = {0};
        NLCopyBodyParameters p = {0}, fp = {0};
        CHECK(a.call == x.call && a.arm != x.arm &&
              nl_checked_copy_body_view(a.arm, a.call, &view) &&
              nl_checked_copy_body_view(x.arm, x.call, &foreign) &&
              nl_checked_copy_body_parameters(a.arm, a.call, &p) &&
              nl_checked_copy_body_parameters(x.arm, x.call, &fp) &&
              nl_checked_copy_body_validate(a.arm, a.call, &view) ==
                  NL_CHECK_OK &&
              nl_checked_copy_body_parameters_validate(a.arm, a.call, &p) ==
                  NL_CHECK_OK);
#define BODY(change)                                                           \
    do {                                                                       \
        NLCopyBodyView forged = view;                                           \
        change;                                                                \
        CHECK(nl_checked_copy_body_validate(a.arm, a.call, &forged) !=         \
              NL_CHECK_OK);                                                    \
        ++rejections;                                                          \
    } while (0)
#define PARAM(change)                                                          \
    do {                                                                       \
        NLCopyBodyParameters forged = p;                                        \
        change;                                                                \
        CHECK(nl_checked_copy_body_parameters_validate(a.arm, a.call,         \
                                                        &forged) !=             \
              NL_CHECK_OK);                                                    \
        ++rejections;                                                          \
    } while (0)
        BODY(forged = foreign);
        BODY(forged.root++);
        BODY(forged.event_count++);
        BODY(forged.function++);
        BODY(forged.result.value++);
        BODY(forged.live_exit = foreign.live_exit);
        CHECK(nl_checked_copy_body_validate(a.parent, a.call, &view) !=
              NL_CHECK_OK);
        ++rejections;
        const NLCheckedFragment *sibling =
            nl_checked_match_arm(a.parent, a.match, 0);
        CHECK(sibling && sibling != a.arm &&
              nl_checked_copy_body_validate(sibling, a.call, &view) !=
                  NL_CHECK_OK);
        ++rejections;
        PARAM(forged = fp);
        PARAM(forged.entry = fp.entry);
        PARAM(forged.handoff = fp.handoff);
        PARAM(forged.loan++);
        PARAM(forged.field.parent++);
        PARAM(forged.field.child++);
        PARAM(forged.field.index = 1 - p.field.index);
        PARAM(forged.field.child_fact++);
        PARAM(forged.field.parent_fact++);
        PARAM(forged.field.old_value++);
        PARAM(forged.field.parent_incarnation++);
        PARAM(forged.arguments[0]++);
        PARAM(forged.parameters[0]++);
        NLSemanticPlaceView child = {0}, parent = {0}, other = {0};
        CHECK(nl_semantic_place_view(p.handoff, p.field.child, &child) &&
              nl_semantic_place_view(p.handoff, p.field.parent, &parent) &&
              nl_semantic_place_view(
                  p.handoff, parent.fixed_fields[1 - p.field.index], &other));
        BODY(forged.result.value = child.current_value);
        BODY(forged.result.value = other.current_value);
#undef BODY
#undef PARAM
        CHECK(nl_checked_copy_body_validate(a.arm, a.call, &view) ==
                  NL_CHECK_OK &&
              nl_checked_copy_body_parameters_validate(a.arm, a.call, &p) ==
                  NL_CHECK_OK);
    }
    printf("R787_NEGATIVE path=%s original_seals_unchanged=1 "
           "copied_view_rejections=%zu foreign_world=REJECT "
           "wrong_arm=REJECT graph=REJECT field_loan_facts=REJECT\n",
           path, rejections);
    node_checked_destroy(&alien);
    node_checked_destroy(&own);
    return rejections >= expected * 20;
}
int main(int argc, char **argv)
{
    return argc == 3 && test(argv[1], 2) && test(argv[2], 1) ? 0 : 1;
}
