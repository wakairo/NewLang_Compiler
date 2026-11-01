#include "../p1062/support.h"
#include "newlang/if_field.h"
static bool observe(const char *path)
{
    NLSemanticContext *base = NULL;
    TestNode n = {0};
    P1062Call calls[2];
    CHECK(registry(path, &base) && checked(base, &n) &&
          p1062_calls(&n, calls) && p1062_valid(calls));
    const NLCheckedFragment *f = calls[0].caller;
    const NLSemanticContext *c = nl_checked_context(f);
    size_t choices = 0;
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        if (v->kind == NL_CHECKED_IF) {
            NLIfFieldView view;
            CHECK(nl_checked_if_field_view(f, id, &view) &&
                  nl_checked_if_field_validate(f, id, &view) == NL_CHECK_OK);
            ++choices;
            NLSemanticPlaceView before, after, sibling;
            NLSemanticValueView result;
            CHECK(
                nl_semantic_place_view(view.entry, view.child, &before) &&
                nl_semantic_place_view(view.post, view.child, &after) &&
                nl_semantic_place_view(view.post, view.sibling, &sibling) &&
                nl_semantic_value_view(view.post, view.result.value, &result) &&
                !result.scalar_known &&
                before.current_fact != after.current_fact);
            printf("{\"kind\":\"IF\",\"node\":%zu,\"current\":0,\"root\":%zu,"
                   "\"child\":%zu,\"index\":%zu,\"common_result_known\":false,"
                   "\"common_child_value\":%zu,\"post_fact\":%zu}\n",
                   id, view.root, view.child, view.index, after.current_value,
                   after.current_fact);
            for (size_t a = 0; a < 2; ++a) {
                NLIfFieldEvent e;
                NLSemanticValueView old, incoming;
                CHECK(view.arms[a].event_count == 1 &&
                      view.arms[a].change_count == 1 &&
                      nl_checked_if_field_event(f, id, a, 0, &e) && e.change &&
                      nl_checked_if_field_event_validate(f, id, a, 0, &e) ==
                          NL_CHECK_OK &&
                      nl_semantic_value_view(e.before, e.result, &old) &&
                      old.scalar_known &&
                      nl_semantic_value_view(e.before, e.incoming, &incoming) &&
                      incoming.scalar_known);
                printf("{\"kind\":\"event\",\"arm\":%zu,\"old\":%zu,\"next\":%"
                       "zu,\"current\":0}\n",
                       a, old.scalar_value, incoming.scalar_value);
            }
        }
        if (v->kind == NL_CHECKED_BINDING && v->symbol &&
            v->symbol <= c->binding_count) {
            const char *name = c->bindings[v->symbol - 1].name;
            if (!strcmp(name, "old_primary") ||
                !strcmp(name, "old_secondary") ||
                !strcmp(name, "selected_old") || !strcmp(name, "last_old")) {
                const NLCheckedNodeView *value =
                    nl_checked_node_view(f, v->initializer);
                NLSemanticValueView result;
                CHECK(value && value->result_count == 1 &&
                      nl_semantic_value_view(c, value->results[0].value,
                                             &result));
                printf("{\"kind\":\"binding\",\"name\":\"%s\",\"symbol\":%zu,"
                       "\"known\":%s,\"value\":%zu}\n",
                       name, v->symbol, result.scalar_known ? "true" : "false",
                       result.scalar_value);
            }
        }
    }
    CHECK(choices == 1 && nl_h_origin_validate(n.entry) &&
          nl_h_custody_validate(n.entry));
    puts("{\"kind\":\"ownership\",\"origin\":true,\"custody\":true,\"Copy_"
         "current\":0}");
    node_checked_destroy(&n);
    nl_semantic_destroy(base);
    return true;
}
int main(int argc, char **argv)
{
    (void)node_backend;
    return argc == 2 && observe(argv[1]) ? 0 : 1;
}
