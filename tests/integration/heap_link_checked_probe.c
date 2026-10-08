#include "../support/node_checked.h"
static size_t heap_value, lexical_value, roots, fields;
static const NLCheckedNodeView *root_refs[3], *field_refs[3];
static bool scalar(const NLCheckedFragment *f, NLCheckedNodeId id, size_t *out)
{
    const NLCheckedNodeView *v = nl_checked_node_view(f, id);
    CHECK(v != NULL && v->kind == NL_CHECKED_AGGREGATE);
    NLCheckedNodeId i = v->first_argument;
    for (size_t n = 0; n < v->argument_count; ++n) {
        const NLCheckedNodeView *a = nl_checked_node_view(f, i);
        CHECK(a != NULL);
        if (a->field_index == 1) {
            const NLCheckedNodeView *s =
                nl_checked_node_view(f, a->initializer);
            CHECK(s != NULL && s->has_scalar_result && s->scalar_result.known);
            *out = s->scalar_result.value;
            return true;
        }
        i = a->next_argument;
    }
    CHECK(false);
}
static bool walk(const NLCheckedFragment *f)
{
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_INITIALIZE) {
            const NLCheckedNodeView *a =
                nl_checked_node_view(f, v->first_argument);
            CHECK(a != NULL && scalar(f, a->next_argument, &heap_value));
        }
        if (v->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *r =
                nl_checked_node_view(f, v->first_argument);
            if (r != NULL && nl_semantic_recursive_local_type(
                                 nl_checked_context(f), r->type))
                CHECK(scalar(f, v->initializer, &lexical_value));
        }
        if (v->kind == NL_CHECKED_REF_FROM_PTR) {
            CHECK(roots < 3);
            root_refs[roots++] = v;
            const NLCheckedNodeView *p =
                nl_checked_node_view(f, v->first_argument);
            const NLCheckedNodeView *s =
                p ? nl_checked_node_view(f, p->next_argument) : NULL;
            CHECK(p != NULL && s != NULL && p->symbol != 0 && s->symbol != 0);
            printf("ROOT %zu %zu %zu %zu %zu %zu\n", p->symbol, s->symbol,
                   v->lifetime_place, v->lifetime_incarnation,
                   v->lifetime_domain, v->reference_result.scope);
        }
        if (v->kind == NL_CHECKED_FIELD_REF) {
            CHECK(fields < 3);
            field_refs[fields++] = v;
            printf("FIELD %zu %zu %zu %zu %zu\n", v->field.base,
                   v->field.nominal, v->field.child, v->field.child_incarnation,
                   v->reference_result.scope);
        }
        const NLCheckedFragment *b = nl_checked_call_body(f, i);
        if (b != NULL)
            CHECK(walk(b));
        if (v->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < v->item_count; ++a) {
                b = nl_checked_match_arm(f, i, a);
                CHECK(b != NULL && walk(b));
            }
    }
    return true;
}
int main(int argc, char **argv)
{
    TestNode n = {0};
    if (argc != 2 || !node_checked_load(argv[1], &n) || !walk(n.entry))
        return 1;
    if (roots != 3 || fields != 3) {
        node_checked_destroy(&n);
        return 1;
    }
    printf("VALUES %zu %zu\n", heap_value, lexical_value);
    node_checked_destroy(&n);
    return 0;
}
