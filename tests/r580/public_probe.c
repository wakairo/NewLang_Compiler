/* R580 independent first-pass. Uses registered source and public checked APIs
 * already available in baseline; no candidate private layout, fake roots or emitter.
 * This does not substitute for a new candidate-specific published validator. */
#include "node_checked.h"
#include "newlang/checked.h"
#include <stdio.h>
#include <string.h>

static bool inspect(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f != NULL);
    const NLSemanticContext *ctx = nl_checked_context(f);
    CHECK(ctx != NULL);
    struct { const char *name; NLSymbolId symbol; NLPlaceId place; } roots[3] = {
        {"c", 0, 0}, {"b", 0, 0}, {"a", 0, 0}
    };
    size_t root_bindings = 0, changes = 0, write_loans = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        CHECK(v != NULL);
        if (v->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *ident = nl_checked_node_view(f, v->first_argument);
            NLSourceView name;
            CHECK(ident && nl_source_view(nl_checked_source(f), v->name, &name));
            for (size_t j = 0; j < 3; ++j) {
                if (strlen(roots[j].name) != name.length ||
                    memcmp(name.bytes, roots[j].name, name.length) != 0) continue;
                NLSemanticBindingView binding;
                NLSemanticPlaceView place;
                CHECK(!roots[j].symbol && !roots[j].place);
                CHECK(nl_semantic_binding_view(ctx, ident->symbol, &binding));
                CHECK(binding.place &&
                      nl_semantic_place_view(ctx, binding.place, &place) &&
                      place.independent_root && place.implicit_local &&
                      place.governing_domain == 0 && place.placement.region == 0);
                roots[j].symbol = ident->symbol;
                roots[j].place = binding.place;
                printf("ROOT role=%s symbol=%zu place=%zu inc=%zu field0=%zu\n",
                       roots[j].name, roots[j].symbol, roots[j].place,
                       place.incarnation, place.fixed_fields[0]);
                ++root_bindings;
            }
        }
        if (v->kind == NL_CHECKED_LOAN_HEADER &&
            v->loan.access == NL_ACCESS_WRITE) {
            CHECK(!v->loan.from_ptr && v->loan.body_nonescape_proved &&
                  v->loan.prevent_conflicting_access &&
                  v->loan.implicit_local);
            ++write_loans;
        }
        if (v->kind == NL_CHECKED_REPLACE) {
            CHECK(changes < 2);
            const NLCheckedField *field = &v->field;
            size_t root_index = changes == 0 ? 2 : 1; /* alpha then beta */
            CHECK(roots[root_index].symbol && roots[root_index].place);
            CHECK(field->present && field->dependency_compatible &&
                  field->base == roots[root_index].symbol &&
                  field->parent == roots[root_index].place &&
                  field->index == 0 && field->access == NL_ACCESS_WRITE &&
                  field->child && field->parent_incarnation &&
                  field->child_incarnation &&
                  field->parent_fact != field->parent_post_fact &&
                  field->child_fact != field->child_post_fact);
            NLSemanticValueView old_value, new_value;
            CHECK(nl_semantic_value_view(ctx, field->old_value, &old_value) &&
                  nl_semantic_value_view(ctx, field->new_value, &new_value) &&
                  old_value.variant == 1 && new_value.variant == 2 &&
                  new_value.sum_payload);
            NLSemanticValueView ptr;
            CHECK(nl_semantic_value_view(ctx, new_value.sum_payload, &ptr) &&
                  ptr.reference_count == 0 &&
                  ptr.reference.provenance == NL_PROVENANCE_VALID &&
                  ptr.reference.scope == 0 && !ptr.reference.writable &&
                  ptr.reference.place == roots[changes == 0 ? 1 : 0].place);
            printf("CHANGE step=%zu parent=%zu field=%zu parentFact=%zu:%zu childFact=%zu:%zu occurrence=%zu:%zu ptrTo=%zu\n",
                   changes + 1, field->parent, field->child,
                   field->parent_fact, field->parent_post_fact,
                   field->child_fact, field->child_post_fact,
                   field->payload_occurrence, field->post_payload_occurrence,
                   ptr.reference.place);
            ++changes;
        }
    }
    CHECK(root_bindings == 3 && changes == 2 && write_loans == 2 &&
          roots[0].place != roots[1].place &&
          roots[1].place != roots[2].place &&
          roots[0].place != roots[2].place);
    puts("R580 BASELINE PUBLIC INSPECTION PASS; candidate validator not tested");
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && inspect(argv[1]) ? 0 : 1;
}
