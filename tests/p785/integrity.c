#include "arm.h"
static bool rejected(ArmCall a)
{
    NLCopyBodyView v;
    NLCopyBodyParameters p;
    return nl_checked_copy_body_view(a.caller, a.call, &v) &&
           nl_checked_copy_body_parameters(a.caller, a.call, &p) &&
           nl_checked_copy_body_validate(a.caller, a.call, &v) != NL_CHECK_OK &&
           nl_checked_copy_body_parameters_validate(a.caller, a.call, &p) !=
               NL_CHECK_OK;
}
static bool run(const char *path)
{
    TestNode n = {0}, foreign = {0};
    CHECK(node_checked_load(path, &n) && node_checked_load(path, &foreign));
    ArmCall actual[8], alien[8];
    size_t count = 0, other = 0;
    CHECK(collect(n.entry, NULL, 0, 0, actual, &count, 0) &&
          collect(foreign.entry, NULL, 0, 0, alien, &other, 0) && count &&
          count == other);
    size_t mutations = 0;
    for (size_t i = 0; i < count; ++i) {
        ArmCall a = actual[i], f = alien[i];
        NLCopyBodyView v, fv;
        NLCopyBodyParameters p, fp;
        CHECK(valid(a) && valid(f) && a.call == f.call &&
              a.caller != f.caller &&
              nl_checked_copy_body_view(a.caller, a.call, &v) &&
              nl_checked_copy_body_view(f.caller, f.call, &fv) &&
              nl_checked_copy_body_parameters(a.caller, a.call, &p) &&
              nl_checked_copy_body_parameters(f.caller, f.call, &fp) &&
              p.field.parent == fp.field.parent && p.entry != fp.entry);
        CHECK(nl_checked_copy_body_validate(a.caller, a.call, &fv) !=
                  NL_CHECK_OK &&
              nl_checked_copy_body_parameters_validate(a.caller, a.call, &fp) !=
                  NL_CHECK_OK);
        const NLCheckedFragment *wrong[] = {
            a.parent, nl_checked_match_arm(a.parent, a.match, 1 - a.index),
            nl_checked_match_arm(a.parent, a.match, 2),
            nl_checked_match_arm(a.parent, a.match + 1, a.index)};
        for (size_t j = 0; j < 4; ++j)
            CHECK(nl_checked_copy_body_validate(wrong[j], a.call, &v) !=
                  NL_CHECK_OK);
#define MUTATE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        CHECK(rejected(a));                                                    \
        restore;                                                               \
        CHECK(valid(a));                                                       \
        ++mutations;                                                           \
    } while (0)
        NLCheckedNodeView root = a.caller->nodes[a.caller->root - 1],
                          call = a.caller->nodes[a.call - 1],
                          loan = a.caller->nodes[p.loan - 1];
        MUTATE(a.caller->nodes[a.caller->root - 1].variant = 1,
               a.caller->nodes[a.caller->root - 1] = root);
        size_t root_id = a.caller->root, total = a.caller->count;
        MUTATE(a.caller->root = 1, a.caller->root = root_id);
        MUTATE(a.caller->count--, a.caller->count = total);
        const NLSource *source = a.caller->source;
        MUTATE(a.caller->source = f.caller->source, a.caller->source = source);
        NLCheckedNodeView *nodes = a.caller->nodes;
        NLCheckedNodeView *clone = malloc(total * sizeof(*clone));
        CHECK(clone);
        memcpy(clone, nodes, total * sizeof(*clone));
        MUTATE(a.caller->nodes = clone, a.caller->nodes = nodes);
        free(clone);
        MUTATE(a.caller->nodes[a.call - 1] = loan,
               a.caller->nodes[a.call - 1] = call);
        MUTATE(a.caller->nodes[p.loan - 1].initializer = a.call,
               a.caller->nodes[p.loan - 1] = loan);
        MUTATE(a.caller->nodes[p.loan - 1].loan.scope++,
               a.caller->nodes[p.loan - 1] = loan);
        MUTATE(a.caller->nodes[p.loan - 1].field.index = 1 - p.field.index,
               a.caller->nodes[p.loan - 1] = loan);
        MUTATE(a.caller->nodes[p.loan - 1].field.child_fact++,
               a.caller->nodes[p.loan - 1] = loan);
        MUTATE(a.caller->nodes[p.loan - 1].field.parent_fact++,
               a.caller->nodes[p.loan - 1] = loan);
        MUTATE(a.caller->nodes[p.loan - 1].field.parent++,
               a.caller->nodes[p.loan - 1] = loan);
        MUTATE(a.caller->nodes[a.call - 1].results[0].value++,
               a.caller->nodes[a.call - 1] = call);
        NLCheckedNodeId arg = p.arguments[1];
        NLCheckedNodeView value = a.caller->nodes[arg - 1];
        MUTATE(a.caller->nodes[arg - 1].results[0].value++,
               a.caller->nodes[arg - 1] = value);
        for (size_t j = 1; j <= a.body->count; ++j) {
            NLCheckedNodeView saved = a.body->nodes[j - 1];
            if (saved.kind == NL_CHECKED_IDENTIFIER) {
                MUTATE(a.body->nodes[j - 1].symbol++,
                       a.body->nodes[j - 1] = saved);
                break;
            }
        }
        NLCopyBodyParameters forged = p;
        forged.entry = fp.entry;
        CHECK(nl_checked_copy_body_parameters_validate(a.caller, a.call,
                                                       &forged) != NL_CHECK_OK);
        forged = p;
        forged.handoff = fp.handoff;
        CHECK(nl_checked_copy_body_parameters_validate(a.caller, a.call,
                                                       &forged) != NL_CHECK_OK);
        NLSemanticPlaceView child, parent, sibling;
        CHECK(nl_semantic_place_view(p.handoff, p.field.child, &child) &&
              nl_semantic_place_view(p.handoff, p.field.parent, &parent) &&
              nl_semantic_place_view(
                  p.handoff, parent.fixed_fields[1 - p.field.index], &sibling));
        NLCopyBodyView bad = v;
        bad.result.value = child.current_value;
        CHECK(nl_checked_copy_body_validate(a.caller, a.call, &bad) !=
              NL_CHECK_OK);
        bad = v;
        bad.result.value = sibling.current_value;
        CHECK(nl_checked_copy_body_validate(a.caller, a.call, &bad) !=
              NL_CHECK_OK);
#undef MUTATE
    }
    printf(
        "INTEGRITY same-number alien worlds refused; parent/sibling/index "
        "lookup refused; graph/source/variant/facts/Copy/formal mutations=%zu "
        "restore=PASS\n",
        mutations);
    node_checked_destroy(&n);
    node_checked_destroy(&foreign);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && run(argv[1]) ? 0 : 1;
}
