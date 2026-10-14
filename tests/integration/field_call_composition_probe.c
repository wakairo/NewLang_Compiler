/* P421: immutable public views of actual-source checked-owned composition. */
#include "../support/node_checked.h"
#include <stdlib.h>

static size_t scalar(const NLSemanticContext *c, NLValueId id)
{
    NLSemanticValueView v;
    if (!nl_semantic_value_view(c, id, &v) || !v.scalar_known)
        abort();
    return v.scalar_value;
}

static bool inspect(TestNode *n)
{
    const NLCheckedFragment *f =
        nl_checked_call_body(n->entry, nl_checked_root(n->entry));
    CHECK(f != NULL && nl_checked_context(f) == n->context);
    size_t roots = 0, changes = 0, calls = 0, reads = 0;
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_BINDING) {
            NLSemanticBindingView b;
            CHECK(nl_semantic_binding_view(n->context, v->symbol, &b));
            const NLCheckedNodeView *rhs =
                nl_checked_node_view(f, v->initializer);
            if (rhs->kind == NL_CHECKED_AGGREGATE) {
                NLSemanticValueView value;
                NLSemanticPlaceView place;
                CHECK(nl_semantic_value_view(n->context, rhs->results[0].value,
                                             &value));
                CHECK(value.field_count == 2 &&
                      nl_semantic_place_view(n->context, b.place, &place));
                printf(
                    "{\"kind\":\"root\",\"symbol\":%zu,\"place\":%zu,"
                    "\"incarnation\":%zu,\"initial\":[%zu,%zu],\"ended\":%s}\n",
                    v->symbol, b.place, place.incarnation,
                    scalar(n->context, value.fields[0]),
                    scalar(n->context, value.fields[1]),
                    place.live ? "false" : "true");
                ++roots;
            } else {
                printf("{\"kind\":\"binding\",\"symbol\":%zu,\"package\":%zu,"
                       "\"value\":%zu}\n",
                       v->symbol, b.value, scalar(n->context, b.value));
            }
        }
        if (v->kind == NL_CHECKED_LOAN_HEADER) {
            CHECK(v->loan.body_nonescape_proved &&
                  v->loan.normal_result_forwarded &&
                  v->loan.access == NL_ACCESS_WRITE);
            printf("{\"kind\":\"loan\",\"source\":%zu,\"scope\":%zu,\"ref\":%"
                   "zu,\"place\":%zu,\"incarnation\":%zu,\"result\":%zu}\n",
                   v->loan.source, v->loan.scope, v->loan.ref_symbol,
                   v->loan.place, v->loan.incarnation, v->results[0].value);
        }
        if (v->kind == NL_CHECKED_REPLACE) {
            const NLCheckedField *e = &v->field;
            CHECK(e->present && e->dependency_compatible &&
                  e->access == NL_ACCESS_WRITE);
            CHECK(e->parent_fact != e->parent_post_fact &&
                  e->child_fact != e->child_post_fact);
            CHECK(e->old_value == v->results[0].value);
            printf(
                "{\"kind\":\"change\",\"node\":%zu,\"source\":%zu,\"index\":%"
                "zu,\"parent\":%zu,\"child\":%zu,\"parent_incarnation\":%zu,"
                "\"child_incarnation\":%zu,\"parent_fact\":%zu,\"parent_post_"
                "fact\":%zu,\"child_fact\":%zu,\"child_post_fact\":%zu,\"old_"
                "package\":%zu,\"new_package\":%zu,\"old\":%zu,\"new\":%zu}\n",
                i, e->base, e->index, e->parent, e->child,
                e->parent_incarnation, e->child_incarnation, e->parent_fact,
                e->parent_post_fact, e->child_fact, e->child_post_fact,
                e->old_value, e->new_value, scalar(n->context, e->old_value),
                scalar(n->context, e->new_value));
            ++changes;
        }
        if (v->kind == NL_CHECKED_REGISTERED_CALL) {
            const NLCheckedFragment *body = nl_checked_call_body(f, i);
            const NLCheckedNodeView *root =
                body == NULL
                    ? NULL
                    : nl_checked_node_view(body, nl_checked_root(body));
            const NLCheckedNodeView *arg =
                nl_checked_node_view(f, v->first_argument);
            CHECK(body != NULL && nl_checked_context(body) == n->context &&
                  v->argument_count == 1);
            CHECK(root->result_count == 1 &&
                  root->results[0].value == v->results[0].value);
            printf("{\"kind\":\"call\",\"node\":%zu,\"actual_symbol\":%zu,"
                   "\"actual_package\":%zu,\"return_package\":%zu,\"actual\":%"
                   "zu,\"returned\":%zu}\n",
                   i, arg->symbol, arg->results[0].value, v->results[0].value,
                   scalar(n->context, arg->results[0].value),
                   scalar(n->context, v->results[0].value));
            ++calls;
        }
        if (v->kind == NL_CHECKED_FIELD_READ) {
            const NLCheckedField *e = &v->field;
            CHECK(e->present && e->dependency_compatible);
            printf("{\"kind\":\"read\",\"source\":%zu,\"index\":%zu,\"parent\":"
                   "%zu,\"child\":%zu,\"parent_fact\":%zu,\"child_fact\":%zu,"
                   "\"value\":%zu}\n",
                   e->base, e->index, e->parent, e->child, e->parent_fact,
                   e->child_fact, scalar(n->context, v->results[0].value));
            ++reads;
        }
    }
    CHECK(roots == 2 && changes == 2 && calls == 1 && reads == 2);
    puts("{\"kind\":\"status\",\"parse\":0,\"register\":0,\"check\":0,"
         "\"owned\":true,\"roots\":2,\"changes\":2,\"calls\":1,\"reads\":2}");
    return true;
}

int main(int argc, char **argv)
{
    TestNode n = {0};
    if (argc != 2 || !node_checked_load(argv[1], &n) || !inspect(&n))
        return 1;
    node_checked_destroy(&n);
    return 0;
}
