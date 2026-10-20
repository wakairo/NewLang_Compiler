/* Read-only public observations of real registered source after AST teardown.
 */
#include "../support/node_checked.h"
static bool observe(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f != NULL);
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *join = nl_checked_node_view(f, id);
        if (join->kind != NL_CHECKED_IF)
            continue;
        printf("IF %zu condition=%zu\n", id, join->initializer);
        for (size_t a = 0; a < 2; ++a) {
            const NLCheckedFragment *arm = nl_checked_if_arm(f, id, a);
            CHECK(arm && nl_checked_source(arm) == nl_checked_source(f));
            const NLSemanticContext *c = nl_checked_context(arm);
            NLSemanticSnapshot s;
            CHECK(nl_semantic_snapshot(c, &s));
            printf("ARM %zu nodes=%zu bindings=%zu places=%zu values=%zu "
                   "fact=%zu scopes=%zu domains=%zu regions=%zu\n",
                   a, nl_checked_node_count(arm), s.bindings, s.places,
                   s.values, s.last_value_fact, s.scopes, s.domains,
                   s.backing_regions);
            for (size_t i = 1; i <= nl_checked_node_count(arm); ++i) {
                const NLCheckedNodeView *v = nl_checked_node_view(arm, i);
                NLSemanticValueView value = {0};
                if (v->result_count)
                    CHECK(
                        nl_semantic_value_view(c, v->results[0].value, &value));
                printf("NODE %zu kind=%d symbol=%zu value=%zu scalar=%d/%zu "
                       "carrier=%d owner=%zu init=%zu first=%zu count=%zu "
                       "next=%zu tail=%zu bytes=%zu:%zu name=%zu:%zu\n",
                       i, (int)v->kind, v->symbol, v->results[0].value,
                       value.scalar_known, value.scalar_value,
                       (int)value.carrier, value.owner_place, v->initializer,
                       v->first_item, v->item_count, v->next_item, v->tail,
                       v->span.start_byte, v->span.end_byte, v->name.start_byte,
                       v->name.end_byte);
            }
            for (size_t b = 1; b <= s.bindings; ++b) {
                NLSemanticBindingView v;
                NLSemanticPlaceView p;
                CHECK(nl_semantic_binding_view(c, b, &v) &&
                      nl_semantic_place_view(c, v.place, &p));
                printf("BIND %zu type=%zu value=%zu availability=%d place=%zu "
                       "live=%d incarnation=%zu fact=%zu current=%zu root=%d\n",
                       b, v.type, v.value, (int)v.availability, v.place, p.live,
                       p.incarnation, p.current_fact, p.current_value,
                       p.independent_root);
            }
        }
    }
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && observe(argv[1]) ? 0 : 1;
}
