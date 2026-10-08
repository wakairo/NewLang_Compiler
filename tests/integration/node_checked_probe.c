#include "../support/node_checked.h"
/* Independent read-only semantic oracle. It does not call the C emitter or
 * interpret runtime representation. Spans label test roles only; values,
 * carrier IDs and mutation facts come exclusively from checked evidence. */
static bool inspect(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f != NULL);
    const NLSemanticContext *c = nl_checked_context(f);
    struct {
        size_t target, tag;
    } values[NL_SEMANTIC_MAX_ENTRIES] = {{0}};
    size_t old_tag = 0, old_target = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *r =
                nl_checked_node_view(f, v->first_argument);
            const NLCheckedNodeView *init =
                nl_checked_node_view(f, v->initializer);
            NLSourceView name;
            CHECK(r != NULL && init != NULL &&
                  nl_source_view(nl_checked_source(f), v->name, &name));
            NLSemanticTypeView type;
            CHECK(nl_semantic_type_view(c, r->type, &type));
            size_t kind = nl_semantic_recursive_local_type(c, r->type) ? 1
                          : type.kind == NL_TYPE_PTR                   ? 2
                          : type.kind == NL_TYPE_SUM                   ? 3
                                                                       : 4;
            size_t evidence = 0;
            if (kind == 1) {
                NLCheckedNodeId field = init->first_argument;
                for (size_t j = 0; j < init->argument_count; ++j) {
                    const NLCheckedNodeView *a = nl_checked_node_view(f, field);
                    CHECK(a != NULL);
                    if (a->field_index == 1) {
                        const NLCheckedNodeView *p =
                            nl_checked_node_view(f, a->initializer);
                        CHECK(p != NULL && p->has_scalar_result &&
                              p->scalar_result.known);
                        evidence = p->scalar_result.value;
                    }
                    field = a->next_argument;
                }
            } else if (kind == 2) {
                CHECK(init->kind == NL_CHECKED_LOAN_HEADER &&
                      !init->loan.from_ptr);
                evidence = init->loan.source;
            }
            CHECK(r->symbol < NL_SEMANTIC_MAX_ENTRIES);
            if (kind == 2)
                values[r->symbol].target = evidence;
            if (kind == 3) {
                if (init->kind == NL_CHECKED_LOAN_HEADER) {
                    values[r->symbol].tag = old_tag;
                    values[r->symbol].target = old_target;
                } else if (init->kind == NL_CHECKED_FIELD_READ) {
                    values[r->symbol] = values[init->field.base];
                } else if (init->kind == NL_CHECKED_IDENTIFIER) {
                    values[r->symbol] = values[init->symbol];
                } else {
                    CHECK(false);
                }
            }
            CHECK(printf("B %.*s %zu %zu %zu %zu %zu\n", (int)name.length,
                         name.bytes, r->symbol, kind, evidence,
                         values[r->symbol].tag, values[r->symbol].target) > 0);
        }
        if (v->kind == NL_CHECKED_REPLACE) {
            const NLCheckedNodeView *ref =
                nl_checked_node_view(f, v->first_argument);
            const NLCheckedNodeView *replacement =
                ref == NULL ? NULL
                            : nl_checked_node_view(f, ref->next_argument);
            NLSemanticValueView old;
            CHECK(v->field.present && v->field.dependency_compatible &&
                  replacement != NULL &&
                  replacement->kind == NL_CHECKED_SUM_CONSTRUCTOR &&
                  nl_semantic_value_view(c, v->field.old_value, &old));
            const NLCheckedNodeView *p =
                nl_checked_node_view(f, replacement->initializer);
            CHECK(v->field.base < NL_SEMANTIC_MAX_ENTRIES);
            old_tag = values[v->field.base].tag;
            old_target = values[v->field.base].target;
            CHECK(old.variant == old_tag + 1);
            size_t target = p == NULL ? 0 : values[p->symbol].target;
            CHECK(printf("W %zu %zu %zu %zu %zu\n", v->field.base, old_tag,
                         old_target, replacement->variant - 1, target) > 0);
            values[v->field.base].tag = replacement->variant - 1;
            values[v->field.base].target = target;
        }
        if (v->kind == NL_CHECKED_MATCH) {
            CHECK(v->normal_frame_unchanged && v->normal_arms == 2 &&
                  v->match_binding_prefix != 0);
            const NLCheckedNodeView *input =
                nl_checked_node_view(f, v->initializer);
            CHECK(input != NULL && input->kind == NL_CHECKED_IDENTIFIER);
            CHECK(printf("M %zu %zu\n", values[input->symbol].tag,
                         values[input->symbol].target) > 0);
            for (size_t j = 0; j < v->item_count; ++j) {
                const NLCheckedFragment *arm = nl_checked_match_arm(f, i, j);
                const NLCheckedNodeView *a =
                    nl_checked_node_view(arm, nl_checked_root(arm));
                CHECK(a != NULL && a->variant != 0 &&
                      (a->variant == 1 || a->symbol > v->match_binding_prefix));
                CHECK(printf("A %zu %zu\n", a->variant, a->symbol) > 0);
                if (a->variant == 2) {
                    for (size_t k = 1; k <= nl_checked_node_count(arm); ++k) {
                        const NLCheckedNodeView *l =
                            nl_checked_node_view(arm, k);
                        if (l->kind == NL_CHECKED_LOAN_HEADER &&
                            l->loan.from_ptr) {
                            CHECK(l->loan.source == a->symbol &&
                                  l->loan.body_nonescape_proved);
                            size_t target = values[input->symbol].target;
                            NLSemanticBindingView b;
                            NLSemanticPlaceView place;
                            CHECK(nl_semantic_binding_view(c, target, &b) &&
                                  nl_semantic_place_view(c, b.place, &place) &&
                                  l->loan.place == b.place &&
                                  l->loan.incarnation == place.incarnation);
                            CHECK(printf("R %zu\n", target) > 0);
                        }
                    }
                }
            }
        }
    }
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && inspect(argv[1]) ? 0 : 1;
}
