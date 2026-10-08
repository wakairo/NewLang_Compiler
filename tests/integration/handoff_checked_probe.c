/* Independent checked-only expectation probe. Original AST already disposed. */
#include "../support/node_checked.h"
static bool walk(const NLCheckedFragment *f)
{
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_INITIALIZE) {
            const NLCheckedNodeView *slot =
                nl_checked_node_view(f, v->first_argument);
            const NLCheckedNodeView *h =
                nl_checked_node_view(f, slot->next_argument);
            const NLCheckedNodeView *field =
                nl_checked_node_view(f, h->first_argument);
            while (field != NULL && field->field_index != 1)
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
            CHECK(p != NULL && s != NULL);
            printf("ROOT %zu %zu %zu %zu %zu %zu %u\n", p->symbol, s->symbol,
                   v->lifetime_place, v->lifetime_incarnation,
                   v->lifetime_domain, v->reference_result.scope,
                   v->reference_result.writable ? 1u : 0u);
        }
        if (v->kind == NL_CHECKED_FIELD_REF)
            printf("FIELD %zu %zu %zu %zu %zu\n", v->field.base,
                   v->field.nominal, v->field.child, v->field.child_incarnation,
                   v->reference_result.scope);
        if (v->producer.entry_proved) {
            CHECK(nl_checked_producer_valid(f, i));
            printf("PRODUCER %zu %zu %zu %zu %zu\n", v->function,
                   v->producer.root, v->producer.range.region,
                   v->producer.domain, v->producer.result);
        }
        if (v->owner_call.entry_proved) {
            CHECK(v->owner_call.post_proved);
            printf("CALL %zu %zu %zu %zu %zu\n", v->function,
                   v->owner_call.parameters[0], v->owner_call.parameters[1],
                   v->owner_call.parameters[2],
                   v->owner_call.definition.step_count == 5 ? (size_t)1
                                                            : (size_t)0);
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
    node_checked_destroy(&n);
    return 0;
}
