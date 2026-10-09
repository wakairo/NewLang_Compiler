/* Independent public checked-only expectations, after source/AST teardown.
 * Walk only the independently validated all-Some world for five originals
 * and its six source field operations. Other terminal worlds are covered by
 * the certificate revalidator and native failure-path observer. */
#include "../support/node_checked.h"
#include "newlang/captured_closure.h"
static bool walk(const NLCheckedFragment *f)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_INITIALIZE) {
            const NLCheckedNodeView *slot =
                nl_checked_node_view(f, v->first_argument);
            const NLCheckedNodeView *h =
                nl_checked_node_view(f, slot->next_argument);
            const NLCheckedNodeView *field =
                nl_checked_node_view(f, h->first_argument);
            while (field != NULL && field->field_index != 3)
                field = nl_checked_node_view(f, field->next_argument);
            CHECK(field != NULL);
            const NLCheckedNodeView *scalar =
                nl_checked_node_view(f, field->initializer);
            CHECK(scalar != NULL && scalar->has_scalar_result &&
                  scalar->scalar_result.known);
            printf("INIT %zu %zu %zu %zu %zu\n", v->backing, v->lifetime_place,
                   v->lifetime_incarnation, v->lifetime_domain,
                   scalar->scalar_result.value);
        }
        if (v->kind == NL_CHECKED_REF_FROM_PTR) {
            const NLCheckedNodeView *p =
                nl_checked_node_view(f, v->first_argument);
            const NLCheckedNodeView *s =
                p ? nl_checked_node_view(f, p->next_argument) : NULL;
            CHECK(p && s && p->kind == NL_CHECKED_IDENTIFIER &&
                  s->kind == NL_CHECKED_IDENTIFIER);
            printf("ROOT %zu %zu %zu %zu %zu %zu %u\n", p->symbol, s->symbol,
                   v->lifetime_place, v->lifetime_incarnation,
                   v->lifetime_domain, v->reference_result.scope,
                   v->reference_result.writable ? 1u : 0u);
        }
        if (v->kind == NL_CHECKED_FIELD_REF)
            printf("FIELD %zu %zu %zu %zu %zu %zu %u\n", v->field.parent,
                   v->field.index, v->field.nominal, v->field.child,
                   v->field.child_incarnation, v->reference_result.scope,
                   v->reference_result.writable ? 1u : 0u);
        if (v->kind == NL_CHECKED_REPLACE)
            printf("CHANGE %zu %zu\n", v->field.payload_occurrence,
                   v->field.post_payload_occurrence);
        if (v->kind == NL_CHECKED_MATCH) {
            CHECK(nl_checked_captured_closure_validate(f, i) == NL_CHECK_OK);
            for (size_t a = 0; a < v->item_count; ++a) {
                const NLCheckedFragment *arm = nl_checked_match_arm(f, i, a);
                CHECK(arm != NULL);
                const NLCheckedNodeView *root =
                    nl_checked_node_view(arm, nl_checked_root(arm));
                if (root->variant == 2)
                    CHECK(walk(arm));
            }
        }
    }
    return true;
}
int main(int argc, char **argv)
{
    TestNode n = {0};
    if (argc != 2 || !node_checked_load(argv[1], &n))
        return 1;
    const NLCheckedFragment *body =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    bool ok = body != NULL && walk(body);
    node_checked_destroy(&n);
    return ok ? 0 : 1;
}
