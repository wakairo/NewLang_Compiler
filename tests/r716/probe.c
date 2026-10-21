#include "../support/node_checked.h"
#include "newlang/if_field.h"

static bool inspect(const char *path, size_t index, size_t first, size_t second)
{
    TestNode node = {0};
    CHECK(node_checked_load(path, &node));
    const NLCheckedFragment *body =
        nl_checked_call_body(node.entry, nl_checked_root(node.entry));
    CHECK(body);
    size_t found = 0;
    for (size_t id = 1; id <= nl_checked_node_count(body); ++id) {
        NLIfFieldView view;
        if (!nl_checked_if_field_view(body, id, &view))
            continue;
        CHECK(++found == 1 && view.index == index);
        CHECK(nl_checked_if_field_validate(body, id, &view) == NL_CHECK_OK);
        NLSemanticPlaceView initial, post, child, sibling, sibling_initial;
        CHECK(nl_semantic_place_view(view.entry, view.root, &initial) &&
              nl_semantic_place_view(view.post, view.root, &post) &&
              nl_semantic_place_view(view.post, view.child, &child) &&
              nl_semantic_place_view(view.post, view.sibling, &sibling) &&
              nl_semantic_place_view(view.entry, view.sibling, &sibling_initial));
        CHECK(initial.incarnation == post.incarnation &&
              post.incarnation == view.incarnation &&
              child.incarnation == view.child_incarnation &&
              initial.current_fact != post.current_fact &&
              sibling_initial.current_fact == sibling.current_fact &&
              sibling_initial.current_value == sibling.current_value);
        NLSemanticValueView target, whole;
        CHECK(nl_semantic_value_view(view.post, child.current_value, &target) &&
              !target.scalar_known && target.reference.place == 0 &&
              target.domain == 0 && target.allocation_region == 0 &&
              nl_semantic_value_view(view.post, post.current_value, &whole) &&
              whole.field_count == 2 &&
              whole.fields[index] == child.current_value &&
              whole.fields[1 - index] == sibling.current_value);
        const size_t expected[2] = {first, second};
        printf("{\"if\":%zu,\"base\":%zu,\"root\":%zu,\"child\":%zu,"
               "\"index\":%zu,\"initial_root_fact\":%zu,\"post_root_fact\":%zu,"
               "\"initial_sibling_fact\":%zu,\"post_sibling_fact\":%zu,"
               "\"arms\":[",
               id, view.base, view.root, view.child, view.index,
               initial.current_fact, post.current_fact,
               sibling_initial.current_fact, sibling.current_fact);
        for (size_t arm = 0; arm != 2; ++arm) {
            const NLIfFieldArm *a = &view.arms[arm];
            NLSemanticPlaceView parent, field, sib;
            CHECK(a->artifact == nl_checked_if_arm(body, id, arm) &&
                  a->world != view.entry && a->world != view.post &&
                  view.arms[0].world != view.arms[1].world &&
                  nl_semantic_place_view(a->world, view.root, &parent) &&
                  nl_semantic_place_view(a->world, view.child, &field) &&
                  nl_semantic_place_view(a->world, view.sibling, &sib) &&
                  parent.incarnation == view.incarnation &&
                  field.incarnation == view.child_incarnation &&
                  sib.current_fact == sibling_initial.current_fact &&
                  sib.current_value == sibling_initial.current_value &&
                  a->change_count == expected[arm]);
            if (!expected[arm])
                CHECK(parent.current_fact == initial.current_fact);
            NLSemanticSnapshot snapshot;
            CHECK(nl_semantic_snapshot(a->world, &snapshot));
            for (size_t s = 1; s <= snapshot.scopes; ++s) {
                NLSemanticScopeView scope;
                CHECK(nl_semantic_scope_view(a->world, s, &scope) && !scope.active);
            }
            size_t count = 0;
            printf("%s{\"changes\":%zu,\"parent_fact\":%zu,\"child_fact\":%zu,"
                   "\"events\":[",
                   arm ? "," : "", a->change_count,
                   parent.current_fact, field.current_fact);
            for (size_t e = 0; e < a->event_count; ++e) {
                NLIfFieldEvent event;
                CHECK(nl_checked_if_field_event(body, id, arm, e, &event) &&
                      nl_checked_if_field_event_validate(body, id, arm, e,
                                                         &event) == NL_CHECK_OK);
                const NLCheckedNodeView *operation =
                    nl_checked_node_view(a->artifact, event.node);
                size_t reference = 0;
                if (event.change) {
                    const NLCheckedNodeView *arg =
                        nl_checked_node_view(a->artifact, operation->first_argument);
                    CHECK(arg && event.field.index == index &&
                          event.field.parent == view.root &&
                          event.field.child == view.child);
                    reference = arg->symbol;
                    ++count;
                }
                printf("%s{\"node\":%zu,\"change\":%s,\"ref\":%zu,"
                       "\"old_id\":%zu,\"incoming_id\":%zu,\"result_id\":%zu}",
                       e ? "," : "", event.node,
                       event.change ? "true" : "false", reference,
                       event.field.old_value, event.incoming, event.result);
            }
            CHECK(count == expected[arm]);
            fputs("]}", stdout);
        }
        size_t binding = 0;
        for (size_t i = 1; i <= nl_checked_node_count(body); ++i) {
            const NLCheckedNodeView *v = nl_checked_node_view(body, i);
            if (v->kind == NL_CHECKED_BINDING && v->initializer == id)
                binding = v->symbol;
        }
        CHECK(binding);
        size_t refused = 0;
#define BAD(change)                                                            \
    do {                                                                       \
        NLIfFieldView corrupt = view;                                          \
        change;                                                                \
        CHECK(nl_checked_if_field_validate(body, id, &corrupt) ==              \
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);                              \
        CHECK(nl_checked_if_field_validate(body, id, &view) == NL_CHECK_OK);    \
        ++refused;                                                             \
    } while (0)
        BAD(corrupt.root++);
        BAD(corrupt.child++);
        BAD(corrupt.sibling++);
        BAD(corrupt.index = 1 - corrupt.index);
        BAD(corrupt.base++);
        BAD(corrupt.nominal++);
        BAD(corrupt.incarnation++);
        BAD(corrupt.child_incarnation++);
        BAD(corrupt.condition++);
        BAD(corrupt.conditional++);
        BAD(corrupt.entry = view.post);
        BAD(corrupt.post = view.entry);
        BAD(corrupt.result.value++);
        BAD(corrupt.arms[0].event_count++);
        BAD(corrupt.arms[1].change_count++);
        BAD(corrupt.arms[0].world = view.post);
        BAD(corrupt.arms[0] = corrupt.arms[1]);
#undef BAD
        printf("],\"result_binding\":%zu,\"rejected_public\":%zu}\n",
               binding, refused);
    }
    CHECK(found == 1);
    node_checked_destroy(&node);
    return true;
}

int main(int argc, char **argv)
{
    return argc == 5 && inspect(argv[1], (size_t)atoi(argv[2]),
                                (size_t)atoi(argv[3]),
                                (size_t)atoi(argv[4])) ? 0 : 1;
}
