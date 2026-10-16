#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"
int main(int argc, char **argv)
{
    TestNode n = {0};
    if (argc != 2 || !node_checked_load(argv[1], &n))
        return 1;
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    const NLSemanticContext *c = nl_checked_context(f);
    V0Program p = {
        .u8 = nl_semantic_core_type(c, NL_TYPE_U8), .unit = 1, .count = 1};
    for (size_t i = 1; i <= nl_checked_node_count(f); i++) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_IF) {
            const NLCheckedNodeView *q =
                nl_checked_node_view(f, v->initializer);
            printf("IF=%zu span=%zu-%zu condition=%zu kind=%d origin=%zu "
                   "bool_root=%d boolean_scalar=%d\n",
                   i, v->span.start_byte, v->span.end_byte, v->initializer,
                   q->kind, bool_source_origin(f, v->initializer),
                   bool_root_value(f, v->initializer),
                   boolean_scalar(f, v->initializer, &p));
            FILE *out = tmpfile();
            printf(
                "existing_scalar_renderer=%d existing_equality_renderer=%d\n",
                v1_emit_scalar(out, q), emit_quantity_bool(out, f, q));
            fclose(out);
            for (size_t a = 0; a < 2; a++) {
                const NLCheckedFragment *arm = nl_checked_if_arm(f, i, a);
                const NLSemanticContext *w = nl_checked_context(arm);
                for (size_t j = 0; j < w->binding_count; j++) {
                    NLSemanticBindingView b = w->bindings[j].view;
                    NLSemanticPlaceView r = w->places[b.place - 1];
                    if (r.fixed_field_count == 2) {
                        printf("ARM%zu original_binding=%zu place=%zu inc=%zu "
                               "fact=%zu value=%zu children=%zu,%zu "
                               "known_old=%zu right=%zu\n",
                               a, j + 1, b.place, r.incarnation, r.current_fact,
                               r.current_value, r.fixed_fields[0],
                               r.fixed_fields[1],
                               w->values[w->places[r.fixed_fields[0] - 1]
                                             .current_value -
                                         1]
                                   .scalar_value,
                               w->values[w->places[r.fixed_fields[1] - 1]
                                             .current_value -
                                         1]
                                   .scalar_value);
                    }
                }
            }
        }
        if (v->kind == NL_CHECKED_REPLACE || v->kind == NL_CHECKED_FIELD_READ)
            printf("OP=%zu kind=%d base=%zu parent=%zu child=%zu inc=%zu,%zu "
                   "facts=%zu,%zu post=%zu,%zu old=%zu new=%zu\n",
                   i, v->kind, v->field.base, v->field.parent, v->field.child,
                   v->field.parent_incarnation, v->field.child_incarnation,
                   v->field.parent_fact, v->field.child_fact,
                   v->field.parent_post_fact, v->field.child_post_fact,
                   v->field.old_value, v->field.new_value);
    }
    node_checked_destroy(&n);
    return 0;
}
