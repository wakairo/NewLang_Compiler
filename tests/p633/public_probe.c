/* Public readback of genuine registered source after AST teardown. No new
 * public validator or host-created semantic values/permissions. */
#include "../support/node_checked.h"
static bool observe(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f != NULL);
    const NLSemanticContext *c = nl_checked_context(f);
    NLCheckedNodeId id = 0;
    NLSymbolId selected = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_IF) {
            CHECK(id == 0);
            id = i;
        }
        if (v->kind == NL_CHECKED_BINDING && v->initializer == id && id)
            selected = v->symbol;
    }
    const NLCheckedNodeView *join = nl_checked_node_view(f, id);
    CHECK(join && selected && join->normal_arms == 2 &&
          join->result_count == 1);
    const NLCheckedNodeView *cond = nl_checked_node_view(f, join->initializer);
    NLSemanticValueView value, truth;
    CHECK(cond && nl_semantic_value_view(c, cond->results[0].value, &truth) &&
          truth.scalar_known && truth.scalar_value <= 1 &&
          nl_semantic_value_view(c, join->results[0].value, &value) &&
          !value.scalar_known && value.dependencies == NL_DEPENDENCY_FREE &&
          value.value_dependency_count == 0 && value.domain == 0 &&
          value.reference.place == 0 && value.allocation_region == 0);
    size_t results[2], locals[2];
    const NLSemanticContext *worlds[2];
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm = nl_checked_if_arm(f, id, a);
        CHECK(arm && nl_checked_source(arm) == nl_checked_source(f));
        worlds[a] = nl_checked_context(arm);
        NLSemanticSnapshot s;
        CHECK(nl_semantic_snapshot(worlds[a], &s) && s.domains == 0 &&
              s.backing_regions == 0 && s.occurrences == 0 && s.scopes == 0);
        const NLCheckedNodeView *r =
            nl_checked_node_view(arm, nl_checked_root(arm));
        const NLCheckedNodeView *b =
            r ? nl_checked_node_view(arm, r->initializer) : NULL;
        CHECK(b && b->item_count <= 1 && r->result_count == 1 &&
              !r->terminates &&
              nl_semantic_value_view(worlds[a], r->results[0].value, &value) &&
              value.scalar_known && value.scalar_value <= 255 &&
              value.dependencies == NL_DEPENDENCY_FREE &&
              value.value_dependency_count == 0 && value.domain == 0 &&
              value.reference.place == 0 && value.allocation_region == 0);
        results[a] = value.scalar_value;
        locals[a] = b->item_count;
        if (locals[a]) {
            const NLCheckedNodeView *decl =
                nl_checked_node_view(arm, b->first_item);
            const NLCheckedNodeView *tail = nl_checked_node_view(arm, b->tail);
            NLSemanticBindingView binding;
            NLSemanticPlaceView place;
            CHECK(decl && tail && tail->symbol == decl->symbol &&
                  tail->value_use == NL_VALUE_COPIED &&
                  nl_semantic_binding_view(worlds[a], decl->symbol, &binding) &&
                  binding.availability == NL_CONSUMED &&
                  nl_semantic_place_view(worlds[a], binding.place, &place) &&
                  !place.live && place.current_fact == 0 &&
                  place.current_value == 0 && place.governing_domain == 0);
        }
    }
    CHECK(worlds[0] != worlds[1] && worlds[0] != c && worlds[1] != c);
    printf("{\"if\":%zu,\"condition_symbol\":%zu,\"bool\":%zu,\"selected_"
           "symbol\":%zu,\"arms\":[%zu,%zu],\"arm_locals\":[%zu,%zu],"
           "\"authority\":0,\"joined_unknown\":true,\"arm_locals_ended\":true,"
           "\"worlds_distinct\":true}\n",
           id, cond->symbol, truth.scalar_value, selected, results[0],
           results[1], locals[0], locals[1]);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && observe(argv[1]) ? 0 : 1;
}
