/* Public-only oracle over real checked source. Does not invoke the emitter.
 * Predict runtime events from source-issued concrete entry/selected arm;
 * abstract joined semantic values remain abstract. No host seed or guest write.
 */
#include "../support/node_checked.h"
#include "newlang/node_join.h"
typedef struct {
    size_t tag, target;
} Option;
static Option values[NL_SEMANTIC_MAX_ENTRIES];
static size_t root_symbol(const NLCheckedFragment *f, NLPlaceId place)
{
    NLSemanticSnapshot n;
    const NLSemanticContext *c = nl_checked_context(f);
    if (!nl_semantic_snapshot(c, &n))
        return 0;
    for (size_t i = 1; i <= n.bindings; ++i) {
        NLSemanticBindingView b;
        if (nl_semantic_binding_view(c, i, &b) && b.place == place &&
            nl_semantic_recursive_local_type(c, b.type))
            return i;
    }
    return 0;
}
static bool inspect(const NLCheckedFragment *f, bool branch)
{
    Option old = {0};
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_REPLACE) {
            const NLCheckedNodeView *a =
                nl_checked_node_view(f, v->first_argument);
            const NLCheckedNodeView *sum =
                a ? nl_checked_node_view(f, a->next_argument) : NULL;
            CHECK(sum != NULL && sum->kind == NL_CHECKED_SUM_CONSTRUCTOR);
            old = values[v->field.base];
            Option incoming = {.tag = sum->variant - 1};
            if (sum->variant == 2) {
                const NLCheckedNodeView *ptr =
                    nl_checked_node_view(f, sum->initializer);
                CHECK(ptr != NULL && ptr->has_reference_result);
                incoming.target = root_symbol(f, ptr->reference_result.place);
                CHECK(incoming.target != 0);
            }
            printf("W %zu %zu %zu %zu %zu\n", v->field.base, old.tag,
                   old.target, incoming.tag, incoming.target);
            values[v->field.base] = incoming;
        }
        if (v->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *r = nl_checked_node_view(
                                        f, v->first_argument),
                                    *init =
                                        nl_checked_node_view(f, v->initializer);
            NLSemanticTypeView type;
            CHECK(r != NULL && init != NULL &&
                  r->symbol < NL_SEMANTIC_MAX_ENTRIES);
            CHECK(nl_semantic_type_view(nl_checked_context(f), r->type, &type));
            size_t kind =
                nl_semantic_recursive_local_type(nl_checked_context(f), r->type)
                    ? 1
                : type.kind == NL_TYPE_PTR ? 2
                : type.kind == NL_TYPE_SUM ? 3
                                           : 4;
            size_t scalar = 0;
            Option val = {0};
            if (kind == 1) {
                NLCheckedNodeId field = init->first_argument;
                for (size_t j = 0; j < 2; ++j) {
                    const NLCheckedNodeView *a = nl_checked_node_view(f, field);
                    CHECK(a != NULL);
                    if (a->field_index == 1) {
                        const NLCheckedNodeView *lit =
                            nl_checked_node_view(f, a->initializer);
                        CHECK(lit != NULL && lit->has_scalar_result);
                        scalar = lit->scalar_result.value;
                    }
                    field = a->next_argument;
                }
            } else if (kind == 2) {
                CHECK(init->kind == NL_CHECKED_LOAN_HEADER);
                scalar = root_symbol(f, init->loan.place);
                CHECK(scalar != 0);
            } else if (kind == 3) {
                if (init->kind == NL_CHECKED_LOAN_HEADER)
                    val = old;
                else if (init->kind == NL_CHECKED_FIELD_READ)
                    val = values[init->field.base];
                else if (init->kind == NL_CHECKED_IDENTIFIER)
                    val = values[init->symbol];
                else
                    CHECK(false);
            } else
                CHECK(false);
            values[r->symbol] = val;
            printf("B %u %zu %zu %zu %zu %zu\n", branch ? 1u : 0u, r->symbol,
                   kind, scalar, val.tag, val.target);
        }
        if (v->kind == NL_CHECKED_MATCH) {
            NLNodeJoinView proof;
            CHECK(!branch && nl_checked_node_join_view(f, i, &proof) &&
                  nl_checked_node_join_validate(f, i, &proof) == NL_CHECK_OK);
            const NLCheckedNodeView *input =
                nl_checked_node_view(f, v->initializer);
            CHECK(input != NULL && input->kind == NL_CHECKED_IDENTIFIER);
            Option scrutinee = values[input->symbol];
            NLSemanticValueView exact;
            CHECK(nl_semantic_value_view(proof.entry, proof.input, &exact) &&
                  exact.variant == scrutinee.tag + 1);
            printf("M %zu %zu\n", scrutinee.tag, scrutinee.target);
            bool selected = false;
            for (size_t k = 0; k < 2; ++k) {
                const NLCheckedFragment *arm = proof.arms[k].artifact;
                const NLCheckedNodeView *a =
                    nl_checked_node_view(arm, nl_checked_root(arm));
                if (a->variant != scrutinee.tag + 1)
                    continue;
                CHECK(!selected);
                selected = true;
                if (a->symbol != 0) {
                    CHECK(a->symbol < NL_SEMANTIC_MAX_ENTRIES);
                    /* Same numeric post binding may belong to parent later;
                     * restore the private binder slot after this arm. */
                }
                Option saved = a->symbol == 0 ? (Option){0} : values[a->symbol];
                if (a->symbol != 0)
                    values[a->symbol] = scrutinee;
                CHECK(inspect(arm, true));
                if (a->symbol != 0)
                    values[a->symbol] = saved;
            }
            CHECK(selected);
        }
        if (v->kind == NL_CHECKED_LOAN_HEADER && v->loan.from_ptr) {
            size_t target = root_symbol(f, v->loan.place);
            CHECK(target != 0);
            printf("R %zu\n", target);
        }
    }
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    TestNode n = {0};
    if (!node_checked_load(argv[1], &n))
        return 1;
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    bool ok = inspect(f, false);
    node_checked_destroy(&n);
    return ok ? 0 : 1;
}
